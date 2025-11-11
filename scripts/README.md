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
