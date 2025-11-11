# Practical Setup Guide: 9 Competing Fuzz Driver Generation Tools

**Document Purpose:** Hands-on instructions for installing and running 9 state-of-the-art fuzz driver generation tools for comparative analysis.

**Created:** November 2025
**Status:** Ready for Execution

---

## OVERVIEW

This guide provides step-by-step instructions for setting up:

1. **OSS-Fuzz-Gen** (LLM-based, Google)
2. **AFGen** (Whole-function fuzzing, IEEE S&P 2024)
3. **FUDGE** (Industrial-scale, Facebook) - Paper study only
4. **Oracle-Guided Harnessing** (ICSE 2025)
5. **Hopper** (Interpretative fuzzing, CCS 2023)
6. **GraphFuzz** (Lifetime-aware, ICSE 2022)
7. **CKGFuzzer** (CKG + LLM, ICSE 2025)
8. **WildSync** (Ecosystem mining, ISSTA 2025)
9. **Utopia** (Test mining, IEEE S&P 2023)

**Expected Time:** 2-4 weeks for complete setup

---

## PHASE 0: SHARED INFRASTRUCTURE SETUP

### Step 0.1: Create Evaluation Environment

```bash
# Create dedicated workspace
mkdir -p ~/pin-evaluation
cd ~/pin-evaluation

# Directory structure
mkdir -p {tools,benchmarks,results,scripts}

# Install system dependencies
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    python3 python3-pip python3-venv \
    clang-14 llvm-14 llvm-14-dev libclang-14-dev \
    libc++-dev libc++abi-dev \
    libfuzzer-14-dev \
    protobuf-compiler libprotobuf-dev \
    wget curl jq

# Install AFL++
cd ~/pin-evaluation/tools
git clone https://github.com/AFLplusplus/AFLplusplus
cd AFLplusplus
make
sudo make install
cd ..

# Install libFuzzer (already included with clang-14)
# Verify
clang-14 --version | grep libFuzzer
```

### Step 0.2: Python Environment

```bash
cd ~/pin-evaluation
python3 -m venv venv
source venv/bin/activate

# Install common dependencies
pip install --upgrade pip
pip install \
    libclang==14.0.6 \
    pycparser==2.21 \
    protobuf==3.20.3 \
    openai \
    anthropic \
    requests \
    beautifulsoup4 \
    networkx \
    tree-sitter \
    tree-sitter-c \
    tree-sitter-cpp

# Save for later
pip freeze > requirements.txt
```

### Step 0.3: Common Benchmarks

```bash
cd ~/pin-evaluation/benchmarks

# 1. libtiff (PIN's current focus)
git clone https://gitlab.com/libtiff/libtiff
cd libtiff
./autogen.sh
./configure --disable-shared --enable-static
make
cd ..

# 2. mongoose (PIN's failed case)
git clone https://github.com/cesanta/mongoose
cd mongoose
# Mongoose is header-only, no build needed
cd ..

# 3. libpng
git clone https://github.com/glennrp/libpng
cd libpng
./autogen.sh
./configure --disable-shared --enable-static
make
cd ..

# 4. cJSON
git clone https://github.com/DaveGamble/cJSON
cd cJSON
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
cd ../..

# 5. Small test targets
mkdir -p simple-targets
cd simple-targets

# Create test_parser.c
cat > test_parser.c << 'EOF'
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Simple parser function for testing
int parse_packet(const uint8_t *data, size_t len) {
    if (!data || len < 4) return -1;

    // Header check
    if (data[0] != 0x42) return -1;

    // Length field
    uint16_t payload_len = (data[1] << 8) | data[2];

    // Vulnerable: no bounds check
    if (payload_len > 0) {
        // This can overflow if payload_len > len - 4
        memcpy(&data[4], data, payload_len);  // VULN: buffer overflow
    }

    return 0;
}
EOF

# Compile
clang-14 -c test_parser.c -o test_parser.o

cd ..  # Back to benchmarks/
```

---

## PHASE 1: HIGH-PRIORITY TOOLS (Start Here)

### Tool #1: OSS-Fuzz-Gen (HIGHEST PRIORITY)

**Status:** ✅ Publicly available, production-ready

**Why First:** LLM-based, 160+ projects, 30 bugs, immediate insights

**Setup:**

```bash
cd ~/pin-evaluation/tools

# Clone repository
git clone https://github.com/google/oss-fuzz-gen
cd oss-fuzz-gen

# Install dependencies
pip install -r requirements.txt

# Set up LLM API key (choose one)
# Option 1: OpenAI
export OPENAI_API_KEY="your-key-here"

# Option 2: Vertex AI (Google)
export GOOGLE_APPLICATION_CREDENTIALS="/path/to/service-account.json"

# Test installation
python -m fuzz_generator.generate --help
```

**Example Run: Generate Harness for libtiff**

```bash
# Generate harness for TIFFReadDirectory
python -m fuzz_generator.generate \
    --target-library=libtiff \
    --target-function=TIFFReadDirectory \
    --output-dir=~/pin-evaluation/results/oss-fuzz-gen/libtiff \
    --model=gpt-4

# Review generated harness
cat ~/pin-evaluation/results/oss-fuzz-gen/libtiff/fuzz_target.cc

# Compile and test
cd ~/pin-evaluation/results/oss-fuzz-gen/libtiff
clang++-14 -g -O1 -fsanitize=fuzzer,address \
    fuzz_target.cc \
    -I/path/to/libtiff/libtiff \
    -L/path/to/libtiff/libtiff/.libs \
    -ltiff \
    -o fuzz_target

# Run fuzzer for 60 seconds
./fuzz_target -max_total_time=60
```

**What to Observe:**

1. **LLM Prompt:** Check `fuzz_generator/prompts/` for prompt templates
   - How does it describe the function?
   - What context does it provide?
   - How does it request initialization code?

2. **Generated Code:** Examine `fuzz_target.cc`
   - Does it include initialization (TIFFOpen)?
   - How does it handle external types (TIFF*)?
   - What's the structure of LLMFuzzerTestOneInput?

3. **Feedback Loop:** Check logs for repair iterations
   - Does it retry on compilation errors?
   - How does it fix errors?

**Expected Insights:**
- LLM generates `TIFFOpen("/tmp/fuzz.tif", "r")` automatically
- Handles external types via temporary files
- May include error handling and cleanup code
- **Key takeaway:** LLM knows API usage patterns without static analysis

**Time:** 1-2 days

---

### Tool #2: WildSync (IF AVAILABLE)

**Status:** ⚠️ May not be publicly released yet (ISSTA 2025)

**Why Second:** 469 harnesses deployed, 7 bugs, ecosystem mining approach

**Setup (Hypothetical):**

```bash
cd ~/pin-evaluation/tools

# Check if available
git clone https://github.com/wildsync/wildsync  # May not exist yet

# Alternative: Contact authors
# Email: Check ISSTA 2025 paper for author contacts
# Request: Artifact evaluation access

# If unavailable: Implement key technique
mkdir wildsync-reimpl
cd wildsync-reimpl

# Create GitHub miner script (see below)
```

**DIY Implementation: GitHub API Mining**

```python
# ~/pin-evaluation/tools/wildsync-reimpl/mine_github.py
import requests
import json
import time

GITHUB_TOKEN = "your-github-token"  # Get from https://github.com/settings/tokens

def search_github_code(query, language="C", max_results=100):
    """Search GitHub code for API usage patterns."""
    headers = {
        "Authorization": f"token {GITHUB_TOKEN}",
        "Accept": "application/vnd.github.v3+json"
    }

    results = []
    page = 1

    while len(results) < max_results:
        url = f"https://api.github.com/search/code"
        params = {
            "q": f"{query} language:{language}",
            "per_page": 100,
            "page": page
        }

        response = requests.get(url, headers=headers, params=params)

        if response.status_code == 403:  # Rate limited
            print("Rate limited, waiting...")
            time.sleep(60)
            continue

        if response.status_code != 200:
            print(f"Error: {response.status_code}")
            break

        data = response.json()
        results.extend(data.get("items", []))

        if len(data.get("items", [])) < 100:
            break

        page += 1
        time.sleep(2)  # Rate limiting

    return results

def extract_call_context(code, target_function):
    """Extract lines before and after target function call."""
    lines = code.split('\n')
    contexts = []

    for i, line in enumerate(lines):
        if target_function in line and '(' in line:
            # Extract context: 5 lines before, current, 5 lines after
            start = max(0, i - 5)
            end = min(len(lines), i + 6)
            context = {
                'before': lines[start:i],
                'call': line,
                'after': lines[i+1:end]
            }
            contexts.append(context)

    return contexts

def mine_api_usage(api_function, max_examples=100):
    """Mine GitHub for API usage patterns."""
    print(f"Mining usage patterns for {api_function}...")

    # Search for function calls
    query = f'"{api_function}("'
    results = search_github_code(query, max_results=max_examples)

    print(f"Found {len(results)} code files")

    patterns = []
    for result in results:
        # Fetch raw code
        raw_url = result['html_url'].replace('github.com', 'raw.githubusercontent.com')
        raw_url = raw_url.replace('/blob/', '/')

        try:
            code_response = requests.get(raw_url)
            if code_response.status_code == 200:
                code = code_response.text
                contexts = extract_call_context(code, api_function)
                patterns.extend(contexts)
        except Exception as e:
            print(f"Error fetching {raw_url}: {e}")

        time.sleep(1)  # Rate limiting

    # Save patterns
    with open(f'{api_function}_patterns.json', 'w') as f:
        json.dump(patterns, f, indent=2)

    print(f"Extracted {len(patterns)} usage patterns")
    return patterns

def analyze_patterns(patterns):
    """Analyze patterns to find common API sequences."""
    # Count function calls in 'before' context
    init_funcs = {}
    cleanup_funcs = {}

    for pattern in patterns:
        # Before context
        for line in pattern['before']:
            for func in ['Open', 'Create', 'Init', 'New', 'Alloc']:
                if func in line and '(' in line:
                    init_funcs[func] = init_funcs.get(func, 0) + 1

        # After context
        for line in pattern['after']:
            for func in ['Close', 'Free', 'Destroy', 'Delete', 'Cleanup']:
                if func in line and '(' in line:
                    cleanup_funcs[func] = cleanup_funcs.get(func, 0) + 1

    print("\nCommon initialization functions:")
    for func, count in sorted(init_funcs.items(), key=lambda x: x[1], reverse=True)[:5]:
        percentage = (count / len(patterns)) * 100
        print(f"  {func}: {count} ({percentage:.1f}%)")

    print("\nCommon cleanup functions:")
    for func, count in sorted(cleanup_funcs.items(), key=lambda x: x[1], reverse=True)[:5]:
        percentage = (count / len(patterns)) * 100
        print(f"  {func}: {count} ({percentage:.1f}%)")

    return init_funcs, cleanup_funcs

# Example usage
if __name__ == "__main__":
    # Mine TIFFReadDirectory usage
    patterns = mine_api_usage("TIFFReadDirectory", max_examples=100)

    # Analyze patterns
    init_funcs, cleanup_funcs = analyze_patterns(patterns)

    # Expected output:
    # Common initialization functions:
    #   TIFFOpen: 85 (85.0%)
    #   TIFFClientOpen: 10 (10.0%)
    # Common cleanup functions:
    #   TIFFClose: 90 (90.0%)
```

**Run the script:**

```bash
cd ~/pin-evaluation/tools/wildsync-reimpl

# Get GitHub token from https://github.com/settings/tokens
export GITHUB_TOKEN="your-token-here"

# Mine patterns
python mine_github.py

# Review results
cat TIFFReadDirectory_patterns.json | jq '.[0]'
```

**Expected Output:**

```json
{
  "before": [
    "TIFF *tif;",
    "const char *filename = argv[1];",
    "tif = TIFFOpen(filename, \"r\");",
    "if (!tif) {",
    "  return 1;"
  ],
  "call": "TIFFReadDirectory(tif);",
  "after": [
    "uint32 width, height;",
    "TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);",
    "TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);",
    "TIFFClose(tif);"
  ]
}
```

**Integration with PIN:**

From these patterns, generate multi-step .proto:

```protobuf
message TIFFReadDirSequence {
  // Step 1: TIFFOpen
  string filename = 1;  // Usually "/tmp/fuzz.tif"
  string mode = 2;      // Usually "r"

  // Step 2: TIFFReadDirectory
  // (no parameters, uses handle from TIFFOpen)

  // Step 3: TIFFGetField (optional, from 'after' patterns)
  repeated uint32 tags_to_read = 3;

  // Step 4: TIFFClose (from 90% of patterns)
  bool should_close = 4;  // Default: true
}
```

**Time:** 2-3 days (including mining and analysis)

---

### Tool #3: Utopia (Test Mining)

**Status:** ⚠️ May require author contact (IEEE S&P 2023)

**Why Third:** 77.8% success, 80% valid inputs, test-based approach

**Setup (Hypothetical):**

```bash
cd ~/pin-evaluation/tools

# Check if available
git clone https://github.com/utopia-fuzzer/utopia  # May not exist

# Alternative: Contact authors
# Paper: "Automatic Generation of Fuzz Driver Using Unit Tests" (S&P 2023)
```

**DIY Implementation: Test Tracing**

```python
# ~/pin-evaluation/tools/utopia-reimpl/trace_tests.py
import subprocess
import re
import json

def find_test_files(project_dir):
    """Find test files in project."""
    import os
    test_files = []

    for root, dirs, files in os.walk(project_dir):
        for file in files:
            if 'test' in file.lower() and file.endswith(('.c', '.cc', '.cpp')):
                test_files.append(os.path.join(root, file))

    return test_files

def extract_test_cases(test_file):
    """Extract test case functions from file."""
    with open(test_file, 'r') as f:
        content = f.read()

    # Find test functions (common patterns)
    patterns = [
        r'void\s+test_(\w+)\s*\(',
        r'TEST\s*\(\s*\w+\s*,\s*(\w+)\s*\)',  # Google Test
        r'void\s+(\w+_test)\s*\(',
    ]

    test_cases = []
    for pattern in patterns:
        matches = re.finditer(pattern, content)
        for match in matches:
            test_cases.append(match.group(1))

    return test_cases

def trace_test_execution(test_binary, test_name, target_function):
    """Trace test execution to capture API sequence."""
    # Compile with tracing (GDB or ltrace)
    try:
        # Use ltrace to trace library calls
        result = subprocess.run(
            ['ltrace', '-f', '-o', f'trace_{test_name}.txt', test_binary, test_name],
            capture_output=True,
            timeout=10
        )

        # Parse trace
        with open(f'trace_{test_name}.txt', 'r') as f:
            trace_lines = f.readlines()

        # Extract API sequence
        api_sequence = []
        for line in trace_lines:
            # Look for target function and related APIs
            if target_function in line or any(x in line for x in ['Open', 'Close', 'Init', 'Free']):
                api_sequence.append(line.strip())

        return api_sequence

    except Exception as e:
        print(f"Error tracing {test_name}: {e}")
        return []

def extract_api_patterns(project_dir, target_function):
    """Extract API usage patterns from unit tests."""
    print(f"Extracting patterns for {target_function} from {project_dir}")

    # Find test files
    test_files = find_test_files(project_dir)
    print(f"Found {len(test_files)} test files")

    patterns = []

    for test_file in test_files:
        # Check if file references target function
        with open(test_file, 'r') as f:
            content = f.read()

        if target_function not in content:
            continue

        print(f"Found {target_function} in {test_file}")

        # Extract test cases
        test_cases = extract_test_cases(test_file)

        # For each test case, extract calling pattern
        for test_case in test_cases:
            # Simple pattern extraction (without execution tracing)
            # Find the function and surrounding context
            pattern = extract_call_pattern(content, target_function)
            if pattern:
                patterns.append({
                    'test_file': test_file,
                    'test_case': test_case,
                    'pattern': pattern
                })

    # Save patterns
    with open(f'{target_function}_test_patterns.json', 'w') as f:
        json.dump(patterns, f, indent=2)

    print(f"Extracted {len(patterns)} patterns")
    return patterns

def extract_call_pattern(code, target_function):
    """Extract calling pattern from code."""
    lines = code.split('\n')

    for i, line in enumerate(lines):
        if target_function in line and '(' in line:
            # Extract context
            start = max(0, i - 10)
            end = min(len(lines), i + 10)

            context = '\n'.join(lines[start:end])
            return context

    return None

# Example usage
if __name__ == "__main__":
    # Extract patterns from libtiff tests
    patterns = extract_api_patterns(
        "/home/user/pin-evaluation/benchmarks/libtiff",
        "TIFFReadDirectory"
    )

    print("\nExample pattern:")
    if patterns:
        print(patterns[0]['pattern'])
```

**Run the script:**

```bash
cd ~/pin-evaluation/tools/utopia-reimpl

# Extract patterns from libtiff tests
python trace_tests.py

# Review patterns
cat TIFFReadDirectory_test_patterns.json | jq '.[0]'
```

**Expected Pattern:**

```c
// From libtiff/test/test_directory.c
void test_tiff_read_directory() {
    TIFF *tif;
    const char *filename = "test.tif";

    // Pattern: Open before use
    tif = TIFFOpen(filename, "r");
    assert(tif != NULL);

    // Target function
    int result = TIFFReadDirectory(tif);
    assert(result == 1);

    // Cleanup
    TIFFClose(tif);
}
```

**Integration with PIN:**

Generate seed corpus from test inputs:

```python
# Generate PIN seed from test pattern
def generate_pin_seed(pattern):
    # Parse test pattern
    # Extract parameter values
    filename = extract_param(pattern, "filename")  # "test.tif"

    # Create protobuf seed
    seed = {
        'filename': filename,
        'mode': 'r',
        # ... other parameters from test
    }

    # Serialize to protobuf
    # ...

    return seed
```

**Time:** 2-3 days

---

## PHASE 2: MEDIUM-PRIORITY TOOLS

### Tool #4: Hopper (Interpretative Fuzzing)

**Status:** ⚠️ Unknown availability (CCS 2023)

**Paper:** "Interpretative Fuzzing for Libraries" (CCS 2023)

```bash
cd ~/pin-evaluation/tools

# Check repository
# GitHub: Search for "hopper fuzzer" or contact authors

# If unavailable: Study paper for key techniques
wget https://dl.acm.org/doi/pdf/10.1145/3576915.3616610  # Get paper PDF
```

**Key Technique to Study:** Mock object creation

**DIY Implementation: Mock Generator**

```python
# ~/pin-evaluation/tools/hopper-reimpl/mock_generator.py
import clang.cindex

def analyze_external_type(typedef_name, header_path):
    """Analyze external type structure from headers."""
    index = clang.cindex.Index.create()
    tu = index.parse(header_path)

    # Find struct definition
    struct_fields = []

    def visit_node(node):
        if node.kind == clang.cindex.CursorKind.STRUCT_DECL:
            if node.spelling == typedef_name or typedef_name in node.displayname:
                # Extract fields
                for child in node.get_children():
                    if child.kind == clang.cindex.CursorKind.FIELD_DECL:
                        struct_fields.append({
                            'name': child.spelling,
                            'type': child.type.spelling
                        })

        for child in node.get_children():
            visit_node(child)

    visit_node(tu.cursor)
    return struct_fields

def generate_mock_struct(typedef_name, fields):
    """Generate mock struct definition."""
    code = f"// Mock for {typedef_name}\n"
    code += f"struct Mock{typedef_name} {{\n"

    for field in fields:
        code += f"  {field['type']} {field['name']};\n"

    code += "};\n\n"
    return code

def generate_mock_acquire(typedef_name, proto_message):
    """Generate mock acquisition function."""
    code = f"{typedef_name}* pin_acquire_handle_{typedef_name.lower()}(const Input *msg) {{\n"
    code += f"  Mock{typedef_name} *mock = calloc(1, sizeof(Mock{typedef_name}));\n"
    code += f"  if (!mock) return NULL;\n\n"

    code += f"  // Populate from protobuf\n"
    code += f"  mock->width = msg->mock_{typedef_name.lower()}_width;\n"
    code += f"  mock->height = msg->mock_{typedef_name.lower()}_height;\n"
    code += f"  // ... other fields ...\n\n"

    code += f"  return ({typedef_name}*)mock;\n"
    code += "}\n"

    return code

# Example: Generate mock for TIFF
if __name__ == "__main__":
    # Analyze TIFF structure (if header available)
    fields = [
        {'name': 'tif_fd', 'type': 'int'},
        {'name': 'tif_mode', 'type': 'int'},
        {'name': 'tif_flags', 'type': 'uint32'},
        {'name': 'tif_diroff', 'type': 'uint64'},
        {'name': 'tif_data', 'type': 'void*'},
        # ... more fields ...
    ]

    mock_struct = generate_mock_struct("TIFF", fields)
    mock_acquire = generate_mock_acquire("TIFF", "Input")

    print(mock_struct)
    print(mock_acquire)
```

**Generated Mock Example:**

```c
// Mock for TIFF
struct MockTIFF {
  int tif_fd;
  int tif_mode;
  uint32 tif_flags;
  uint64 tif_diroff;
  void* tif_data;
};

TIFF* pin_acquire_handle_tiff(const Input *msg) {
  MockTIFF *mock = calloc(1, sizeof(MockTIFF));
  if (!mock) return NULL;

  // Populate from protobuf
  mock->tif_fd = msg->mock_tiff_fd;
  mock->tif_mode = msg->mock_tiff_mode;
  mock->tif_flags = msg->mock_tiff_flags;
  mock->tif_diroff = msg->mock_tiff_diroff;

  // Allocate data buffer
  if (msg->mock_tiff_data.size > 0) {
    mock->tif_data = malloc(msg->mock_tiff_data.size);
    memcpy(mock->tif_data, msg->mock_tiff_data.bytes, msg->mock_tiff_data.size);
  }

  return (TIFF*)mock;
}
```

**Time:** 2-3 days

---

### Tool #5: CKGFuzzer (Code Knowledge Graph + LLM)

**Status:** ⚠️ Likely available (ICSE 2025, arXiv paper exists)

**Paper:** https://arxiv.org/abs/2411.11532

```bash
cd ~/pin-evaluation/tools

# Check repository
git clone https://github.com/ckgfuzzer/ckgfuzzer  # Hypothetical

# Or: Implement CKG construction
mkdir ckgfuzzer-reimpl
cd ckgfuzzer-reimpl
```

**DIY Implementation: Basic CKG Construction**

```python
# ~/pin-evaluation/tools/ckgfuzzer-reimpl/build_ckg.py
import clang.cindex
import networkx as nx
import json

class CodeKnowledgeGraph:
    def __init__(self):
        self.graph = nx.DiGraph()

    def add_node(self, node_id, node_type, **attributes):
        self.graph.add_node(node_id, type=node_type, **attributes)

    def add_edge(self, from_node, to_node, edge_type):
        self.graph.add_edge(from_node, to_node, type=edge_type)

    def query_predecessors(self, node_id):
        """Get nodes that must be called before target."""
        return list(self.graph.predecessors(node_id))

    def query_successors(self, node_id):
        """Get nodes typically called after target."""
        return list(self.graph.successors(node_id))

    def export_json(self, filename):
        """Export CKG to JSON."""
        data = nx.node_link_data(self.graph)
        with open(filename, 'w') as f:
            json.dump(data, f, indent=2)

def build_ckg(source_files):
    """Build Code Knowledge Graph from source files."""
    ckg = CodeKnowledgeGraph()
    index = clang.cindex.Index.create()

    for source_file in source_files:
        tu = index.parse(source_file)

        # Extract functions
        functions = {}

        def extract_functions(node):
            if node.kind == clang.cindex.CursorKind.FUNCTION_DECL:
                func_name = node.spelling
                functions[func_name] = {
                    'params': [p.spelling for p in node.get_arguments()],
                    'return_type': node.result_type.spelling,
                    'location': str(node.location)
                }

                # Add to CKG
                ckg.add_node(
                    func_name,
                    'function',
                    params=functions[func_name]['params'],
                    return_type=functions[func_name]['return_type']
                )

            for child in node.get_children():
                extract_functions(child)

        extract_functions(tu.cursor)

        # Extract call graph
        def extract_calls(node, current_function=None):
            if node.kind == clang.cindex.CursorKind.FUNCTION_DECL:
                current_function = node.spelling

            if node.kind == clang.cindex.CursorKind.CALL_EXPR:
                if current_function:
                    callee = node.spelling
                    if callee:
                        # Add call edge
                        ckg.add_edge(current_function, callee, 'calls')

            for child in node.get_children():
                extract_calls(child, current_function)

        extract_calls(tu.cursor)

    return ckg

def query_ckg_for_function(ckg, target_function):
    """Query CKG to find API usage patterns."""
    print(f"\nQuerying CKG for: {target_function}")

    # Find predecessors (what's called before)
    predecessors = ckg.query_predecessors(target_function)
    print(f"Typically called after: {predecessors}")

    # Find successors (what's called after)
    successors = ckg.query_successors(target_function)
    print(f"Typically followed by: {successors}")

    return {
        'before': predecessors,
        'after': successors
    }

# Example usage
if __name__ == "__main__":
    # Build CKG from libtiff source
    import os

    libtiff_src = "/home/user/pin-evaluation/benchmarks/libtiff/libtiff"
    source_files = []

    for file in os.listdir(libtiff_src):
        if file.endswith('.c'):
            source_files.append(os.path.join(libtiff_src, file))

    print(f"Building CKG from {len(source_files)} files...")
    ckg = build_ckg(source_files[:10])  # Limit for speed

    # Export
    ckg.export_json("libtiff_ckg.json")

    # Query for TIFFReadDirectory
    pattern = query_ckg_for_function(ckg, "TIFFReadDirectory")

    # Use with LLM
    prompt = f"""
    Generate a fuzz harness for TIFFReadDirectory.

    Code knowledge graph analysis shows:
    - Typically called after: {pattern['before']}
    - Typically followed by: {pattern['after']}

    Generate initialization code that includes these functions.
    """

    print("\n\nLLM Prompt:")
    print(prompt)
```

**Run the script:**

```bash
cd ~/pin-evaluation/tools/ckgfuzzer-reimpl
python build_ckg.py

# Review CKG
cat libtiff_ckg.json | jq '.nodes | .[0]'
```

**Integration with PIN + LLM:**

```python
# Use CKG to ground LLM generation
def generate_harness_with_ckg(target_function, ckg):
    # Query CKG
    pattern = query_ckg_for_function(ckg, target_function)

    # Create grounded LLM prompt
    prompt = f"""
    Generate C code to fuzz {target_function}.

    Based on codebase analysis:
    - Functions called before {target_function}: {pattern['before']}
    - Functions called after {target_function}: {pattern['after']}

    Include proper initialization and cleanup.
    """

    # Call LLM
    llm_response = call_llm(prompt)

    return llm_response
```

**Time:** 3-4 days

---

## PHASE 3: RESEARCH TOOLS (Study Papers)

### Tool #6: GraphFuzz

**Status:** ⚠️ Unknown availability (ICSE 2022)

**Action:** Study paper for lifetime analysis technique

### Tool #7: AFGen

**Status:** ⚠️ Unknown availability (IEEE S&P 2024)

**Action:** Study paper for constraint extraction

### Tool #8: Oracle-Guided Harnessing

**Status:** ⚠️ Too new (ICSE 2025)

**Action:** Study paper when proceedings released

### Tool #9: FUDGE

**Status:** ❌ Closed-source (Facebook internal)

**Action:** Study paper only

---

## QUICK START: 3-Day Evaluation Plan

**Day 1: Setup + OSS-Fuzz-Gen**
- Morning: Setup evaluation environment (Step 0)
- Afternoon: Install OSS-Fuzz-Gen, run on libtiff
- Evening: Analyze LLM prompts and generated code

**Day 2: DIY WildSync + Utopia**
- Morning: Run GitHub mining script (WildSync-style)
- Afternoon: Extract test patterns (Utopia-style)
- Evening: Compare patterns from both sources

**Day 3: Mock Generation + Integration**
- Morning: Implement mock generator (Hopper-style)
- Afternoon: Test mock generation on TIFF*
- Evening: Document findings and integration plan

**Expected Outcome After 3 Days:**
- ✅ Hands-on experience with LLM-based generation
- ✅ GitHub mining infrastructure working
- ✅ Test pattern extraction working
- ✅ Mock handle generation prototype
- ✅ Clear understanding of what to integrate into PIN

---

## SUMMARY: TOOL PRIORITY MATRIX

| Priority | Tool | Setup Difficulty | Expected Insight | Time |
|----------|------|-----------------|------------------|------|
| **1** | OSS-Fuzz-Gen | ✅ Easy (public) | LLM seed generation | 1-2 days |
| **2** | WildSync (DIY) | ⚠️ Medium (reimpl) | Ecosystem mining | 2-3 days |
| **3** | Utopia (DIY) | ⚠️ Medium (reimpl) | Test mining | 2-3 days |
| **4** | Hopper (DIY) | ⚠️ Medium (reimpl) | Mock objects | 2-3 days |
| **5** | CKGFuzzer (DIY) | 🔴 Hard (CKG build) | Call graph | 3-4 days |
| **6** | GraphFuzz | 📄 Paper study | Lifetime analysis | 2-3 days |
| **7** | AFGen | 📄 Paper study | Constraints | 2-3 days |
| **8** | Oracle-guided | 📄 Paper study | Oracles | 1-2 days |
| **9** | FUDGE | 📄 Paper study | Patterns | 1-2 days |

**Total Time Estimate:**
- **Full setup:** 15-20 days (all tools)
- **Quick evaluation:** 3 days (top 3 tools)
- **Recommended:** 7-10 days (top 5 tools + paper study)

---

**Next Steps:**

1. Start with OSS-Fuzz-Gen (immediately actionable)
2. Run GitHub mining script (quick insights)
3. Extract test patterns from libtiff
4. Study papers for unavailable tools
5. Document findings in technique extraction matrix

**Ready to Execute:** Yes ✅
