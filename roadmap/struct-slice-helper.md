# Struct Slice Helper

## Goal
Implement generic support for pointer+length struct slices across schema generation, wrapper reconstruction, and the C++ reference runner.

## Key Tasks
- Extend pointer metadata to emit struct slice helpers (repeated message + length).
- Update `generate_wrapper_ast.py` to allocate/free arrays of structs and register cleanup.
- Mirror struct slice reconstruction in the C++ reference harness using RAII containers.
- Add regression fixtures covering array-of-structs benchmarks.
- Update EMI guards to validate struct slice lengths before differential comparison.

## Validation
- Re-run pointer fixture suite; ensure `array_of_structs_example` passes Stage B.
- Add unit tests around struct slice helper generation and cleanup tracking.
