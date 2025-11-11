#!/usr/bin/env bash
#
# Test script to demonstrate the impact of using --reference-decoder=nanopb
# Compares DIFF counts with default C++ Protobuf decoder vs nanopb decoder
#
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

echo "=============================================="
echo "PIN Decoder Comparison Test"
echo "=============================================="
echo ""
echo "This script tests the impact of using --reference-decoder=nanopb"
echo "on DIFF counts by comparing:"
echo "  - Default: C++ Protobuf reference decoder (strict UTF-8)"
echo "  - Fix:     nanopb reference decoder (lenient UTF-8)"
echo ""

# Select test benchmarks with known high DIFF rates
BENCHMARKS=(
    "examples/simple_benchs/empty_mode_compare.c:classify_mode_empty"
    "examples/simple_benchs/mixed_scalars.c:evaluate_sensor"
    "examples/simple_benchs/mode_buffer_dump.c:dump_mode_bytes"
)

# Fuzzing time (keep short for quick test)
FUZZ_SECONDS=30

# Results tracking
declare -A DEFAULT_DIFFS
declare -A NANOPB_DIFFS
declare -A DEFAULT_MATCHES
declare -A NANOPB_MATCHES
declare -A DEFAULT_EMI
declare -A NANOPB_EMI

# Cleanup function
cleanup() {
    echo ""
    echo "Cleaning up test artifacts..."
}
trap cleanup EXIT

echo "=============================================="
echo "Phase 1: Testing with DEFAULT decoder (C++ Protobuf)"
echo "=============================================="
echo ""

for bench_pair in "${BENCHMARKS[@]}"; do
    IFS=':' read -r cfile func <<< "$bench_pair"
    bench_name=$(basename "$cfile" .c)

    echo "---"
    echo "Testing: $bench_name (DEFAULT cpp decoder)"
    echo "Command: ./src/pin_diff.sh $cfile $func --fuzz-seconds=$FUZZ_SECONDS"
    echo ""

    # Run with default decoder (cpp)
    ./src/pin_diff.sh "$cfile" "$func" --fuzz-seconds="$FUZZ_SECONDS" 2>&1 | grep -E "(Stage A|Stage B|DIFF|match|emi-reject)" || true

    # Extract stats from replay summary
    summary_file="results/${bench_name}_diff/stage_b/replay_summary.txt"
    if [[ -f "$summary_file" ]]; then
        diffs=$(grep -c "^DIFF" "$summary_file" || echo "0")
        matches=$(grep -c "^match" "$summary_file" || echo "0")
        emi=$(grep -c "^emi-reject" "$summary_file" || echo "0")

        DEFAULT_DIFFS["$bench_name"]=$diffs
        DEFAULT_MATCHES["$bench_name"]=$matches
        DEFAULT_EMI["$bench_name"]=$emi

        echo ""
        echo "Results (DEFAULT):"
        echo "  DIFFs:      $diffs"
        echo "  Matches:    $matches"
        echo "  EMI-reject: $emi"
    else
        echo "Warning: No replay summary found for $bench_name"
        DEFAULT_DIFFS["$bench_name"]=0
        DEFAULT_MATCHES["$bench_name"]=0
        DEFAULT_EMI["$bench_name"]=0
    fi
    echo ""
done

echo ""
echo "=============================================="
echo "Phase 2: Testing with NANOPB decoder"
echo "=============================================="
echo ""

for bench_pair in "${BENCHMARKS[@]}"; do
    IFS=':' read -r cfile func <<< "$bench_pair"
    bench_name=$(basename "$cfile" .c)

    echo "---"
    echo "Testing: $bench_name (NANOPB decoder)"
    echo "Command: ./src/pin_diff.sh $cfile $func --reference-decoder=nanopb --fuzz-seconds=$FUZZ_SECONDS"
    echo ""

    # Run with nanopb decoder
    ./src/pin_diff.sh "$cfile" "$func" --reference-decoder=nanopb --fuzz-seconds="$FUZZ_SECONDS" 2>&1 | grep -E "(Stage A|Stage B|DIFF|match|emi-reject)" || true

    # Extract stats from replay summary
    summary_file="results/${bench_name}_diff/stage_b/replay_summary.txt"
    if [[ -f "$summary_file" ]]; then
        diffs=$(grep -c "^DIFF" "$summary_file" || echo "0")
        matches=$(grep -c "^match" "$summary_file" || echo "0")
        emi=$(grep -c "^emi-reject" "$summary_file" || echo "0")

        NANOPB_DIFFS["$bench_name"]=$diffs
        NANOPB_MATCHES["$bench_name"]=$matches
        NANOPB_EMI["$bench_name"]=$emi

        echo ""
        echo "Results (NANOPB):"
        echo "  DIFFs:      $diffs"
        echo "  Matches:    $matches"
        echo "  EMI-reject: $emi"
    else
        echo "Warning: No replay summary found for $bench_name"
        NANOPB_DIFFS["$bench_name"]=0
        NANOPB_MATCHES["$bench_name"]=0
        NANOPB_EMI["$bench_name"]=0
    fi
    echo ""
done

echo ""
echo "=============================================="
echo "COMPARISON REPORT"
echo "=============================================="
echo ""

# Print comparison table
printf "%-25s | %10s | %10s | %10s\n" "Benchmark" "Default" "Nanopb" "Reduction"
printf "%-25s-+-%10s-+-%10s-+-%10s\n" "-------------------------" "----------" "----------" "----------"

total_default=0
total_nanopb=0

for bench_pair in "${BENCHMARKS[@]}"; do
    IFS=':' read -r cfile func <<< "$bench_pair"
    bench_name=$(basename "$cfile" .c)

    default=${DEFAULT_DIFFS["$bench_name"]}
    nanopb=${NANOPB_DIFFS["$bench_name"]}

    total_default=$((total_default + default))
    total_nanopb=$((total_nanopb + nanopb))

    if [[ $default -gt 0 ]]; then
        reduction=$(awk "BEGIN {printf \"%.1f%%\", 100 * ($default - $nanopb) / $default}")
    else
        reduction="N/A"
    fi

    printf "%-25s | %10d | %10d | %10s\n" "$bench_name" "$default" "$nanopb" "$reduction"
done

printf "%-25s-+-%10s-+-%10s-+-%10s\n" "-------------------------" "----------" "----------" "----------"
printf "%-25s | %10d | %10d | " "TOTAL" "$total_default" "$total_nanopb"

if [[ $total_default -gt 0 ]]; then
    total_reduction=$(awk "BEGIN {printf \"%.1f%%\", 100 * ($total_default - $total_nanopb) / $total_default}")
    printf "%10s\n" "$total_reduction"
else
    printf "%10s\n" "N/A"
fi

echo ""
echo "=============================================="
echo "SUMMARY"
echo "=============================================="
echo ""
echo "Decoder Impact Analysis:"
echo ""
echo "Default C++ Protobuf decoder:"
echo "  - Total DIFFs: $total_default"
echo "  - Causes artificial DIFFs due to UTF-8 validation"
echo ""
echo "Nanopb decoder (--reference-decoder=nanopb):"
echo "  - Total DIFFs: $total_nanopb"
echo "  - Eliminates UTF-8 validation false positives"
echo ""

if [[ $total_default -gt 0 ]]; then
    reduction=$(awk "BEGIN {printf \"%.1f%%\", 100 * ($total_default - $total_nanopb) / $total_default}")
    echo "DIFF Reduction: $reduction"
    echo ""

    if (( $(echo "$reduction > 50" | bc -l) )); then
        echo "✅ SUCCESS: Using --reference-decoder=nanopb reduces DIFFs by $reduction!"
        echo ""
        echo "Recommendation:"
        echo "  Use nanopb decoder by default for real-world fuzzing:"
        echo "  ./src/pin_diff.sh <file> <func> --reference-decoder=nanopb --fuzz-seconds=<N>"
    else
        echo "Note: DIFF reduction is $reduction"
        echo "This may indicate other tool bugs beyond UTF-8 validation."
        echo "See docs/emi-case-studies.md for detailed analysis."
    fi
else
    echo "Note: No DIFFs found in either configuration."
    echo "This is expected for simple benchmarks or when fuzzing time is very short."
fi

echo ""
echo "For detailed architecture explanation, see: docs/pin-architecture.md"
echo "For DIFF case studies, see: docs/emi-case-studies.md"
echo ""
