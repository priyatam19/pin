# PIN Architecture Deep Dive: Normalized vs Reference Binaries

## Your Understanding vs. Current Implementation

### Your Initial Understanding ✅ **CORRECT!**

You understood it correctly:

**Normalized Binary** = Wrapper (decode bytes → call original function) using **nanopb**

**Reference Binary** = Wrapper (decode bytes → call original function) using **C++ Protobuf** (by default)

**Both binaries**:
- Read the same serialized protobuf bytes from disk
- Decode those bytes into structured data
- Call the **exact same original C function** with the decoded parameters
- The only difference is **which protobuf decoder** is used

---

## Detailed Architecture Breakdown

### 1. Normalized Binary (`normalized_bin`)

**Components**:
```
normalized_bin = main.c (wrapper)
                 + input.pb.c (nanopb generated code)
                 + pb_decode.o (nanopb runtime)
                 + pb_common.o (nanopb runtime)
                 + original_plain.o (original C function)
```

**Workflow**:
```c
// main.c (auto-generated wrapper)
int pin_wrapper_entry(const uint8_t *data, size_t len) {
    Input input = Input_init_zero;

    // 1. Decode bytes using NANOPB
    pb_istream_t stream = pb_istream_from_buffer(data, len);
    if (!pb_decode(&stream, Input_fields, &input)) {
        return 1;  // Decode failed
    }

    // 2. EMI validation guards
    if (emi_checks_fail) {
        return 86;  // EMI rejection
    }

    // 3. Call original function
    checkNum(input.N);  // Original C function!

    return 0;
}

int main(int argc, char *argv[]) {
    // Read bytes from file
    uint8_t *buf = read_file(argv[1]);

    // Call wrapper
    int rc = pin_wrapper_entry(buf, len);

    return rc;
}
```

**Key Point**: Uses **nanopb decoder** (C implementation, lenient UTF-8 validation)

---

### 2. Reference Binary (Option 1: C++ Protobuf) - DEFAULT

**Components**:
```
original_replay_bin = reference_runner.cc (wrapper)
                      + input.pb.cc (C++ protobuf generated code)
                      + original_plain.o (same original C function!)
                      + libprotobuf.so (Google C++ Protobuf runtime)
```

**Workflow**:
```cpp
// reference_runner.cc (auto-generated wrapper)
#include "cpp_proto/input.pb.h"

extern "C" int checkNum(int N);  // Same original function!

int pin_reference_entry(const uint8_t *data, size_t len) {
    Input msg;  // C++ protobuf message

    // 1. Decode bytes using C++ PROTOBUF
    if (!msg.ParseFromArray(data, len)) {
        return 1;  // Decode failed (includes UTF-8 validation!)
    }

    // 2. Extract values from protobuf message
    int N = msg.n();

    // 3. Call SAME original function
    checkNum(N);  // Same function as normalized!

    return 0;
}

int main(int argc, char *argv[]) {
    // Read bytes from file
    std::vector<uint8_t> buffer = read_file(argv[1]);

    // Call wrapper
    int rc = pin_reference_entry(buffer.data(), buffer.size());

    return rc;
}
```

**Key Point**: Uses **C++ Protobuf decoder** (C++ implementation, strict UTF-8 validation)

---

### 3. Reference Binary (Option 2: nanopb) - ALTERNATIVE

**Components**:
```
reference_nanopb_bin = reference_nanopb_main.c (thin main wrapper)
                       + wrapper.o (SAME wrapper as normalized!)
                       + input.pb.c (nanopb generated code)
                       + pb_decode.o (nanopb runtime)
                       + original_plain.o (same original C function)
```

**Workflow**:
```c
// reference_nanopb_main.c (minimal main)
extern int pin_wrapper_entry(const uint8_t *data, size_t len);

int main(int argc, char *argv[]) {
    uint8_t *buf = read_file(argv[1]);

    // Calls EXACT SAME wrapper as normalized!
    int rc = pin_wrapper_entry(buf, len);

    return rc;
}
```

**Key Point**: Uses **exact same nanopb decoder as normalized binary**

---

## Why Two Binaries? Purpose and Rationale

### The Purpose is **Differential Testing**

The idea is to **detect bugs** by comparing:

1. **Normalized binary** (the "system under test")
   - Uses nanopb decoder
   - Has EMI guards
   - Our generated wrapper code

2. **Reference binary** (the "oracle")
   - Different decoder (by default C++ Protobuf)
   - No EMI guards (just decodes and calls)
   - Simpler wrapper

**Goal**: If both binaries:
- Read the same bytes
- Decode successfully
- Call the same original function

Then they **should produce identical output** (stdout, stderr, return code).

**If outputs differ → We found a bug!**

### What Kind of Bugs?

This differential testing can reveal:

1. **Tool bugs** (what we found):
   - Uninitialized buffers in wrapper generation
   - Code generation errors (void pointers)
   - Malformed protobuf handling differences

2. **Decoder bugs/differences**:
   - UTF-8 validation inconsistencies
   - Wiretype handling differences
   - Edge case parsing mismatches

3. **EMI guard bugs** (false positives/negatives):
   - Guards rejecting valid inputs
   - Guards accepting invalid inputs

4. **Original program bugs** (if both succeed but differ):
   - Non-determinism in original code
   - Undefined behavior

---

## The Critical Problem: Decoder Inconsistency

### Current Default Setup (CAUSES PROBLEMS)

```
Normalized:  bytes → [nanopb decoder] → struct → call original()
Reference:   bytes → [C++ protobuf decoder] → struct → call original()
                          ↑ DIFFERENT DECODERS ↑
```

### Why This Causes 67-73% of DIFFs

**Example**: Invalid UTF-8 byte `0xFF` in string field

```
Input bytes: [0x0a 0x01 0xff]  (string field with invalid UTF-8)

┌─────────────────────────────────────────────────────────────┐
│ Normalized (nanopb)                                         │
├─────────────────────────────────────────────────────────────┤
│ 1. Decode: ✓ SUCCESS (nanopb doesn't validate UTF-8)       │
│ 2. Value: mode = "\xff" (accepts byte)                      │
│ 3. Call: original_function("\xff")                          │
│ 4. RC: 0                                                     │
│ 5. Output: "mode:other:\xff"                                │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ Reference (C++ Protobuf)                                    │
├─────────────────────────────────────────────────────────────┤
│ 1. Decode: ✗ FAILURE (C++ validates UTF-8)                 │
│ 2. Error: "String field contains invalid UTF-8"             │
│ 3. Call: NEVER REACHED                                      │
│ 4. RC: 1 (decode error)                                     │
│ 5. Output: (error message)                                  │
└─────────────────────────────────────────────────────────────┘

Result: DIFF! (But not a real bug - just decoder difference)
```

**This is an ARTIFICIAL DIFF** caused by inconsistent decoders, not a bug in the original program or wrapper!

---

## Solution: Use Single Decoder

### Option A: Use nanopb for BOTH binaries ✅ **RECOMMENDED**

```bash
./src/pin_diff.sh examples/test.c func --reference-decoder=nanopb
```

**What happens**:
```
Normalized:  bytes → [nanopb] → struct → call original()
Reference:   bytes → [nanopb] → struct → call original()
                      ↑ SAME DECODER ↑
```

**Benefits**:
- ✅ Eliminates 67-73% of artificial DIFFs immediately
- ✅ Both binaries behave identically for decoder edge cases
- ✅ Only real bugs (wrapper code, EMI guards, original program) cause DIFFs
- ✅ Simpler: no need for libprotobuf dependency
- ✅ C-friendly: nanopb matches C semantics (char* = bytes, no UTF-8 enforcement)

**Drawbacks**:
- ❌ Loses protobuf spec compliance checking (no UTF-8 validation)
- ❌ Reference binary is almost identical to normalized (less independent verification)

---

### Option B: Keep C++ Protobuf + Change to `bytes` Type

**Alternative Fix**: Instead of changing decoder, change the proto schema:

```protobuf
# Current (causes problems):
message Input {
  string mode = 1;  // ← C++ Protobuf enforces UTF-8!
}

# Proposed (eliminates DIFFs):
message Input {
  bytes mode = 1;   // ← Both decoders handle bytes identically!
}
```

**In src/pycparser_generate_proto.py**:
```python
# Current mapping:
"char*" → "string"
"char[]" → "string"

# Proposed mapping:
"char*" → "bytes"
"char[]" → "bytes"
```

**Benefits**:
- ✅ Eliminates UTF-8 validation DIFFs
- ✅ Matches C semantics (char* is raw bytes, not UTF-8 strings)
- ✅ Both decoders handle bytes identically
- ✅ Keeps independent decoders (C++ vs nanopb)
- ✅ Better for differential testing (two implementations verify each other)

**Drawbacks**:
- ❌ Loses semantic info (bytes vs string)
- ❌ If you want to test UTF-8 handling, this won't work

---

### Option C: Add UTF-8 Validation to nanopb ⚠️ **NOT RECOMMENDED**

Make nanopb strict like C++ Protobuf:

```c
bool decode_string(pb_istream_t *stream, ...) {
    // ... existing decode logic ...

    // Add UTF-8 validation:
    if (!is_valid_utf8(buffer, len)) {
        return false;  // Reject like C++
    }

    return true;
}
```

**Benefits**:
- ✅ Both decoders reject invalid UTF-8
- ✅ Protobuf spec compliant

**Drawbacks**:
- ❌ Doesn't match C semantics (C doesn't validate UTF-8)
- ❌ Requires implementing UTF-8 validator
- ❌ Original C program might legitimately use non-UTF-8 bytes
- ❌ Fuzzer won't explore non-UTF-8 inputs

---

## Would Single Decoder Solve ALL Problems?

### YES ✅ - Problems It WOULD Solve:

1. **UTF-8 validation mismatches** (67-73% of DIFFs) ✅
2. **Wiretype handling differences** (malformed protobuf) ✅
3. **Edge case parsing differences** ✅
4. **Decode error inconsistencies** ✅

### NO ❌ - Problems It WOULD NOT Solve:

1. **Uninitialized buffers** (Categories 1, 5, 6, 10) ❌
   - Still need to fix: `char buf[128] = {0};`

2. **Void pointer code generation** (Category 7) ❌
   - Still need to fix code generator

3. **EMI guard bugs** (if any exist) ❌
   - Still need EMI guards for semantic constraints

4. **Double pointer semantic ambiguity** (Category 8) ❌
   - Still a design issue

**Bottom Line**: Using a single decoder would eliminate ~70% of DIFFs, but you still need to fix the other tool bugs.

---

## Which Decoder is Best for Real-World Programs?

### For Real-World Fuzzing: **nanopb** ✅

**Reasons**:

1. **Matches C semantics**
   - C `char*` is raw bytes, not UTF-8
   - C programs don't validate encodings
   - Real-world C code uses arbitrary byte sequences

2. **Better fuzzing coverage**
   - Explores non-UTF-8 inputs
   - Tests edge cases C++ Protobuf rejects
   - Finds bugs hidden by strict validation

3. **Simpler dependencies**
   - No libprotobuf.so needed
   - Smaller binary
   - Easier deployment

4. **Consistent behavior**
   - Using nanopb for both binaries eliminates decoder-related DIFFs
   - All DIFFs become actionable (real bugs or EMI issues)

5. **Real-world compatibility**
   - Libraries like libtiff, libpng, curl work with raw bytes
   - No encoding assumptions
   - Matches actual API contracts

### Current Recommendation

**Default invocation**:
```bash
./src/pin_diff.sh examples/libtiff/tif_dirread.c TIFFReadDirectory \
  --libs="-ltiff" \
  --headers-dir=utils/libtiff_headers \
  --reference-decoder=nanopb \  # ← Use nanopb for reference!
  --fuzz-seconds=300
```

**Proto schema change** (in parallel):
```python
# In src/pycparser_generate_proto.py
# Change C string mapping:
def map_c_type_to_proto(c_type):
    if c_type in ["char*", "const char*"]:
        return "bytes"  # NOT "string"
    # ...
```

---

## Summary: Is the Current Workflow Correct?

### Your Understanding: **100% CORRECT** ✅

Both binaries:
1. Read bytes from disk
2. Decode bytes into structured data
3. Call the SAME original C function
4. Only difference: which decoder is used

### The Problem is NOT the Workflow ❌

The workflow is correct! The problem is:

1. **Using different decoders by default** (nanopb vs C++ Protobuf)
   - Causes 67-73% of artificial DIFFs
   - Solution: Use `--reference-decoder=nanopb`

2. **Tool bugs in wrapper generation**
   - Uninitialized buffers
   - Void pointer code generation
   - Solution: Fix code generator

3. **Wrong type mapping** (string vs bytes)
   - C char* should map to bytes, not string
   - Solution: Change proto generation

### The Workflow is NOT Redundant ✅

Having two binaries serves a critical purpose:

**Differential testing catches bugs that unit tests miss!**

Example: The uninitialized buffer bug would be hard to catch with unit tests (depends on stack state), but differential testing immediately shows:

```
Same input → Different output → BUG!
```

Even with the same decoder, differential testing is valuable because:
- Normalized uses generated wrapper code (complex)
- Reference uses simpler wrapper (hand-written template)
- Differences reveal wrapper generation bugs

---

## Actionable Plan

### Immediate (Fix 90% of DIFFs)

1. **Use nanopb for reference decoder**:
   ```bash
   --reference-decoder=nanopb
   ```

2. **Change string → bytes mapping**:
   ```python
   # src/pycparser_generate_proto.py
   "char*" → "bytes"  # NOT "string"
   ```

3. **Fix uninitialized buffers**:
   ```python
   # src/generate_wrapper_ast.py
   char buf[128] = {0};  # NOT char buf[128];
   ```

### Medium Term (Fix remaining bugs)

4. **Fix void pointer code generation**
5. **Add wiretype validation** to nanopb
6. **Design double pointer modeling**

### Long Term (Real-world fuzzing)

7. Use nanopb exclusively for production fuzzing
8. Keep C++ Protobuf option for spec compliance testing
9. Integrate with libtiff, libpng, curl, etc.

---

## Final Answer to Your Questions

1. **Does current code match your understanding?** YES ✅
2. **Did we diverge from the idea?** NO ✅
3. **Is having two binaries redundant?** NO - critical for bug detection ✅
4. **Do different decoders cause inconsistencies?** YES - 67-73% of DIFFs ✅
5. **Would single decoder solve all problems?** 70% yes, 30% no ⚠️
6. **Which decoder for real-world programs?** **nanopb** ✅
