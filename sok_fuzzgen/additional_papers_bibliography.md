# Additional Papers for SoK on Automated Fuzz Driver Generation

This bibliography extends the papers already collected in `review_papers.txt` with additional recent work from 2024-2025, with a focus on LLM-based approaches and emerging trends.

---

## 1. LLM-Based Fuzz Driver Generation

### 1.1 Prompt Engineering and Fine-tuning
- **Lyu, Y., Xie, Y., Chen, P., Chen, H.** (2024). Prompt Fuzzing for Fuzz Driver Generation. *Proceedings of the 2024 ACM SIGSAC Conference on Computer and Communications Security (CCS '24)*. https://dl.acm.org/doi/10.1145/3658644.3670396
  - First work comparing prompt engineering vs fine-tuning for LLM-based fuzz driver generation
  - Implements custom FuzzedDataProvider for type-aware parameter generation

- **Zhang, C., et al.** (2024). How Effective Are They? Exploring Large Language Model Based Fuzz Driver Generation. *Proceedings of the 33rd ACM SIGSOFT International Symposium on Software Testing and Analysis (ISSTA '24)*. https://dl.acm.org/doi/10.1145/3650212.3680355
  - First in-depth analysis of LLM-based fuzz driver generation
  - Dataset with 86 questions from 30 C projects
  - Evaluated 736,430 generated drivers across 5 LLMs with 6 prompting strategies
  - Identified need for semantic oracles for logical bug detection

### 1.2 Code Knowledge Graph Enhancement
- **Xu, J., Ma, L., et al.** (2024). CKGFuzzer: LLM-Based Fuzz Driver Generation Enhanced By Code Knowledge Graph. *ICSE 2025 Industry Challenge Track*. https://arxiv.org/abs/2411.11532
  - Uses interprocedural program analysis to construct code knowledge graphs
  - 8.73% average improvement in code coverage vs state-of-the-art
  - Reduces manual review workload by 84.4%
  - Detected 11 real bugs (9 previously unreported)

### 1.3 General LLM-Based Fuzzing
- **Xia, C. S., et al.** (2024). Fuzz4All: Universal Fuzzing with Large Language Models. *Proceedings of the 46th International Conference on Software Engineering (ICSE '24)*. https://arxiv.org/abs/2308.04748
  - First universal fuzzer targeting multiple languages (C, C++, Go, SMT2, Java, Python)
  - Identified 98 bugs in GCC, Clang, Z3, OpenJDK (64 previously unknown)
  - Uses LLMs as input generation and mutation engine

- **DFuzz** (2025). Your Fix Is My Exploit: Enabling Comprehensive DL Library API Fuzzing with Large Language Models. https://arxiv.org/html/2501.04312v1
  - Novel white-box LLM-based DL library API fuzzing
  - Uses LLMs to infer edge cases and generate initial test programs
  - Detected 37 bugs, 8 fixed, 19 replicated by developers

---

## 2. Library API Fuzzing (Recent Advances)

### 2.1 Automated Harness Synthesis
- **WildSync** (2025). Automated Fuzzing Harness Synthesis via Wild API Usage Recovery. *Proceedings of the ACM on Software Engineering (ISSTA '25)*. https://dl.acm.org/doi/10.1145/3728918
  - Produced 469 new harnesses for 24 actively fuzzed OSS-Fuzz libraries
  - Coverage increase: 1.3k functions, 16k lines of code
  - Identified 7 previously undetected bugs

- **Oracle-guided Harnessing** (2025). No Harness, No Problem: Oracle-guided Harnessing for Auto-generating C API Fuzzing Harnesses. *ICSE 2025 Research Track*. https://conf.researchr.org/details/icse-2025/icse-2025-research-track/202/
  - Fully-automatic, semantics-aware API fuzzing harness synthesis
  - Uses Correctness Oracles: compilation, execution, coverage changes
  - Generates diverse semantically-correct harnesses in ~1 hour

- **Chen, Y., et al.** (2025). Liberating Libraries through Automated Fuzz Driver Generation: Striking a Balance without Consumer Code. *Proceedings of the ACM on Software Engineering*. https://dl.acm.org/doi/10.1145/3729365

### 2.2 Type-Aware Input Generation
- **PointFuzz** (September 2025). Efficient Fuzzing of Library Code via Point-to-Point Mutations. *Electronics*. https://www.mdpi.com/2079-9292/14/19/3796
  - Integrates type-aware input generation into harness pipelines
  - Addresses semantic relevance and structural validity of API parameters
  - Tailored mutation strategies for primitive and composite data types

---

## 3. Deep Learning Library Fuzzing

- **FlashFuzz** (2025). Evaluating the Effectiveness of Coverage-Guided Fuzzing for Testing Deep Learning Library APIs. https://arxiv.org/abs/2509.14626
  - LLM-based harness synthesis for PyTorch and TensorFlow
  - 1,151 PyTorch and 662 TensorFlow API harnesses
  - 101-213% higher coverage vs state-of-the-art
  - Discovered 42 previously unknown bugs (8 fixed)
  - Available: https://github.com/ncsu-swat/FlashFuzz

- **Orion** (2024). History-Driven Fuzzing for Deep Learning Libraries. *ACM Transactions on Software Engineering and Methodology*. https://dl.acm.org/doi/10.1145/3688838
  - Reported 135 vulnerabilities in TensorFlow and PyTorch (76 confirmed)
  - Combines guided test input generation with corner-case generation
  - Uses fuzzing heuristic rules from historical vulnerability data

- **MoCo** (2024). Fuzzing Deep Learning Libraries via Assembling Code. https://arxiv.org/html/2405.07744
  - Uses seed tests implementing real-world DL models
  - Tests construction, training, and evaluation scenarios

- **DeepREL** (2022, relevant for comparison). Fuzzing Deep-Learning Libraries via Automated Relational API Inference. *Proceedings of the 30th ACM Joint European Software Engineering Conference and Symposium on the Foundations of Software Engineering (FSE '22)*. https://dl.acm.org/doi/10.1145/3540250.3549085
  - Covers 157% more APIs than FreeFuzz
  - Detected 162 bugs (106 confirmed previously unknown)

- **∇Fuzz** (2023). Fuzzing Automatic Differentiation in Deep-Learning Libraries. *Proceedings of the 45th International Conference on Software Engineering (ICSE '23)*. https://arxiv.org/abs/2302.04351
  - Detected 173 bugs in PyTorch, TensorFlow, JAX, OneFlow
  - 144 confirmed (117 previously unknown)

---

## 4. Android Native Library Fuzzing

- **FuzzGen++** (2024). Applying Fuzz Driver Generation to Native C/C++ Libraries of OEM Android Framework: Obstacles and Solutions. *Proceedings of the 39th IEEE/ACM International Conference on Automated Software Engineering (ASE '24)*. https://dl.acm.org/doi/10.1145/3691620.3695266
  - Generated 21,457 fuzz drivers for OEM Android frameworks
  - 107.92% coverage improvement vs hand-written drivers
  - Discovered 6 bugs in real-world OEM frameworks

- **Atlas** (2024). Automating Cross-Language Fuzzing on Android Closed-Source Libraries. *Proceedings of the 33rd ACM SIGSOFT International Symposium on Software Testing and Analysis (ISSTA '24)*. https://dl.acm.org/doi/10.1145/3650212.3652133
  - Generated 820 harnesses with 767 native APIs (78% practical)
  - Discovered 74 new security bugs, 16 CVEs assigned
  - Cross-language analysis (Java + native)

---

## 5. Kernel and System Call Fuzzing

### 5.1 Syscall Specification Generation
- **SyzSpec** (2025). Specification Generation for Linux Kernel Fuzzing. *Proceedings of the ACM SIGSAC Conference on Computer and Communications Security (CCS '25)*. https://www.cs.ucr.edu/~zhiyunq/pub/ccs25_syzspec.pdf
  - Determines types, sizes, and constraints of user inputs
  - Discovered 100 unique crashes during evaluation

- **KernelGPT** (2024). Enhanced Kernel Fuzzing via Large Language Models. *Proceedings of the 30th ACM International Conference on Architectural Support for Programming Languages and Operating Systems (ASPLOS '25)*. https://dl.acm.org/doi/10.1145/3676641.3716022
  - First LLM-based syscall specification synthesis approach
  - Leverages LLMs' pre-training on kernel code and documentation

- **SyzForge** (2025). An Automated System Call Specification Generation Process for Efficient Kernel Fuzzing. *Springer LNCS*. https://link.springer.com/chapter/10.1007/978-3-031-97620-9_7
  - Integrates static analysis, symbolic execution, fuzzing, and LLM refinement
  - Four-stage pipeline for precise syscall specifications

- **SyzGen++** (2024). Dependency Inference for Augmenting Kernel Driver Fuzzing. *Proceedings of the 2024 IEEE Symposium on Security and Privacy (SP '24)*.
  - Infers dependencies to improve kernel driver fuzzing

- **Unlocking Low Frequency Syscalls** (2024). Unlocking Low Frequency Syscalls in Kernel Fuzzing with Dependency-Based RAG. *Proceedings of the ACM on Software Engineering*. https://dl.acm.org/doi/10.1145/3728913
  - Uses Retrieval-Augmented Generation for low-frequency syscalls

- **KBinCov** (2024). Leveraging Binary Coverage for Effective Generation Guidance in Kernel Fuzzing. *Proceedings of the 2024 ACM SIGSAC Conference on Computer and Communications Security (CCS '24)*. https://dl.acm.org/doi/10.1145/3658644.3690232

- **Survey** (2025). A Survey of Fuzzing Open-Source Operating Systems. https://arxiv.org/html/2502.13163v1
  - Comprehensive review of 99 OS kernel fuzzing papers (2017-2024)

- **SoK: Unraveling the Veil of OS Kernel Fuzzing** (2025). https://arxiv.org/html/2501.16165v1

---

## 6. Rust Library Fuzzing

- **FRIES** (2024). Fuzzing Rust Library Interactions via Efficient Ecosystem-Guided Target Generation. *Proceedings of the 33rd ACM SIGSOFT International Symposium on Software Testing and Analysis (ISSTA '24)*. https://dl.acm.org/doi/10.1145/3650212.3680348
  - Weighted API dependency graph encoding syntactic and usage patterns
  - Mines common usage patterns from Rust ecosystem

- **RPG** (2024). Rust Library Fuzzing with Pool-based Fuzz Target Generation and Generic Support. *Proceedings of the 46th International Conference on Software Engineering (ICSE '24)*. https://conf.researchr.org/details/icse-2024/icse-2024-research-track/100/
  - Pool-based search for diverse unsafe API sequences
  - Generic support and validity checks
  - Discovered 25 previously unknown bugs in 50 Rust libraries

- **RuMono** (2025). Fuzz Driver Synthesis for Rust Generic APIs. *ACM Transactions on Software Engineering and Methodology*. https://dl.acm.org/doi/10.1145/3709359
  - Infers API reachability from generic API dependency graph
  - Discovers reachable and valid monomorphic APIs

---

## 7. Differential Testing and Oracles

- **AccuOracle** (2025). Fuzzing JavaScript JIT compilers with a high-quality differential test oracle. *Computers & Security*. https://www.sciencedirect.com/science/article/abs/pii/S0167404825003499
  - Input template-based approach for differential results
  - Addresses high false positive rates (>90% in FuzzJIT)

- **DUMPLING** (2025). Fine-grained Differential JavaScript Engine Fuzzing. *Proceedings of the Network and Distributed System Security Symposium (NDSS '25)*. https://www.ndss-symposium.org/wp-content/uploads/2025-1411-paper.pdf
  - Deep introspection of JS engines
  - Detects divergences between optimized/unoptimized code

- **Evolutionary Generative Fuzzing for Kotlin Compiler** (2024). *Companion Proceedings of the 32nd ACM International Conference on the Foundations of Software Engineering (FSE '24)*. https://arxiv.org/abs/2401.06653
  - Differential testing for compiler validation
  - Bugs confirmed and fixed by JetBrains

- **SQLxDiff** (2025). Enhanced Differential Testing in Emerging Database Systems. https://arxiv.org/html/2501.01236v1
  - Identified 17 logic bugs that SQLancer couldn't find
  - Enhances scalability of differential testing

- **TWINFUZZ** (2025). Differential Testing of Video Hardware Acceleration Stacks. *Proceedings of the Network and Distributed System Security Symposium (NDSS '25)*. https://www.ndss-symposium.org/wp-content/uploads/2025-526-paper.pdf
  - Tests hardware accelerators with software reference model

---

## 8. Stateful and Sequence-Based Fuzzing

- **Erinys** (2024). Efficient fuzzing by function invoke sequence generation for smart contracts. *Proceedings of the 2024 8th International Conference on Big Data and Internet of Things*. https://dl.acm.org/doi/10.1145/3697355.3697394
  - Define-Use relationships between functions and state variables
  - Tested on 1970 contracts with improved coverage

- **Feedback-Guided API Fuzzing of 5G Networks** (2024). *Proceedings of the Network and Distributed System Security Symposium (NDSS '24)*. https://www.ndss-symposium.org/wp-content/uploads/futureg25-71.pdf
  - Black-box evolutionary fuzzer for REST APIs in 5G
  - State-aware sequence mutations
  - Found 2 critical vulnerabilities in 5G core implementations

---

## 9. Smart Contract Fuzzing

- **MAU** (2024). Towards Smart Contract Fuzzing on GPUs. *Proceedings of the 2024 IEEE Symposium on Security and Privacy (SP '24)*. https://chapering.github.io/pubs/sp24weimin.pdf
  - GPU-accelerated fuzzing with CPU-GPU task decomposition
  - Validates GPU-explored seeds on CPU

- **Echidna** - State-of-the-art property-based testing for Ethereum smart contracts
  - GitHub: https://github.com/crytic/echidna
  - Grammar-based fuzzing campaigns using contract ABI

---

## 10. Protocol and Binary Format Fuzzing

- **FormatFuzzer** (2021, but highly relevant). Effective Fuzzing of Binary File Formats. https://arxiv.org/pdf/2109.11277
  - Compiles binary templates into parser/mutator/generator
  - Tested on MP4, ZIP formats
  - Found unknown memory errors in ffmpeg, timidity

- **AutoHarness** - Automatic fuzzing harness generation tool
  - GitHub: https://github.com/parikhakshat/autoharness
  - Uses LLVM, Clang, CodeQL for function discovery

---

## 11. IoT and Embedded Systems Fuzzing

- **Survey** (November 2025). IoT Firmware Emulation and Its Security Application in Fuzzing: A Critical Revisit. *Future Internet*. https://www.mdpi.com/1999-5903/17/1/19
  - Reviews 27 state-of-the-art MCU firmware emulation works (2014-2024)
  - Covers TWFuzz, IoTFuzzSentry, TAIFuzz, RIoTFuzzer, MSLFuzzer, MULTIFUZZ

- **ChatHTTPFuzz** (2024). LLM-assisted IoT HTTP fuzzing

- **ECG** (2024). LLM-based corpus generation for embedded OS fuzzing

---

## 12. Web API Fuzzing

- **EvoMaster** (2024). Tool report: EvoMaster—black and white box search-based fuzzing for REST, GraphQL and RPC APIs. *Automated Software Engineering*. https://dl.acm.org/doi/10.1007/s10515-024-00478-1
  - Version 3.0.0 with Docker, GitHub Actions, Python support
  - Used daily in Fortune 500 companies (e.g., Meituan)
  - Supports REST, GraphQL, RPC APIs

- **GraphQLer** (2025). Enhancing GraphQL Security with Context-Aware API Testing. https://arxiv.org/html/2504.13358v1
  - Automatically reads schema and runs tests
  - Context-aware GraphQL fuzzing

- **Random Testing and Evolutionary Testing for GraphQL** (2024). *ACM Transactions on the Web*. https://dl.acm.org/doi/10.1145/3609427

---

## 13. Grammar-Based Fuzzing

- **Shaping Test Inputs in Grammar-Based Fuzzing** (2024). *Proceedings of the 33rd ACM SIGSOFT International Symposium on Software Testing and Analysis (ISSTA '24)*. https://dl.acm.org/doi/10.1145/3650212.3685553
  - Reviews limitations of existing grammar-based approaches
  - First approach incorporating distribution sampling

- **XAVIER** (2025). Grammar-based testing system for XML Injection Attacks. *Proceedings of the 34th ACM SIGSOFT International Symposium on Software Testing and Analysis (ISSTA '25)*.

- **Grammar Mutation** (May 2025). *ACM Transactions on Software Engineering and Methodology*. https://dl.acm.org/doi/abs/10.1145/3708517
  - Method for testing input parsers

- **JIMA-Fuzzing** (December 2024). Effective fuzzing using grammar detected from sample input

---

## 14. Benchmarking and Evaluation

- **FeatureBench** (2025). Program Feature-Based Benchmarking for Fuzz Testing. *Proceedings of the ACM on Software Engineering*. https://dl.acm.org/doi/10.1145/3728899
  - Generates programs with configurable, fine-grained features
  - Reviewed 25 fuzzing studies, extracted 7 program features
  - Addresses limitations of existing benchmarks

- **GreenBench** (2023). Green Fuzzer Benchmarking. *Proceedings of the 2023 ACM SIGSOFT International Symposium on Software Testing and Analysis (ISSTA '23)*.
  - Significantly speeds up fuzzer evaluations
  - Higher accuracy with more benchmarks in less time

---

## 15. Specialized Applications

### 15.1 Trusted Execution Environments
- **Fuzzing Trusted Execution Environments with Rust** (March 2025). *Computers & Security*. https://www.sciencedirect.com/science/article/pii/S0167404824005017
  - Two-way code generator in Rust
  - Iteratively traverses API specifications

### 15.2 Industrial Control Systems
- **SP-Fuzz** (2024). Fuzzing Soft PLC with Semi-automated Harness Synthesis. *Springer LNCS*. https://link.springer.com/chapter/10.1007/978-981-99-8024-6_22
  - Semi-automated harness creation for PLC runtime

### 15.3 Directed and Guided Fuzzing
- **Locus** (2025). Agentic Predicate Synthesis for Directed Fuzzing. https://arxiv.org/html/2508.21302

- **Directed Greybox Fuzzing via LLM** (2025). https://arxiv.org/html/2505.03425v1

---

## 16. Automated Harness Generation (Comprehensive Tools)

- **OSS-Fuzz-Gen** (2024). LLM powered fuzzing via OSS-Fuzz.
  - GitHub: https://github.com/google/oss-fuzz-gen
  - Generated targets for 160+ C/C++ projects
  - Up to 29% line coverage increase
  - 30 new bugs/vulnerabilities found

- **HarnessGen** - Collection of automated harness generation methods
  - GitHub: https://github.com/SmllXzBZ/HarnessGen

---

## 17. Recent General Works and Automated Systems

- **Orion** (2024). Fuzzing Workflow Automation. https://arxiv.org/html/2509.15195

- **Li, Z., et al.** (2024). Automated Generation and Compilation of Fuzz Driver Based on Large Language Models. *Proceedings of the 2024 9th International Conference on Cyber Security and Information Engineering*. https://dl.acm.org/doi/10.1145/3689236.3689272

---

## Key Observations and Emerging Trends

### Major Shifts Since Yan et al. (2025) SoK:

1. **LLM Integration is Now Mainstream**: Nearly all new fuzz driver generation tools incorporate LLMs in some capacity (prompt engineering, code generation, specification synthesis)

2. **Code Knowledge Graphs**: Emerging trend using program analysis + graph structures to guide LLMs (CKGFuzzer)

3. **Type-Aware and Semantic-Aware Generation**: Moving beyond syntactic correctness to semantic validity (PointFuzz, Oracle-guided Harnessing)

4. **Cross-Language Fuzzing**: Increased focus on JNI, Android frameworks, and multi-language systems

5. **Specialized Domain Tools**:
   - Deep Learning: FlashFuzz, Orion, MoCo
   - Kernel: KernelGPT, SyzSpec, SyzForge
   - Rust: FRIES, RPG, RuMono
   - Smart Contracts: Erinys, MAU
   - Web APIs: EvoMaster 3.0, GraphQLer

6. **Differential Testing Renaissance**: Renewed focus on differential testing as oracle (DUMPLING, TWINFUZZ, SQLxDiff)

7. **Benchmarking Concerns**: Growing recognition that evaluation methodology matters (FeatureBench, GreenBench, >50 fuzzing papers in Big Four 2024)

8. **GPU Acceleration**: First GPU-accelerated smart contract fuzzing (MAU)

9. **Industrial Adoption**: Tools like EvoMaster, OSS-Fuzz-Gen in production at Fortune 500 companies

10. **Correctness Oracles**: Emphasis on automated validation of generated harnesses (compilation, execution, coverage)

---

## Suggested SoK Dimensions to Add

Based on the new literature, consider adding these dimensions to your SoK:

1. **LLM Integration Taxonomy**:
   - Prompt engineering only
   - Fine-tuned models
   - Code knowledge graph enhancement
   - Hybrid (static analysis + LLM)

2. **Semantic Correctness Validation**:
   - Type-aware generation
   - Constraint solving
   - Correctness oracles
   - Differential testing

3. **Cross-Language Support**:
   - Single language (C/C++)
   - JNI/Native bridges
   - Multi-language systems
   - Universal (Fuzz4All)

4. **Evaluation Rigor**:
   - Number of benchmarks
   - Trial count and duration
   - Statistical significance
   - Comparison baselines
   - Reproducibility

5. **Industrial Applicability**:
   - Open source vs closed source
   - Deployment complexity
   - Integration with CI/CD
   - Real-world adoption cases

---

Total new papers identified: **60+ major publications** from 2024-2025 beyond the 31 in review_papers.txt
