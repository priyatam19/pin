# Intelligent Seed Generation Strategies for PIN

**Created**: November 11, 2025
**Goal**: Generate high-quality seed corpus to achieve >50% valid input rate without sharing full source code with LLMs
**Context**: PIN already generates `.proto` schemas and has rich metadata from AST analysis

---

## Executive Summary

**Problem**: Random protobuf bytes produce <1% valid inputs (vs Utopia: 80%, OSS-Fuzz-Gen: 60%+)

**Solution Space**: 7 strategies ranked by robustness, universality, and privacy

| Strategy | Valid Input Rate | Universality | Privacy | Complexity | Priority |
|----------|-----------------|--------------|---------|------------|----------|
| **1. Type-Driven Value Enumeration** | 30-40% | ✅ High | ✅ Full | 🟢 Low | ⚡ P0 |
| **2. Proto-Aware Constraint Mining** | 40-60% | ✅ High | ✅ Full | 🟡 Medium | ⚡ P0 |
| **3. Field-Name Semantic Analysis** | 50-70% | 🟡 Medium | ✅ Full | 🟢 Low | ⚡ P1 |
| **4. AST-Guided Coverage Seeding** | 60-80% | ✅ High | ✅ Full | 🟡 Medium | ⚡ P1 |
| **5. Minimal LLM (Proto-Only)** | 60-80% | ✅ High | 🟡 Proto only | 🟢 Low | 🔵 P2 |
| **6. Test Mining (Utopia-style)** | 70-90% | 🔴 Depends on tests | ✅ Full | 🟡 Medium | 🔵 P2 |
| **7. Example Mining (WildSync-style)** | 70-90% | 🟡 Medium | 🟡 Queries only | 🔴 High | 🔵 P3 |

**Recommended Combo**: **Strategy 1 + 2 + 4** (fully local, universal, 60-80% valid rate)

---

## Implementation Status (Nov 12, 2025)

`src/generate_intelligent_seeds.py` implements Strategy #1 (type-driven enumeration) and is now wired into `pin_diff.sh` via `--generate-seeds[=N]`. When the flag is set, PIN generates the `.proto` schema as usual, enumerates deterministic boundary cases per field, writes them to `seed_corpus/`, and copies the `.bin` files into the Stage A corpus before fuzzing.

| Target / Run | Seeds? | Fuzz Flags | Time | Coverage (`cov`, `ft`) | Stage B Valid Inputs |
|--------------|--------|------------|------|------------------------|----------------------|
| `cJSON_Parse` (examples/cJSON/cJSON.c) | No | `-detect_leaks=0`, 1× ASAN | 30 s | `cov: 184`, `ft: 1939`, corpus 1005 | `1004 / 1004` (`match`) |
| `cJSON_Parse` (same command) | `--generate-seeds` (default 512) | `-detect_leaks=0`, 1× ASAN | 30 s | `cov: 184`, `ft: 1874`, corpus 988 | `986 / 986` (`match`) |

Even though the end-state coverage is virtually identical on this single-function benchmark, the seeded run begins with several hundred deterministic protobuf instances instead of a single zero input, which makes the experiment reproducible and provides an on-box corpus that can be handed directly to future mutators (e.g., libprotobuf-mutator). Upcoming work (Strategies #2 and #4) will mine constraints/coverage to bias those seeds further toward difficult branches.

---

## Strategy 1: Type-Driven Value Enumeration ⚡ **PRIORITY 0**

### Overview

**Idea**: Generate boundary-value seeds directly from proto field types without any code analysis.

**Privacy**: ✅ **100% local** - uses only proto schema (already generated)

**Universality**: ✅ **Works for every proto schema** - no code analysis needed

### Implementation

#### Step 1: Parse Proto Schema

```python
# Already available: build/<target>_diff/input.proto
import google.protobuf.descriptor_pb2 as desc_pb2

def load_proto_schema(proto_file):
    """Parse .proto file to extract field types and structure"""
    # Use protoc to generate descriptor
    # Returns: {field_name: field_type, ...}
    pass
```

#### Step 2: Type-Based Value Generators

```python
class TypeDrivenGenerator:
    """Generate boundary values for each proto type"""

    def generate_int32_values(self, field_name):
        """Boundary values for int32 fields"""
        return [
            0,           # Zero
            1,           # Minimal positive
            -1,          # Minimal negative
            127,         # Max int8
            128,         # Min int8 overflow
            255,         # Max uint8
            256,         # Min uint8 overflow
            32767,       # Max int16
            -32768,      # Min int16
            65535,       # Max uint16
            2147483647,  # Max int32
            -2147483648, # Min int32
        ]

    def generate_uint32_values(self, field_name):
        """Boundary values for uint32"""
        return [0, 1, 255, 256, 65535, 65536, 4294967295]

    def generate_string_values(self, field_name):
        """Boundary values for strings"""
        return [
            "",                    # Empty
            "a",                   # Single char
            "test",                # Short
            "A" * 127,             # Max short string
            "B" * 128,             # Overflow threshold
            "C" * 255,             # Max uint8 length
            "D" * 256,             # Overflow uint8
            "E" * 4096,            # Page size
            "\x00\x01\xff",        # Binary data
            "../../../../etc/passwd",  # Path traversal
            "../" * 100,           # Deep traversal
            "\n" * 100,            # Newlines
        ]

    def generate_bytes_values(self, field_name):
        """Boundary values for bytes fields"""
        return [
            b"",                   # Empty
            b"\x00",               # Null byte
            b"\xff",               # Max byte
            b"\x00" * 8,           # Nulls
            b"\xff" * 8,           # Max values
            bytes(range(256)),     # Full byte range
            b"A" * 4096,           # Large buffer
        ]

    def generate_bool_values(self, field_name):
        """All possible bool values"""
        return [False, True]

    def generate_repeated_counts(self):
        """How many elements in repeated fields"""
        return [
            0,      # Empty array
            1,      # Single element
            2,      # Pair
            10,     # Small array
            100,    # Medium array
            1000,   # Large array
        ]
```

#### Step 3: Combinatorial Seed Generation

```python
def generate_seeds_from_proto(proto_file, output_dir, max_seeds=1000):
    """
    Generate seed corpus by enumerating type-driven values

    Strategy:
    1. For each message type, generate seeds varying ONE field at a time
    2. Use default values for other fields
    3. Covers boundary conditions systematically
    """
    schema = load_proto_schema(proto_file)
    gen = TypeDrivenGenerator()

    seeds = []

    # Single-field variation (base case)
    for field_name, field_type in schema.fields.items():
        default_msg = create_default_message(schema)

        if field_type == "int32":
            for value in gen.generate_int32_values(field_name):
                msg = copy(default_msg)
                setattr(msg, field_name, value)
                seeds.append(msg)

        elif field_type == "string":
            for value in gen.generate_string_values(field_name):
                msg = copy(default_msg)
                setattr(msg, field_name, value)
                seeds.append(msg)

        # ... handle all types

    # Two-field combinations (for key relationships)
    for (field1, type1), (field2, type2) in itertools.combinations(schema.fields.items(), 2):
        # Generate seeds with both fields varied
        # Focus on length/pointer pairs (buf + len)
        if is_length_pair(field1, field2):
            seeds.extend(generate_length_mismatch_seeds(field1, field2))

    # Serialize to protobuf binary
    for i, seed in enumerate(seeds[:max_seeds]):
        serialize_proto(seed, f"{output_dir}/seed_{i:04d}.bin")

    return len(seeds)
```

#### Step 4: Special Generators for Common Patterns

```python
def generate_length_mismatch_seeds(ptr_field, len_field):
    """
    Generate seeds testing length mismatches (EMI violation triggers)
    Example: buf.len=10 but actual buf.data has 5 bytes
    """
    seeds = []
    for claimed_len in [0, 1, 10, 100, 1000]:
        for actual_len in [0, 1, claimed_len-1, claimed_len, claimed_len+1]:
            msg = Message()
            setattr(msg, len_field, claimed_len)
            setattr(msg, ptr_field, "A" * actual_len)
            seeds.append(msg)
    return seeds

def generate_nested_message_seeds(schema, field_name, nested_schema):
    """Generate seeds with nested messages at various depths"""
    seeds = []

    # Null nested message
    msg = Message()
    setattr(msg, field_name, None)
    seeds.append(msg)

    # Empty nested message
    msg = Message()
    setattr(msg, field_name, NestedMessage())
    seeds.append(msg)

    # Fully populated nested
    msg = Message()
    nested = create_populated_message(nested_schema)
    setattr(msg, field_name, nested)
    seeds.append(msg)

    return seeds
```

### Expected Results

**Valid Input Rate**: 30-40% (boundary values often satisfy basic constraints)

**Coverage**: Moderate (exercises different code paths per field type)

**Pros**:
- ✅ 100% local, no external dependencies
- ✅ Works for ANY proto schema
- ✅ Deterministic, reproducible
- ✅ Fast (generates 1000 seeds in <1 second)

**Cons**:
- ⚠️ No semantic understanding (cmd=99999 may be invalid enum)
- ⚠️ No field relationships (buf and len independent)
- ⚠️ No protocol-specific knowledge

**Timeline**: 2-3 days implementation

---

## Strategy 2: Proto-Aware Constraint Mining ⚡ **PRIORITY 0**

### Overview

**Idea**: Scan target function AST for simple guards (`if`, `switch`, `memcmp`) and extract constants/constraints. Use these to generate seeds that pass validation.

**Privacy**: ✅ **100% local** - uses libclang AST (already parsed)

**Universality**: ✅ **Works for any C function** - no external dependencies

### Implementation

#### Step 1: AST Scanning for Constraints

```python
from clang.cindex import CursorKind, TypeKind

class ConstraintExtractor:
    """Extract constraints from function body AST"""

    def __init__(self, func_cursor):
        self.constraints = {
            'enums': {},        # field → [valid_values]
            'ranges': {},       # field → (min, max)
            'lengths': {},      # field → [boundary_lengths]
            'strings': {},      # field → [expected_strings]
        }
        self.func_cursor = func_cursor

    def extract_constraints(self):
        """Walk AST and find validation patterns"""
        for node in self.func_cursor.walk_preorder():
            if node.kind == CursorKind.IF_STMT:
                self.extract_if_constraint(node)
            elif node.kind == CursorKind.SWITCH_STMT:
                self.extract_switch_constraint(node)
            elif node.kind == CursorKind.CALL_EXPR:
                self.extract_call_constraint(node)

        return self.constraints

    def extract_if_constraint(self, if_node):
        """
        Extract constraints from if statements

        Example:
          if (msg->cmd == MQTT_CONNECT) { ... }
          → constraint: msg.cmd ∈ {MQTT_CONNECT}

          if (msg->len < 4) return ERROR;
          → constraint: msg.len ≥ 4
        """
        condition = if_node.get_children()[0]  # First child is condition

        # Pattern: field == constant
        if self.is_equality_check(condition):
            field, value = self.parse_equality(condition)
            if field not in self.constraints['enums']:
                self.constraints['enums'][field] = []
            self.constraints['enums'][field].append(value)

        # Pattern: field < constant (range constraint)
        elif self.is_comparison(condition):
            field, op, value = self.parse_comparison(condition)
            if op == '<':
                self.constraints['ranges'][field] = (None, value - 1)
            elif op == '>':
                self.constraints['ranges'][field] = (value + 1, None)
            elif op == '<=':
                self.constraints['ranges'][field] = (None, value)
            elif op == '>=':
                self.constraints['ranges'][field] = (value, None)

    def extract_switch_constraint(self, switch_node):
        """
        Extract enum values from switch statements

        Example:
          switch (msg->cmd) {
            case MQTT_CONNECT: ...
            case MQTT_PUBLISH: ...
          }
          → constraint: msg.cmd ∈ {MQTT_CONNECT, MQTT_PUBLISH}
        """
        cases = []
        for child in switch_node.walk_preorder():
            if child.kind == CursorKind.CASE_STMT:
                # Extract case value
                case_value = self.get_case_value(child)
                cases.append(case_value)

        # Find switched-on field
        switched_field = self.get_switch_expr_field(switch_node)
        if switched_field:
            self.constraints['enums'][switched_field] = cases

    def extract_call_constraint(self, call_node):
        """
        Extract constraints from function calls

        Example:
          if (memcmp(msg->magic, "MQTT", 4) != 0) return;
          → constraint: msg.magic = "MQTT"

          if (strcmp(msg->version, "3.1.1") == 0) { ... }
          → constraint: msg.version ∈ {"3.1.1"}
        """
        func_name = call_node.spelling

        if func_name in ('memcmp', 'strcmp', 'strncmp'):
            args = list(call_node.get_arguments())
            if len(args) >= 2:
                field = self.try_extract_field(args[0])
                const_str = self.try_extract_string_literal(args[1])
                if field and const_str:
                    if field not in self.constraints['strings']:
                        self.constraints['strings'][field] = []
                    self.constraints['strings'][field].append(const_str)
```

#### Step 2: Generate Constraint-Satisfying Seeds

```python
def generate_constraint_satisfying_seeds(proto_file, constraints, output_dir):
    """
    Generate seeds that satisfy extracted constraints

    Strategy:
    1. Use type-driven defaults as base
    2. Override with constraint-satisfying values
    3. Generate both satisfying and violating seeds (for edge testing)
    """
    schema = load_proto_schema(proto_file)
    seeds = []

    # Baseline: all constraints satisfied
    base_msg = create_default_message(schema)
    for field, values in constraints['enums'].items():
        if field in schema.fields:
            # Try each valid enum value
            for value in values:
                msg = copy(base_msg)
                setattr(msg, field, value)
                seeds.append(msg)

    for field, (min_val, max_val) in constraints['ranges'].items():
        if field in schema.fields:
            # Boundary values: min-1, min, min+1, max-1, max, max+1
            if min_val is not None:
                for delta in [-1, 0, 1]:
                    msg = copy(base_msg)
                    setattr(msg, field, min_val + delta)
                    seeds.append(msg)
            if max_val is not None:
                for delta in [-1, 0, 1]:
                    msg = copy(base_msg)
                    setattr(msg, field, max_val + delta)
                    seeds.append(msg)

    for field, strings in constraints['strings'].items():
        if field in schema.fields:
            for s in strings:
                msg = copy(base_msg)
                setattr(msg, field, s)
                seeds.append(msg)

                # Also generate near-misses
                msg2 = copy(base_msg)
                setattr(msg2, field, s + "X")  # Wrong suffix
                seeds.append(msg2)

    # Serialize
    for i, seed in enumerate(seeds):
        serialize_proto(seed, f"{output_dir}/constraint_{i:04d}.bin")

    return len(seeds)
```

#### Step 3: Emit Constraint Hints File

```python
def emit_constraint_hints(constraints, proto_file):
    """
    Save extracted constraints as JSON for reuse

    Output: build/<target>_diff/constraint_hints.json
    """
    hints = {
        'proto_file': proto_file,
        'constraints': {
            'enums': {k: list(v) for k, v in constraints['enums'].items()},
            'ranges': {k: {'min': v[0], 'max': v[1]}
                      for k, v in constraints['ranges'].items()},
            'strings': {k: list(v) for k, v in constraints['strings'].items()},
        },
        'examples': {}  # Filled by other strategies
    }

    output_path = proto_file.replace('.proto', '_hints.json')
    with open(output_path, 'w') as f:
        json.dump(hints, f, indent=2)

    print(f"[+] Constraint hints saved to {output_path}")
    return output_path
```

### Expected Results

**Valid Input Rate**: 40-60% (constraints often cover major validation checks)

**Coverage**: High (seeds designed to pass early guards and reach deeper code)

**Pros**:
- ✅ 100% local
- ✅ Semantic understanding (knows what values matter)
- ✅ Reusable hints file
- ✅ Deterministic

**Cons**:
- ⚠️ Only finds simple constraints (misses complex predicates)
- ⚠️ May miss constraints in called functions
- ⚠️ Requires accurate AST traversal

**Timeline**: 3-5 days implementation

---

## Strategy 3: Field-Name Semantic Analysis ⚡ **PRIORITY 1**

### Overview

**Idea**: Extract semantic meaning from field names and generate appropriate values.

**Privacy**: ✅ **100% local** - uses only proto field names

**Example**:
- `cmd` → likely enum, try common command values (0, 1, 2, ...)
- `version` → try known versions (1, 2, 3, 4, "1.0", "2.0")
- `len` / `length` / `size` → boundary values (0, 1, 127, 128, 255, 256)
- `timeout` → small positive values (0, 1, 10, 100, 1000, -1)

### Implementation

```python
class SemanticFieldAnalyzer:
    """Infer field semantics from names"""

    # Field name patterns → value generators
    SEMANTIC_PATTERNS = {
        # Commands and types
        r'(cmd|command|type|kind|op|opcode)': lambda: [0, 1, 2, 3, 4, 0xFF, 0x100],

        # Versions
        r'(version|ver)': lambda: [0, 1, 2, 3, 4, 5, 255],

        # Lengths and sizes
        r'(len|length|size|count)': lambda: [
            0, 1, 2, 4, 8, 16, 32, 64, 127, 128, 255, 256, 512, 1024, 4096, 65535
        ],

        # Timeouts and delays
        r'(timeout|delay|wait)': lambda: [0, 1, 10, 100, 1000, -1],

        # Flags and booleans
        r'(flag|enable|disable|is_|has_|should_)': lambda: [0, 1, True, False],

        # IDs and indices
        r'(id|index|idx)': lambda: [0, 1, 2, 100, 0xFFFF, 0xFFFFFFFF],

        # Ports and addresses
        r'(port)': lambda: [0, 80, 443, 8080, 65535],

        # Quality of Service (MQTT-specific)
        r'(qos|quality)': lambda: [0, 1, 2],

        # Magic bytes
        r'(magic|signature|header)': lambda: [b"\x00\x00", b"\xFF\xFF", b"MQTT"],
    }

    def infer_values(self, field_name, field_type):
        """Infer good values based on field name"""
        for pattern, generator in self.SEMANTIC_PATTERNS.items():
            if re.search(pattern, field_name, re.IGNORECASE):
                values = generator()
                # Filter by type compatibility
                return self.filter_by_type(values, field_type)

        # Fallback: type-driven defaults
        return TypeDrivenGenerator().generate_values(field_type)

    def filter_by_type(self, values, field_type):
        """Ensure values match field type"""
        if field_type in ('int32', 'uint32', 'int64', 'uint64'):
            return [v for v in values if isinstance(v, int)]
        elif field_type == 'string':
            return [str(v) for v in values]
        elif field_type == 'bytes':
            return [v if isinstance(v, bytes) else str(v).encode()
                   for v in values]
        else:
            return values
```

### Expected Results

**Valid Input Rate**: 50-70% (semantic hints often match real constraints)

**Pros**:
- ✅ 100% local
- ✅ Fast
- ✅ Works well for well-named fields
- ✅ Complements type-driven generation

**Cons**:
- ⚠️ Depends on naming conventions
- ⚠️ May miss fields with non-standard names

**Timeline**: 1-2 days implementation

---

## Strategy 4: AST-Guided Coverage Seeding ⚡ **PRIORITY 1**

### Overview

**Idea**: Run instrumented binary with random proto inputs, save any that hit new coverage.

**Privacy**: ✅ **100% local** - binary runs locally with coverage feedback

**Universality**: ✅ **Works for any target** - coverage-guided, no analysis needed

### Implementation

#### Step 1: Instrumented Build

```bash
# Already available in PIN pipeline
# Fuzzer binary has libFuzzer coverage instrumentation built-in
FUZZ_BIN="build/<target>_diff/fuzz_bytes"
```

#### Step 2: Coverage-Guided Seed Mining

```python
import subprocess
import os

class CoverageSeedMiner:
    """Mine seeds by running target and collecting coverage-increasing inputs"""

    def __init__(self, fuzz_binary, proto_schema):
        self.fuzz_bin = fuzz_binary
        self.schema = proto_schema
        self.seen_coverage = set()

    def mine_seeds(self, output_dir, iterations=10000, timeout_per_run=0.1):
        """
        Run binary with structured inputs, save coverage-expanding ones

        Strategy:
        1. Generate random-ish proto messages (using type-driven + semantic)
        2. Serialize to binary
        3. Run through fuzz_bytes wrapper
        4. If new coverage, save to corpus
        """
        type_gen = TypeDrivenGenerator()
        semantic_gen = SemanticFieldAnalyzer()

        seeds_found = 0

        for i in range(iterations):
            # Generate candidate input
            msg = self.generate_structured_candidate(type_gen, semantic_gen)
            input_bytes = serialize_proto(msg)

            # Run target
            coverage_hash = self.run_and_get_coverage(input_bytes)

            if coverage_hash not in self.seen_coverage:
                # New coverage! Save seed
                self.seen_coverage.add(coverage_hash)
                seed_path = f"{output_dir}/cov_seed_{seeds_found:04d}.bin"
                with open(seed_path, 'wb') as f:
                    f.write(input_bytes)
                seeds_found += 1
                print(f"[+] New coverage seed {seeds_found}: {coverage_hash}")

        return seeds_found

    def generate_structured_candidate(self, type_gen, semantic_gen):
        """
        Generate semi-random but structured proto message
        Mix of:
        - Type-driven boundary values (30%)
        - Semantic-driven values (40%)
        - Random values (30%)
        """
        msg = create_empty_message(self.schema)

        for field_name, field_type in self.schema.fields.items():
            choice = random.random()

            if choice < 0.3:
                # Type-driven
                values = type_gen.generate_values(field_type)
                setattr(msg, field_name, random.choice(values))
            elif choice < 0.7:
                # Semantic
                values = semantic_gen.infer_values(field_name, field_type)
                setattr(msg, field_name, random.choice(values))
            else:
                # Random
                setattr(msg, field_name, generate_random_value(field_type))

        return msg

    def run_and_get_coverage(self, input_bytes):
        """
        Run fuzz target and extract coverage

        Use libFuzzer's built-in coverage tracking:
        - Write input to temp file
        - Run: fuzz_bytes temp_file
        - Parse coverage from output or use -print_coverage=1
        """
        with tempfile.NamedTemporaryFile(delete=False) as tmp:
            tmp.write(input_bytes)
            tmp_path = tmp.name

        try:
            # Run with coverage printing
            result = subprocess.run(
                [self.fuzz_bin, tmp_path],
                capture_output=True,
                timeout=0.1,
                text=True
            )

            # Extract coverage (simplified - use actual libFuzzer coverage format)
            # For now, hash stdout+stderr as coverage proxy
            coverage_hash = hashlib.sha256(
                (result.stdout + result.stderr).encode()
            ).hexdigest()[:16]

            return coverage_hash
        except subprocess.TimeoutExpired:
            return "timeout"
        finally:
            os.unlink(tmp_path)
```

#### Step 3: Integration with PIN Pipeline

```bash
# Add to pin_diff.sh before fuzzing
if [[ "$GENERATE_SEEDS" == "1" ]]; then
  echo "[*] Mining coverage-guided seeds..."
  python3 src/mine_coverage_seeds.py \
    --fuzz-bin="$BUILD_DIR/fuzz_bytes" \
    --proto="$BUILD_DIR/input.proto" \
    --output="$BUILD_DIR/seed_corpus" \
    --iterations=10000

  # Copy seeds to libFuzzer corpus
  cp $BUILD_DIR/seed_corpus/* $BUILD_DIR/corpus/
fi
```

### Expected Results

**Valid Input Rate**: 60-80% (coverage feedback naturally selects valid-ish inputs)

**Coverage**: Very High (by definition, seeds hit different code paths)

**Pros**:
- ✅ 100% local
- ✅ Automatic, no manual analysis
- ✅ Adapts to any target
- ✅ Coverage-optimal

**Cons**:
- ⚠️ Requires running target (may be slow)
- ⚠️ May timeout on complex inputs
- ⚠️ Depends on initial seed quality

**Timeline**: 3-4 days implementation

---

## Strategy 5: Minimal LLM (Proto-Only Context) 🔵 **PRIORITY 2**

### Overview

**Idea**: Send ONLY proto schema + function signature to LLM (not full source), ask for example inputs.

**Privacy**: 🟡 **Proto + signature only** - no implementation details leaked

**Universality**: ✅ **Works for any target** - LLM has broad knowledge

### Implementation

```python
class ProtoOnlyLLMGenerator:
    """Generate seeds using LLM with minimal context"""

    def __init__(self, api_key, model="gpt-4"):
        self.client = openai.OpenAI(api_key=api_key)
        self.model = model

    def generate_seeds(self, proto_file, func_signature, num_seeds=10):
        """
        Ask LLM to generate example proto messages

        Context shared with LLM:
        - Proto schema (structure, field names, types)
        - Function signature (name, parameters)
        - Nothing else!
        """
        proto_schema = read_file(proto_file)

        prompt = f"""You are a fuzzing expert. Generate {num_seeds} example protobuf messages for testing this function:

**Function Signature:**
```c
{func_signature}
```

**Protobuf Schema:**
```protobuf
{proto_schema}
```

**Task:** Generate {num_seeds} diverse, realistic test inputs as Python code that fills the protobuf message.

**Requirements:**
1. Cover common use cases (normal operation)
2. Cover edge cases (empty values, max values, boundary conditions)
3. Cover error cases (invalid values, missing required fields)
4. Use realistic values based on field names (e.g., if field is "version", use actual version numbers like 3, 4, 5)

**Output format:** Python code using protobuf API:
```python
# Seed 1: Normal case
msg = InputMessage()
msg.field1 = value1
msg.field2 = value2
serialize_to_file(msg, "seed_001.bin")

# Seed 2: Edge case
...
```

Generate the code:"""

        response = self.client.chat.completions.create(
            model=self.model,
            messages=[{"role": "user", "content": prompt}],
            temperature=0.7,  # Some creativity
        )

        code = response.choices[0].message.content

        # Execute LLM-generated code in sandbox
        seeds = self.execute_llm_code_safely(code, proto_file)

        return seeds

    def execute_llm_code_safely(self, code, proto_file):
        """Execute LLM-generated code in restricted environment"""
        # Import proto definitions
        exec(f"from {proto_module} import *")

        # Provide serialize helper
        def serialize_to_file(msg, filename):
            with open(filename, 'wb') as f:
                f.write(msg.SerializeToString())

        # Execute in restricted namespace
        namespace = {
            'InputMessage': InputMessage,  # From generated proto
            'serialize_to_file': serialize_to_file,
        }

        try:
            exec(code, namespace)
            # Collect generated .bin files
            seeds = glob.glob("seed_*.bin")
            return seeds
        except Exception as e:
            print(f"[-] LLM code execution failed: {e}")
            return []
```

### Example LLM Output

```python
# LLM would generate code like:

# Seed 1: Normal MQTT CONNECT
msg = InputMessage()
msg.cmd = 1  # MQTT_CONNECT
msg.version = 4  # MQTT v3.1.1
msg.buf.length = 10
msg.buf.data.extend([0x00, 0x04, 0x4d, 0x51, 0x54, 0x54, 0x04, 0x02, 0x00, 0x3c])
serialize_to_file(msg, "seed_001.bin")

# Seed 2: Empty message (edge case)
msg = InputMessage()
serialize_to_file(msg, "seed_002.bin")

# Seed 3: Maximum length
msg = InputMessage()
msg.buf.length = 65535
msg.buf.data.extend([0xff] * 1000)
serialize_to_file(msg, "seed_003.bin")

# ... 7 more seeds
```

### Expected Results

**Valid Input Rate**: 60-80% (LLM understands common patterns)

**Pros**:
- ✅ High-quality seeds
- ✅ Semantic understanding
- ✅ Works for any target
- 🟡 Minimal privacy leak (proto only)

**Cons**:
- ⚠️ Requires API key ($$$)
- ⚠️ Non-deterministic
- ⚠️ May hallucinate invalid protobuf code
- 🟡 Leaks proto schema (usually not sensitive)

**Timeline**: 2-3 days implementation

---

## Strategy 6: Test Mining (Utopia-Style) 🔵 **PRIORITY 2**

### Overview

**Idea**: If target has unit tests, trace them and convert test inputs to proto seeds.

**Privacy**: ✅ **100% local** - test tracing happens locally

**Universality**: 🔴 **Depends on tests** - only works if tests exist

### Implementation

```python
class TestMiner:
    """Extract seeds from existing unit tests"""

    def mine_from_tests(self, test_binary, proto_schema, output_dir):
        """
        Run unit tests under instrumentation, capture inputs

        Strategy:
        1. Instrument test binary with LD_PRELOAD hook
        2. Hook captures arguments to target function
        3. Convert captured args to proto messages
        4. Serialize as seed corpus
        """
        # Build LD_PRELOAD hook
        hook_lib = self.build_capture_hook(proto_schema)

        # Run tests with hook
        env = os.environ.copy()
        env['LD_PRELOAD'] = hook_lib
        env['CAPTURE_OUTPUT'] = output_dir

        subprocess.run([test_binary], env=env)

        # Hook writes proto files to output_dir
        seeds = glob.glob(f"{output_dir}/test_*.bin")
        return len(seeds)

    def build_capture_hook(self, proto_schema):
        """
        Build shared library that intercepts function calls

        Example for mg_mqtt_parse:
        ```c
        int mg_mqtt_parse(uint8_t *buf, size_t len, uint8_t version,
                         struct mg_mqtt_message *m) {
          // Capture args to proto
          InputMessage msg;
          msg.set_buf(buf, len);
          msg.set_version(version);
          // ... fill other fields from *m

          // Serialize
          write_proto(msg, "test_001.bin");

          // Call real function
          return real_mg_mqtt_parse(buf, len, version, m);
        }
        ```
        """
        # Generate wrapper code
        # Compile to shared library
        # Return path
        pass
```

### Expected Results

**Valid Input Rate**: 70-90% (tests use valid inputs by definition)

**Pros**:
- ✅ 100% local
- ✅ Very high quality seeds
- ✅ Covers real-world usage

**Cons**:
- 🔴 Only works if tests exist
- ⚠️ Requires building hook library
- ⚠️ May miss edge cases tests don't cover

**Timeline**: 1 week implementation

---

## Strategy 7: Example Mining (WildSync-Style) 🔵 **PRIORITY 3**

### Overview

**Idea**: Search GitHub/docs for usage examples, extract input patterns.

**Privacy**: 🟡 **Queries only** - search terms reveal target name

**Universality**: 🟡 **Depends on popularity** - obscure APIs may have no examples

### Implementation

```python
class ExampleMiner:
    """Mine GitHub for API usage examples"""

    def __init__(self, github_token):
        self.token = github_token

    def mine_examples(self, function_name, proto_schema, output_dir):
        """
        Search GitHub for function usage, extract patterns

        Example query: "mg_mqtt_parse language:C"
        """
        examples = self.search_github_code(
            query=f"{function_name} language:C",
            max_results=100
        )

        seeds = []
        for example in examples:
            # Download code snippet
            code = self.fetch_file_content(example['url'])

            # Parse to find function calls
            calls = self.extract_function_calls(code, function_name)

            # Convert call arguments to proto
            for call_args in calls:
                proto_msg = self.args_to_proto(call_args, proto_schema)
                seeds.append(proto_msg)

        # Serialize
        for i, seed in enumerate(seeds):
            serialize_proto(seed, f"{output_dir}/example_{i:04d}.bin")

        return len(seeds)

    def search_github_code(self, query, max_results):
        """Search GitHub Code Search API"""
        headers = {"Authorization": f"token {self.token}"}
        results = []

        for page in range(1, max_results // 30 + 1):
            response = requests.get(
                "https://api.github.com/search/code",
                params={"q": query, "page": page, "per_page": 30},
                headers=headers
            )
            results.extend(response.json().get("items", []))

        return results
```

### Expected Results

**Valid Input Rate**: 70-90% (real-world examples use valid inputs)

**Pros**:
- ✅ Real-world patterns
- ✅ Covers common use cases
- ✅ May find interesting edge cases

**Cons**:
- 🟡 Leaks function name to GitHub
- ⚠️ Rate limits (5000 requests/hour)
- 🔴 Doesn't work for unpopular/proprietary APIs
- 🔴 Requires parsing C code correctly

**Timeline**: 1-2 weeks implementation

---

## Recommended Implementation Plan

### Phase 1: Local + Universal (Week 1-2) ⚡

Implement **Strategy 1 + 2 + 3**:

```python
# Seed generation pipeline
def generate_seed_corpus(proto_file, func_cursor, output_dir):
    """Combined approach for maximum coverage"""

    # Step 1: Type-driven baseline (1000 seeds)
    type_gen = TypeDrivenGenerator()
    type_seeds = type_gen.generate_from_proto(proto_file)
    print(f"[+] Generated {len(type_seeds)} type-driven seeds")

    # Step 2: Extract constraints from AST
    constraint_extractor = ConstraintExtractor(func_cursor)
    constraints = constraint_extractor.extract_constraints()
    emit_constraint_hints(constraints, proto_file)

    # Step 3: Constraint-satisfying seeds (500 seeds)
    constraint_seeds = generate_constraint_satisfying_seeds(
        proto_file, constraints, output_dir
    )
    print(f"[+] Generated {constraint_seeds} constraint-aware seeds")

    # Step 4: Semantic field analysis (300 seeds)
    semantic_gen = SemanticFieldAnalyzer()
    semantic_seeds = semantic_gen.generate_from_proto(proto_file)
    print(f"[+] Generated {len(semantic_seeds)} semantic seeds")

    total = len(type_seeds) + constraint_seeds + len(semantic_seeds)
    print(f"[+] Total seed corpus: {total} inputs")

    return total

# Integration with PIN
# In pin_diff.sh:
python3 src/generate_seed_corpus.py \
  --proto="$BUILD_DIR/input.proto" \
  --func-ast="$BUILD_DIR/func_ast.json" \
  --output="$BUILD_DIR/seed_corpus"

# Copy to libFuzzer corpus
cp -r $BUILD_DIR/seed_corpus/* $BUILD_DIR/corpus/
```

**Expected Results**: 60-70% valid input rate, fully local

### Phase 2: Coverage-Guided Refinement (Week 3) ⚡

Add **Strategy 4**:

```bash
# After initial corpus, run coverage mining
python3 src/mine_coverage_seeds.py \
  --fuzz-bin="$BUILD_DIR/fuzz_bytes" \
  --proto="$BUILD_DIR/input.proto" \
  --initial-corpus="$BUILD_DIR/seed_corpus" \
  --output="$BUILD_DIR/refined_corpus" \
  --iterations=10000
```

**Expected Results**: 70-80% valid input rate

### Phase 3: Optional Enhancements (Week 4+) 🔵

Add **Strategy 5** (LLM) if needed:

```bash
# Only if local strategies insufficient
python3 src/llm_seed_gen.py \
  --proto="$BUILD_DIR/input.proto" \
  --func-sig="$(extract_signature)" \
  --output="$BUILD_DIR/llm_seeds" \
  --api-key="$OPENAI_API_KEY" \
  --num-seeds=50
```

Add **Strategy 6** (Test Mining) if tests available:

```bash
# If unit tests exist
python3 src/mine_test_seeds.py \
  --test-binary="$TARGET/tests/test_parser" \
  --proto="$BUILD_DIR/input.proto" \
  --output="$BUILD_DIR/test_seeds"
```

---

## Viability Assessment of Existing Ideas

### From `/home/priyatam/pin/sok_fuzzgen/llm_for_seed_generation.md`:

**Idea 1: Proto-Aware Constraint Mining**
- ✅ **VIABLE** - Matches Strategy 2 above
- ✅ **ROBUST** - Works for any C function
- ✅ **UNIVERSAL** - No external dependencies
- **Recommendation**: ✅ **IMPLEMENT** (Priority 0)

**Idea 2: AST-Guided Trace Replayer**
- ✅ **VIABLE** - Matches Strategy 4 above
- ✅ **ROBUST** - Coverage feedback is reliable
- ✅ **UNIVERSAL** - Works for any instrumented binary
- **Recommendation**: ✅ **IMPLEMENT** (Priority 1)

**Missing from existing doc**:
- Type-driven value enumeration (easiest, should do first)
- Field-name semantic analysis (quick win)
- Minimal LLM approach (better privacy than full source)

---

## Success Metrics

| Metric | Current (Random) | Target (After Seed Gen) |
|--------|------------------|-------------------------|
| **Valid Input Rate** | <1% | >50% (P0), >70% (P1) |
| **Coverage (1 min)** | 8 blocks | >40 blocks |
| **EMI Rejection Rate** | 99%+ | <50% |
| **Time to First Crash** | Never | <5 minutes |
| **Corpus Growth Rate** | 2-3 inputs/min | 20+ inputs/min |

---

## Conclusion

**Recommended Approach**: **Local-First Hybrid (Strategies 1+2+3+4)**

1. ✅ **100% privacy** - no source code leaves your machine
2. ✅ **Universal** - works for any C function with proto schema
3. ✅ **Robust** - combines static analysis + dynamic feedback
4. ✅ **Fast** - generates 1000+ seeds in <10 seconds

**Timeline**: 2-3 weeks for full implementation

**Expected Improvement**: <1% → 70%+ valid input rate

**Next Steps**:
1. Implement Strategy 1 (type-driven) - 2 days
2. Implement Strategy 2 (constraint mining) - 3 days
3. Implement Strategy 3 (semantic analysis) - 1 day
4. Implement Strategy 4 (coverage mining) - 3 days
5. Integrate into `pin_diff.sh` - 1 day
6. Validate on benchmarks - 2 days

---

**Document Status**: ✅ READY FOR IMPLEMENTATION
**Owner**: Research team
**Priority**: High (Checkpoint C dependency)
