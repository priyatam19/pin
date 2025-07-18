#!/bin/bash
set -e

# Clean up all proto and nanopb files from previous runs
rm -f *.proto *.pb.h *.pb.c

CFILE="cjson_test.c"  # Your test harness C file using cJSON (rename if needed)
STRUCTGEN="pycparser_generate_proto.py"
WRAPGEN="generate_wrapper_ast.py"
NANOPB_DIR="nanopb"
LOGIC_FUNC="P"
WRAPPER_SRC="main.c"
CJSON_DIR="cJSON"

STRIPPED="tmp_structs.c"

grep -v '^#include' "$CFILE" > "$STRIPPED"

# Extract only typedef struct {...} Name; blocks for struct parsing
# Step 1: Generate .proto from struct typedefs
# awk '/typedef struct([[:space:]]|{)/{p=1; buf=$0; next} p{buf=buf"\n"$0} /};/{if(p){print buf"\n"; p=0}}' "$CFILE" > "$STRIPPED"

# Step 1: Generate .proto from struct typedefs
echo "Generating .proto from $STRIPPED..."
python3 $STRUCTGEN "$STRIPPED"

# Step 2: Get the newly created proto file (should be only one!)
PROTOFILE=$(ls *.proto)
PROTO_BASE=$(awk '/^message /{print $2; exit}' "$PROTOFILE")
PROTO_BASE_LOWER=$(echo "$PROTO_BASE" | tr '[:upper:]' '[:lower:]')
echo "Top-level message/struct detected: $PROTO_BASE (filename: $PROTO_BASE_LOWER)"

# If proto filename does not match message name, rename to match
if [[ "$PROTOFILE" != "${PROTO_BASE_LOWER}.proto" ]]; then
    mv "$PROTOFILE" "${PROTO_BASE_LOWER}.proto"
    PROTOFILE="${PROTO_BASE_LOWER}.proto"
fi

# === Generate Python input bindings and input.bin ===
protoc --python_out=. "$PROTOFILE"

cat > gen_input.py <<EOF
import ${PROTO_BASE_LOWER}_pb2
import json
import random

def random_json():
    return json.dumps({
        "array": [random.randint(0,100), random.uniform(0,1)],
        "bool": random.choice([True, False]),
        "obj": {"a": random.randint(1,99)}
    })

m = ${PROTO_BASE_LOWER}_pb2.${PROTO_BASE}()
m.s = random_json()
with open("input.bin", "wb") as f:
    f.write(m.SerializeToString())
print("Generated JSON string:", m.s)
EOF

python3 gen_input.py

# Step 3: Generate wrapper (main.c)
echo "Generating wrapper..."
python3 $WRAPGEN "$STRIPPED" $LOGIC_FUNC $PROTO_BASE_LOWER $PROTO_BASE

# Step 4: Generate nanopb serialization code
echo "Generating nanopb files..."
protoc --nanopb_out=. "$PROTOFILE"

# Step 5: Compile everything
echo "Compiling all sources..."
gcc -I./$NANOPB_DIR -I./$CJSON_DIR -o pin_test \
    "$CFILE" "$WRAPPER_SRC" "${PROTO_BASE_LOWER}.pb.c" \
    "$NANOPB_DIR/pb_decode.c" "$NANOPB_DIR/pb_common.c" \
    "$CJSON_DIR/cJSON.c"

echo "Build complete: ./pin_test"
if [ -f input.bin ]; then
    echo "Running binary with input.bin..."
    ./pin_test input.bin
else
    echo "No input.bin found, skipping run."
fi

rm -f "$STRIPPED"