# PIN Utility Scripts

This directory contains utility scripts for PIN development and testing.

## test_nanopb_decoder_fix.sh

**Purpose**: Demonstrates the impact of using `--reference-decoder=nanopb` on reducing artificial DIFFs caused by UTF-8 validation inconsistencies.

**What it does**:
1. Runs 3 representative benchmarks with default C++ Protobuf decoder
2. Re-runs the same benchmarks with nanopb decoder
3. Compares DIFF counts and generates a reduction report

**Usage**:
```bash
./scripts/test_nanopb_decoder_fix.sh
```

**Expected Results**:
- **Default decoder**: High DIFF counts (67-73% from UTF-8 validation)
- **Nanopb decoder**: Significantly reduced DIFFs (only real bugs remain)

**Example Output**:
```
Benchmark                 |    Default |     Nanopb | Reduction
--------------------------+------------+------------+-----------
empty_mode_compare        |        102 |         32 |     68.6%
mixed_scalars             |         18 |          6 |     66.7%
mode_buffer_dump          |        100 |        100 |      0.0%
--------------------------+------------+------------+-----------
TOTAL                     |        220 |        138 |     37.3%
```

**Note**:
- The script runs each benchmark for only 30 seconds (configurable via `FUZZ_SECONDS`)
- For comprehensive testing, increase fuzzing time or add more benchmarks
- Results vary based on fuzzer coverage and random mutations

**References**:
- Architecture explanation: `docs/pin-architecture.md`
- DIFF case studies: `docs/emi-case-studies.md`
- Decoder recommendations: See "Decoder Choice and Recommendations" in `CLAUDE.md`

## test_pass_through_mode.sh

Smoke-tests the raw byte pass-through pipeline on a tiny parser target so CI can
ensure the `--input-mode=raw` plumbing stays working.

**Usage**:
```bash
./scripts/test_pass_through_mode.sh
```

The script points `pin_diff.sh` at `examples/simple_benchs/raw_passthrough_example.c`
with pass-through mode enabled, fuzzes for one second, and verifies that Stage B
produces a replay summary.

## Intelligent seed generation (via `pin_diff.sh --generate-seeds`)

When you pass `--generate-seeds[=N]` to `pin_diff.sh`, it invokes
`src/generate_intelligent_seeds.py` inside the build directory to enumerate
type-driven boundary corpora from the already-generated `.proto`. The resulting
`.bin` files land in `build/<target>_diff/seed_corpus/` and are copied into the
libFuzzer corpus before Stage A starts, giving structured targets a deterministic
starting point without leaving the local machine.
