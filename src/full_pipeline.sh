#!/bin/bash
#
# PIN Full Pipeline - End-to-End C Program Normalization
#
# This script orchestrates the complete PIN normalization process:
# 1. Preprocesses C files to remove includes and handle headers
# 2. Generates Protocol Buffer schemas from C structs
# 3. Creates nanopb C bindings for deserialization
# 4. Generates wrapper code to call original functions
# 5. Compiles normalized binary that accepts .bin input
# 6. Performs differential testing between original and normalized versions
#
# Usage: ./full_pipeline.sh <c_file> [logic_func] [--parser=<pycparser|libclang>] [--headers-dir=<dir>]
# Example: ./full_pipeline.sh examples/mqtt.c main --parser=libclang --headers-dir=utils/fake_headers
#
# Author: PIN Development Team

set -e

# Parse flags
PARSER="pycparser"  # Default
HEADERS_DIR=""
LIBS=""
AFL_MODE=""
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --parser=*) PARSER="${1#*=}"; shift ;;
        --headers-dir=*) HEADERS_DIR="${1#*=}"; shift ;;
        --libs=*) LIBS="${1#*=}"; shift ;;
        --afl) AFL_MODE="true"; shift ;;
        *) if [ -z "$CFILE" ]; then CFILE=$1; else LOGIC_FUNC=$1; fi; shift ;;
    esac
done
LOGIC_FUNC=${LOGIC_FUNC:-main}

EXAMPLE_NAME=$(basename "$CFILE" .c)
BUILD_DIR="build/$EXAMPLE_NAME"
RESULTS_DIR="results/$EXAMPLE_NAME"

# Set compiler based on AFL mode
if [ "$AFL_MODE" = "true" ]; then
    CC="afl-gcc"
    echo "Using AFL instrumentation (afl-gcc)"
else
    CC="gcc"
fi

# Create directories
mkdir -p "$BUILD_DIR" "$RESULTS_DIR"

# Change to build directory for operations
cd "$BUILD_DIR"

# Define relative paths back to project root
ROOT_DIR="../.."
STRIPPED="tmp_structs.c"
STRUCTGEN="$ROOT_DIR/src/pycparser_generate_proto.py"
WRAPGEN="$ROOT_DIR/src/generate_wrapper_ast.py"
NANOPB_DIR="$ROOT_DIR/nanopb"
FAKE_INC_DIR="$ROOT_DIR/utils/fake_headers"
WRAPPER_SRC="main.c"
ORIGINAL_OBJ="original.o"
ORIGINAL_BIN="original_bin"

# Clean up previous generated files in build dir
rm -f *.proto *.pb.h *.pb.c *.py input.bin *.o main.c original_bin *.log native_input.json native_input.txt test_original.c
rm -rf __pycache__

# Preprocess: Remove # lines and run cpp with fake includes and user headers
temp_file="temp_no_pp.c"
grep -v '^#' "$ROOT_DIR/$CFILE" > "$temp_file"
# Add user headers if specified and check existence
if [ -n "$HEADERS_DIR" ]; then
    if [ -d "$ROOT_DIR/$HEADERS_DIR" ]; then
        # Only include mongoose.h for mqtt.c
        if [[ "$(basename "$CFILE")" == "mqtt.c" ]]; then
            HEADER_PATH="$ROOT_DIR/$HEADERS_DIR/mongoose.h"
            if [ -f "$HEADER_PATH" ]; then
                sed -i '1i #include "mongoose.h"\n' "$temp_file"
            else
                echo "Error: mongoose.h not found in $ROOT_DIR/$HEADERS_DIR for mqtt.c"
                exit 1
            fi
        fi
    else
        echo "Error: --headers-dir=$HEADERS_DIR is not a valid directory"
        exit 1
    fi
fi
cpp -I"$FAKE_INC_DIR" -I"$ROOT_DIR/$HEADERS_DIR" -D__THROW= -D__BEGIN_DECLS= -D__END_DECLS= -D"__attribute__(x)=" "$temp_file" > "$STRIPPED" 2> cpp_errors.log || {
    echo "Preprocessing failed. See cpp_errors.log for details."
    cat cpp_errors.log
    exit 1
}
rm "$temp_file"

# Step 1: Generate enhanced .proto from function signature
echo "Generating enhanced .proto for function $LOGIC_FUNC..."
if [ -f "$ROOT_DIR/src/enhanced_proto_generator.py" ]; then
    echo "Using enhanced function-aware proto generation..."
    python3 "$ROOT_DIR/src/enhanced_proto_generator.py" "$ROOT_DIR/$CFILE" "$LOGIC_FUNC" > proto_gen.log 2>&1
    if [ $? -eq 0 ]; then
        echo "Enhanced proto generation successful"
    else
        echo "Enhanced proto generation failed, falling back to original method..."
        python3 "$STRUCTGEN" "$STRIPPED" "$LOGIC_FUNC" --parser="$PARSER" --headers-dir="$ROOT_DIR/$HEADERS_DIR" > proto_gen.log 2>&1 || {
            echo "Error: Proto generation failed. See proto_gen.log for details."
            cat proto_gen.log
            exit 1
        }
    fi
else
    echo "Using original proto generation from $STRIPPED..."
    python3 "$STRUCTGEN" "$STRIPPED" "$LOGIC_FUNC" --parser="$PARSER" --headers-dir="$ROOT_DIR/$HEADERS_DIR" > proto_gen.log 2>&1 || {
        echo "Error: Proto generation failed. See proto_gen.log for details."
        cat proto_gen.log
        exit 1
    }
fi

# Step 2: Get the newly created proto file
PROTOFILE=$(ls *.proto 2>/dev/null || echo "")
if [ -z "$PROTOFILE" ]; then
    echo "Error: No .proto file generated"
    exit 1
fi
PROTO_BASE=$(awk '/^message /{print $2}' "$PROTOFILE" | head -n 1)
if [ -z "$PROTO_BASE" ]; then
    PROTO_BASE="Input"
    echo "DEBUG: No message found in proto, defaulting to Input"
fi
PROTO_BASE_LOWER=$(echo "$PROTO_BASE" | tr '[:upper:]' '[:lower:]')
echo "Top-level message detected: $PROTO_BASE (filename: $PROTO_BASE_LOWER)"

# If proto filename does not match message name, rename to match
if [[ "$PROTOFILE" != "${PROTO_BASE_LOWER}.proto" ]]; then
    mv "$PROTOFILE" "${PROTO_BASE_LOWER}.proto"
    PROTOFILE="${PROTO_BASE_LOWER}.proto"
fi

# Step 3: Generate Python input bindings and input.bin
protoc --python_out=. "$PROTOFILE" || {
    echo "Error: Protoc failed to generate Python bindings"
    exit 1
}

# Generate dynamic random input script
cat > gen_input.py <<EOF
import ${PROTO_BASE_LOWER}_pb2 as pb2
import random
from google.protobuf.descriptor import FieldDescriptor

def fill_random(msg):
    for field in msg.DESCRIPTOR.fields:
        if field.label == FieldDescriptor.LABEL_REPEATED:
            num_items = random.randint(0, 3)
            for _ in range(num_items):
                if field.type == FieldDescriptor.TYPE_MESSAGE:
                    sub_msg = getattr(msg, field.name).add()
                    fill_random(sub_msg)
                elif field.type == FieldDescriptor.TYPE_INT32:
                    getattr(msg, field.name).append(random.randint(-100, 100))
                elif field.type == FieldDescriptor.TYPE_INT64:
                    getattr(msg, field.name).append(random.randint(-1000, 1000))
                elif field.type == FieldDescriptor.TYPE_FLOAT:
                    getattr(msg, field.name).append(random.uniform(-10.0, 10.0))
                elif field.type == FieldDescriptor.TYPE_DOUBLE:
                    getattr(msg, field.name).append(random.uniform(-100.0, 100.0))
                elif field.type == FieldDescriptor.TYPE_BOOL:
                    getattr(msg, field.name).append(random.choice([True, False]))
                elif field.type == FieldDescriptor.TYPE_STRING:
                    getattr(msg, field.name).append("random_str_" + str(random.randint(0, 100)))
                else:
                    pass
        else:
            if field.type == FieldDescriptor.TYPE_MESSAGE:
                sub_msg = getattr(msg, field.name)
                fill_random(sub_msg)
            elif field.type == FieldDescriptor.TYPE_INT32:
                setattr(msg, field.name, random.randint(-100, 100))
            elif field.type == FieldDescriptor.TYPE_INT64:
                setattr(msg, field.name, random.randint(-1000, 1000))
            elif field.type == FieldDescriptor.TYPE_FLOAT:
                setattr(msg, field.name, random.uniform(-10.0, 10.0))
            elif field.type == FieldDescriptor.TYPE_DOUBLE:
                setattr(msg, field.name, random.uniform(-100.0, 100.0))
            elif field.type == FieldDescriptor.TYPE_BOOL:
                setattr(msg, field.name, random.choice([True, False]))
            elif field.type == FieldDescriptor.TYPE_STRING:
                setattr(msg, field.name, "random_str_" + str(random.randint(0, 100)))
            else:
                pass

m = pb2.${PROTO_BASE}()
fill_random(m)
with open("input.bin", "wb") as f:
    f.write(m.SerializeToString())
print("Generated message:", str(m))
EOF

python3 gen_input.py || {
    echo "Error: Failed to generate input.bin. See gen_input.py output."
    exit 1
}

# Step 4: Generate wrapper (main.c)
echo "Generating wrapper..."
if [ -f "proto_info.txt" ] && [ -f "$ROOT_DIR/src/enhanced_wrapper_generator.py" ]; then
    echo "Using enhanced function-aware wrapper generation..."
    MESSAGE_NAME=$(grep "message_name:" proto_info.txt | cut -d' ' -f2)
    python3 "$ROOT_DIR/src/enhanced_wrapper_generator.py" "$PROTOFILE" "$LOGIC_FUNC" "$MESSAGE_NAME" > wrap_gen.log 2>&1 || {
        echo "Enhanced wrapper generation failed, falling back to original..."
        python3 "$WRAPGEN" "$STRIPPED" "$LOGIC_FUNC" "$PROTO_BASE" "$PROTO_BASE" --parser="$PARSER" --headers-dir="$ROOT_DIR/$HEADERS_DIR" --original-file="$ROOT_DIR/$CFILE" > wrap_gen.log 2>&1 || {
            echo "Error: Wrapper generation failed. See wrap_gen.log for details."
            cat wrap_gen.log
            exit 1
        }
    }
else
    echo "Using original wrapper generation..."
    python3 "$WRAPGEN" "$STRIPPED" "$LOGIC_FUNC" "$PROTO_BASE" "$PROTO_BASE" --parser="$PARSER" --headers-dir="$ROOT_DIR/$HEADERS_DIR" --original-file="$ROOT_DIR/$CFILE" > wrap_gen.log 2>&1 || {
        echo "Error: Wrapper generation failed. See wrap_gen.log for details."
        cat wrap_gen.log
        exit 1
    }
fi

# Step 5: Generate nanopb serialization code
echo "Generating nanopb files..."
protoc --nanopb_out=. "$PROTOFILE" || {
    echo "Error: Protoc failed to generate nanopb files"
    exit 1
}

# Step 6: Compile coreutils stubs only if processing coreutils programs
COREUTILS_STUBS_OBJ=""
if [[ "$CFILE" == *"coreutils"* ]]; then
    echo "Compiling coreutils stubs..."
    $CC -c "$ROOT_DIR/utils/coreutils_headers/coreutils_stubs.c" -o "coreutils_stubs.o" -I"$ROOT_DIR/$HEADERS_DIR" -D__THROW= -D__BEGIN_DECLS= -D__END_DECLS= -D"__attribute__(x)=" || {
        echo "Error: Compilation of coreutils stubs failed"
        exit 1
    }
    COREUTILS_STUBS_OBJ="coreutils_stubs.o"
else
    echo "Skipping coreutils stubs for non-coreutils program"
fi

# Step 6.5: Compile additional library files first if provided
LIB_OBJECTS=""
if [ -n "$LIBS" ]; then
    echo "Compiling additional library files..."
    IFS=',' read -ra LIB_ARRAY <<< "$LIBS"
    for lib_file in "${LIB_ARRAY[@]}"; do
        lib_name=$(basename "$lib_file" .c)
        lib_obj="${lib_name}.o"
        echo "Compiling $lib_file -> $lib_obj"
        $CC -c "$ROOT_DIR/$lib_file" -o "$lib_obj" -I"$ROOT_DIR/$HEADERS_DIR" $COREUTILS_INCLUDES || {
            echo "Error: Compilation of $lib_file failed"
            exit 1
        }
        LIB_OBJECTS="$LIB_OBJECTS $lib_obj"
    done
fi

echo "Compiling original as object..."

# Detect if this is a coreutils program and add appropriate include paths
COREUTILS_INCLUDES=""
if [[ "$CFILE" == *"coreutils"* ]]; then
    COREUTILS_BASE=$(dirname $(dirname "$ROOT_DIR/$CFILE"))  # Get the coreutils-x.y directory  
    COREUTILS_INCLUDES="-I$COREUTILS_BASE/lib -I$COREUTILS_BASE/src"
    echo "Detected coreutils program, adding include paths: $COREUTILS_INCLUDES"
fi

$CC -c "$ROOT_DIR/$CFILE" -o "$ORIGINAL_OBJ" -I"$NANOPB_DIR" -I"$ROOT_DIR/$HEADERS_DIR" $COREUTILS_INCLUDES -D__THROW= -D__BEGIN_DECLS= -D__END_DECLS= -D"__attribute__(x)=" -D"__builtin_va_arg_pack()=0" || {
    echo "Error: Compilation of original object failed"
    exit 1
}

# Always rename original main symbol to avoid conflicts
objcopy --redefine-sym main=pin_original_main "$ORIGINAL_OBJ" || true

# Compile original binary for comparison (if main exists)
echo "Compiling original binary for comparison..."
$CC "$ROOT_DIR/$CFILE" $COREUTILS_STUBS_OBJ $LIB_OBJECTS -o "$ORIGINAL_BIN" -I"$NANOPB_DIR" -I"$ROOT_DIR/$HEADERS_DIR" $COREUTILS_INCLUDES -D__THROW= -D__BEGIN_DECLS= -D__END_DECLS= -D"__attribute__(x)=" -D"__builtin_va_arg_pack()=0" || echo "Original has no main, skipping original_bin compilation."

# Step 7: Compile nanopb files separately (without coreutils headers)
echo "Compiling nanopb files..."
$CC -c "$NANOPB_DIR/pb_decode.c" -o "pb_decode.o" -I"$NANOPB_DIR" || {
    echo "Error: Compilation of pb_decode.c failed"
    exit 1
}
$CC -c "$NANOPB_DIR/pb_common.c" -o "pb_common.o" -I"$NANOPB_DIR" || {
    echo "Error: Compilation of pb_common.c failed"
    exit 1
}

# Step 8: Compile wrapper and protobuf files (with coreutils headers for wrapper)
echo "Compiling wrapper and protobuf files..."
$CC -c "$WRAPPER_SRC" -o "wrapper.o" -I"$NANOPB_DIR" -I"$ROOT_DIR/$HEADERS_DIR" $COREUTILS_INCLUDES -D__THROW= -D__BEGIN_DECLS= -D__END_DECLS= -D"__attribute__(x)=" -D"__builtin_va_arg_pack()=0" || {
    echo "Error: Compilation of wrapper failed"
    exit 1
}
$CC -c "${PROTO_BASE_LOWER}.pb.c" -o "proto.o" -I"$NANOPB_DIR" || {
    echo "Error: Compilation of protobuf file failed"
    exit 1
}


# Step 9: Link everything together
echo "Linking all objects..."
if [ "$AFL_MODE" = "true" ]; then
    # For AFL mode we still need the original object for target symbols
    # (its main symbol was already renamed above via objcopy).
    echo "AFL mode: Linking PIN-normalized binary with original object"
    $CC -o pin_test \
        "$ORIGINAL_OBJ" "wrapper.o" "proto.o" \
        "pb_decode.o" "pb_common.o" \
        $COREUTILS_STUBS_OBJ $LIB_OBJECTS \
        -I"$NANOPB_DIR" -I"$ROOT_DIR/$HEADERS_DIR" $COREUTILS_INCLUDES || {
        echo "Error: AFL linking failed"
        exit 1
    }
else
    # Normal mode: Link with original object for comparison
    $CC -o pin_test \
        "$ORIGINAL_OBJ" "wrapper.o" "proto.o" \
        "pb_decode.o" "pb_common.o" \
        $COREUTILS_STUBS_OBJ $LIB_OBJECTS \
        -I"$NANOPB_DIR" -I"$ROOT_DIR/$HEADERS_DIR" $COREUTILS_INCLUDES || {
        echo "Error: Linking failed"
        exit 1
    }
fi

echo "Build complete: ./pin_test"

# Step 8: Run normalized
if [ -f input.bin ]; then
    echo "Running normalized binary with input.bin..."
    ./pin_test input.bin > output_normalized.log 2>&1
else
    echo "No input.bin found, skipping normalized run." > output_normalized.log
fi

# Step 9: Run original with native input
if [ -f "$ORIGINAL_BIN" ] && [ -f native_input.txt ]; then
    echo "Running original binary with native input..."
    ./$ORIGINAL_BIN < native_input.txt > output_original.log 2>&1
else
    echo "No original_bin or native_input.txt, skipping original run." > output_original.log
fi

# Step 10: Compare outputs
echo "Comparing outputs..."
if diff output_original.log output_normalized.log > comparison.log; then
    echo "Outputs match" >> comparison.log
else
    echo "Outputs differ" >> comparison.log
fi

# Store results (move generated files to results dir)
mv pin_test original_bin input.bin output_normalized.log output_original.log comparison.log *.proto *.pb.h *.pb.c main.c native_input.json native_input.txt "$ROOT_DIR/$RESULTS_DIR/" || true

# Clean up temporary files in build dir
rm -f "$STRIPPED" gen_input.py *.py *.o
rm -rf __pycache__

# Return to project root
cd "$ROOT_DIR"
