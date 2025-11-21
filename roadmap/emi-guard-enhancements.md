# EMI Guard Enhancements

## Objectives
Refine our equivalence-modulo-input-validation (EMI) policy so differential testing only considers well-formed inputs while keeping fuzzing throughput high.

## Work Items
- Capture pointer/length invariants and enum ranges in metadata for guard generation.
- Add rejection logging (reason codes, counts) to Stage B outputs.
- Expose guard evaluation status to fuzzers (distinct return codes) for feedback-guided learning.
- Build unit tests covering guard synthesis across scalar pointers, slices, structs, and future struct slices.
- Document guard semantics and troubleshooting workflow.

## Acceptance Criteria
- Stage B reports show accepted vs rejected inputs with reasons.
- Guards prevent false diffs on invalid data yet allow known valid fixtures through.
- Fuzzer campaigns confirm guard return codes improve convergence.
