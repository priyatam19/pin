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
- **Differential Fuzzing Pipeline**: Stage A libFuzzer harness plus Stage B replay scripts triage normalized vs reference executions.
- **Containerized Environment**: Dockerfile reproduces the full toolchain with pinned dependencies and auto-populated nanopb sources.
- **Extensibility**: Modular for future additions like pointers (as optional/repeated), enums (Protobuf enums), unions (oneof).
- **Output Preservation**: Normalized binary produces equivalent outputs (e.g., printf) for equivalent inputs.

## Installation

### Prerequisites
- Python 3.8+ with required packages (pinned in the Docker image):
  - `pip install pycparser` for C AST parsing
  - `pip install libclang` for alternative parsing backend
  - `pip install protobuf==3.20.3 nanopb==0.4.7` to match Ubuntu 22.04's `libprotoc 3.12`.
- Protobuf compiler (`protoc`): Install via package manager (e.g., `apt install protobuf-compiler` on Ubuntu).
- Nanopb runtime sources: Initialize the `nanopb/` submodule or copy the release from `/opt/nanopb` when using Docker.
- GCC/Clang toolchain for compilation.
- (Optional) Fake libc headers for pycparser: Clone https://github.com/eliben/pycparser/tree/master/utils/fake_libc_include into the project root.
- (Optional) Docker: build the provided `pin-dev` image for a turnkey environment.

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

### Containerized Setup (Recommended)
1. Build the image once: `docker build -t pin-dev .`
2. Start a reusable container with your workspace mounted: `docker run -d --name pin-dev-container -v "$(pwd)":/workspace pin-dev tail -f /dev/null`
3. Open a shell whenever you need it: `docker exec -it pin-dev-container bash` (or create an alias).
   - The entrypoint auto-populates `/workspace/nanopb` from `/opt/nanopb`; set `PIN_AUTO_POPULATE_NANOPB=0` to skip this copy.
   - Restart with `docker start -ai pin-dev-container` to resume work without rebuilding.


## Usage

Run the differential pipeline to fuzz and replay normalized inputs:

```
./src/pin_diff.sh <path_to_c_file> <function_name> [--headers-dir=<dir>] [--fuzz-seconds=<n>] [--replay-dir=<dir>] [--fuzz-flags="..."]
```

**Parameters:**
- `<path_to_c_file>`: Target C source file (e.g., `examples/check_num.c`).
- `<function_name>`: Entry function to normalize and exercise (e.g., `checkNum`).
- `[--headers-dir=<dir>]`: Additional include directory for preprocessing (fake headers, vendor headers, etc.).
- `[--fuzz-seconds=<n>]`: Optional Stage A libFuzzer duration (in seconds). Omit or set to `0` to skip fuzzing and run Stage B replay only.
- `[--replay-dir=<dir>]`: Override corpus directory used for Stage B differential replay (defaults to the Stage A corpus).
- `[--fuzz-flags="..."]`: Extra libFuzzer flags appended during Stage A discovery.

### Example Commands
- Fuzz `checkNum` for 60 s and replay discoveries:
  ```
  ./src/pin_diff.sh examples/check_num.c checkNum --fuzz-seconds=60
  ```
- Replay an existing corpus without fuzzing:
  ```
  ./src/pin_diff.sh examples/check_num.c checkNum --fuzz-seconds=0 --replay-dir=results/check_num_diff/stage_b
  ```
- Provide custom headers while fuzzing `mqtt`:
  ```
  ./src/pin_diff.sh examples/mqtt.c main --headers-dir=utils/user_headers --fuzz-seconds=120
  ```

> **Legacy Workflow**: `full_pipeline.sh` still exists for single-run normalization and differential comparison.
> It reuses the same proto/wrapper generators but stops after building `pin_test`. For thorough fuzzing and replay, prefer `pin_diff.sh`.

### Output
- Stage A: fuzz harness, corpus, and artifacts under `build/<example_name>_diff/`.
- Stage B: replay logs (`replay_summary.txt`, per-input stdout/stderr) in `results/<example_name>_diff/stage_b/`.
- Standalone binaries (`normalized_bin`, `reference_bin`, `fuzz_bytes`) reside in the build directory for local repro.

## Project Structure

- `src/`: Core scripts (`pin_diff.sh`, `full_pipeline.sh`, proto/wrapper generators).
- `examples/`: Sample C programs (e.g., check_num.c, myprog.c, basename.c).
- `nanopb/`: Submodule for lightweight Protobuf.
- `build/`: Per-example build directories (temporary).
- `results/`: Per-example output directories with normalized binaries and artifacts (see `<example>_diff/` for differential runs).
- `fake_libc_include/`: Fake headers for pycparser to handle standard types.
- `Dockerfile`: Reproducible build environment with pinned dependencies and nanopb bootstrap.
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
- **Differential Harness MVP**: Stage A libFuzzer discovery with automated Stage B replay and logging for normalized vs reference binaries
- **Reproducible Container Image**: Dockerfile with pinned protobuf/nanopb versions and auto-populated nanopb checkout

### 🔄 **Current Limitations**
- **Coreutils Programs**: Limited support for complex programs like basename.c, cat.c due to:
  - argc/argv normalization not implemented
  - Missing comprehensive GNU libc header system
  - Complex dependency chains and conditional compilation
- **Pointers**: Basic fallback to bytes; full support (optional, repeated, allocation) in progress
- **Enums/Unions**: Not yet mapped (enums to Protobuf enums, unions to oneof)
- **Global Variables**: Not handled in current implementation
- **Complex Memory**: No deep copy for pointers or cycles
- **Fuzz Replay Robustness**: Stage B exit status still surfaces protobuf decode failures and needs triage automation.

## Development Roadmap

### September 2025 – Stability & Automation
1. **Stage B Resilience**: Treat protobuf decode failures as triaged findings instead of hard exits; emit machine-readable replay logs.
2. **Corpus Management**: Auto-archive/prune corpora per target, attach metadata (coverage, discovery timestamps).
3. **Turnkey Environment**: Publish prebuilt images and lockfiles so CI/teammates inherit the working toolchain.

### October 2025 – Richer Input Modeling
4. **Pointer Semantics**: Map scalar pointers to optional/repeated fields and struct pointers to nested messages with nanopb callbacks.
5. **CLI Normalization**: Model `argc/argv` for main-style entry points, including UTF-8 argv decoding and environment stubs.
6. **Enums & Unions**: Emit Protobuf enums/oneofs with codegen for safe dispatch in the wrapper.

### November 2025 – Real-World Targets
7. **Header Toolkit**: Ship a curated fake header set for GNU/libc-heavy codebases and automate preprocessing hints.
8. **Coreutils Coverage**: Graduate tranche of coreutils demos (basename, cat, grep) through end-to-end differential replay.
9. **Scalability**: Parallelize normalization builds and fuzz sessions across multiple targets.

### By December 17, 2025 – Ecosystem & Deliverables
10. **Fuzzer Integrations**: Wire AFL, libFuzzer, and custom engines into a shared corpus exchange service.
11. **Differential Diagnostics**: Generate minimal counterexamples and human-readable triage bundles when normalized vs reference diverge.
12. **Static/Hybrid Analysis**: Expose structured inputs to symbolic execution and verification frameworks via stable APIs.

Contributions welcome—open issues/PRs for bugs or features.

## Acknowledgments

- nanopb for C Protobuf.
- pycparser for C AST parsing.
- Inspired by program analysis research and Protobuf fuzzing tools.

Licensed under MIT. See LICENSE for details.