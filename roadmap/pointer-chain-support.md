# Pointer Chain Passthrough

## Goal
Handle pointer-chain parameters (e.g., `struct Foo **out`) without falling back to opaque bytes, keeping the pipeline generic.

## Planned Work
- Extend pointer metadata to classify `depth > 1` as `pointer_chain` with context.
- Generate wrapper code that allocates storage or reuses buffers while respecting ownership semantics.
- Update the C++ reference runner to mirror pointer-chain handling (unique_ptr wrapper or optional storage).
- Plumb EMI guards to skip comparisons when pointer-chain outputs are intentionally `NULL`.
- Add fixtures (e.g., `double_pointer_example`) to run end-to-end.

## Validation
- Stage B replay for pointer-chain fixtures matches outputs.
- Fuzzer campaigns demonstrate no crashes due to incorrect pointer ownership.
