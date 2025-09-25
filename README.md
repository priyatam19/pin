# PIN: Program Input Normalization

[![GitHub license](https://img.shields.io/github/license/priyatam19/pin)](https://github.com/priyatam19/pin/blob/main/LICENSE)
[![GitHub issues](https://img.shields.io/github/issues/priyatam19/pin)](https://github.com/priyatam19/pin/issues)
[![GitHub stars](https://img.shields.io/github/stars/priyatam19/pin)](https://github.com/priyatam19/pin/stargazers)

## Overview

PIN (Program Input Normalization) is a tool designed to standardize the input interfaces of arbitrary C programs by transforming them to accept serialized byte inputs using Google Protocol Buffers (Protobuf). This enables uniform program analysis, optimization, and testing (e.g., fuzzing) across diverse input types, ranging from primitives (integers, floats) to complex structures (structs, pointers, arrays).

The project addresses the challenge of varying input spaces in program analysis by constructing a decoder function that maps bytes to the program's native input space, preserving semantics. As of August 19, 2025, PIN supports basic primitives, structs, nested/anonymous structs, strings, and arrays with both pycparser and libclang parsing backends. Recent improvements include enhanced string buffer handling and robust support for complex nested structs.

This tool is inspired by optimization-based program analysis techniques and aims to simplify pipelines for verification, fuzzing, and benchmarking. It is built with pycparser for C parsing, nanopb for lightweight Protobuf in C, and automated pipelines for end-to-end normalization.

## Motivation

Program analysis is complicated by diverse input interfaces (e.g., integers, strings, pointers, hybrids). PIN transforms any C program to accept inputs from a uniform domain (bytes), enabling standardized techniques like coverage-guided fuzzing or optimization solvers. This homogeneity drastically simplifies analysis, as highlighted in the project proposal.

## Features

- **Automatic Schema Generation**: Parses C code to extract input structs or function parameters, generating corresponding .proto files.
- **Decoder Composition**: Wraps the original program with nanopb-based deserialization to handle byte inputs.
- **Dual Parser Support**: Both pycparser and libclang backends for robust C code parsing
- **Support for Input Types**:
  - Primitives: int, float, double, bool.
  - Strings: char[] or char* as Protobuf strings with proper buffer management.
  - Arrays: Fixed-length as repeated fields.
  - Structs: Nested and anonymous structs as messages with cleaned naming.
- **String Buffer Handling**: Advanced nanopb callback system for complex string field deserialization
- **Pipeline Automation**: Full build process from C file to normalized executable, with random input generation for testing.
- **Differential Testing**: Automated comparison between original and normalized program outputs
- **Extensibility**: Modular for future additions like pointers (as optional/repeated), enums (Protobuf enums), unions (oneof).
- **Output Preservation**: Normalized binary produces equivalent outputs (e.g., printf) for equivalent inputs.

## Installation

### Prerequisites
- Python 3.8+ with required packages:
  - `pip install pycparser` for C AST parsing
  - `pip install libclang` for alternative parsing backend
- Protobuf compiler (`protoc`): Install via package manager (e.g., `apt install protobuf-compiler` on Ubuntu).
- Nanopb: Included as a submodule; run `git submodule update --init`.
- GCC for compilation.
- (Optional) Fake libc headers for pycparser: Clone https://github.com/eliben/pycparser/tree/master/utils/fake_libc_include into the project root.

### Setup
1. Clone the repository:
   ```
   git clone https://github.com/priyatam19/pin.git
   cd pin
   git submodule update --init
   ```
2. (Optional) Create a virtual environment:
   ```
   python -m venv venv
   source venv/bin/activate
   pip install pycparser
   ```

## Usage

Run the pipeline script to normalize a C program:

```
./src/full_pipeline.sh <path_to_c_file> [function_name] [--parser=<pycparser|libclang>] [--headers-dir=<dir>]
```

**Parameters:**
- `<path_to_c_file>`: Relative path to the C file (e.g., examples/myprog.c).
- `[function_name]`: Optional; the function to normalize (defaults to "main"). For whole-program normalization, use "main".
- `[--parser=<backend>]`: Parser backend to use ("pycparser" or "libclang"). Defaults to pycparser.
- `[--headers-dir=<dir>]`: Directory containing custom header files for complex programs.

### Example Commands
- Normalize a simple function with pycparser:
  ```
  ./src/full_pipeline.sh examples/check_num.c checkNum
  ```
- Normalize a complex struct function with libclang:
  ```
  ./src/full_pipeline.sh examples/myprog.c P --parser=libclang
  ```
- Normalize with custom headers:
  ```
  ./src/full_pipeline.sh examples/mqtt.c main --parser=libclang --headers-dir=utils/user_headers
  ```

### Output
- Build artifacts in `build/<example_name>/` (temporary).
- Results (normalized binary `pin_test`, input.bin, output.log, .proto, .pb.c/h, main.c) in `results/<example_name>/`.
- Run the normalized binary manually: `./results/<example_name>/pin_test <serialized_input.bin>`.

## Project Structure

- `src/`: Core scripts (full_pipeline.sh, pycparser_generate_proto.py, generate_wrapper_ast.py).
- `examples/`: Sample C programs (e.g., check_num.c, myprog.c, basename.c).
- `nanopb/`: Submodule for lightweight Protobuf.
- `build/`: Per-example build directories (temporary).
- `results/`: Per-example output directories with normalized binaries and artifacts.
- `fake_libc_include/`: Fake headers for pycparser to handle standard types.
- `README.md`: This file.
- `PIN_Proposal.pdf`: Original project proposal.

## How It Works

1. **Preprocessing**: Strip includes and preprocess to create parseable C code.
2. **Parsing & Schema Generation**: Use pycparser to extract structs or function params, map to Protobuf schema (.proto).
3. **Random Input Generation**: Dynamic Python script fills the message with random values based on descriptors.
4. **Wrapper Generation**: Create main.c with decoder (nanopb) and call to original function.
5. **Nanopb Code Gen**: Generate .pb.c/h for serialization/deserialization.
6. **Compilation & Execution**: Compile original as object, link with wrapper and nanopb to produce pin_test, run with random input.
7. **Run & Results**: Execute with random input, store outputs.

## Current Status & Limitations

### ✅ **Completed Features**
- **Enhanced String Buffer Handling**: Fixed libclang parsing for complex nested structs with string fields
- **Dual Parser Support**: Both pycparser and libclang backends work correctly with proper type mapping
- **Anonymous Struct Support**: Proper handling and naming of anonymous/unnamed structs
- **Complex Nested Structs**: Full support for arbitrarily nested struct hierarchies
- **Comprehensive Type Mapping**: C primitive types correctly mapped to Protocol Buffer equivalents

### 🔄 **Current Limitations**
- **Coreutils Programs**: Limited support for complex programs like basename.c, cat.c due to:
  - argc/argv normalization not implemented
  - Missing comprehensive GNU libc header system
  - Complex dependency chains and conditional compilation
- **Pointers**: Basic fallback to bytes; full support (optional, repeated, allocation) in progress
- **Enums/Unions**: Not yet mapped (enums to Protobuf enums, unions to oneof)
- **Global Variables**: Not handled in current implementation
- **Complex Memory**: No deep copy for pointers or cycles

## Development Roadmap

### Phase 2: Core Extensions (September 2025)
1. **Pointer Support**: Map simple pointers (e.g., int*) to optional fields, struct* to nested optional messages. Use nanopb callbacks for dynamic allocation and null handling. **Status**: Planning
2. **Enums and Unions**: Map C enums to Protobuf enums, unions to oneof. Extend parser for variant types. **Status**: Design phase
3. **CLI Normalization (argc/argv)**: Special-case main signatures; map argv to repeated string, argc as implicit length. Allocate char** in decoder. **Critical for coreutils support**. **Status**: High priority

### Phase 3: Coreutils & Real-World Support (October 2025)
4. **Comprehensive Header System**: Build extensive fake header system for GNU libc compatibility
   - Support for complex system includes like `<config.h>`, `<getopt.h>`
   - Custom type definitions (`idx_t`, `ptrdiff_t`, etc.)
   - Conditional compilation support
5. **Advanced Preprocessing**: Enhanced C preprocessing pipeline to handle complex build systems
6. **GNU Utilities Testing**: Validate with coreutils programs (basename, cat, grep, etc.)

### Phase 4: Analysis Integration (November 2025)
7. **Fuzzer Integration**: Hook into AFL/libFuzzer with byte inputs; generate seed corpora from schemas. Add enhanced differential testing capabilities.
8. **Performance Optimization**: Minimize deserialization overhead and memory usage
9. **Benchmarks & Evaluation**: Performance metrics using FuzzBench/Magma test suites

### Phase 5: Advanced Features (December 2025)
10. **Function Decomposer Integration**: Modular analysis of large codebases with per-function normalization
11. **Static Analysis Integration**: Compatibility with verification and symbolic execution tools
12. **Language Extensions**: Support for C++ constructs and modern C features

Contributions welcome—open issues/PRs for bugs or features.

## Acknowledgments

- nanopb for C Protobuf.
- pycparser for C AST parsing.
- Inspired by program analysis research and Protobuf fuzzing tools.

Licensed under MIT. See LICENSE for details.