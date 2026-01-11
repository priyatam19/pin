# Void Pointer Handling

## Problem
`void *` parameters still map to byte wrappers but the generated code emits invalid storage (e.g., `void data_storage = 0`). We need a generic fix that allocates `uint8_t` buffers and keeps C/C++ paths in sync.

## Tasks
- Update pointer classification to tag `void*` distinctly from scalar pointers.
- Emit `uint8_t` storage and pointer rewrites in both the nanopb wrapper and reference runner.
- Ensure EMI treats `void*` inputs as opaque blobs but still validates length companions when present.
- Add regression fixtures covering `void_pointer_example` and real-world APIs.

## Definition of Done
- Void pointer wrappers compile without errors.
- Stage B replay passes for existing fixtures.
- Documentation explains how `void*` is handled and any limitations.
