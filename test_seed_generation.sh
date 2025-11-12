#!/bin/bash
# Test script to evaluate intelligent seed generation
set -e

TARGET="check_num"
BUILD_DIR="/home/priyatam/pin/build/check_num_diff"
EXAMPLES_DIR="/home/priyatam/pin/examples/simple_benchs"
ROOT_DIR="/home/priyatam/pin"

echo "=== Intelligent Seed Generation Test ==="
echo ""
echo "Target: $TARGET"
echo "Build directory: $BUILD_DIR"
echo ""

# Step 1: Check if target already built
if [ ! -f "$BUILD_DIR/fuzz_bytes" ]; then
    echo "[*] Building target first..."
    cd "$ROOT_DIR"
    ./src/pin_diff.sh "$EXAMPLES_DIR/check_num.c" checkNum --fuzz-seconds=0
fi

# Step 2: Baseline fuzzing (no seeds)
echo ""
echo "=== BASELINE: Fuzzing without intelligent seeds ==="
cd "$BUILD_DIR"

# Clear existing corpus
if [ -d "corpus_baseline" ]; then
    rm -rf corpus_baseline
fi
mkdir -p corpus_baseline

# Run fuzzer for 30 seconds with empty corpus
timeout 30 ./fuzz_bytes corpus_baseline -max_total_time=30 -print_final_stats=1 2>&1 | tee baseline_stats.txt || true

# Extract baseline stats
BASELINE_COV=$(grep -oP 'cov: \K\d+' baseline_stats.txt | tail -1 || echo "0")
BASELINE_CORP=$(ls corpus_baseline/*.bin 2>/dev/null | wc -l || echo "0")
BASELINE_EXECS=$(grep -oP 'exec/s: \K\d+' baseline_stats.txt | tail -1 || echo "0")

echo ""
echo "Baseline Results:"
echo "  Coverage: $BASELINE_COV blocks"
echo "  Corpus size: $BASELINE_CORP inputs"
echo "  Exec/s: $BASELINE_EXECS"
echo ""

# Step 3: Generate intelligent seeds
echo "=== Generating Intelligent Seeds ==="
cd "$BUILD_DIR"

# Check if py_proto exists
if [ ! -d "py_proto" ]; then
    echo "[-] py_proto directory not found, regenerating..."
    cd "$ROOT_DIR"
    ./src/pin_diff.sh "$EXAMPLES_DIR/check_num.c" checkNum --fuzz-seconds=0
    cd "$BUILD_DIR"
fi

# Generate seeds
if [ -d "seed_corpus" ]; then
    rm -rf seed_corpus
fi

python3 "$ROOT_DIR/src/generate_intelligent_seeds.py" \
    --py-proto-dir py_proto \
    --module input_pb2 \
    --message Input \
    --output seed_corpus \
    --max-seeds 500 \
    --clean-output

SEEDS_GENERATED=$(ls seed_corpus/*.bin 2>/dev/null | wc -l || echo "0")
echo "[+] Generated $SEEDS_GENERATED seeds"
echo ""

# Step 4: Fuzzing with intelligent seeds
echo "=== EXPERIMENT: Fuzzing WITH intelligent seeds ==="

# Clear existing corpus
if [ -d "corpus_with_seeds" ]; then
    rm -rf corpus_with_seeds
fi
mkdir -p corpus_with_seeds

# Copy seeds to corpus
cp seed_corpus/*.bin corpus_with_seeds/

# Run fuzzer for 30 seconds starting from seed corpus
timeout 30 ./fuzz_bytes corpus_with_seeds -max_total_time=30 -print_final_stats=1 2>&1 | tee seeds_stats.txt || true

# Extract stats
SEEDS_COV=$(grep -oP 'cov: \K\d+' seeds_stats.txt | tail -1 || echo "0")
SEEDS_CORP=$(ls corpus_with_seeds/*.bin 2>/dev/null | wc -l || echo "0")
SEEDS_EXECS=$(grep -oP 'exec/s: \K\d+' seeds_stats.txt | tail -1 || echo "0")

echo ""
echo "With Seeds Results:"
echo "  Coverage: $SEEDS_COV blocks"
echo "  Corpus size: $SEEDS_CORP inputs"
echo "  Exec/s: $SEEDS_EXECS"
echo ""

# Step 5: Compare results
echo "=== COMPARISON ==="
echo ""
printf "%-20s %10s %10s %10s\n" "Metric" "Baseline" "With Seeds" "Gain"
printf "%-20s %10s %10s %10s\n" "--------------------" "----------" "----------" "----------"
printf "%-20s %10d %10d %10s\n" "Coverage (blocks)" "$BASELINE_COV" "$SEEDS_COV" "+$(($SEEDS_COV - $BASELINE_COV))"
printf "%-20s %10d %10d %10s\n" "Corpus size" "$BASELINE_CORP" "$SEEDS_CORP" "+$(($SEEDS_CORP - $BASELINE_CORP))"
printf "%-20s %10d %10d %10s\n" "Exec/s" "$BASELINE_EXECS" "$SEEDS_EXECS" "+$(($SEEDS_EXECS - $BASELINE_EXECS))"

# Calculate percentage improvement
if [ "$BASELINE_COV" -gt 0 ]; then
    COV_IMPROVEMENT=$((($SEEDS_COV - $BASELINE_COV) * 100 / $BASELINE_COV))
    echo ""
    echo "Coverage improvement: ${COV_IMPROVEMENT}%"
fi

echo ""
echo "=== Seed Quality Analysis ==="
echo ""

# Test how many seeds are valid (don't crash or get rejected)
VALID_SEEDS=0
TOTAL_SEEDS=$(ls seed_corpus/*.bin 2>/dev/null | wc -l || echo "0")

echo "Testing individual seeds for validity..."
for seed in seed_corpus/*.bin; do
    if [ -f "$seed" ]; then
        timeout 0.1 ./fuzz_bytes "$seed" &>/dev/null && ((VALID_SEEDS++)) || true
    fi
done

VALID_RATE=$((VALID_SEEDS * 100 / TOTAL_SEEDS))

echo "Valid seeds: $VALID_SEEDS / $TOTAL_SEEDS ($VALID_RATE%)"
echo ""

# Step 6: Sample some seeds to inspect
echo "=== Sample Generated Seeds ==="
echo ""
for i in 0 1 2; do
    seed_file=$(ls seed_corpus/*.bin | head -$((i+1)) | tail -1)
    if [ -f "$seed_file" ]; then
        echo "Seed $i: $(basename $seed_file)"
        xxd "$seed_file" | head -3
        echo ""
    fi
done

echo "=== Test Complete ==="
echo ""
echo "Detailed logs:"
echo "  Baseline: $BUILD_DIR/baseline_stats.txt"
echo "  With seeds: $BUILD_DIR/seeds_stats.txt"
echo "  Seed corpus: $BUILD_DIR/seed_corpus/"
echo ""
