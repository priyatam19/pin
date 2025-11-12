#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
TARGET="$ROOT_DIR/examples/simple_benchs/raw_passthrough_example.c"
HEADER_BASENAME="raw_passthrough_example.h"
BUILD_NAME="raw_passthrough_example_diff"

echo "[+] Testing pass-through mode on ${TARGET}"
PIN_BUILD_DIR="$ROOT_DIR/build/${BUILD_NAME}_test" \
PIN_RESULTS_DIR="$ROOT_DIR/results/${BUILD_NAME}_test" \
"$ROOT_DIR/src/pin_diff.sh" \
    "$TARGET" raw_passthrough_parser \
    --input-mode=raw \
    --pass-through-header="$HEADER_BASENAME" \
    --headers-dir="$ROOT_DIR/examples/simple_benchs" \
    --fuzz-seconds=1 \
    >/tmp/pin_pass_through_test.log 2>&1

SUMMARY_FILE="$ROOT_DIR/results/${BUILD_NAME}_test/stage_b/replay_summary.txt"
if [[ ! -s "$SUMMARY_FILE" ]]; then
    echo "[-] Pass-through smoke test failed: replay summary missing"
    cat /tmp/pin_pass_through_test.log
    exit 1
fi

LINES=$(wc -l < "$SUMMARY_FILE")
echo "[+] Pass-through smoke test completed (replay entries: $LINES)"
