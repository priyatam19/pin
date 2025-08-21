# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

PIN (Program Input Normalization) transforms C programs to accept serialized byte inputs using Google Protocol Buffers, enabling uniform program analysis and testing. The tool parses C code to extract input structures, generates corresponding .proto files, and creates wrapper code using nanopb for deserialization.

## Core Development Commands

### Running the Full Pipeline
```bash
./src/full_pipeline.sh <path_to_c_file> [function_name] [--parser=<pycparser|libclang>] [--headers-dir=<dir>]
```

Examples:
- Normalize a function: `./src/full_pipeline.sh examples/check_num.c checkNum`
- Normalize main entry: `./src/full_pipeline.sh examples/basename.c main`
- With custom headers: `./src/full_pipeline.sh examples/mqtt.c main --parser=libclang --headers-dir=utils/fake_headers`

### Prerequisites and Setup
- Python 3.8+ with pycparser: `pip install pycparser`
- Protobuf compiler: `apt install protobuf-compiler` (Ubuntu)
- Nanopb submodule: `git submodule update --init`
- GCC for compilation

### Build Dependencies
The project uses nanopb as a git submodule. If nanopb libraries are missing:
```bash
git submodule update --init
cd nanopb
cmake .
make
```

## Architecture

### Core Components

1. **src/pycparser_generate_proto.py**: Parses C files and generates .proto schemas
   - Supports both pycparser and libclang parsers
   - Maps C types to Protobuf types (int→int32, char[]→string, etc.)
   - Handles structs, arrays, pointers, and primitives

2. **src/generate_wrapper_ast.py**: Creates wrapper C code with nanopb integration
   - Generates main.c with deserialization logic
   - Handles string buffers and callback functions
   - Creates extern declarations for original functions

3. **src/full_pipeline.sh**: End-to-end automation script
   - Preprocesses C files (removes #includes, runs cpp)
   - Generates proto files and nanopb C code
   - Compiles original as object file with symbol renaming
   - Links wrapper with nanopb runtime
   - Performs differential testing (original vs normalized)

### Directory Structure

- `src/`: Core scripts for parsing, generation, and pipeline automation
- `examples/`: Sample C programs including coreutils and ITC benchmarks
- `nanopb/`: Submodule providing lightweight Protobuf runtime for C
- `build/<example>/`: Per-example temporary build artifacts  
- `results/<example>/`: Final outputs (normalized binary, proto files, test results)
- `utils/fake_headers/`: Fake libc headers for pycparser compatibility
- `utils/user_headers/`: Custom headers (e.g., mongoose.h for networking examples)

### Type Mapping Strategy

The tool maps C types to Protobuf as follows:
- Primitives: `int`→`int32`, `float`→`float`, `double`→`double`, `bool`→`bool`
- Strings: `char[]` and `char*`→`string` with nanopb callbacks
- Arrays: Fixed-length→`repeated` fields
- Structs: Nested→Protobuf messages, anonymous→auto-generated names
- Pointers: Simple pointers→`optional` fields, complex→`bytes` fallback

### Build Process Flow

1. **Preprocessing**: Strip includes, run cpp with fake headers
2. **Proto Generation**: Parse C code, extract input structures, generate .proto
3. **Input Generation**: Create random test inputs using Python Protobuf bindings
4. **Wrapper Generation**: Create main.c with nanopb deserialization
5. **Compilation**: Compile original as object, link with wrapper and nanopb
6. **Testing**: Run both original and normalized binaries, compare outputs

## Working with Examples

### Testing Individual Programs
After running the pipeline, results are in `results/<example_name>/`:
- `pin_test`: Normalized binary accepting .bin input
- `input.bin`: Generated test input 
- `original_bin`: Original binary for comparison
- Output logs and comparison results

### Adding New Examples
1. Place C file in `examples/`
2. Run pipeline: `./src/full_pipeline.sh examples/yourfile.c [function_name]`
3. Check `results/yourfile/` for outputs

### Debugging Build Issues
- Check `build/<example>/cpp_errors.log` for preprocessing errors
- Check `build/<example>/proto_gen.log` for proto generation issues  
- Check `build/<example>/wrap_gen.log` for wrapper generation problems

## Current Limitations and Extensions

### Supported Features
- Basic primitives, structs, arrays, strings
- Nested and anonymous structs
- Function parameter normalization
- CLI-style main() functions
- Differential testing infrastructure

### Known Limitations
- Pointers: Basic fallback to bytes; full support in development
- Enums/Unions: Not yet mapped to Protobuf equivalents
- CLI Args (argc/argv): Fallback to bytes
- Complex memory structures and globals

### Development Roadmap
- Phase 2: Pointer support with optional fields and allocation
- Phase 2: Enums→Protobuf enums, Unions→oneof mappings
- Phase 3: CLI normalization for argc/argv
- Phase 4: Fuzzer integration and performance benchmarks