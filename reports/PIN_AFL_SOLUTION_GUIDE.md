# PIN vs AFL: Practical Solution Guide

## TL;DR: Root Cause

❌ **Current State**: PIN cannot find the same crashes as AFL because:
- AFL fuzzes **raw MQTT bytes** → `mg_mqtt_parse` expects this ✅
- PIN fuzzes **protobuf bytes** → `mg_mqtt_parse` rejects this ❌

## Two Solutions

### Solution 1: Target Structured-Input Functions ✅ RECOMMENDED

Use PIN on functions that expect **parsed structures**, not raw bytes.

### Solution 2: Add Pass-Through Mode to PIN 🔧 REQUIRES MODIFICATION

Modify PIN to detect parser functions and bypass protobuf encoding.

---

## Solution 1: Testing PIN on Appropriate Functions

### Good Candidates for PIN (Structured Inputs)

From `mongoose.h`, these functions take **structs**, not raw bytes:

```c
// ✅ GOOD: Takes parsed struct
size_t mg_mqtt_next_sub(struct mg_mqtt_message *msg,
                        struct mg_str *topic,
                        uint8_t *qos,
                        size_t pos);

// ✅ GOOD: Takes parsed struct
size_t mg_mqtt_next_unsub(struct mg_mqtt_message *msg,
                          struct mg_str *topic,
                          size_t pos);

// ✅ GOOD: Takes parsed struct
size_t mg_mqtt_next_prop(struct mg_mqtt_message *msg,
                         struct mg_mqtt_prop *prop,
                         size_t ofs);
```

### Bad Candidates for PIN (Raw Byte Parsers)

```c
// ❌ BAD: Takes raw bytes
int mg_mqtt_parse(const uint8_t *buf, size_t len,
                  uint8_t version,
                  struct mg_mqtt_message *m);
```

### How to Test Hypothesis Correctly

**Step 1**: Create AFL harness for `mg_mqtt_next_sub`

```c
// afl_next_sub_harness.c
#include "mongoose.h"
#include <stdint.h>
#include <stdlib.h>

int main() {
    uint8_t input[8192];
    size_t len = read(0, input, sizeof(input));

    // Parse into struct first (same as real usage)
    struct mg_mqtt_message msg = {0};
    if (mg_mqtt_parse(input, len, 4, &msg) == MQTT_OK) {
        // Now test mg_mqtt_next_sub with parsed struct
        struct mg_str topic;
        uint8_t qos;
        size_t pos = 0;

        while ((pos = mg_mqtt_next_sub(&msg, &topic, &qos, pos)) > 0) {
            // Process subscription
        }
    }

    return 0;
}
```

**Step 2**: Run AFL on structured function

```bash
cd /home/priyatam/eboss/evaluation-1SourceFuzzing/mqtt-server-main

# Compile AFL harness
afl-gcc -o afl_next_sub_harness afl_next_sub_harness.c mongoose.c \
    -fsanitize=address -g

# Fuzz it
afl-fuzz -i inputs_mqtt -o afl_out_next_sub -- ./afl_next_sub_harness
```

**Step 3**: Run PIN on same function

```bash
cd /home/priyatam/pin

# Target mg_mqtt_next_sub with PIN
./src/pin_diff.sh \
    /home/priyatam/eboss/.../mongoose.c \
    mg_mqtt_next_sub \
    --fuzz-seconds=3600 \
    --reference-decoder=nanopb
```

**Step 4**: Compare Results

```bash
# Compare crashes found
AFL crashes: afl_out_next_sub/default/crashes/
PIN crashes: build/mg_mqtt_next_sub_diff/artifacts/

# Expected: Both should find similar vulnerabilities!
# This validates your hypothesis ✅
```

---

## Solution 2: Add Pass-Through Mode to PIN

### Modify PIN to Support Raw Byte Functions

**Step 1**: Detect parser functions in `pycparser_generate_proto.py`

```python
# Add to src/pycparser_generate_proto.py

def is_raw_byte_parser(func_decl):
    """Detect if function takes raw bytes (uint8_t *buf, size_t len)"""
    params = func_decl.type.args.params if func_decl.type.args else []

    if len(params) < 2:
        return False

    # Check for (uint8_t *buf, size_t len) pattern
    first_param = params[0]
    second_param = params[1]

    # First param should be uint8_t * or const uint8_t *
    if isinstance(first_param.type, c_ast.PtrDecl):
        base_type = get_type_string(first_param.type.type)
        if 'uint8_t' in base_type or 'unsigned char' in base_type:
            # Second param should be size_t
            second_type = get_type_string(second_param.type)
            if 'size_t' in second_type or 'int' in second_type:
                return True

    return False

def generate_passthrough_proto(func_name):
    """Generate simple pass-through proto for raw byte parsers"""
    return f"""syntax = "proto3";

message Input {{
  bytes raw_data = 1;  // Pass raw bytes directly to parser
}}
"""

# In main proto generation logic:
if is_raw_byte_parser(func_decl):
    print(f"DETECTED: {func_name} is a raw byte parser")
    print("Generating pass-through wrapper...")
    proto_content = generate_passthrough_proto(func_name)
```

**Step 2**: Generate pass-through wrapper in `generate_wrapper_ast.py`

```python
# Add to src/generate_wrapper_ast.py

def generate_passthrough_wrapper(func_name, params):
    """Generate wrapper that passes bytes directly without protobuf decoding"""

    # For mg_mqtt_parse(const uint8_t *buf, size_t len, uint8_t version, struct mg_mqtt_message *m)
    # We want to pass libFuzzer bytes directly as buf

    return f"""
int pin_wrapper_entry(const uint8_t *data, size_t len) {{
    // Pass-through mode: use fuzzer bytes directly as parser input

    // Additional parameters (use sensible defaults)
    uint8_t version = 4;  // MQTT v4
    struct mg_mqtt_message msg = {{0}};

    // Call parser directly with raw fuzzer bytes
    {func_name}(data, len, version, &msg);

    return 0;
}}
"""
```

**Step 3**: Test the modified PIN

```bash
cd /home/priyatam/pin

# After applying modifications
./src/pin_diff.sh mongoose.c mg_mqtt_parse --fuzz-seconds=300

# Should now see:
# DETECTED: mg_mqtt_parse is a raw byte parser
# Generating pass-through wrapper...

# Result: PIN should now find the same crashes as AFL! ✅
```

---

## Quick Comparison Test

### Test Case: CVE-mongoose-0001

**What AFL found**:
```
Input: 82 82 00 02 00 (raw MQTT)
Crash: heap-buffer-overflow in mg_mqtt_next_topic
```

**What PIN currently does**:
```
Input: 30 02 00 00 00 (protobuf)
Result: mg_mqtt_parse rejects invalid MQTT → NO CRASH
```

**What PIN with pass-through should do**:
```
Input: 82 82 00 02 00 (raw bytes passed directly)
Result: Same crash as AFL → HYPOTHESIS VALIDATED ✅
```

---

## Recommended Experimental Plan

### Phase 1: Validate PIN on Structured Functions (1 day)

```bash
# Pick 3 structured-input functions
TARGETS=(
    "mg_mqtt_next_sub"
    "mg_mqtt_next_unsub"
    "mg_mqtt_next_prop"
)

# For each:
# 1. Write AFL harness
# 2. Fuzz with AFL for 1 hour
# 3. Fuzz with PIN for 1 hour
# 4. Compare crashes and coverage
```

**Expected Result**: PIN finds similar bugs to AFL on structured functions ✅

### Phase 2: Implement Pass-Through Mode (2-3 days)

1. Add parser detection to `pycparser_generate_proto.py`
2. Modify wrapper generation in `generate_wrapper_ast.py`
3. Test on `mg_mqtt_parse`
4. Verify crashes match AFL

**Expected Result**: PIN finds CVE-mongoose-0001 ✅

### Phase 3: Full Evaluation (1 week)

Run comprehensive comparison:

| Target | Input Type | AFL Crashes | PIN Crashes | Match? |
|--------|-----------|-------------|-------------|--------|
| `mg_mqtt_parse` | Raw bytes | 10 | ? | Test with pass-through |
| `mg_mqtt_next_sub` | Struct | ? | ? | Test baseline |
| `mg_mqtt_next_unsub` | Struct | ? | ? | Test baseline |
| `mg_mqtt_send_header` | Params | 0 | ? | Test both |

---

## Immediate Action Items

### Option A: Quick Validation (Recommended)

Test PIN on a structured function RIGHT NOW to see if it works:

```bash
cd /home/priyatam/pin

# Target mg_mqtt_next_sub (takes struct, not raw bytes)
./src/pin_diff.sh \
    /home/priyatam/eboss/evaluation-1SourceFuzzing/mqtt-server-main/mongoose.c \
    mg_mqtt_next_sub \
    --fuzz-seconds=600 \
    --extra-sources=/home/priyatam/eboss/.../fs.c \
    --reference-decoder=nanopb

# Check if crashes are found
ls -lh build/mg_mqtt_next_sub_diff/artifacts/
```

### Option B: Implement Pass-Through (More work)

Modify PIN to support raw byte functions:

1. Edit `src/pycparser_generate_proto.py` (add detection)
2. Edit `src/generate_wrapper_ast.py` (add pass-through wrapper)
3. Test on `mg_mqtt_parse`
4. Compare with AFL crashes

---

## Expected Paper Contributions

### Before Fix

> "PIN successfully handles structured inputs but requires modification for raw-byte parser functions."

### After Fix

> "PIN achieves X% crash coverage compared to AFL across Y test functions, including both structured inputs and raw-byte parsers via pass-through mode."

### Key Insight

> "The choice of input encoding (protobuf vs. raw bytes) significantly impacts attack surface coverage. We extended PIN with automatic detection and pass-through mode for parser functions, achieving coverage parity with AFL."

---

## Files Created

- **Analysis**: `/home/priyatam/PIN_AFL_HYPOTHESIS_ANALYSIS.md` (detailed root cause)
- **Solution**: This file (practical solutions)
- **Reference**: `/home/priyatam/MQTT_FUZZING_QUICK_GUIDE.md` (AFL status)

---

## Summary

**Problem**: PIN can't find AFL's crashes because input format mismatch

**Root Cause**: PIN uses protobuf, `mg_mqtt_parse` expects raw MQTT bytes

**Solution 1** ✅: Test PIN on structured functions (works today)

**Solution 2** 🔧: Add pass-through mode to PIN (requires modification)

**Next Step**: Run Option A quick test to validate PIN works on structured inputs
