#!/usr/bin/env bash
# PIN Differential Fuzzing (Stage A scaffold)
#
# Usage: pin/src/pin_diff.sh <c_file> <function_name> [--headers-dir=DIR]
#        [--extra-sources=PATH[,PATH...]]
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

PIN_EMI_REJECT_RC=86

CFILE=${1:-}
FUNC=${2:-}
shift 2 || true

HEADERS_DIR=""
REPLAY_DIR=""
FUZZ_SECONDS=0
FUZZ_EXTRA_FLAGS=""
REFERENCE_DECODER="nanopb"
LIBS=""
EXTRA_SOURCES_RAW=""
INPUT_MODE="proto"
PASS_THROUGH_HEADER=""
GENERATE_SEEDS=0
GENERATE_SEEDS_COUNT=512
for arg in "$@"; do
  case "$arg" in
    --headers-dir=*) HEADERS_DIR="${arg#*=}" ;;
    --replay-dir=*) REPLAY_DIR="${arg#*=}" ;;
    --fuzz-seconds=*) FUZZ_SECONDS="${arg#*=}" ;;
    --fuzz-flags=*) FUZZ_EXTRA_FLAGS="${arg#*=}" ;;
    --reference-decoder=*) REFERENCE_DECODER="${arg#*=}" ;;
    --libs=*) LIBS="${arg#*=}" ;;
    --extra-sources=*) EXTRA_SOURCES_RAW="${arg#*=}" ;;
    --input-mode=*) INPUT_MODE="${arg#*=}" ;;
    --pass-through) INPUT_MODE="raw" ;;
    --pass-through-header=*) PASS_THROUGH_HEADER="${arg#*=}" ;;
    --generate-seeds) GENERATE_SEEDS=1 ;;
    --generate-seeds=*) GENERATE_SEEDS=1; GENERATE_SEEDS_COUNT="${arg#*=}" ;;
    --seed-count=*) GENERATE_SEEDS=1; GENERATE_SEEDS_COUNT="${arg#*=}" ;;
  esac
done

PIN_EXTRA_CFLAGS_ARR=()
PIN_EXTRA_LDFLAGS_ARR=()
if [[ -n "${PIN_EXTRA_CFLAGS:-}" ]]; then
  read -r -a PIN_EXTRA_CFLAGS_ARR <<< "${PIN_EXTRA_CFLAGS}"
fi
if [[ -n "${PIN_EXTRA_LDFLAGS:-}" ]]; then
  read -r -a PIN_EXTRA_LDFLAGS_ARR <<< "${PIN_EXTRA_LDFLAGS}"
fi

if [[ "$REFERENCE_DECODER" != "cpp" && "$REFERENCE_DECODER" != "nanopb" ]]; then
  echo "[-] Unsupported reference decoder: $REFERENCE_DECODER (use cpp or nanopb)"
  exit 1
fi

if [[ "$INPUT_MODE" != "proto" && "$INPUT_MODE" != "raw" ]]; then
  echo "[-] Unsupported input mode: $INPUT_MODE (use proto or raw)"
  exit 1
fi
PASS_THROUGH_ENABLED=0
if [[ "$INPUT_MODE" == "raw" ]]; then
  PASS_THROUGH_ENABLED=1
fi

if [[ -z "$CFILE" || -z "$FUNC" ]]; then
  echo "Usage: pin/src/pin_diff.sh <c_file> <function_name> [--headers-dir=DIR] [--replay-dir=DIR] [--fuzz-seconds=N] [--fuzz-flags=FLAGS] [--reference-decoder={cpp|nanopb}] [--libs=LIBS] [--extra-sources=PATH[,PATH...]] [--input-mode={proto|raw}] [--pass-through-header=HEADER] [--generate-seeds[=N]]"
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

if [[ "$CFILE" = /* ]]; then
  ORIGINAL_SRC="$CFILE"
elif [[ -f "$ROOT_DIR/$CFILE" ]]; then
  ORIGINAL_SRC="$ROOT_DIR/$CFILE"
else
  ORIGINAL_SRC="$(realpath -m "$CFILE" 2>/dev/null || echo "")"
fi

if [[ -z "$ORIGINAL_SRC" || ! -f "$ORIGINAL_SRC" ]]; then
  echo "[-] Source file $CFILE not found"
  exit 1
fi

EXAMPLE_NAME=$(basename "$ORIGINAL_SRC" .c)
BUILD_DIR="$ROOT_DIR/build/${EXAMPLE_NAME}_diff"
RESULTS_DIR="$ROOT_DIR/results/${EXAMPLE_NAME}_diff"
NANOPB_DIR="$ROOT_DIR/nanopb"
FAKE_INC_DIR="$ROOT_DIR/utils/fake_headers"

normalize_path() {
  local path="$1"
  if [[ "$path" = /* ]]; then
    echo "$path"
  else
    echo "$ROOT_DIR/$path"
  fi
}

if [[ -n "${PIN_BUILD_DIR:-}" ]]; then
  BUILD_DIR="$(normalize_path "$PIN_BUILD_DIR")"
fi

if [[ -n "${PIN_RESULTS_DIR:-}" ]]; then
  RESULTS_DIR="$(normalize_path "$PIN_RESULTS_DIR")"
fi

HEADERS_ABS=""
if [[ -n "$HEADERS_DIR" ]]; then
  if [[ "$HEADERS_DIR" = /* ]]; then
    HEADERS_ABS="$HEADERS_DIR"
  elif [[ -d "$ROOT_DIR/$HEADERS_DIR" ]]; then
    HEADERS_ABS="$ROOT_DIR/$HEADERS_DIR"
  else
    HEADERS_ABS="$(realpath -m "$HEADERS_DIR" 2>/dev/null || echo "")"
  fi
  if [[ -n "$HEADERS_ABS" && ! -d "$HEADERS_ABS" ]]; then
    echo "[-] Headers directory $HEADERS_DIR not found"
    exit 1
  fi
fi

HEADERS_FLAG=()
HEADERS_INCLUDES=()
if [[ -n "$HEADERS_ABS" ]]; then
  HEADERS_FLAG=("--headers-dir=$HEADERS_ABS")
  HEADERS_INCLUDES=(-I"$HEADERS_ABS")
fi

resolve_source_path() {
  local source_path="$1"
  local resolved=""
  if [[ "$source_path" = /* ]]; then
    resolved="$source_path"
  elif [[ -f "$ROOT_DIR/$source_path" ]]; then
    resolved="$ROOT_DIR/$source_path"
  else
    resolved="$(realpath -m "$source_path" 2>/dev/null || echo "")"
  fi
  if [[ -z "$resolved" || ! -f "$resolved" ]]; then
    echo ""
  else
    echo "$resolved"
  fi
}

EXTRA_SOURCES=()
if [[ -n "$EXTRA_SOURCES_RAW" ]]; then
  IFS=',' read -r -a extra_arr <<< "$EXTRA_SOURCES_RAW"
  for item in "${extra_arr[@]}"; do
    [[ -z "$item" ]] && continue
    resolved=$(resolve_source_path "$item")
    if [[ -z "$resolved" ]]; then
      echo "[-] Extra source $item not found"
      exit 1
    fi
    EXTRA_SOURCES+=("$resolved")
  done
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

calc_sha1() {
  local file="$1"
  if command -v sha1sum >/dev/null 2>&1; then
    sha1sum "$file" | awk '{print $1}'
  elif command -v shasum >/dev/null 2>&1; then
    shasum "$file" | awk '{print $1}'
  else
    echo "[-] Neither sha1sum nor shasum found; cannot hash crashes" >&2
    return 1
  fi
}

mkdir -p "$BUILD_DIR/corpus" "$RESULTS_DIR"
cd "$BUILD_DIR"

if (( GENERATE_SEEDS == 1 )) && compgen -G "seed_corpus/*.bin" > /dev/null; then
  SEED_COUNT_PRIME=$(find seed_corpus -maxdepth 1 -name '*.bin' | wc -l | tr -d ' ')
  if (( SEED_COUNT_PRIME > 0 )); then
    echo "[+] Priming corpus with ${SEED_COUNT_PRIME} intelligent seeds"
    cp seed_corpus/*.bin corpus/ 2>/dev/null || true
  fi
fi

PARSER_USED="libclang"
PARSE_INPUT="$ORIGINAL_SRC"
PROTO_BASE=""
PROTO_BASE_LOWER=""
PROTO_MODULE=""
CPP_PROTO_DIR=""
PY_PROTO_DIR=""

if (( PASS_THROUGH_ENABLED == 0 )); then
  echo "[+] Generate .proto (libclang parser)"
  run_proto_generation "$PARSER_USED" "$PARSE_INPUT"

  if (( PROTO_STATUS != 0 || ${#PROTO_FILES[@]} == 0 )); then
    echo "[-] Proto generation failed (libclang). See proto_gen.log"
    exit 2
  fi
  if [[ -f input.proto ]]; then
    PROTOFILE="input.proto"
  else
    PROTOFILE="${PROTO_FILES[0]}"
  fi
  PROTO_BASE=$(awk '/^message /{print $2; exit}' "$PROTOFILE")
  [[ -z "$PROTO_BASE" ]] && PROTO_BASE=Input
  if grep -q '^message Input ' "$PROTOFILE"; then
    PROTO_BASE=Input
  fi
  PROTO_BASE_LOWER=$(echo "$PROTO_BASE" | tr '[:upper:]' '[:lower:]')
  if [[ "$PROTOFILE" != "${PROTO_BASE_LOWER}.proto" ]]; then mv "$PROTOFILE" "${PROTO_BASE_LOWER}.proto"; PROTOFILE="${PROTO_BASE_LOWER}.proto"; fi

  CPP_PROTO_DIR="cpp_proto"
  PY_PROTO_DIR="py_proto"
  [[ "$REFERENCE_DECODER" == "cpp" ]] && mkdir -p "$CPP_PROTO_DIR"
  mkdir -p "$PY_PROTO_DIR"

  rm -f *.pb.c *.pb.h
  if [[ "$REFERENCE_DECODER" == "cpp" ]]; then
    rm -f "$CPP_PROTO_DIR"/*.pb.cc "$CPP_PROTO_DIR"/*.pb.h
  fi

  if [[ "$REFERENCE_DECODER" == "cpp" ]]; then
    echo "[+] Generate C++ protobuf (for original replay)"
    protoc --cpp_out="$CPP_PROTO_DIR" "$PROTOFILE"
  else
    echo "[i] Skipping C++ protobuf generation (reference decoder=$REFERENCE_DECODER)"
  fi

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

  if (( GENERATE_SEEDS == 1 )); then
    echo "[+] Generating intelligent seed corpus (max ${GENERATE_SEEDS_COUNT})"
    python3 "$ROOT_DIR/src/generate_intelligent_seeds.py" \
      --py-proto-dir "$PY_PROTO_DIR" \
      --module "$PROTO_MODULE" \
      --message "$PROTO_BASE" \
      --output seed_corpus \
      --max-seeds "$GENERATE_SEEDS_COUNT" \
      --clean-output
  fi
else
  echo "[+] Pass-through mode enabled: skipping protobuf generation"
fi

echo "[+] Generate wrapper with pin_wrapper_entry() and standalone main"
if (( PASS_THROUGH_ENABLED == 1 )); then
  WRAPPER_CMD=(python3 "$ROOT_DIR/src/generate_pass_through_wrapper.py" "$PARSE_INPUT" "$FUNC")
  if [[ -n "$HEADERS_ABS" ]]; then
    WRAPPER_CMD+=("--headers-dir=$HEADERS_ABS")
  fi
  if [[ -n "$PASS_THROUGH_HEADER" ]]; then
    WRAPPER_CMD+=("--include=$PASS_THROUGH_HEADER")
  fi
  set +e
  "${WRAPPER_CMD[@]}" > wrap_gen.log 2>&1
  WRAP_STATUS=$?
  set -e
  if (( WRAP_STATUS != 0 )); then
    echo "[-] Pass-through wrapper generation failed. See wrap_gen.log"
    exit 4
  fi
else
  if [[ "$FUNC" == "process_command" ]]; then
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
fi

echo "[+] Compile original object (instrumented) and plain object"
CLANG_INCLUDE_ARGS=()
if [[ -n "$HEADERS_ABS" ]]; then
  CLANG_INCLUDE_ARGS+=(-I"$HEADERS_ABS")
fi
CLANG_INCLUDE_ARGS+=(-I"$ROOT_DIR/examples")
clang -fsanitize=fuzzer-no-link -c "$ORIGINAL_SRC" "${PIN_EXTRA_CFLAGS_ARR[@]}" "${CLANG_INCLUDE_ARGS[@]}" -O2 -o original.o || {
  echo "[-] Failed compiling original"; exit 5; }
objcopy --redefine-sym main=pin_original_main original.o || true
clang -fPIC -c "$ORIGINAL_SRC" "${PIN_EXTRA_CFLAGS_ARR[@]}" "${CLANG_INCLUDE_ARGS[@]}" -O2 -o original_plain.o || {
  echo "[-] Failed compiling original (plain)"; exit 5; }
objcopy --redefine-sym main=pin_original_main original_plain.o || true

EXTRA_INST_OBJS=()
EXTRA_PLAIN_OBJS=()
if (( ${#EXTRA_SOURCES[@]} > 0 )); then
  echo "[+] Compile extra sources (${#EXTRA_SOURCES[@]})"
  idx=0
  for extra_src in "${EXTRA_SOURCES[@]}"; do
    extra_base=$(basename "$extra_src")
    extra_stem="${extra_base%.*}"
    inst_obj="extra_${idx}_${extra_stem}.instrumented.o"
    plain_obj="extra_${idx}_${extra_stem}.plain.o"
    clang -fsanitize=fuzzer-no-link -c "$extra_src" "${PIN_EXTRA_CFLAGS_ARR[@]}" "${CLANG_INCLUDE_ARGS[@]}" -O2 -o "$inst_obj" || {
      echo "[-] Failed compiling extra source (instrumented): $extra_src"; exit 5; }
    objcopy --redefine-sym main=pin_original_main "$inst_obj" || true
    clang -fPIC -c "$extra_src" "${PIN_EXTRA_CFLAGS_ARR[@]}" "${CLANG_INCLUDE_ARGS[@]}" -O2 -o "$plain_obj" || {
      echo "[-] Failed compiling extra source (plain): $extra_src"; exit 5; }
    objcopy --redefine-sym main=pin_original_main "$plain_obj" || true
    EXTRA_INST_OBJS+=("$inst_obj")
    EXTRA_PLAIN_OBJS+=("$plain_obj")
    idx=$((idx + 1))
  done
fi

if (( PASS_THROUGH_ENABLED == 0 )); then
  echo "[+] Compile nanopb runtime and wrapper"
  clang -c "$NANOPB_DIR/pb_decode.c" -I"$NANOPB_DIR" "${PIN_EXTRA_CFLAGS_ARR[@]}" -O2 -o pb_decode.o
  clang -c "$NANOPB_DIR/pb_common.c" -I"$NANOPB_DIR" "${PIN_EXTRA_CFLAGS_ARR[@]}" -O2 -o pb_common.o
  clang -c "${PROTO_BASE_LOWER}.pb.c" -I"$NANOPB_DIR" "${PIN_EXTRA_CFLAGS_ARR[@]}" -O2 -o input.nanopb.o
else
  echo "[i] Pass-through mode: nanopb runtime not required"
fi

echo "[+] Compile wrapper object (no main) for fuzz_bytes"
clang -DPIN_WRAPPER_NO_MAIN -I"$NANOPB_DIR" "${HEADERS_INCLUDES[@]}" "${PIN_EXTRA_CFLAGS_ARR[@]}" -O2 -c main.c -o wrapper.o

REF_BIN=""
REF_LABEL=""

if (( PASS_THROUGH_ENABLED == 0 )); then
  if [[ "$REFERENCE_DECODER" == "cpp" ]]; then
    echo "[+] Compile C++ protobuf support"
    CPP_OBJS=()
    for cc in "$CPP_PROTO_DIR"/*.cc; do
      obj="$(basename "$cc" .cc).o"
      clang++ -std=c++17 -I"$CPP_PROTO_DIR" "${HEADERS_INCLUDES[@]}" "${PIN_EXTRA_CFLAGS_ARR[@]}" -O2 -c "$cc" -o "$obj"
      CPP_OBJS+=("$obj")
    done

    echo "[+] Compile reference runner (protobuf decode)"
    clang++ -std=c++17 -I"$CPP_PROTO_DIR" "${HEADERS_INCLUDES[@]}" "${PIN_EXTRA_CFLAGS_ARR[@]}" -O2 -c reference_runner.cc -o reference_runner.o

    PROTOBUF_LIBS=$(pkg-config --libs protobuf 2>/dev/null || echo "-lprotobuf -pthread")

    clang++ -std=c++17 -O2 -o original_replay_bin reference_runner.o "${CPP_OBJS[@]}" original_plain.o "${EXTRA_PLAIN_OBJS[@]}" $PROTOBUF_LIBS "${PIN_EXTRA_LDFLAGS_ARR[@]}" $LIBS || {
      echo "[-] Failed linking original_replay_bin"; exit 6; }
    REF_BIN="./original_replay_bin"
    REF_LABEL="original"
  else
    echo "[+] Build nanopb reference replay binary"
    cat > reference_nanopb_main.c <<'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

extern int pin_wrapper_entry(const uint8_t *data, size_t len);

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s input.bin\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(f);
        return 1;
    }

    long len = ftell(f);
    if (len < 0) {
        perror("ftell");
        fclose(f);
        return 1;
    }
    rewind(f);

    uint8_t *buf = malloc((size_t)len);
    if (!buf) {
        perror("malloc");
        fclose(f);
        return 1;
    }

    size_t read_len = fread(buf, 1, (size_t)len, f);
    if (read_len != (size_t)len) {
        perror("fread");
        free(buf);
        fclose(f);
        return 1;
    }
    fclose(f);

    int rc = pin_wrapper_entry(buf, (size_t)len);
    free(buf);
    return rc;
}
EOF
    clang -I"$NANOPB_DIR" "${HEADERS_INCLUDES[@]}" -O2 -o reference_nanopb_bin reference_nanopb_main.c wrapper.o pb_decode.o pb_common.o input.nanopb.o original_plain.o "${EXTRA_PLAIN_OBJS[@]}" "${PIN_EXTRA_LDFLAGS_ARR[@]}" $LIBS || {
      echo "[-] Failed linking reference_nanopb_bin"; exit 6; }
    rm -f reference_nanopb_main.c
    REF_BIN="./reference_nanopb_bin"
    REF_LABEL="reference_npb"
  fi
else
  echo "[i] Pass-through mode: reference replay binary skipped"
fi

echo "[+] Build normalized standalone runner (reads bytes file)"
if (( PASS_THROUGH_ENABLED == 0 )); then
  clang -I"$NANOPB_DIR" "${HEADERS_INCLUDES[@]}" -O2 -o normalized_bin main.c pb_decode.o pb_common.o input.nanopb.o original_plain.o "${EXTRA_PLAIN_OBJS[@]}" "${PIN_EXTRA_LDFLAGS_ARR[@]}" $LIBS || {
    echo "[-] Failed linking normalized_bin"; exit 6; }
else
  clang "${HEADERS_INCLUDES[@]}" "${PIN_EXTRA_CFLAGS_ARR[@]}" -O2 -o normalized_bin main.c original_plain.o "${EXTRA_PLAIN_OBJS[@]}" "${PIN_EXTRA_LDFLAGS_ARR[@]}" $LIBS || {
    echo "[-] Failed linking normalized_bin"; exit 6; }
fi

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
clang++ -fsanitize=fuzzer,address -std=c++17 -I. -I"$NANOPB_DIR" "${PIN_EXTRA_CFLAGS_ARR[@]}" -O2 -c bytes_fuzz.cc -o bytes_fuzz.o

echo "[+] Link fuzz_bytes"
if (( PASS_THROUGH_ENABLED == 0 )); then
  clang++ -fsanitize=fuzzer,address -O2 \
      -o fuzz_bytes bytes_fuzz.o wrapper.o pb_decode.o pb_common.o input.nanopb.o original.o "${EXTRA_INST_OBJS[@]}" \
      -lpthread "${PIN_EXTRA_LDFLAGS_ARR[@]}" $LIBS
else
  clang++ -fsanitize=fuzzer,address -O2 \
      -o fuzz_bytes bytes_fuzz.o wrapper.o original.o "${EXTRA_INST_OBJS[@]}" \
      -lpthread "${PIN_EXTRA_LDFLAGS_ARR[@]}" $LIBS
fi

if [[ "$FUZZ_SECONDS" != "0" ]]; then
  echo "[+] Stage A: libFuzzer discovery for ${FUZZ_SECONDS}s"
  mkdir -p corpus artifacts

  if compgen -G "$ROOT_DIR/seed_corpus/*" > /dev/null; then
    cp -n "$ROOT_DIR"/seed_corpus/* corpus/ 2>/dev/null || true
  fi

  FUZZ_CMD=("./fuzz_bytes" "corpus" -max_total_time="$FUZZ_SECONDS" -use_value_profile=1 -print_final_stats=1 -artifact_prefix="$BUILD_DIR/artifacts/" -ignore_crashes=1)
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

declare -A STAGE_B_STATUS
declare -A STAGE_B_NORM_RC
declare -A STAGE_B_REF_RC

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
      ref_out="$STAGE_B_DIR/${base}.${REF_LABEL}.out"
      ref_err="$STAGE_B_DIR/${base}.${REF_LABEL}.err"

      set +e
      ./normalized_bin "$input_path" >"$norm_out" 2>"$norm_err"
      norm_rc=$?
      if [[ -n "$REF_BIN" ]]; then
        "$REF_BIN" "$input_path" >"$ref_out" 2>"$ref_err"
        ref_rc=$?
        has_reference=1
      else
        echo "[pass-through reference skipped]" > "$ref_out"
        echo "[pass-through reference skipped]" > "$ref_err"
        ref_rc=$norm_rc
        has_reference=0
      fi
      set -e

      status="match"
      reason_note=""
      if [[ $norm_rc -eq $PIN_EMI_REJECT_RC ]]; then
        status="emi-reject"
        reason_line=$(grep -m1 '\[PIN_EMI\]' "$norm_err" || true)
        if [[ -n "$reason_line" ]]; then
          reason_line=${reason_line//$'\r'/}
          reason_line=${reason_line//$'\n'/}
          reason_note=" reason=${reason_line}"
        fi
      elif (( has_reference )) && { ! cmp -s "$norm_out" "$ref_out" || ! cmp -s "$norm_err" "$ref_err" || [[ $norm_rc -ne $ref_rc ]]; }; then
        status="DIFF"
      fi
      ref_display="$ref_rc"
      if [[ -z "$REF_BIN" ]]; then
        ref_display="n/a"
      fi
      printf "%s\tRC(norm=%d, ref=%s)%s\n" "$base:$status" "$norm_rc" "$ref_display" "$reason_note" >> "$REPORT"

      STAGE_B_STATUS["$base"]=$status
      STAGE_B_NORM_RC["$base"]=$norm_rc
      if [[ -z "$REF_BIN" ]]; then
        STAGE_B_REF_RC["$base"]="n/a"
      else
        STAGE_B_REF_RC["$base"]=$ref_rc
      fi

      if (( PASS_THROUGH_ENABLED == 0 )); then
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
      else
        DECODED_JSON=$(python3 - "$input_path" <<'PY'
import json
import sys
from pathlib import Path

data = Path(sys.argv[1]).read_bytes()
preview = data[:64].hex()
print(json.dumps({"len": len(data), "preview_hex": preview}))
PY
)
      fi

      {
        printf "=== %s ===\n" "$base"
        printf "[input decoded]\n%s\n" "$DECODED_JSON"
        printf "[normalized rc=%d stdout]\n" "$norm_rc"
        cat "$norm_out"
        printf "[normalized stderr]\n"
        cat "$norm_err"
        if [[ -n "$REF_LABEL" ]]; then
          printf "[%s rc=%d stdout]\n" "$REF_LABEL" "$ref_rc"
          cat "$ref_out"
          printf "[%s stderr]\n" "$REF_LABEL"
          cat "$ref_err"
        else
          printf "[reference stdout]\n"
          cat "$ref_out"
          printf "[reference stderr]\n"
          cat "$ref_err"
        fi
        printf "\n"
      } >> "$OUTPUT_LOG"
    done
    total_inputs=${#inputs[@]}
    echo "[+] Stage B replay summary saved to $REPORT (inputs=$total_inputs)"
    echo "[+] Stage B detailed outputs saved to $OUTPUT_LOG"
  fi
fi

echo "[+] Stage C: crash harvesting"
STAGE_C_DIR="$RESULTS_DIR/stage_c"
mkdir -p "$STAGE_C_DIR/crashes"
RUN_TAG=$(date -u +"%Y%m%dT%H%M%SZ")
CRASH_INDEX="$STAGE_C_DIR/crash_index.tsv"
if [[ ! -f "$CRASH_INDEX" ]]; then
  echo -e "stored_at_utc\tsha1\toriginal_name\tstage_b_status\tstage_b_norm_rc\tstage_b_ref_rc" > "$CRASH_INDEX"
fi

shopt -s nullglob
crash_files=("$BUILD_DIR"/artifacts/crash-*)
shopt -u nullglob
if (( ${#crash_files[@]} == 0 )); then
  echo "[i] Stage C: no crashes to archive from $BUILD_DIR/artifacts"
else
  for crash_file in "${crash_files[@]}"; do
    [[ -f "$crash_file" ]] || continue
    sha=$(calc_sha1 "$crash_file") || continue
    base=$(basename "$crash_file")
    dest_dir="$STAGE_C_DIR/crashes/$sha"
    stage_status="${STAGE_B_STATUS[$base]:-n/a}"
    stage_norm="${STAGE_B_NORM_RC[$base]:-n/a}"
    stage_ref="${STAGE_B_REF_RC[$base]:-n/a}"

    if [[ -d "$dest_dir" ]]; then
      echo "[i] Stage C: crash $base already archived under $sha"
      continue
    fi

    mkdir -p "$dest_dir"
    cp "$crash_file" "$dest_dir/$base"
    stored_at=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    cat > "$dest_dir/metadata.txt" <<EOF
original_name: $base
sha1: $sha
stored_at_utc: $stored_at
stage_b_status: $stage_status
stage_b_norm_rc: $stage_norm
stage_b_ref_rc: $stage_ref
EOF
    echo -e "$stored_at\t$sha\t$base\t$stage_status\t$stage_norm\t$stage_ref" >> "$CRASH_INDEX"
    echo "[+] Stage C: archived crash $base (sha1=$sha)"
  done
fi

if compgen -G "$BUILD_DIR/corpus/*" > /dev/null; then
  SNAP_DIR="$STAGE_C_DIR/corpus/run_$RUN_TAG"
  mkdir -p "$SNAP_DIR"
  cp -a "$BUILD_DIR/corpus/." "$SNAP_DIR/" 2>/dev/null || true
  echo "[+] Stage C: captured corpus snapshot at $SNAP_DIR"
else
  echo "[i] Stage C: corpus directory empty; skipping snapshot"
fi

echo "[+] Ready. Next steps:"
echo "    1) Stage A (discovery): cd $BUILD_DIR && ./fuzz_bytes corpus -max_total_time=60 -use_value_profile=1"
echo "    2) Stage B (replay): add inputs under $REPLAY_INPUT_DIR and rerun to compare normalized vs original outputs"
echo "       Replay artifacts stored under $STAGE_B_DIR"
echo "    3) Stage C (crash vault): review archived crashes under $STAGE_C_DIR"
