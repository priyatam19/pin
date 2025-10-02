#!/usr/bin/env bash
# PIN Differential Fuzzing (Stage A scaffold)
#
# Usage: pin/src/pin_diff.sh <c_file> <function_name> [--headers-dir=DIR]
#
# Stage A: Build normalized wrapper + libFuzzer byte harness that calls
#          pin_wrapper_entry(data, size) to discover interesting serialized
#          protobuf inputs and grow a corpus.
#
# Stage B: Differential replay of saved corpus comparing outputs of
#   - normalized binary (nanopb wrapper reading bytes)
#   - original replay binary (C++ protobuf decode calling original code)
#
set -euo pipefail

CFILE=${1:-}
FUNC=${2:-}
shift 2 || true

HEADERS_DIR=""
REPLAY_DIR=""
FUZZ_SECONDS=0
FUZZ_EXTRA_FLAGS=""
for arg in "$@"; do
  case "$arg" in
    --headers-dir=*) HEADERS_DIR="${arg#*=}" ;;
    --replay-dir=*) REPLAY_DIR="${arg#*=}" ;;
    --fuzz-seconds=*) FUZZ_SECONDS="${arg#*=}" ;;
    --fuzz-flags=*) FUZZ_EXTRA_FLAGS="${arg#*=}" ;;
  esac
done

if [[ -z "$CFILE" || -z "$FUNC" ]]; then
  echo "Usage: pin/src/pin_diff.sh <c_file> <function_name> [--headers-dir=DIR]"
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
EXAMPLE_NAME=$(basename "$CFILE" .c)
BUILD_DIR="$ROOT_DIR/build/${EXAMPLE_NAME}_diff"
RESULTS_DIR="$ROOT_DIR/results/${EXAMPLE_NAME}_diff"
NANOPB_DIR="$ROOT_DIR/nanopb"
FAKE_INC_DIR="$ROOT_DIR/utils/fake_headers"
ORIGINAL_SRC="$ROOT_DIR/$CFILE"
HEADERS_ABS=""
if [[ -n "$HEADERS_DIR" ]]; then
  HEADERS_ABS="$ROOT_DIR/$HEADERS_DIR"
fi

prepare_pycparser_source() {
  local source_path="$1"
  local output_path="$2"

  grep -v '^#' "$source_path" > "$output_path"

  local cpp_args=(cpp "-I$FAKE_INC_DIR")
  if [[ -n "$HEADERS_ABS" ]]; then
    cpp_args+=("-I$HEADERS_ABS")
  fi
  cpp_args+=(-D__THROW= -D__BEGIN_DECLS= -D__END_DECLS= "-D__attribute__(x)=" "$output_path")

  "${cpp_args[@]}" > "$output_path.pp" 2> cpp_errors.log || true
  if [[ -s "$output_path.pp" ]]; then
    mv "$output_path.pp" "$output_path"
  fi
}

run_proto_generation() {
  local parser="$1"
  local source_path="$2"

  find . -maxdepth 1 -name '*.proto' -delete 2>/dev/null || true

  local cmd=(python3 "$ROOT_DIR/src/pycparser_generate_proto.py" "$source_path" "$FUNC" "--parser=$parser")
  if [[ -n "$HEADERS_ABS" ]]; then
    cmd+=("--headers-dir=$HEADERS_ABS")
  fi

  set +e
  "${cmd[@]}" > proto_gen.log 2>&1
  PROTO_STATUS=$?
  set -e

  shopt -s nullglob
  PROTO_FILES=(*.proto)
  shopt -u nullglob
}

mkdir -p "$BUILD_DIR/corpus" "$RESULTS_DIR"
cd "$BUILD_DIR"

echo "[+] Generate .proto (libclang parser)"
PARSER_USED="libclang"
PARSE_INPUT="$ORIGINAL_SRC"
run_proto_generation "$PARSER_USED" "$PARSE_INPUT"

if (( PROTO_STATUS != 0 || ${#PROTO_FILES[@]} == 0 )); then
  echo "[-] Proto generation failed (libclang). See proto_gen.log"
  exit 2
fi
# if (( PROTO_STATUS != 0 || ${#PROTO_FILES[@]} == 0 )); then
#   echo "[i] libclang proto generation failed, attempting pycparser fallback"
#   PARSER_USED="pycparser"
#   PARSE_INPUT="tmp_structs.c"
#   prepare_pycparser_source "$ORIGINAL_SRC" "$PARSE_INPUT"
#   run_proto_generation "$PARSER_USED" "$PARSE_INPUT"
#   if (( PROTO_STATUS != 0 || ${#PROTO_FILES[@]} == 0 )); then
#     echo "[-] Proto generation failed. See proto_gen.log"
#     exit 2
#   fi
# fi
if [[ -f input.proto ]]; then
  PROTOFILE="input.proto"
else
  PROTOFILE="${PROTO_FILES[0]}"
fi
PROTO_BASE=$(awk '/^message /{print $2; exit}' "$PROTOFILE")
[[ -z "$PROTO_BASE" ]] && PROTO_BASE=Input
PROTO_BASE_LOWER=$(echo "$PROTO_BASE" | tr '[:upper:]' '[:lower:]')
if [[ "$PROTOFILE" != "${PROTO_BASE_LOWER}.proto" ]]; then mv "$PROTOFILE" "${PROTO_BASE_LOWER}.proto"; PROTOFILE="${PROTO_BASE_LOWER}.proto"; fi

CPP_PROTO_DIR="cpp_proto"
PY_PROTO_DIR="py_proto"
mkdir -p "$CPP_PROTO_DIR" "$PY_PROTO_DIR"

echo "[+] Generate C++ protobuf (for original replay)"
protoc --cpp_out="$CPP_PROTO_DIR" "$PROTOFILE"

echo "[+] Generate Python protobuf helpers"
protoc --python_out="$PY_PROTO_DIR" "$PROTOFILE"

PROTO_MODULE="${PROTO_BASE_LOWER}_pb2"

echo "[+] Generate nanopb protobuf (for wrapper decode)"
set +e
PROTOC_GEN_NANOPB="$NANOPB_DIR/generator/protoc-gen-nanopb"
protoc --plugin=protoc-gen-nanopb="$PROTOC_GEN_NANOPB" --nanopb_out=. "$PROTOFILE"
NPB_RC=$?
set -e
if [[ $NPB_RC -ne 0 || ! -f ${PROTO_BASE_LOWER}.pb.h ]]; then
  echo "[-] nanopb codegen failed (requires Python protobuf runtime)."
  echo "    Please install: pip install protobuf"
  exit 3
fi

echo "[+] Generate wrapper with pin_wrapper_entry() and standalone main"
if [[ "$FUNC" == "process_command" ]]; then
  # Special-case: optimized wrapper for const char* command input
  python3 "$ROOT_DIR/src/enhanced_wrapper_generator.py" "$PROTOFILE" "$FUNC" "$PROTO_BASE" > wrap_gen.log 2>&1 || {
    echo "[-] Enhanced wrapper generation failed. See wrap_gen.log"; exit 4; }
else
  WRAPPER_CMD=(python3 "$ROOT_DIR/src/generate_wrapper_ast.py" "$PARSE_INPUT" "$FUNC" "$PROTO_BASE" "$PROTO_BASE" "--parser=$PARSER_USED")
  if [[ -n "$HEADERS_ABS" ]]; then
    WRAPPER_CMD+=("--headers-dir=$HEADERS_ABS")
  fi
  set +e
  "${WRAPPER_CMD[@]}" > wrap_gen.log 2>&1
  WRAP_STATUS=$?
  set -e
  if (( WRAP_STATUS != 0 )); then
    echo "[-] Wrapper generation failed. See wrap_gen.log"
    exit 4
  fi
fi

echo "[+] Compile original object (instrumented) and plain object"
CLANG_INCLUDE_ARGS=()
if [[ -n "$HEADERS_ABS" ]]; then
  CLANG_INCLUDE_ARGS+=(-I"$HEADERS_ABS")
fi
CLANG_INCLUDE_ARGS+=(-I"$ROOT_DIR/examples")
clang -fsanitize=fuzzer-no-link -c "$ORIGINAL_SRC" "${CLANG_INCLUDE_ARGS[@]}" -O2 -o original.o || {
  echo "[-] Failed compiling original"; exit 5; }
objcopy --redefine-sym main=pin_original_main original.o || true
clang -fPIC -c "$ORIGINAL_SRC" "${CLANG_INCLUDE_ARGS[@]}" -O2 -o original_plain.o || {
  echo "[-] Failed compiling original (plain)"; exit 5; }
objcopy --redefine-sym main=pin_original_main original_plain.o || true

# Optional dependency: cJSON for json_parser_logic.c
if [[ -f "$ROOT_DIR/examples/cJSON/cJSON.c" ]]; then
  echo "[+] Compiling cJSON dependency"
  clang -c "$ROOT_DIR/examples/cJSON/cJSON.c" -I"$ROOT_DIR/examples" -O2 -o cJSON.o || true
else
  echo "[i] cJSON sources not found; skipping"
fi

echo "[+] Compile nanopb runtime and wrapper"
clang -c "$NANOPB_DIR/pb_decode.c" -I"$NANOPB_DIR" -O2 -o pb_decode.o
clang -c "$NANOPB_DIR/pb_common.c" -I"$NANOPB_DIR" -O2 -o pb_common.o
clang -c "${PROTO_BASE_LOWER}.pb.c" -I"$NANOPB_DIR" -O2 -o input.nanopb.o

echo "[+] Compile wrapper object (no main) for fuzz_bytes"
clang -DPIN_WRAPPER_NO_MAIN -I"$NANOPB_DIR" -O2 -c main.c -o wrapper.o

CJSON_OBJ=""; [[ -f cJSON.o ]] && CJSON_OBJ="cJSON.o"

echo "[+] Compile C++ protobuf support"
CPP_OBJS=()
for cc in "$CPP_PROTO_DIR"/*.cc; do
  obj="$(basename "$cc" .cc).o"
  clang++ -std=c++17 -I"$CPP_PROTO_DIR" -O2 -c "$cc" -o "$obj"
  CPP_OBJS+=("$obj")
done

echo "[+] Compile reference runner (protobuf decode)"
clang++ -std=c++17 -I"$CPP_PROTO_DIR" -O2 -c reference_runner.cc -o reference_runner.o

PROTOBUF_LIBS=$(pkg-config --libs protobuf 2>/dev/null || echo "-lprotobuf -pthread")

clang++ -std=c++17 -O2 -o original_replay_bin reference_runner.o "${CPP_OBJS[@]}" original_plain.o $CJSON_OBJ $PROTOBUF_LIBS || {
  echo "[-] Failed linking original_replay_bin"; exit 6; }

echo "[+] Build normalized standalone runner (reads bytes file)"
clang -I"$NANOPB_DIR" -O2 -o normalized_bin main.c pb_decode.o pb_common.o input.nanopb.o original_plain.o $CJSON_OBJ || {
  echo "[-] Failed linking normalized_bin"; exit 6; }

echo "[+] Build libFuzzer byte harness to call pin_wrapper_entry (Stage A)"
cat > bytes_fuzz.cc <<EOF
#include <cstdint>
#include <stddef.h> // for size_t in global namespace
extern "C" int pin_wrapper_entry(const uint8_t *data, size_t len);
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  (void)pin_wrapper_entry(data, size);
  return 0;
}
EOF
clang++ -fsanitize=fuzzer,address -std=c++17 -I. -I"$NANOPB_DIR" -O2 -c bytes_fuzz.cc -o bytes_fuzz.o

echo "[+] Link fuzz_bytes"
# clang++ -fsanitize=fuzzer,address -O2 \
#   -o fuzz_bytes bytes_fuzz.o pb_decode.o pb_common.o input.nanopb.o original.o $CJSON_OBJ \
#   -lpthread
clang++ -fsanitize=fuzzer,address -O2 \
    -o fuzz_bytes bytes_fuzz.o wrapper.o pb_decode.o pb_common.o input.nanopb.o original.o $CJSON_OBJ \
    -lpthread

if [[ "$FUZZ_SECONDS" != "0" ]]; then
  echo "[+] Stage A: libFuzzer discovery for ${FUZZ_SECONDS}s"
  mkdir -p corpus artifacts
  FUZZ_CMD=("./fuzz_bytes" "corpus" -max_total_time="$FUZZ_SECONDS" -use_value_profile=1 -print_final_stats=1 -artifact_prefix="$BUILD_DIR/artifacts/")
  if [[ -n "$FUZZ_EXTRA_FLAGS" ]]; then
    # shellcheck disable=SC2206
    EXTRA_ARR=($FUZZ_EXTRA_FLAGS)
    FUZZ_CMD+=("${EXTRA_ARR[@]}")
  fi
  echo "    Running: ${FUZZ_CMD[*]}"
  "${FUZZ_CMD[@]}" || echo "[i] Stage A fuzzing exited with status $?"
fi

echo "[+] Stage B: differential replay"
STAGE_B_DIR="$RESULTS_DIR/stage_b"
mkdir -p "$STAGE_B_DIR"

if [[ -z "$REPLAY_DIR" ]]; then
  REPLAY_INPUT_DIR="$BUILD_DIR/corpus"
else
  REPLAY_INPUT_DIR="$REPLAY_DIR"
fi

if [[ ! -d "$REPLAY_INPUT_DIR" ]]; then
  echo "[i] Stage B: replay directory $REPLAY_INPUT_DIR not found; skipping"
else
  shopt -s nullglob
  inputs=("$REPLAY_INPUT_DIR"/*)
  if (( ${#inputs[@]} == 0 )); then
    echo "[i] Stage B: no inputs in $REPLAY_INPUT_DIR"
  else
    REPORT="$STAGE_B_DIR/replay_summary.txt"
    OUTPUT_LOG="$STAGE_B_DIR/replay_outputs.txt"
    : > "$REPORT"
    : > "$OUTPUT_LOG"
    for input_path in "${inputs[@]}"; do
      base=$(basename "$input_path")
      norm_out="$STAGE_B_DIR/${base}.normalized.out"
      norm_err="$STAGE_B_DIR/${base}.normalized.err"
      orig_out="$STAGE_B_DIR/${base}.original.out"
      orig_err="$STAGE_B_DIR/${base}.original.err"

      set +e
      ./normalized_bin "$input_path" >"$norm_out" 2>"$norm_err"
      norm_rc=$?
      ./original_replay_bin "$input_path" >"$orig_out" 2>"$orig_err"
      orig_rc=$?
      set -e

      status="match"
      if ! cmp -s "$norm_out" "$orig_out" || ! cmp -s "$norm_err" "$orig_err" || [[ $norm_rc -ne $orig_rc ]]; then
        status="DIFF"
      fi
      printf "%s\tRC(norm=%d, orig=%d)\n" "$base:$status" "$norm_rc" "$orig_rc" >> "$REPORT"

      set +e
      DECODED_JSON=$(PY_PROTO_DIR="$PY_PROTO_DIR" PROTO="$PROTO_BASE" MODULE="$PROTO_MODULE" INPUT_PATH="$input_path" python3 - <<'PY' 2>&1
import json
import os
import sys
from google.protobuf.descriptor import FieldDescriptor

sys.path.insert(0, os.environ["PY_PROTO_DIR"])
module = __import__(os.environ["MODULE"])
msg_cls = getattr(module, os.environ["PROTO"])
msg = msg_cls()
with open(os.environ["INPUT_PATH"], "rb") as fh:
    msg.ParseFromString(fh.read())

def normalize_value(field, value):
    if field.type == FieldDescriptor.TYPE_MESSAGE:
        return message_to_dict(value)
    if field.type == FieldDescriptor.TYPE_BYTES:
        try:
            decoded = value.decode("utf-8")
            if decoded.isprintable() or decoded == "":
                return decoded
        except UnicodeDecodeError:
            pass
        return value.hex()
    return value

def message_to_dict(message):
    result = {}
    for field in message.DESCRIPTOR.fields:
        val = getattr(message, field.name)
        if field.label == FieldDescriptor.LABEL_REPEATED:
            result[field.name] = [normalize_value(field, item) for item in val]
        else:
            result[field.name] = normalize_value(field, val)
    return result

print(json.dumps(message_to_dict(msg), sort_keys=True))
PY
)
      DECODE_STATUS=$?
      set -e
      if (( DECODE_STATUS != 0 )); then
        DECODED_JSON="{\"error\": \"decode failed (rc=$DECODE_STATUS)\"}"
      fi

      {
        printf "=== %s ===\n" "$base"
        printf "[input decoded]\n%s\n" "$DECODED_JSON"
        printf "[normalized rc=%d stdout]\n" "$norm_rc"
        cat "$norm_out"
        printf "[normalized stderr]\n"
        cat "$norm_err"
        printf "[original rc=%d stdout]\n" "$orig_rc"
        cat "$orig_out"
        printf "[original stderr]\n"
        cat "$orig_err"
        printf "\n"
      } >> "$OUTPUT_LOG"
    done
    total_inputs=${#inputs[@]}
    echo "[+] Stage B replay summary saved to $REPORT (inputs=$total_inputs)"
    echo "[+] Stage B detailed outputs saved to $OUTPUT_LOG"
  fi
fi

echo "[+] Ready. Next steps:"
echo "    1) Stage A (discovery): cd $BUILD_DIR && ./fuzz_bytes corpus -max_total_time=60 -use_value_profile=1"
echo "    2) Stage B (replay): add inputs under $REPLAY_INPUT_DIR and rerun to compare normalized vs original outputs"
echo "       Replay artifacts stored under $STAGE_B_DIR"
