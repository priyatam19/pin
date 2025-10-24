# Libtiff Integration Plan

## Objectives
Demonstrate PIN on real-world libtiff CVEs by:
1. Building a reproducible libtiff fuzz harness (AFL++ baseline).
2. Validating EMI correctness via function-level normalization.
3. Comparing time-to-crash/coverage before and after normalization.

## Tasks
- Select libtiff CVEs with reachable function entry points (e.g., `TIFFReadDirectory`).
- Automate libtiff build + reduced harness compilation.
- Reproduce CVE using AFL++ to establish baseline.
- Normalize the target function with PIN, verify EMI guards, and re-run fuzzing.
- Capture metrics (exec/s, coverage, crash time) and document findings.

## Deliverables
- `benchmarks/libtiff/` harness and build scripts.
- Report comparing AFL++ baseline vs normalized fuzzing.
- Updated documentation describing setup and results.
