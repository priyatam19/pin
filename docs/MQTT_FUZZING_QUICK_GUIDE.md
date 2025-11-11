# MQTT Fuzzing Quick Reference Guide

## Your Current Status

✅ **AFL is working perfectly** - keeps fuzzing after crashes
⚠️ **PIN/LibFuzzer stops on first crash** - needs the fix below

## Key Findings

**Total Crashes Found**: 11
- CVE-mongoose-0001 (heap overflow): 10 crashes (90.9%) ✅
- CVE-mongoose-0002 (stack overflow): 0 crashes ❌
- Unknown (OOM): 1 crash

## Location of Key Files

```
AFL Setup:
/home/priyatam/eboss/evaluation-1SourceFuzzing/mqtt-server-main/
├── fuzz_driver_unified          # Your AFL binary
├── afl_out/default/crashes/     # 11 crashes found
├── classify_crash.sh            # Classify single crash
├── analyze_all_crashes.sh       # Classify all crashes
├── FUZZING_ANALYSIS.md          # Detailed analysis
└── FUZZING_RESULTS_SUMMARY.md   # Complete summary

PIN Setup:
/home/priyatam/pin/
├── build/mqtt_parse_diff/       # mg_mqtt_parse fuzzing
├── build/mqtt_send_header_diff/ # mg_mqtt_send_header fuzzing
└── fix_libfuzzer_crashes.sh     # Fix script for crash continuation
```

## Quick Commands

### Check AFL Status
```bash
# Attach to running AFL session
tmux attach -t mqtt_afl

# View AFL statistics
afl-whatsup /home/priyatam/eboss/evaluation-1SourceFuzzing/mqtt-server-main/afl_out
```

### Analyze Crashes
```bash
cd /home/priyatam/eboss/evaluation-1SourceFuzzing/mqtt-server-main

# Classify all crashes (already done, results in crash_analysis_YYYYMMDD_HHMMSS/)
./analyze_all_crashes.sh

# Classify a specific crash
./classify_crash.sh afl_out/default/crashes/<crash_file> ./fuzz_driver_unified
```

### Fix PIN/LibFuzzer Crash Continuation
```bash
cd /home/priyatam/pin

# Automatically update all PIN harnesses
./fix_libfuzzer_crashes.sh

# Or manually update a specific harness:
cd /home/priyatam/pin/build/mqtt_parse_diff
cp bytes_fuzz.cc bytes_fuzz.cc.orig
# Then edit bytes_fuzz.cc with the crash continuation code (see FUZZING_ANALYSIS.md)

# Rebuild
clang++ -g -O1 -fsanitize=fuzzer,address \
    bytes_fuzz.cc main.c input.nanopb.o pb_decode.o pb_common.o \
    original_plain.o extra_0_mongoose_stubs.plain.o extra_0_fs.plain.o \
    -o fuzz_bytes

# Run fuzzing with crash continuation
./fuzz_bytes -max_total_time=3600 -artifact_prefix=artifacts/ corpus/
```

## Why LibFuzzer Stops vs AFL Continues

**AFL**: Runs each test in a separate child process
→ Crash kills child, parent continues

**LibFuzzer**: Runs tests in-process (faster)
→ Crash kills fuzzer (stops)
→ FIX: Add signal handlers to catch crashes

## Understanding the CVEs

### CVE-mongoose-0001 ✅ FOUND
- **Function**: `mg_mqtt_parse()`
- **Type**: Heap buffer overflow
- **Trigger**: MQTT packet with malformed length field
- **Found**: 10 crashes
- **Signature**: `heap-buffer-overflow in mg_mqtt_next_topic`

### CVE-mongoose-0002 ❌ NOT FOUND YET
- **Function**: `mg_mqtt_send_header()`
- **Type**: Stack buffer overflow
- **Trigger**: Very large length value (>268M, needs 5-byte encoding)
- **Why not found**: Current harness focuses on parsing (input), not sending (output)
- **To find**: Target `mg_mqtt_send_header` directly with large length values

## Next Steps

1. ✅ AFL is working - let it continue running
2. ⚠️ Fix PIN/LibFuzzer: Run `/home/priyatam/pin/fix_libfuzzer_crashes.sh`
3. 📊 Review crash analysis: Check `crash_analysis_YYYYMMDD_HHMMSS/SUMMARY.txt`
4. 🎯 Target CVE-mongoose-0002: Create harness for `mg_mqtt_send_header`

## Full Documentation

- Detailed analysis: `/home/priyatam/eboss/evaluation-1SourceFuzzing/mqtt-server-main/FUZZING_ANALYSIS.md`
- Results summary: `/home/priyatam/eboss/evaluation-1SourceFuzzing/mqtt-server-main/FUZZING_RESULTS_SUMMARY.md`
- Crash reports: `crash_analysis_YYYYMMDD_HHMMSS/` directory
