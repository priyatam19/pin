#!/usr/bin/env bash
# Automated testing of top 100 coreutils functions with PIN
#
# Usage: test_coreutils_100.sh [--count=100] [--fuzz-seconds=60] [--start=1]
#
# This script:
# 1. Reads ranked functions from JSON
# 2. Extracts each function automatically
# 3. Tests with PIN differential fuzzing
# 4. Collects coverage metrics with LLVM
# 5. Generates comprehensive CSV results

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PIN_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
COREUTILS_SRC="/home/priyatam/ws/coreutils/src"
OUTPUT_DIR="$PIN_ROOT/examples/coreutils"
RESULTS_CSV="$OUTPUT_DIR/coreutils_top100_coverage.csv"
DETAILED_LOG="$OUTPUT_DIR/coreutils_coverage_detailed.log"
JSON_FILE="$OUTPUT_DIR/coreutils_functions_ranked.json"

# Default parameters
TEST_COUNT=100
FUZZ_SECONDS=60
START_RANK=1

# Parse arguments
for arg in "$@"; do
  case $arg in
    --count=*)
      TEST_COUNT="${arg#*=}"
      ;;
    --fuzz-seconds=*)
      FUZZ_SECONDS="${arg#*=}"
      ;;
    --start=*)
      START_RANK="${arg#*=}"
      ;;
    *)
      echo "Unknown argument: $arg"
      exit 1
      ;;
  esac
done

echo "╔════════════════════════════════════════════════════════════════════════╗"
echo "║        Coreutils Automated Testing - Top $TEST_COUNT Functions               ║"
echo "╚════════════════════════════════════════════════════════════════════════╝"
echo ""
echo "[*] Configuration:"
echo "    Functions to test: $TEST_COUNT (starting from rank $START_RANK)"
echo "    Fuzz duration: $FUZZ_SECONDS seconds per function"
echo "    Source: $COREUTILS_SRC"
echo "    Output: $RESULTS_CSV"
echo ""

# Check prerequisites
if [[ ! -f "$JSON_FILE" ]]; then
  echo "[ERROR] Function analysis not found: $JSON_FILE"
  echo "[i] Run: python3 scripts/analyze_coreutils_functions.py $COREUTILS_SRC"
  exit 1
fi

if ! command -v jq &>/dev/null; then
  echo "[WARNING] jq not found, using Python fallback for JSON parsing"
  USE_JQ=false
else
  USE_JQ=true
fi

if ! command -v llvm-cov-14 &>/dev/null; then
  echo "[WARNING] llvm-cov-14 not found, coverage metrics will be limited"
fi

# Create CSV header
echo "rank,score,program,function,extraction_status,build_status,stage_a_status,stage_b_status,fuzz_seconds,corpus_size,branches_total,branches_covered,branch_pct,lines_total,lines_executed,line_pct,exec_per_sec,total_execs,coverage_blocks" > "$RESULTS_CSV"

# Clear detailed log
> "$DETAILED_LOG"

# Function to extract JSON field
get_json_field() {
  local rank=$1
  local field=$2

  if $USE_JQ; then
    jq -r ".[$((rank-1))].$field // empty" "$JSON_FILE"
  else
    python3 -c "import json; data=json.load(open('$JSON_FILE')); print(data[$((rank-1))]['$field'] if $((rank-1)) < len(data) else '')"
  fi
}

echo "[*] Starting automated testing..."
echo "" | tee -a "$DETAILED_LOG"

# Test each function
END_RANK=$((START_RANK + TEST_COUNT - 1))

for rank in $(seq $START_RANK $END_RANK); do
  # Get function details from JSON
  program=$(get_json_field $rank "program")
  function=$(get_json_field $rank "function")
  score=$(get_json_field $rank "score")

  if [[ -z "$program" || -z "$function" ]]; then
    echo "[$rank/$END_RANK] Rank $rank not found in JSON, stopping"
    break
  fi

  echo "═══════════════════════════════════════════════════════════════════════" | tee -a "$DETAILED_LOG"
  echo "[$rank/$END_RANK] Testing: $program:$function (score: $score)" | tee -a "$DETAILED_LOG"
  echo "═══════════════════════════════════════════════════════════════════════" | tee -a "$DETAILED_LOG"

  # Initialize metrics
  extraction_status="FAIL"
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

  # Step 1: Extract function
  echo "  [*] Extracting function..." | tee -a "$DETAILED_LOG"

  extracted_file="$OUTPUT_DIR/${program}_${function}.c"

  if [[ -f "$extracted_file" ]]; then
    echo "  [✓] Already extracted: $extracted_file" | tee -a "$DETAILED_LOG"
    extraction_status="CACHED"
  else
    if bash "$SCRIPT_DIR/extract_coreutils_function.sh" "$program" "$function" >> "$DETAILED_LOG" 2>&1; then
      extraction_status="OK"
      echo "  [✓] Extraction succeeded" | tee -a "$DETAILED_LOG"
    else
      echo "  [✗] Extraction failed" | tee -a "$DETAILED_LOG"
      echo "$rank,$score,$program,$function,$extraction_status,$build_status,$stage_a_status,$stage_b_status,$FUZZ_SECONDS,$corpus_size,$branches_total,$branches_covered,$branch_pct,$lines_total,$lines_executed,$line_pct,$exec_per_sec,$total_execs,$coverage_blocks" >> "$RESULTS_CSV"
      echo "" | tee -a "$DETAILED_LOG"
      continue
    fi
  fi

  # Step 2: Build with PIN (with coverage instrumentation)
  echo "  [*] Building with PIN..." | tee -a "$DETAILED_LOG"

  # Compute build directory name from extracted file (matches pin_diff.sh behavior)
  extracted_basename="$(basename "$extracted_file" .c)"
  build_dir="$PIN_ROOT/build/${extracted_basename}_diff"

  export PIN_EXTRA_CFLAGS="-fprofile-instr-generate -fcoverage-mapping"
  export PIN_EXTRA_LDFLAGS="-fprofile-instr-generate"

  if "$PIN_ROOT/src/pin_diff.sh" \
    "$extracted_file" \
    "$function" \
    --headers-dir="$PIN_ROOT/utils/coreutils_headers" \
    --fuzz-seconds=0 \
    --reference-decoder=nanopb \
    >> "$build_dir/build.log" 2>&1; then

    build_status="OK"
    echo "  [✓] Build succeeded" | tee -a "$DETAILED_LOG"
  else
    echo "  [✗] Build failed (see $build_dir/build.log)" | tee -a "$DETAILED_LOG"
    echo "$rank,$score,$program,$function,$extraction_status,$build_status,$stage_a_status,$stage_b_status,$FUZZ_SECONDS,$corpus_size,$branches_total,$branches_covered,$branch_pct,$lines_total,$lines_executed,$line_pct,$exec_per_sec,$total_execs,$coverage_blocks" >> "$RESULTS_CSV"
    echo "" | tee -a "$DETAILED_LOG"
    continue
  fi

  # Check if fuzz_bytes exists
  if [[ ! -f "$build_dir/fuzz_bytes" ]]; then
    echo "  [✗] fuzz_bytes binary not found" | tee -a "$DETAILED_LOG"
    echo "$rank,$score,$program,$function,$extraction_status,$build_status,$stage_a_status,$stage_b_status,$FUZZ_SECONDS,$corpus_size,$branches_total,$branches_covered,$branch_pct,$lines_total,$lines_executed,$line_pct,$exec_per_sec,$total_execs,$coverage_blocks" >> "$RESULTS_CSV"
    echo "" | tee -a "$DETAILED_LOG"
    continue
  fi

  cd "$build_dir" || exit 1

  # Step 3: Stage A - Fuzzing
  echo "  [*] Stage A: Fuzzing for $FUZZ_SECONDS seconds..." | tee -a "$DETAILED_LOG"

  profile_raw="/tmp/coreutils_${program}_${function}.profraw"
  profile_data="/tmp/coreutils_${program}_${function}.profdata"
  rm -f "$profile_raw" "$profile_data"

  fuzz_log="$build_dir/fuzz.log"
  start_ts=$(date +%s)

  if timeout $((FUZZ_SECONDS + 10)) ./fuzz_bytes corpus \
    -max_total_time=$FUZZ_SECONDS \
    -use_value_profile=1 \
    -print_final_stats=1 \
    > "$fuzz_log" 2>&1; then
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
  actual_fuzz_seconds=$((end_ts - start_ts))

  # Parse fuzzing stats
  if [[ -f "$fuzz_log" ]]; then
    corpus_size=$(find corpus -type f 2>/dev/null | wc -l)

    if grep -q "exec/s:" "$fuzz_log"; then
      exec_per_sec=$(grep "exec/s:" "$fuzz_log" | tail -1 | awk '{print $3}' | sed 's/[^0-9]//g')
    fi

    if grep -q "stat::number_of_executed_units:" "$fuzz_log"; then
      total_execs=$(grep "stat::number_of_executed_units:" "$fuzz_log" | awk '{print $2}')
    fi

    if grep -q "cov:" "$fuzz_log"; then
      coverage_blocks=$(grep "cov:" "$fuzz_log" | tail -1 | awk '{print $2}' | sed 's/cov://')
    fi

    echo "  [i] Corpus: $corpus_size inputs, $total_execs executions, $exec_per_sec exec/sec" | tee -a "$DETAILED_LOG"
  fi

  # Step 4: Collect coverage
  if command -v llvm-cov-14 &>/dev/null; then
    echo "  [*] Collecting coverage data..." | tee -a "$DETAILED_LOG"

    LLVM_PROFILE_FILE="$profile_raw" ./fuzz_bytes corpus -runs=0 > /dev/null 2>&1 || true

    if [[ -f "$profile_raw" ]]; then
      llvm-profdata-14 merge "$profile_raw" -o "$profile_data" 2>/dev/null || true
    fi

    if [[ -f "$profile_data" ]]; then
      report=$(llvm-cov-14 report --show-functions ./fuzz_bytes \
        -instr-profile="$profile_data" \
        -path-equivalence="$build_dir,$PIN_ROOT" \
        "$extracted_file" 2>/dev/null || echo "")

      line=$(awk -v fn="$function" '$1==fn {print}' <<< "$report")

      if [[ -n "$line" ]]; then
        read -r _ regions missed_regions region_pct lines_total lines_miss line_pct branches_total branches_miss branch_pct <<< "$line"

        branches_covered=$((branches_total - branches_miss))
        lines_executed=$((lines_total - lines_miss))

        echo "  [✓] Coverage: $branches_covered/$branches_total branches ($branch_pct), $lines_executed/$lines_total lines ($line_pct)" | tee -a "$DETAILED_LOG"
      else
        echo "  [⚠] Function not found in coverage report" | tee -a "$DETAILED_LOG"
      fi
    fi

    rm -f "$profile_raw" "$profile_data"
  fi

  # Step 5: Stage B - Differential replay
  echo "  [*] Stage B: Differential replay..." | tee -a "$DETAILED_LOG"

  if "$PIN_ROOT/src/pin_diff.sh" \
    "$extracted_file" \
    "$function" \
    --headers-dir="$PIN_ROOT/utils/coreutils_headers" \
    --fuzz-seconds=0 \
    --replay-dir="$build_dir/corpus" \
    --reference-decoder=nanopb \
    >> "$build_dir/replay.log" 2>&1; then
    stage_b_status="OK"
    echo "  [✓] Replay completed" | tee -a "$DETAILED_LOG"
  else
    stage_b_status="FAIL"
    echo "  [✗] Replay failed" | tee -a "$DETAILED_LOG"
  fi

  # Write CSV row
  echo "$rank,$score,$program,$function,$extraction_status,$build_status,$stage_a_status,$stage_b_status,$actual_fuzz_seconds,$corpus_size,$branches_total,$branches_covered,$branch_pct,$lines_total,$lines_executed,$line_pct,$exec_per_sec,$total_execs,$coverage_blocks" >> "$RESULTS_CSV"

  cd "$PIN_ROOT" || exit 1

  echo "" | tee -a "$DETAILED_LOG"

  # Progress indicator
  completed=$((rank - START_RANK + 1))
  pct=$((100 * completed / TEST_COUNT))
  echo "[Progress: $completed/$TEST_COUNT ($pct%)]" | tee -a "$DETAILED_LOG"
  echo "" | tee -a "$DETAILED_LOG"
done

echo "" | tee -a "$DETAILED_LOG"
echo "╔════════════════════════════════════════════════════════════════════════╗" | tee -a "$DETAILED_LOG"
echo "║                        TESTING COMPLETE                                 ║" | tee -a "$DETAILED_LOG"
echo "╚════════════════════════════════════════════════════════════════════════╝" | tee -a "$DETAILED_LOG"
echo "" | tee -a "$DETAILED_LOG"

# Generate summary statistics
echo "═══════════════════════════════════════════════════════════════════════" | tee -a "$DETAILED_LOG"
echo "SUMMARY STATISTICS" | tee -a "$DETAILED_LOG"
echo "═══════════════════════════════════════════════════════════════════════" | tee -a "$DETAILED_LOG"
echo "" | tee -a "$DETAILED_LOG"

total_tested=$(tail -n +2 "$RESULTS_CSV" | wc -l)
extraction_ok=$(awk -F, 'NR>1 && ($5=="OK" || $5=="CACHED") {count++} END {print count+0}' "$RESULTS_CSV")
build_ok=$(awk -F, 'NR>1 && $6=="OK" {count++} END {print count+0}' "$RESULTS_CSV")
stage_a_ok=$(awk -F, 'NR>1 && $7=="OK" {count++} END {print count+0}' "$RESULTS_CSV")
stage_b_ok=$(awk -F, 'NR>1 && $8=="OK" {count++} END {print count+0}' "$RESULTS_CSV")

echo "Functions tested: $total_tested" | tee -a "$DETAILED_LOG"
echo "Extraction success: $extraction_ok / $total_tested ($((100 * extraction_ok / total_tested))%)" | tee -a "$DETAILED_LOG"
echo "Build success: $build_ok / $total_tested ($((100 * build_ok / total_tested))%)" | tee -a "$DETAILED_LOG"
echo "Stage A (fuzzing) success: $stage_a_ok / $total_tested ($((100 * stage_a_ok / total_tested))%)" | tee -a "$DETAILED_LOG"
echo "Stage B (replay) success: $stage_b_ok / $total_tested ($((100 * stage_b_ok / total_tested))%)" | tee -a "$DETAILED_LOG"
echo "" | tee -a "$DETAILED_LOG"

# Top 10 by branch coverage
echo "Top 10 by branch coverage:" | tee -a "$DETAILED_LOG"
awk -F, 'NR>1 && $12 > 0 {print $4, $12}' "$RESULTS_CSV" | sort -k2 -rn | head -10 | nl | tee -a "$DETAILED_LOG"
echo "" | tee -a "$DETAILED_LOG"

# Top 10 by throughput
echo "Top 10 by throughput (exec/sec):" | tee -a "$DETAILED_LOG"
awk -F, 'NR>1 && $17 > 0 {print $4, $17}' "$RESULTS_CSV" | sort -k2 -rn | head -10 | nl | tee -a "$DETAILED_LOG"
echo "" | tee -a "$DETAILED_LOG"

echo "[+] Results saved to: $RESULTS_CSV" | tee -a "$DETAILED_LOG"
echo "[+] Detailed log saved to: $DETAILED_LOG" | tee -a "$DETAILED_LOG"
echo "" | tee -a "$DETAILED_LOG"

echo "Done! 🎉"
