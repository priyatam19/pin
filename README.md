# PIN: Program Input Normalization

[![GitHub license](https://img.shields.io/github/license/priyatam19/pin)](https://github.com/priyatam19/pin/blob/main/LICENSE)
[![GitHub issues](https://img.shields.io/github/issues/priyatam19/pin)](https://github.com/priyatam19/pin/issues)
[![GitHub stars](https://img.shields.io/github/stars/priyatam19/pin)](https://github.com/priyatam19/pin/stargazers)

## Overview

PIN (Program Input Normalization) is a tool designed to standardize the input interfaces of arbitrary C programs by transforming them to accept serialized byte inputs using Google Protocol Buffers (Protobuf). This enables uniform program analysis, optimization, and testing (e.g., fuzzing) across diverse input types, ranging from primitives (integers, floats) to complex structures (structs, pointers, arrays).

The project addresses the challenge of varying input spaces in program analysis by constructing a decoder function that maps bytes to the program's native input space, preserving semantics. As of July 23, 2025, PIN supports basic primitives, structs, nested/anonymous structs, strings, and arrays, with ongoing extensions for pointers, enums, unions, and real-world CLI programs.

This tool is inspired by optimization-based program analysis techniques and aims to simplify pipelines for verification, fuzzing, and benchmarking. It is built with pycparser for C parsing, nanopb for lightweight Protobuf in C, and automated pipelines for end-to-end normalization.

## Motivation

Program analysis is complicated by diverse input interfaces (e.g., integers, strings, pointers, hybrids). PIN transforms any C program to accept inputs from a uniform domain (bytes), enabling standardized techniques like coverage-guided fuzzing or optimization solvers. This homogeneity drastically simplifies analysis, as highlighted in the project proposal.

## Features

- **Automatic Schema Generation**: Parses C code to extract input structs or function parameters, generating corresponding .proto files.
- **Decoder Composition**: Wraps the original program with nanopb-based deserialization to handle byte inputs.
- **Support for Input Types**:
  - Primitives: int, float, double, bool.
  - Strings: char[] or char* as Protobuf strings.
  - Arrays: Fixed-length as repeated fields.
  - Structs: Nested and anonymous structs as messages.
- **Pipeline Automation**: Full build process from C file to normalized executable, with random input generation for testing.
- **Extensibility**: Modular for future additions like pointers (as optional/repeated), enums (Protobuf enums), unions (oneof).
- **Output Preservation**: Normalized binary produces equivalent outputs (e.g., printf) for equivalent inputs.

## Installation

### Prerequisites
- Python 3.8+ with pycparser (`pip install pycparser`).
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
./src/full_pipeline.sh <path_to_c_file> [function_name]
```

- `<path_to_c_file>`: Relative path to the C file (e.g., examples/myprog.c).
- `[function_name]`: Optional; the function to normalize (defaults to "main"). For whole-program normalization, use "main".

### Example Commands
- Normalize a simple function:
  ```
  ./src/full_pipeline.sh examples/check_num.c checkNum
  ```
- Normalize the main entry point:
  ```
  ./src/full_pipeline.sh examples/basename.c main
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

## Limitations

- Pointers: Basic fallback to bytes; full support (optional, repeated, allocation) in progress.
- Enums/Unions: Not yet mapped (enums to Protobuf enums, unions to oneof).
- CLI Args (argc/argv): Fallback to bytes; repeated string extension planned.
- Globals/Non-Function Inputs: Not handled.
- Complex Memory: No deep copy for pointers or cycles.

## Future Work

Based on the project roadmap (as of July 23, 2025), the following enhancements are prioritized:

1. **Pointer Support (Phase 2)**: Map simple pointers (e.g., int*) to optional fields, struct* to nested optional messages. Use nanopb callbacks for dynamic allocation and null handling. Complexity: Medium; Target: August 15, 2025.
2. **Enums and Unions (Phase 2)**: Map C enums to Protobuf enums, unions to oneof. Extend parser for variant types. Target: August 20, 2025.
3. **CLI Normalization (argc/argv)**: Special-case main signatures; map argv to repeated string, argc as implicit length. Allocate char** in decoder. Useful for coreutils-like programs. Target: September 1, 2025.
4. **Fuzzer Integration (Phase 4)**: Hook into AFL/libFuzzer with byte inputs; generate seed corpora from schemas. Add differential testing (original vs. normalized outputs). Target: September 15, 2025.
5. **Benchmarks & Report**: Add performance metrics (decode time, overhead) and fuzzing efficacy (coverage, bugs) using FuzzBench/Magma. Update success criteria with demos. Target: September 30, 2025.
6. **Error Handling & Docs**: Robust logging for decode failures; expand README with real-world examples (e.g., basename.c fuzzing). Target: Ongoing.
7. **Integration with Function Decomposer**: Integrate PIN with the Function Decomposer tool (Clang-based refactoring for modularizing C code into per-function files and shared headers). The pipeline will first decompose the code, generate metadata.json, then normalize inputs for each modular component using metadata to target functions/types. This creates navigable, input-normalized code for large codebases, enabling uniform analysis (e.g., fuzzing isolated functions). Modes: Modular (decompose + normalize per function), Full (normalize originals), Hybrid (selective). Target: October 15, 2025.

Contributions welcome—open issues/PRs for bugs or features.

## Acknowledgments

- nanopb for C Protobuf.
- pycparser for C AST parsing.
- Inspired by program analysis research and Protobuf fuzzing tools.

Licensed under MIT. See LICENSE for details.