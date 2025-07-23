#!/bin/bash
set -e

# Usage: ./full_pipeline.sh <c_file> [logic_func]
# Example: ./full_pipeline.sh tests/check_num.c checkNum
# Defaults logic_func to "main" if not provided.
# Builds in build/<example_name>/, stores results in results/<example_name>/.

if [ $# -lt 1 ]; then
    echo "Usage: $0 <c_file> [logic_func]"
    exit 1
fi

CFILE=$1
LOGIC_FUNC=${2:-main}
EXAMPLE_NAME=$(basename "$CFILE" .c)
BUILD_DIR="build/$EXAMPLE_NAME"
RESULTS_DIR="results/$EXAMPLE_NAME"

# Create directories
mkdir -p "$BUILD_DIR" "$RESULTS_DIR"

# Change to build directory for operations
cd "$BUILD_DIR"

# Define relative paths back to project root
ROOT_DIR="../.."
STRIPPED="tmp_structs.c"
STRUCTGEN="$ROOT_DIR/src/pycparser_generate_proto.py"  # Assuming src/ from restructure
WRAPGEN="$ROOT_DIR/src/generate_wrapper_ast.py"  # Assuming src/
NANOPB_DIR="$ROOT_DIR/nanopb"
WRAPPER_SRC="main.c"
ORIGINAL_OBJ="original.o"

# Clean up previous generated files in build dir
rm -f *.proto *.pb.h *.pb.c *.py input.bin *.o main.c

# Preprocess: Remove # lines and run cpp to strip comments
temp_file="temp_no_pp.c"
grep -v '^#' "$ROOT_DIR/$CFILE" > "$temp_file"
cpp -nostdinc "$temp_file" > "$STRIPPED"
rm "$temp_file"

# Step 1: Generate .proto from stripped file
echo "Generating .proto from $STRIPPED..."
python3 "$STRUCTGEN" "$STRIPPED" "$LOGIC_FUNC"

# Step 2: Get the newly created proto file (should be only one!)
PROTOFILE=$(ls *.proto)
PROTO_BASE=$(awk '/^message /{print $2; exit}' "$PROTOFILE" || echo "Input")
PROTO_BASE_LOWER=$(echo "$PROTO_BASE" | tr '[:upper:]' '[:lower:]')
echo "Top-level message detected: $PROTO_BASE (filename: $PROTO_BASE_LOWER)"

# If proto filename does not match message name, rename to match
if [[ "$PROTOFILE" != "${PROTO_BASE_LOWER}.proto" ]]; then
    mv "$PROTOFILE" "${PROTO_BASE_LOWER}.proto"
    PROTOFILE="${PROTO_BASE_LOWER}.proto"
fi

# Step 3: Generate Python input bindings and input.bin
protoc --python_out=. "$PROTOFILE"

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
                    pass  # Add more types as needed
        else:  # Optional or required
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
                pass  # Add more types as needed

m = pb2.${PROTO_BASE}()
fill_random(m)
with open("input.bin", "wb") as f:
    f.write(m.SerializeToString())
print("Generated message:", str(m))
EOF

python3 gen_input.py

# Step 4: Generate wrapper (main.c)
echo "Generating wrapper..."
python3 "$WRAPGEN" "$STRIPPED" $LOGIC_FUNC $PROTO_BASE $PROTO_BASE  # Pass PROTO_BASE for both to fix casing

# Step 5: Generate nanopb serialization code
echo "Generating nanopb files..."
protoc --nanopb_out=. "$PROTOFILE"

# Step 6: Compile everything - original as object to avoid duplicates
echo "Compiling original as object..."
gcc -c "$ROOT_DIR/$CFILE" -o "$ORIGINAL_OBJ" -I"$NANOPB_DIR" 

echo "Compiling all sources..."
gcc -o pin_test \
    "$ORIGINAL_OBJ" "$WRAPPER_SRC" "${PROTO_BASE_LOWER}.pb.c" \
    "$NANOPB_DIR/pb_decode.c" "$NANOPB_DIR/pb_common.c" \
    -I"$NANOPB_DIR" 

echo "Build complete: ./pin_test"

# Step 7: Run if input.bin exists and capture output
if [ -f input.bin ]; then
    echo "Running binary with input.bin..."
    ./pin_test input.bin > output.log 2>&1
else
    echo "No input.bin found, skipping run." > output.log
fi

# Store results (move generated files to results dir)
mv pin_test input.bin output.log *.proto *.pb.h *.pb.c main.c "$ROOT_DIR/$RESULTS_DIR/" || true

# Clean up temporary files in build dir
rm -f "$STRIPPED" gen_input.py *.py *.o

# Return to project root
cd "$ROOT_DIR"