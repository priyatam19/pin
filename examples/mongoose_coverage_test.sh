#!/usr/bin/env bash
# Run PIN differential fuzzing against ranked functions and capture coverage metrics.
# Supports both the original Mongoose workflow and the libtiff port.
#
# Usage:
#   ./mongoose_coverage_test.sh \
#       [--top-n=30] [--start-rank=1] [--fuzz-seconds=60] [--append] \
#       [--lib=mongoose|libtiff] [--function-list=/path/to/list.txt] \
#       [--libtiff-root=/path/to/libtiff]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PIN_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
MONGOOSE_SRC="$SCRIPT_DIR/mongoose.c"
MONGOOSE_HDR="$PIN_ROOT/utils/user_headers/mongoose.h"

# Built-in Mongoose target order (from fuzzability analysis)
DEFAULT_MONGOOSE_TARGETS=(
  "mg_log_set"
  "mg_ntohl"
  "mg_ntohs"
  "mg_timer_expired"
  "mg_base64_update"
  "mg_crc32"
  "mg_mqtt_parse"
  "mg_url_decode"
  "mg_base64_final"
  "mg_check_ip_acl"
  "mg_http_get_request_len"
  "mg_md5_final"
  "mg_sha1_final"
  "mg_sntp_parse"
  "mg_str_n"
  "mg_unhexn"
  "mg_dns_parse"
  "mg_iobuf_del"
  "mg_iobuf_init"
  "mg_ws_wrap"
  "mg_globmatch"
  "mg_url_encode"
  "mg_log_prefix"
  "mg_base64_decode"
  "mg_base64_encode"
  "mg_json_get"
  "mg_json_get_bool"
  "mg_json_get_long"
  "mg_json_get_num"
  "mg_mqtt_next_sub"
  "mg_md5_update"
  "mg_ncasecmp"
  "mg_sha1_update"
  "mg_unhex"
  "mg_http_creds"
  "mg_hexdump"
  "mg_iobuf_resize"
  "mg_mgr_poll"
  "mg_queue_add"
  "mg_queue_del"
  "mg_random"
  "mg_timer_poll"
  "mg_iobuf_add"
  "mg_ws_send"
  "mg_http_next_multipart"
  "mg_http_parse"
  "mg_http_write_chunk"
  "mg_queue_book"
  "mg_queue_init"
  "mg_http_get_var"
)

# libtiff source mapping for top-ranked functions
declare -A LIBTIFF_SOURCE_MAP=(
  [LogL10toY]="tif_luv.c"
  [LogL16toY]="tif_luv.c"
  [TIFFDataWidth]="tif_dirinfo.c"
  [TIFFFindCODEC]="tif_codec.c"
  [TIFFGetBitRevTable]="tif_swab.c"
  [TIFFIsCODECConfigured]="tif_codec.c"
  [_TIFFmalloc]="tif_unix.c"
  [LogL10fromY]="tif_luv.c"
  [LogL16fromY]="tif_luv.c"
  [uv_encode]="tif_luv.c"
  [TIFFReverseBits]="tif_swab.c"
  [TIFFSwabArrayOfTriples]="tif_swab.c"
  [TIFFGetConfiguredCODECs]="tif_codec.c"
  [TIFFGetVersion]="tif_version.c"
  [TIFFFieldDataType]="tif_dirinfo.c"
  [TIFFFieldName]="tif_dirinfo.c"
  [TIFFFieldPassCount]="tif_dirinfo.c"
  [TIFFFieldReadCount]="tif_dirinfo.c"
  [TIFFFieldTag]="tif_dirinfo.c"
  [TIFFFieldWriteCount]="tif_dirinfo.c"
  [TIFFSwabDouble]="tif_swab.c"
  [TIFFSwabFloat]="tif_swab.c"
  [TIFFSwabLong]="tif_swab.c"
  [TIFFSwabLong8]="tif_swab.c"
  [TIFFSwabShort]="tif_swab.c"
  [TIFFUnRegisterCODEC]="tif_codec.c"
  [_TIFFfree]="tif_unix.c"
  [LogLuv24fromXYZ]="tif_luv.c"
  [LogLuv24toXYZ]="tif_luv.c"
  [LogLuv32fromXYZ]="tif_luv.c"
)

LIBTIFF_LOG_LUV_FUNCS=(
  "LogL10toY" "LogL16toY" "LogL10fromY" "LogL16fromY" "uv_encode"
  "LogLuv24fromXYZ" "LogLuv24toXYZ" "LogLuv32fromXYZ"
)
LIBTIFF_FIELD_FUNCS=(
  "TIFFFieldDataType" "TIFFFieldName" "TIFFFieldPassCount"
  "TIFFFieldReadCount" "TIFFFieldTag" "TIFFFieldWriteCount"
)

is_in_set() {
  local needle="$1"; shift
  for item in "$@"; do
    [[ "$item" == "$needle" ]] && return 0
  done
  return 1
}

MODE="mongoose"
TARGET_FILE=""
TOP_N=30
START_RANK=1
FUZZ_SECONDS=60
APPEND=0
LIBTIFF_ROOT_OVERRIDE=""

while [[ $# -gt 0 ]]; do
  case $1 in
    --top-n=*) TOP_N="${1#*=}" ;;
    --fuzz-seconds=*) FUZZ_SECONDS="${1#*=}" ;;
    --start-rank=*) START_RANK="${1#*=}" ;;
    --append) APPEND=1 ;;
    --lib=*) MODE="${1#*=}" ;;
    --function-list=*) TARGET_FILE="${1#*=}" ;;
    --libtiff-root=*) LIBTIFF_ROOT_OVERRIDE="${1#*=}" ;;
    *)
      echo "[-] Unknown option: $1" >&2
      exit 1
      ;;
  esac
  shift
done

MODE="${MODE,,}"
if [[ "$MODE" != "mongoose" && "$MODE" != "libtiff" ]]; then
  echo "[-] Unsupported --lib value: $MODE" >&2
  exit 1
fi

if ! command -v llvm-cov-14 &>/dev/null || ! command -v llvm-profdata-14 &>/dev/null; then
  echo "[-] llvm-cov-14 and llvm-profdata-14 are required (apt install llvm-14)" >&2
  exit 1
fi

# Mode-specific defaults
OUTPUT_CSV=""
DETAILED_LOG=""
LIBTIFF_ROOT="${LIBTIFF_ROOT:-}"
LIBTIFF_SRC_DIR=""
LIBTIFF_HEADERS_DIR=""
LIBTIFF_FORCE_INCLUDE=""
LIBTIFF_STUB_SRC="$PIN_ROOT/examples/libtiff_pin_stubs.c"
if [[ "$MODE" == "mongoose" ]]; then
  OUTPUT_CSV="${MONGOOSE_OUTPUT_CSV:-$SCRIPT_DIR/mongoose_top30_coverage.csv}"
  DETAILED_LOG="${MONGOOSE_DETAILED_LOG:-$SCRIPT_DIR/mongoose_coverage_detailed.log}"
else
  OUTPUT_CSV="${LIBTIFF_OUTPUT_CSV:-$SCRIPT_DIR/libtiff_top30_coverage.csv}"
  DETAILED_LOG="${LIBTIFF_DETAILED_LOG:-$SCRIPT_DIR/libtiff_coverage_detailed.log}"
  if [[ -n "$LIBTIFF_ROOT_OVERRIDE" ]]; then
    LIBTIFF_ROOT="$LIBTIFF_ROOT_OVERRIDE"
  else
    LIBTIFF_ROOT="${LIBTIFF_ROOT:-$PIN_ROOT/../libtiff}"
  fi
  if [[ ! -d "$LIBTIFF_ROOT" ]]; then
    echo "[-] libtiff root not found at $LIBTIFF_ROOT (override with --libtiff-root)" >&2
    exit 1
  fi
  LIBTIFF_SRC_DIR="$LIBTIFF_ROOT/libtiff"
  LIBTIFF_HEADERS_DIR="$LIBTIFF_SRC_DIR"
  LIBTIFF_FORCE_INCLUDE="$LIBTIFF_SRC_DIR/tiffio.h"
  if [[ ! -d "$LIBTIFF_SRC_DIR" ]]; then
    echo "[-] libtiff sources not found at $LIBTIFF_SRC_DIR" >&2
    exit 1
  fi
  if [[ ! -f "$LIBTIFF_FORCE_INCLUDE" ]]; then
    echo "[-] Missing tiffio.h at $LIBTIFF_FORCE_INCLUDE" >&2
    exit 1
  fi
  if [[ ! -f "$LIBTIFF_STUB_SRC" ]]; then
    echo "[-] Missing libtiff_pin_stubs.c at $LIBTIFF_STUB_SRC" >&2
    exit 1
  fi
fi

# Default libtiff function list
if [[ -z "$TARGET_FILE" && "$MODE" == "libtiff" ]]; then
  TARGET_FILE="$SCRIPT_DIR/libtiff_functions.top.txt"
fi

# Load targets (from file or defaults)
TARGETS=()
if [[ -n "$TARGET_FILE" ]]; then
  if [[ ! -f "$TARGET_FILE" ]]; then
    echo "[-] Function list not found: $TARGET_FILE" >&2
    exit 1
  fi
  mapfile -t TARGETS < <(awk '!/^#/ && NF {print $1}' "$TARGET_FILE")
else
  TARGETS=("${DEFAULT_MONGOOSE_TARGETS[@]}")
fi

if [[ ${#TARGETS[@]} -eq 0 ]]; then
  echo "[-] Target list is empty" >&2
  exit 1
fi

# Apply start-rank / top-n slicing
if (( START_RANK < 1 )); then
  echo "[-] --start-rank must be >= 1" >&2
  exit 1
fi
start_index=$((START_RANK - 1))
if (( start_index >= ${#TARGETS[@]} )); then
  echo "[-] --start-rank exceeds available targets (${#TARGETS[@]})" >&2
  exit 1
fi
remaining=$(( ${#TARGETS[@]} - start_index ))
if (( TOP_N > remaining )); then
  TOP_N=$remaining
fi
TARGETS=("${TARGETS[@]:start_index:TOP_N}")
total_targets=${#TARGETS[@]}

# Mode-specific prerequisite checks
if [[ "$MODE" == "mongoose" ]]; then
  if [[ ! -f "$MONGOOSE_SRC" ]]; then
    echo "[-] mongoose.c not found at $MONGOOSE_SRC" >&2
    exit 1
  fi
  if [[ ! -f "$MONGOOSE_HDR" ]]; then
    echo "[-] mongoose.h not found at $MONGOOSE_HDR" >&2
    exit 1
  fi
fi

if [[ $APPEND -eq 0 || ! -s "$OUTPUT_CSV" ]]; then
  echo "rank,function,fuzz_seconds,build_status,stage_a_status,stage_b_status,corpus_size,branches_total,branches_covered,branch_pct,lines_total,lines_executed,line_pct,exec_per_sec,total_execs,coverage_blocks" >"$OUTPUT_CSV"
fi

if [[ $APPEND -eq 0 ]]; then
  : >"$DETAILED_LOG"
else
  {
    echo ""
    echo "=== APPEND RUN $(date) ==="
    echo ""
  } >>"$DETAILED_LOG"
fi

echo "[*] PIN Coverage Test ($MODE mode) - ${total_targets} functions, ${FUZZ_SECONDS}s each"
if [[ "$MODE" == "mongoose" ]]; then
  echo "[*] Source: $MONGOOSE_SRC"
else
  echo "[*] libtiff root: $LIBTIFF_ROOT"
  [[ -n "$TARGET_FILE" ]] && echo "[*] Function list: $TARGET_FILE"
fi
echo ""

is_in_set_wrapper() {
  local func="$1"
  shift
  is_in_set "$func" "$@"
}

prepare_target_context() {
  local func="$1"
  TARGET_SRC="$MONGOOSE_SRC"
  HEADERS_DIR="$PIN_ROOT/utils/user_headers"
  COVERAGE_SOURCE="$TARGET_SRC"
  FORCE_INCLUDES_VALUE=""
  FORCE_INTERNAL_VALUE=""
  EXTRA_SOURCES_VALUE=""
  BUCKET_CFLAGS_VALUE=""
  BUCKET_LDFLAGS_VALUE=""
  if [[ "$MODE" == "libtiff" ]]; then
    local rel="${LIBTIFF_SOURCE_MAP[$func]-}"
    if [[ -z "$rel" ]]; then
      echo "  [✗] No libtiff source mapping for $func" | tee -a "$DETAILED_LOG"
      return 1
    fi
    TARGET_SRC="$LIBTIFF_SRC_DIR/$rel"
    HEADERS_DIR="$LIBTIFF_HEADERS_DIR"
    COVERAGE_SOURCE="$TARGET_SRC"
    FORCE_INCLUDES_VALUE="$LIBTIFF_FORCE_INCLUDE"
    if is_in_set_wrapper "$func" "${LIBTIFF_LOG_LUV_FUNCS[@]}"; then
      EXTRA_SOURCES_VALUE="$LIBTIFF_STUB_SRC"
      BUCKET_CFLAGS_VALUE="-DLOGLUV_PUBLIC=1"
      BUCKET_LDFLAGS_VALUE="-lm"
    fi
    if is_in_set_wrapper "$func" "${LIBTIFF_FIELD_FUNCS[@]}"; then
      FORCE_INTERNAL_VALUE="TIFFField,TIFFFieldArray"
    fi
    if [[ ! -f "$TARGET_SRC" ]]; then
      echo "  [✗] Source file missing: $TARGET_SRC" | tee -a "$DETAILED_LOG"
      return 1
    fi
  fi
  return 0
}

COVERAGE_CFLAGS="-fprofile-instr-generate -fcoverage-mapping"
COVERAGE_LDFLAGS="-fprofile-instr-generate"

for index in "${!TARGETS[@]}"; do
  func="${TARGETS[$index]}"
  rank_num=$((START_RANK + index))
  build_prefix="$MODE"
  build_dir="$PIN_ROOT/build/${build_prefix}_${func}_diff"
  result_dir="$PIN_ROOT/results/${build_prefix}_${func}_diff"
  profile_raw="/tmp/${build_prefix}_${func}.profraw"
  profile_data="/tmp/${build_prefix}_${func}.profdata"

  echo "[$rank_num/${total_targets}] Testing: $func" | tee -a "$DETAILED_LOG"

  build_status="FAIL"
  stage_a_status="SKIP"
  stage_b_status="SKIP"
  corpus_size=0
  branches_total=0
  branches_covered=0
  branch_pct="0.0%"
  lines_total=0
  lines_executed=0
  line_pct="0.0%"
  exec_per_sec=0
  total_execs=0
  coverage_blocks=0

  if ! prepare_target_context "$func"; then
    echo "$rank_num,$func,$FUZZ_SECONDS,$build_status,$stage_a_status,$stage_b_status,$corpus_size,$branches_total,$branches_covered,$branch_pct,$lines_total,$lines_executed,$line_pct,$exec_per_sec,$total_execs,$coverage_blocks" >>"$OUTPUT_CSV"
    echo "" | tee -a "$DETAILED_LOG"
    continue
  fi

  PIN_EXTRA_CFLAGS_VALUE="$COVERAGE_CFLAGS"
  PIN_EXTRA_LDFLAGS_VALUE="$COVERAGE_LDFLAGS"
  [[ -n "$BUCKET_CFLAGS_VALUE" ]] && PIN_EXTRA_CFLAGS_VALUE+=" $BUCKET_CFLAGS_VALUE"
  [[ -n "$BUCKET_LDFLAGS_VALUE" ]] && PIN_EXTRA_LDFLAGS_VALUE+=" $BUCKET_LDFLAGS_VALUE"

  mkdir -p "$build_dir"
  build_log="$build_dir/build.log"

  echo "  [*] Building harness for $func..." | tee -a "$DETAILED_LOG"
  PIN_DIFF_BASE_ARGS=(
    "$PIN_ROOT/src/pin_diff.sh"
    "$TARGET_SRC"
    "$func"
    "--headers-dir=$HEADERS_DIR"
    "--reference-decoder=nanopb"
  )
  if [[ -n "$EXTRA_SOURCES_VALUE" ]]; then
    PIN_DIFF_BASE_ARGS+=("--extra-sources=$EXTRA_SOURCES_VALUE")
  fi

  if env \
      PIN_BUILD_DIR="$build_dir" \
      PIN_RESULTS_DIR="$result_dir" \
      PIN_EXTRA_CFLAGS="$PIN_EXTRA_CFLAGS_VALUE" \
      PIN_EXTRA_LDFLAGS="$PIN_EXTRA_LDFLAGS_VALUE" \
      PIN_FORCE_INCLUDES="$FORCE_INCLUDES_VALUE" \
      PIN_FORCE_INTERNAL_TYPES="$FORCE_INTERNAL_VALUE" \
      "${PIN_DIFF_BASE_ARGS[@]}" \
      --fuzz-seconds=0 \
      >"$build_log" 2>&1; then
    build_status="OK"
    echo "  [✓] Build succeeded" | tee -a "$DETAILED_LOG"
  else
    echo "  [✗] Build failed (see $build_log)" | tee -a "$DETAILED_LOG"
    echo "$rank_num,$func,$FUZZ_SECONDS,$build_status,$stage_a_status,$stage_b_status,$corpus_size,$branches_total,$branches_covered,$branch_pct,$lines_total,$lines_executed,$line_pct,$exec_per_sec,$total_execs,$coverage_blocks" >>"$OUTPUT_CSV"
    echo "" | tee -a "$DETAILED_LOG"
    continue
  fi

  if [[ ! -f "$build_dir/fuzz_bytes" ]]; then
    echo "  [✗] fuzz_bytes binary not found" | tee -a "$DETAILED_LOG"
    echo "$rank_num,$func,$FUZZ_SECONDS,$build_status,$stage_a_status,$stage_b_status,$corpus_size,$branches_total,$branches_covered,$branch_pct,$lines_total,$lines_executed,$line_pct,$exec_per_sec,$total_execs,$coverage_blocks" >>"$OUTPUT_CSV"
    echo "" | tee -a "$DETAILED_LOG"
    continue
  fi

  cd "$build_dir"
  rm -f "$profile_raw" "$profile_data"

  echo "  [*] Stage A: Fuzzing for $FUZZ_SECONDS seconds..." | tee -a "$DETAILED_LOG"
  fuzz_log="$build_dir/fuzz.log"
  start_ts=$(date +%s)
  if timeout $((FUZZ_SECONDS + 10)) ./fuzz_bytes corpus \
      -max_total_time=$FUZZ_SECONDS \
      -use_value_profile=1 \
      -print_final_stats=1 \
      >"$fuzz_log" 2>&1; then
    stage_a_status="OK"
    echo "  [✓] Fuzzing completed" | tee -a "$DETAILED_LOG"
  else
    exit_code=$?
    if [[ $exit_code -eq 124 ]]; then
      stage_a_status="TIMEOUT"
      echo "  [⚠] Fuzzing timed out" | tee -a "$DETAILED_LOG"
    else
      stage_a_status="CRASH"
      echo "  [✗] Fuzzing crashed (exit $exit_code)" | tee -a "$DETAILED_LOG"
    fi
  fi
  end_ts=$(date +%s)
  runtime=$((end_ts - start_ts))

  if [[ -f "$fuzz_log" ]]; then
    corpus_size=$(find corpus -type f 2>/dev/null | wc -l)
    if grep -q "exec/s:" "$fuzz_log"; then
      exec_per_sec=$(grep "exec/s:" "$fuzz_log" | tail -1 | awk '{print $3}')
    fi
    if grep -q "stat::number_of_executed_units:" "$fuzz_log"; then
      total_execs=$(grep "stat::number_of_executed_units:" "$fuzz_log" | awk '{print $2}')
    fi
    if grep -q "cov:" "$fuzz_log"; then
      coverage_blocks=$(grep "cov:" "$fuzz_log" | tail -1 | awk '{print $2}' | sed 's/cov://')
    fi
  fi

  echo "  [*] Collecting coverage data..." | tee -a "$DETAILED_LOG"
  profile_log="$build_dir/profile.log"
  LLVM_PROFILE_FILE="$profile_raw" ./fuzz_bytes corpus -runs=0 >"$profile_log" 2>&1 || true
  if [[ -f "$profile_raw" ]]; then
    llvm-profdata-14 merge "$profile_raw" -o "$profile_data" 2>/dev/null || true
  fi

  if [[ -f "$profile_data" ]]; then
    report=$(llvm-cov-14 report --show-functions ./fuzz_bytes \
      -instr-profile="$profile_data" \
      -path-equivalence="$build_dir,$PIN_ROOT" \
      "$COVERAGE_SOURCE" 2>/dev/null || echo "")
    line=$(awk -v fn="$func" '$1==fn {print}' <<<"$report")
    if [[ -n "$line" ]]; then
      read -r _ regions missed_regions region_pct lines_total lines_miss line_pct branches_total branches_miss branch_pct <<<"$line"
      branches_covered=$((branches_total - branches_miss))
      lines_executed=$((lines_total - lines_miss))
      echo "  [✓] Coverage: $branches_covered/$branches_total branches ($branch_pct), $lines_executed/$lines_total lines ($line_pct)" | tee -a "$DETAILED_LOG"
    else
      echo "  [⚠] Function $func not found in coverage report" | tee -a "$DETAILED_LOG"
    fi
  else
    echo "  [⚠] No profile data generated" | tee -a "$DETAILED_LOG"
  fi

  echo "  [*] Stage B: Differential replay..." | tee -a "$DETAILED_LOG"
  replay_log="$build_dir/replay.log"
  if env \
      PIN_BUILD_DIR="$build_dir" \
      PIN_RESULTS_DIR="$result_dir" \
      PIN_EXTRA_CFLAGS="$PIN_EXTRA_CFLAGS_VALUE" \
      PIN_EXTRA_LDFLAGS="$PIN_EXTRA_LDFLAGS_VALUE" \
      PIN_FORCE_INCLUDES="$FORCE_INCLUDES_VALUE" \
      PIN_FORCE_INTERNAL_TYPES="$FORCE_INTERNAL_VALUE" \
      "${PIN_DIFF_BASE_ARGS[@]}" \
      --fuzz-seconds=0 \
      --replay-dir="$build_dir/corpus" \
      >"$replay_log" 2>&1; then
    stage_b_status="OK"
    echo "  [✓] Replay completed" | tee -a "$DETAILED_LOG"
  else
    stage_b_status="FAIL"
    echo "  [✗] Replay failed" | tee -a "$DETAILED_LOG"
  fi

  echo "$rank_num,$func,$runtime,$build_status,$stage_a_status,$stage_b_status,$corpus_size,$branches_total,$branches_covered,$branch_pct,$lines_total,$lines_executed,$line_pct,$exec_per_sec,$total_execs,$coverage_blocks" >>"$OUTPUT_CSV"

  rm -f "$profile_raw" "$profile_data"
  cd "$PIN_ROOT"
  echo "" | tee -a "$DETAILED_LOG"
done

echo "[+] Coverage testing complete!"
echo "[+] Summary saved to: $OUTPUT_CSV"
echo "[+] Detailed log saved to: $DETAILED_LOG"
echo ""

echo "=== SUMMARY STATISTICS ==="
echo ""

build_ok=$(awk -F, 'NR>1 && $4=="OK" {count++} END {print count+0}' "$OUTPUT_CSV")
stage_a_ok=$(awk -F, 'NR>1 && $5=="OK" {count++} END {print count+0}' "$OUTPUT_CSV")
stage_b_ok=$(awk -F, 'NR>1 && $6=="OK" {count++} END {print count+0}' "$OUTPUT_CSV")

echo "Total functions tested: $total_targets"
echo "Build success: $build_ok / $total_targets"
echo "Stage A (fuzzing) success: $stage_a_ok / $total_targets"
echo "Stage B (replay) success: $stage_b_ok / $total_targets"
echo ""

echo "Top 5 by branch coverage:"
awk -F, 'NR>1 {print $2,$9}' "$OUTPUT_CSV" | sort -k2 -rn | head -5 | nl

echo ""

echo "Top 5 by line coverage:"
awk -F, 'NR>1 {print $2,$12}' "$OUTPUT_CSV" | sort -k2 -rn | head -5 | nl

echo ""

echo "Top 5 by throughput (exec/sec):"
awk -F, 'NR>1 {print $2,$14}' "$OUTPUT_CSV" | sort -k2 -rn | head -5 | nl
