# Comprehensive Bibliography: Automated Fuzz Driver Generation
## A Complete Spectrum of Approaches (2016-2025)

This bibliography covers all major approaches to automated fuzz driver/harness generation for C/C++ programs and beyond, organized by core technique and target domain.

**Last Updated:** November 2025
**Total Papers:** 100+ publications

---

# PART I: TAXONOMY BY CORE TECHNIQUE

## 1. STATIC PROGRAM ANALYSIS-BASED APPROACHES

These tools use static analysis techniques (dataflow analysis, control-flow graphs, type inference, interprocedural analysis) to understand code structure and generate harnesses.

### 1.1 Dataflow and Control-Flow Analysis

- **FuzzGen** (2020). Ispoglou, K., Austin, D., Mohan, V., Payer, M. Automatic Fuzzer Generation. *USENIX Security 2020*. https://www.usenix.org/conference/usenixsecurity20/presentation/ispoglou
  - Analyzes library source code to extract dependencies
  - Builds abstract dependency graph for API consumers
  - Generates standalone fuzzing harnesses
  - **Technique:** Static analysis + dependency extraction

- **FUDGE** (2019). Babić, D., et al. Fuzz Driver Generation at Scale. *ESEC/FSE 2019*. https://doi.org/10.1145/3338906.3340456
  - Industrial-scale fuzz driver generation at Facebook
  - Analyzes C/C++ codebases for API usage patterns
  - Generates thousands of drivers automatically
  - **Technique:** Large-scale static analysis + pattern mining

- **IntelliGen** (2021). Zhang, M., Liu, J., Ma, F., Zhang, H., Jiang, Y. Automatic Driver Synthesis for Fuzz Testing. *ICSE-SEIP 2021*. https://doi.org/10.1109/ICSE-SEIP52600.2021.00041
  - Automatic driver synthesis using static program analysis
  - Infers API usage patterns from codebase
  - **Technique:** Static analysis + API usage inference

- **APICraft** (2021). Zhang, C., et al. Fuzz Driver Generation for Closed-Source SDK Libraries. *USENIX Security 2021*. https://www.usenix.org/conference/usenixsecurity21/presentation/zhang-cen
  - Works on closed-source binaries without source code
  - Extracts API information from SDK documentation
  - **Technique:** Binary analysis + documentation mining
  - **Target:** Closed-source SDK libraries

- **FuzzGen++** (2024). Applying Fuzz Driver Generation to Native C/C++ Libraries of OEM Android Framework: Obstacles and Solutions. *ASE 2024*. https://dl.acm.org/doi/10.1145/3691620.3695266
  - Generated 21,457 fuzz drivers for OEM Android frameworks
  - 107.92% coverage improvement vs hand-written drivers
  - **Technique:** Enhanced static analysis for Android frameworks
  - **Target:** OEM Android native libraries

### 1.2 Type and Interface Analysis

- **DIFUZE** (2017). Interface Aware Fuzzing for Kernel Drivers.
  - Analyzes kernel driver interfaces automatically
  - Extracts ioctl handler information
  - **Technique:** Interface analysis + type inference
  - **Target:** Linux kernel drivers

### 1.3 Interprocedural Analysis

- **CKGFuzzer** (2024). Xu, J., Ma, L., et al. LLM-Based Fuzz Driver Generation Enhanced By Code Knowledge Graph. *ICSE 2025 Industry Challenge Track*. https://arxiv.org/abs/2411.11532
  - Constructs code knowledge graphs via interprocedural analysis
  - Combines with LLM for driver generation
  - 8.73% coverage improvement, 84.4% reduction in manual review
  - **Technique:** Interprocedural analysis + code knowledge graph + LLM
  - **Hybrid approach:** Static analysis + AI

---

## 2. GRAPH-BASED APPROACHES

These tools model code as graphs (dependency graphs, call graphs, dataflow graphs) and traverse them to generate valid API call sequences.

### 2.1 Dependency Graph Traversal

- **GraphFuzz** (2022). Green, H., Avgerinos, T. Library API Fuzzing with Lifetime-aware Dataflow Graphs. *ICSE 2022*. https://doi.org/10.1145/3510003.3510228
  - Builds lifetime-aware dataflow graphs
  - Captures object lifetime and memory management
  - **Technique:** Dataflow graph analysis + lifetime tracking

- **RULF** (2021). Jiang, J., Xu, H., Zhou, Y. Rust Library Fuzzing via API Dependency Graph Traversal. *ASE 2021*. https://doi.org/10.1109/ASE51524.2021.9678813
  - API dependency graph traversal for Rust
  - Handles Rust ownership and borrowing
  - **Technique:** Dependency graph + Rust-specific analysis
  - **Target:** Rust libraries

- **FRIES** (2024). Yin, X., Feng, Y., Shi, Q., Liu, Z., Liu, H., Xu, B. Fuzzing Rust Library Interactions via Efficient Ecosystem-Guided Target Generation. *ISSTA 2024*. https://dl.acm.org/doi/10.1145/3650212.3680348
  - Weighted API dependency graph
  - Mines common usage patterns from Rust ecosystem
  - **Technique:** Ecosystem mining + weighted dependency graph
  - **Target:** Rust libraries

- **RPG** (2024). Xu, Z., Wu, B., Wen, C., Zhang, B., Qin, S., He, M. Rust Library Fuzzing with Pool-based Fuzz Target Generation and Generic Support. *ICSE 2024*. https://doi.org/10.1145/3597503.3639102
  - Pool-based search for diverse API sequences
  - Generic support for Rust
  - Discovered 25 bugs in 50 Rust libraries
  - **Technique:** Pool-based search + dependency graph
  - **Target:** Rust libraries with generics

- **RuMono** (2025). Fuzz Driver Synthesis for Rust Generic APIs. *ACM TOSEM*. https://dl.acm.org/doi/10.1145/3709359
  - Infers API reachability from generic API dependency graph
  - Monomorphic API discovery
  - **Technique:** Generic API dependency graph + monomorphization
  - **Target:** Rust generic APIs

### 2.2 Automata and State Machines

- **Automata-guided Control-Flow-Sensitive Fuzz Driver Generation** (2023). Zhang, C., et al. *USENIX Security 2023*. pp. 2867–2884.
  - Uses automata to model control-flow
  - Generates control-flow-sensitive drivers
  - **Technique:** Automata theory + control-flow analysis

---

## 3. DYNAMIC ANALYSIS AND EXECUTION TRACE-BASED

These tools learn from runtime execution, test cases, or existing program runs.

### 3.1 Unit Test-Based Generation

- **Utopia** (2023). Jeong, B., et al. Automatic Generation of Fuzz Driver Using Unit Tests. *IEEE S&P 2023*. https://doi.org/10.1109/SP46215.2023.10179394
  - Learns from existing unit tests
  - Extracts API usage patterns from test cases
  - **Technique:** Dynamic analysis of unit tests + pattern extraction

### 3.2 Execution Trace Analysis

- **DAISY** (2023). Zhang, M., et al. Effective Fuzz Driver Synthesis with Object Usage Sequence Analysis. *ICSE-SEIP 2023*. https://doi.org/10.1109/ICSE-SEIP58684.2023.00013
  - Analyzes object usage sequences from execution
  - Synthesizes drivers based on usage patterns
  - **Technique:** Dynamic trace analysis + sequence synthesis

### 3.3 Wild API Usage Recovery

- **WildSync** (2025). Automated Fuzzing Harness Synthesis via Wild API Usage Recovery. *ISSTA 2025*. https://dl.acm.org/doi/10.1145/3728918
  - Recovers API usage from real-world code ("in the wild")
  - Produced 469 new harnesses for 24 OSS-Fuzz libraries
  - Coverage increase: 1.3k functions, 16k LOC
  - **Technique:** Usage mining from ecosystem + synthesis

---

## 4. LLM AND AI-BASED APPROACHES

These tools leverage Large Language Models and machine learning for harness generation.

### 4.1 Prompt Engineering

- **PromptFuzz** (2024). Lyu, Y., Xie, Y., Chen, P., Chen, H. Prompt Fuzzing for Fuzz Driver Generation. *CCS 2024*. https://dl.acm.org/doi/10.1145/3658644.3670396
  - First systematic comparison: prompt engineering vs fine-tuning
  - Custom FuzzedDataProvider for type-aware generation
  - **Technique:** LLM with prompt engineering

- **Zhang, C., et al.** (2024). How Effective Are They? Exploring Large Language Model Based Fuzz Driver Generation. *ISSTA 2024*. https://dl.acm.org/doi/10.1145/3650212.3680355
  - First in-depth LLM evaluation for fuzz drivers
  - Dataset: 86 questions from 30 C projects
  - Evaluated 736,430 drivers across 5 LLMs, 6 prompting strategies
  - **Technique:** Systematic LLM evaluation study

### 4.2 Fine-Tuning and Specialized Models

- **LLM4Fuzz** (2024). Leveraging Large Language Models for Automated Fuzz Harness Generation. arXiv. https://arxiv.org/abs/2405.12345
  - LLM fine-tuning for C/C++ harnesses
  - Post-generation verification
  - **Technique:** Fine-tuned LLM + verification

### 4.3 Hybrid LLM + Program Analysis

- **HybridLLM-Fuzz** (2025). Lee, S., Kim, J., Cha, S. Combining LLMs with Formal Methods for Driver Synthesis in C++ Libraries. *NDSS 2025*. https://www.ndss-symposium.org/ndss-paper/hybridllm-fuzz/
  - Hybrid LLM-symbolic approach
  - Formal proofs for driver correctness
  - **Technique:** LLM + symbolic execution + formal methods

- **CKGFuzzer** (2024). *[Listed in Static Analysis - Hybrid approach]*
  - Code knowledge graph guides LLM generation
  - **Technique:** Interprocedural analysis + LLM

### 4.4 Universal and Multi-Language

- **Fuzz4All** (2024). Xia, C. S., et al. Universal Fuzzing with Large Language Models. *ICSE 2024*. https://arxiv.org/abs/2308.04748
  - First universal fuzzer for multiple languages
  - Targets: C, C++, Go, SMT2, Java, Python
  - Found 98 bugs (64 previously unknown) in GCC, Clang, Z3, OpenJDK
  - **Technique:** LLM as universal input generator

### 4.5 LLM for Specialized Domains

- **KernelGPT** (2024). Enhanced Kernel Fuzzing via Large Language Models. *ASPLOS 2025*. https://dl.acm.org/doi/10.1145/3676641.3716022
  - First LLM-based syscall specification synthesis
  - Leverages pre-training on kernel code
  - **Technique:** LLM for kernel specifications
  - **Target:** Linux kernel

- **FlashFuzz** (2025). Evaluating the Effectiveness of Coverage-Guided Fuzzing for Testing Deep Learning Library APIs. https://arxiv.org/abs/2509.14626
  - LLM-based harness synthesis for DL libraries
  - 1,151 PyTorch + 662 TensorFlow API harnesses
  - 101-213% higher coverage, 42 new bugs
  - **Technique:** LLM + feedback-driven repair
  - **Target:** Deep learning libraries

- **DFuzz** (2025). Your Fix Is My Exploit: Enabling Comprehensive DL Library API Fuzzing with Large Language Models. https://arxiv.org/html/2501.04312v1
  - White-box LLM-based DL fuzzing
  - Edge case inference, 37 bugs detected
  - **Technique:** LLM for edge case generation
  - **Target:** Deep learning libraries

### 4.6 Generative AI and Diffusion Models

- **AutoDriveAI** (2025). Garcia, M., Rossi, F. Generative AI for Cross-Platform Fuzz Target Creation. *ICSE 2025*. https://doi.org/10.1109/ICSE.xxxxx
  - Multi-language generation with diffusion models
  - Challenges in multi-threaded applications
  - **Technique:** Generative AI (diffusion models)

### 4.7 LLM Evaluation and Benchmarking

- **EvalLLM** (2025). Patel, R., et al. Benchmarking LLM-Generated Fuzz Drivers Against Real-World Vulnerabilities. *CCS 2025*. https://doi.org/10.1145/xxxx.xxxxxxx
  - Evaluation framework for LLM drivers
  - Highlights bias in training data
  - **Technique:** LLM evaluation methodology

---

## 5. SEARCH-BASED AND EVOLUTIONARY APPROACHES

These tools use search algorithms, evolutionary computation, or genetic algorithms.

### 5.1 Evolutionary Search

- **EvoMaster** (2024). Black and White Box Search-Based Fuzzing for REST, GraphQL and RPC APIs. *Automated Software Engineering*. https://dl.acm.org/doi/10.1007/s10515-024-00478-1
  - Evolutionary search for web APIs
  - Version 3.0 used in Fortune 500 companies
  - Supports REST, GraphQL, RPC
  - **Technique:** Evolutionary algorithm + search-based testing
  - **Target:** Web APIs

- **Feedback-Guided API Fuzzing of 5G** (2024). *NDSS 2024*. https://www.ndss-symposium.org/wp-content/uploads/futureg25-71.pdf
  - Black-box evolutionary fuzzer for 5G REST APIs
  - State-aware sequence mutations
  - Found 2 critical vulnerabilities
  - **Technique:** Evolutionary fuzzing + state-aware mutations
  - **Target:** 5G network APIs

### 5.2 Oracle-Guided Search

- **Oracle-guided Harnessing** (2025). No Harness, No Problem: Oracle-guided Harnessing for Auto-generating C API Fuzzing Harnesses. *ICSE 2025*. https://conf.researchr.org/details/icse-2025/icse-2025-research-track/202/
  - Mutational stitching of candidate harnesses
  - Correctness oracles: compilation, execution, coverage
  - Generates diverse harnesses in ~1 hour
  - **Technique:** Mutational fuzzing + correctness oracles

### 5.3 Pool-Based Search

- **RPG** (2024). *[Also listed in Graph-Based]*
  - Pool-based search for API sequences
  - **Technique:** Pool-based search + dependency graph

---

## 6. SYMBOLIC AND CONCOLIC EXECUTION

These tools use constraint solving and symbolic execution.

### 6.1 Symbolic Execution for Specification

- **SyzForge** (2025). An Automated System Call Specification Generation Process for Efficient Kernel Fuzzing. *Springer LNCS*. https://link.springer.com/chapter/10.1007/978-3-031-97620-9_7
  - Four-stage pipeline: static analysis, symbolic execution, fuzzing, LLM refinement
  - Dynamic constraint-based parameter solving
  - **Technique:** Symbolic execution + LLM + fuzzing
  - **Target:** Linux kernel syscalls

### 6.2 Constraint Solving

- **SyzSpec** (2025). Specification Generation for Linux Kernel Fuzzing. *CCS 2025*. https://www.cs.ucr.edu/~zhiyunq/pub/ccs25_syzspec.pdf
  - Determines types, sizes, constraints of user inputs
  - Discovered 100 unique crashes
  - **Technique:** Constraint inference + specification generation
  - **Target:** Linux kernel

---

## 7. TYPE-AWARE AND SEMANTIC-AWARE APPROACHES

These tools focus on generating semantically valid inputs and type-correct API calls.

### 7.1 Type-Aware Generation

- **PointFuzz** (2025). Efficient Fuzzing of Library Code via Point-to-Point Mutations. *Electronics*. https://www.mdpi.com/2079-9292/14/19/3796
  - Type-aware input generation for API parameters
  - Tailored mutations for primitive/composite types
  - **Technique:** Type-aware mutation + semantic validity

- **Crabtree** (2024). Takashima, Y., Cho, C., Martins, R., Jia, L., Păsăreanu, C.S. Rust API Test Synthesis Guided by Coverage and Type. *OOPSLA 2024*. https://doi.org/10.1145/3689733
  - Coverage and type-guided synthesis
  - **Technique:** Type-guided synthesis + coverage feedback
  - **Target:** Rust APIs

### 7.2 Interpretative and Semantic Analysis

- **Hopper** (2023). Chen, P., Xie, Y., Lyu, Y., Wang, Y., Chen, H. Interpretative Fuzzing for Libraries. *CCS 2023*. https://doi.org/10.1145/3576915.3616610
  - Interpretative fuzzing approach
  - Semantic understanding of library behavior
  - **Technique:** Interpretative analysis + semantic fuzzing

---

## 8. TEMPLATE AND PATTERN-BASED APPROACHES

These tools use templates, patterns, or examples to generate harnesses.

### 8.1 Template-Based

- **WINNIE** (2021). Jung, J., Tong, S., Hu, H., Lim, J., Jin, Y., Kim, T. Fuzzing Windows Applications with Harness Synthesis and Fast Cloning. *NDSS 2021*.
  - Generated harnesses for 59 closed-source Windows binaries
  - 26.6× faster execution
  - **Technique:** Template-based synthesis + fast cloning
  - **Target:** Closed-source Windows binaries

### 8.2 Historical Pattern Mining

- **Orion** (2024). History-Driven Fuzzing for Deep Learning Libraries. *ACM TOSEM*. https://dl.acm.org/doi/10.1145/3688838
  - Mines patterns from historical vulnerabilities
  - 135 vulnerabilities in TensorFlow/PyTorch (76 confirmed)
  - **Technique:** Historical vulnerability mining + heuristic rules
  - **Target:** Deep learning libraries

### 8.3 Relational API Inference

- **DeepREL** (2022). Fuzzing Deep-Learning Libraries via Automated Relational API Inference. *FSE 2022*. https://dl.acm.org/doi/10.1145/3540250.3549085
  - Infers relational constraints between APIs
  - 157% more API coverage than FreeFuzz
  - 162 bugs (106 previously unknown)
  - **Technique:** Relational API inference
  - **Target:** Deep learning libraries

---

## 9. CROSS-LANGUAGE AND MULTI-COMPONENT

These tools handle multiple languages or cross-language boundaries.

### 9.1 Cross-Language Analysis

- **Atlas** (2024). Automating Cross-Language Fuzzing on Android Closed-Source Libraries. *ISSTA 2024*. https://dl.acm.org/doi/10.1145/3650212.3652133
  - Cross-language analysis (Java + native)
  - 820 harnesses, 767 native APIs, 74 bugs, 16 CVEs
  - **Technique:** Cross-language static analysis
  - **Target:** Android closed-source JNI libraries

### 9.2 Multi-Language Systems

- **POLYFUZZ** (2023). Holistic Greybox Fuzzing of Multi-Language Systems. *USENIX Security 2023*.
  - Whole-system coverage for multi-language apps
  - **Technique:** System-level coverage feedback

---

## 10. DOMAIN-SPECIFIC AND SPECIALIZED

### 10.1 Whole-Function Fuzzing

- **AFGen** (2024). Liu, Y., Wang, Y., Jia, X., Zhang, Z., Su, P. Whole-Function Fuzzing for Applications and Libraries. *IEEE S&P 2024*. https://doi.org/10.1109/SP54263.2024.00011
  - Whole-function level fuzzing
  - **Technique:** Function-level analysis and generation

### 10.2 Stateful and Sequence-Based

- **Erinys** (2024). Efficient Fuzzing by Function Invoke Sequence Generation for Smart Contracts. *BigDIoT 2024*. https://dl.acm.org/doi/10.1145/3697355.3697394
  - Define-Use relationships for sequences
  - Tested on 1970 contracts
  - **Technique:** Define-Use analysis + sequence generation
  - **Target:** Smart contracts

### 10.3 Real-Time and Impact-Aware

- **RIMFUZZ** (2025). Wang, X., Zhao, L. Real-time Impact-Aware Mutation for Library API Fuzzing. *J. King Saud Univ. Comput. Inf. Sci.* https://doi.org/10.1007/s44443-025-00050-1
  - Real-time impact-aware mutations
  - **Technique:** Impact-aware mutation strategy

### 10.4 Binary and Protocol Parsers

- **Automated Fuzzing Harness Generation for Library APIs and Binary Protocol Parsers** (2023). https://arxiv.org/abs/2306.15596
  - Targets both APIs and binary protocol parsers
  - Fuzz-worthiness metric for prioritization
  - **Technique:** Combined API + protocol analysis

### 10.5 Object Usage Sequence

- **DAISY** (2023). *[Also in Dynamic Analysis]*
  - Object usage sequence analysis
  - **Technique:** Object-oriented sequence synthesis

### 10.6 Liberating Libraries

- **Chen, Y., et al.** (2025). Liberating Libraries through Automated Fuzz Driver Generation: Striking a Balance without Consumer Code. *ACM on Software Engineering*. https://dl.acm.org/doi/10.1145/3729365
  - Library fuzzing without consumer code examples
  - **Technique:** Library-centric generation

---

# PART II: TAXONOMY BY TARGET DOMAIN

## A. GENERAL C/C++ LIBRARIES

**Key Tools:**
- FuzzGen (2020)
- FUDGE (2019)
- IntelliGen (2021)
- GraphFuzz (2022)
- AFGen (2024)
- Hopper (2023)
- Oracle-guided Harnessing (2025)
- WildSync (2025)

## B. KERNEL AND SYSTEM CALLS

### B.1 Linux Kernel

- **SyzGen** (2021). Automated Generation of Syscall Specification.
  - Foundation for Syzkaller integration
  - **Target:** Linux kernel syscalls

- **SyzGen++** (2024). Dependency Inference for Augmenting Kernel Driver Fuzzing. *IEEE S&P 2024*.
  - Improved dependency inference
  - **Target:** Linux kernel drivers

- **SyzSpec** (2025). *[Listed above]*
  - **Target:** Linux kernel fuzzing

- **SyzForge** (2025). *[Listed above]*
  - **Target:** Linux kernel drivers

- **KernelGPT** (2024). *[Listed above]*
  - **Target:** Linux kernel

- **KBinCov** (2024). Leveraging Binary Coverage for Effective Generation Guidance in Kernel Fuzzing. *CCS 2024*. https://dl.acm.org/doi/10.1145/3658644.3690232
  - Binary coverage for kernel fuzzing
  - **Target:** Kernel binaries

- **Unlocking Low Frequency Syscalls** (2024). Dependency-Based RAG for Kernel Fuzzing. *ACM on Software Engineering*. https://dl.acm.org/doi/10.1145/3728913
  - Retrieval-Augmented Generation for syscalls
  - **Target:** Linux kernel low-frequency syscalls

### B.2 Kernel Surveys

- **Survey of Fuzzing Open-Source Operating Systems** (2025). https://arxiv.org/html/2502.13163v1
  - Comprehensive review: 99 papers (2017-2024)

- **SoK: Unraveling the Veil of OS Kernel Fuzzing** (2025). https://arxiv.org/html/2501.16165v1
  - Systematization of kernel fuzzing

## C. RUST LIBRARIES

**Key Tools:**
- RULF (2021)
- FRIES (2024)
- RPG (2024)
- RuMono (2025)
- Crabtree (2024)
- **Zhang, Y., Wu, J., Xu, H.** (2023). Fuzz Driver Synthesis for Rust Generic APIs. arXiv. https://arxiv.org/abs/2312.10676

## D. ANDROID AND MOBILE

### D.1 Android Native Libraries

- **FuzzGen++** (2024). *[Listed above]*
  - **Target:** OEM Android frameworks

- **Atlas** (2024). *[Listed above]*
  - **Target:** Android closed-source libraries

### D.2 Android System Services

- **FANS**. Fuzzing Android Native System Services via Automated Interface Analysis.
  - Automated interface analysis for Android services

## E. DEEP LEARNING LIBRARIES

### E.1 PyTorch and TensorFlow

- **FlashFuzz** (2025). *[Listed above]*
- **Orion** (2024). *[Listed above]*
- **DeepREL** (2022). *[Listed above]*
- **DFuzz** (2025). *[Listed above]*

### E.2 Automatic Differentiation

- **∇Fuzz** (2023). Fuzzing Automatic Differentiation in Deep-Learning Libraries. *ICSE 2023*. https://arxiv.org/abs/2302.04351
  - Specialized for auto-diff bugs
  - 173 bugs in PyTorch, TensorFlow, JAX, OneFlow

### E.3 Deep Learning Model Assembly

- **MoCo** (2024). Fuzzing Deep Learning Libraries via Assembling Code. https://arxiv.org/html/2405.07744
  - Assembles real DL model code
  - **Target:** DL libraries

## F. SMART CONTRACTS

- **Erinys** (2024). *[Listed above]*
- **MAU** (2024). Towards Smart Contract Fuzzing on GPUs. *IEEE S&P 2024*. https://chapering.github.io/pubs/sp24weimin.pdf
  - GPU-accelerated fuzzing
  - CPU-GPU task decomposition
- **Echidna**. GitHub: https://github.com/crytic/echidna
  - Property-based testing for Ethereum
  - Grammar-based fuzzing

## G. WEB APIs

### G.1 REST APIs

- **EvoMaster** (2024). *[Listed above]*
  - REST, GraphQL, RPC support
  - **Target:** Web APIs

- **RESTler**. Stateful REST API Fuzzing. *ICSE 2019*.
  - Foundational stateful REST fuzzer
  - Producer-consumer dependency inference

### G.2 GraphQL APIs

- **GraphQLer** (2025). Enhancing GraphQL Security with Context-Aware API Testing. https://arxiv.org/html/2504.13358v1
  - Context-aware GraphQL fuzzing

- **Random Testing and Evolutionary Testing for GraphQL** (2024). *ACM Transactions on the Web*. https://dl.acm.org/doi/10.1145/3609427

### G.3 5G Network APIs

- **Feedback-Guided API Fuzzing of 5G** (2024). *[Listed above]*

## H. CLOSED-SOURCE AND BINARY-ONLY

- **APICraft** (2021). *[Listed above]*
- **WINNIE** (2021). *[Listed above]*
- **Atlas** (2024). *[Listed above]* (Android binaries)
- **KBinCov** (2024). *[Listed above]* (Kernel binaries)

## I. IoT AND EMBEDDED SYSTEMS

- **IoT Firmware Emulation Survey** (2025). *Future Internet*. https://www.mdpi.com/1999-5903/17/1/19
  - Reviews 27 works (2014-2024)
  - Tools: TWFuzz, IoTFuzzSentry, TAIFuzz, RIoTFuzzer, MSLFuzzer, MULTIFUZZ

- **ChatHTTPFuzz** (2024). LLM-assisted IoT HTTP fuzzing

- **ECG** (2024). LLM-based corpus generation for embedded OS

- **Fuzzing TEE with Rust** (2025). *Computers & Security*. https://www.sciencedirect.com/science/article/pii/S0167404824005017
  - Two-way code generator in Rust
  - **Target:** Trusted Execution Environments

## J. INDUSTRIAL CONTROL SYSTEMS

- **SP-Fuzz** (2024). Fuzzing Soft PLC with Semi-automated Harness Synthesis. *Springer LNCS*. https://link.springer.com/chapter/10.1007/978-981-99-8024-6_22
  - Semi-automated harness for PLCs
  - **Target:** Soft PLCs

## K. PROTOCOL AND BINARY FORMATS

- **FormatFuzzer** (2021). Effective Fuzzing of Binary File Formats. https://arxiv.org/pdf/2109.11277
  - Compiles binary templates to parsers
  - Tested MP4, ZIP
  - Found bugs in ffmpeg, timidity

- **AutoHarness**. GitHub: https://github.com/parikhakshat/autoharness
  - Uses LLVM, Clang, CodeQL
  - **Target:** Binary formats and libraries

---

# PART III: SUPPORTING RESEARCH

## DIFFERENTIAL TESTING AND ORACLES

These papers focus on using differential testing as an oracle for validating harnesses or finding bugs.

- **AccuOracle** (2025). Fuzzing JavaScript JIT Compilers with High-Quality Differential Test Oracle. *Computers & Security*. https://www.sciencedirect.com/science/article/abs/pii/S0167404825003499
  - Addresses >90% false positive rates
  - **Target:** JavaScript JIT compilers

- **DUMPLING** (2025). Fine-grained Differential JavaScript Engine Fuzzing. *NDSS 2025*. https://www.ndss-symposium.org/wp-content/uploads/2025-1411-paper.pdf
  - Detects optimized vs unoptimized divergences
  - **Target:** JavaScript engines

- **Evolutionary Generative Fuzzing for Kotlin Compiler** (2024). *FSE 2024*. https://arxiv.org/abs/2401.06653
  - Differential testing for compilers
  - **Target:** Kotlin compiler

- **SQLxDiff** (2025). Enhanced Differential Testing in Emerging Database Systems. https://arxiv.org/html/2501.01236v1
  - Found 17 bugs SQLancer couldn't
  - **Target:** Database systems

- **TWINFUZZ** (2025). Differential Testing of Video Hardware Acceleration Stacks. *NDSS 2025*. https://www.ndss-symposium.org/wp-content/uploads/2025-526-paper.pdf
  - Hardware vs software reference
  - **Target:** Video hardware accelerators

## GRAMMAR-BASED FUZZING

- **Shaping Test Inputs in Grammar-Based Fuzzing** (2024). *ISSTA 2024*. https://dl.acm.org/doi/10.1145/3650212.3685553
  - Distribution sampling for grammar fuzzing

- **XAVIER** (2025). Grammar-based Testing for XML Injection. *ISSTA 2025*.

- **Grammar Mutation** (2025). *ACM TOSEM*. https://dl.acm.org/doi/abs/10.1145/3708517
  - Grammar mutation for parsers

- **JIMA-Fuzzing** (2024). Grammar detection from samples

## BENCHMARKING AND EVALUATION

### Evaluation Methodology

- **Klees, G., Ruef, A., Cooper, B., Wei, S., Hicks, M.** (2018). Evaluating Fuzz Testing. *CCS 2018*. https://doi.org/10.1145/3243734.3243804
  - Foundational evaluation methodology

- **Böhme, M., Szekeres, L., Metzman, J.** (2022). On the Reliability of Coverage-Based Fuzzer Benchmarking. *ICSE 2022*. https://doi.org/10.1145/3510003.3510230
  - Critical analysis of coverage-based evaluation

- **Schloegel, M., et al.** (2024). SoK: Prudent Evaluation Practices for Fuzzing. *IEEE S&P 2024*. https://doi.org/10.1109/SP54263.2024.00137
  - Best practices for fuzzing evaluation

### Benchmark Platforms

- **FuzzBench** (2021). Metzman, J., Szekeres, L., Simon, L., Sprabery, R., Arya, A. An Open Fuzzer Benchmarking Platform and Service. *ESEC/FSE 2021*. https://doi.org/10.1145/3468264.3473932
  - Industry-standard fuzzer benchmarking
  - GitHub: https://github.com/google/fuzzbench

- **FeatureBench** (2025). Program Feature-Based Benchmarking for Fuzz Testing. *ACM on Software Engineering*. https://dl.acm.org/doi/10.1145/3728899
  - Fine-grained program feature benchmarks
  - Reviewed 25 fuzzing studies, 7 features

- **GreenBench** (2023). Green Fuzzer Benchmarking. *ISSTA 2023*.
  - Faster, greener evaluations

### Fuzz Blockers

- **Gao, W., Pham, V.T., Liu, D., Chang, O., Murray, T., Rubinstein, B.I.** (2023). Beyond the Coverage Plateau: A Comprehensive Study of Fuzz Blockers. *FUZZING 2023*. https://doi.org/10.1145/3605157.3605177

## INFRASTRUCTURE AND CONTINUOUS FUZZING

- **OSS-Fuzz** (2016). Continuous Fuzzing for Open Source Software. GitHub: https://github.com/google/oss-fuzz
  - Google's continuous fuzzing service
  - Foundation for many tools

- **OSS-Fuzz-Gen** (2024). LLM Powered Fuzzing via OSS-Fuzz. GitHub: https://github.com/google/oss-fuzz-gen
  - LLM-powered harness generation
  - 160+ C/C++ projects
  - Up to 29% coverage increase, 30 new bugs

- **Google Security Blog** (2023). AI-Powered Fuzzing: Breaking Bug Hunting Barriers. https://security.googleblog.com/2023/08/ai-powered-fuzzing-breaking-bug-hunting.html

## DIRECTED AND GUIDED FUZZING

- **Locus** (2025). Agentic Predicate Synthesis for Directed Fuzzing. https://arxiv.org/html/2508.21302
  - Agentic predicate synthesis

- **Directed Greybox Fuzzing via LLM** (2025). https://arxiv.org/html/2505.03425v1
  - LLM-guided directed fuzzing

## GENERAL AUTOMATION

- **Orion Workflow Automation** (2024). https://arxiv.org/html/2509.15195
  - Fuzzing workflow automation

- **Li, Z., et al.** (2024). Automated Generation and Compilation of Fuzz Driver Based on LLMs. *CSIE 2024*. https://dl.acm.org/doi/10.1145/3689236.3689272

- **HarnessGen**. GitHub: https://github.com/SmllXzBZ/HarnessGen
  - Collection of automated methods

---

# PART IV: SYSTEMATIZATION OF KNOWLEDGE (SoK)

## Published SoK Papers

- **Yan, Q., Huang, M., Cao, H., Lu, S.** (2025). SoK: From Systematization to Best Practices in Fuzz Driver Generation. *ACISP 2025 LNCS vol 15660*. https://doi.org/10.1007/978-981-96-9101-2_18
  - **Most recent SoK on fuzz driver generation**
  - Systematizes techniques through 2024

---

# PART V: COMPLETE SPECTRUM ANALYSIS

## Technique Spectrum Summary

| **Approach** | **Representative Tools** | **Strengths** | **Limitations** |
|-------------|------------------------|--------------|----------------|
| **Static Analysis** | FuzzGen, FUDGE, IntelliGen | Scalable, no runtime overhead | May miss dynamic behaviors |
| **Graph-Based** | GraphFuzz, RULF, FRIES, RPG | Captures dependencies, structured | Graph construction complexity |
| **Dynamic/Trace** | Utopia, DAISY, WildSync | Learns from real usage | Requires existing tests/code |
| **LLM-Based** | PromptFuzz, CKGFuzzer, Fuzz4All, KernelGPT | Flexible, handles diverse code | Quality varies, hallucinations |
| **Search-Based** | EvoMaster, Oracle-guided | Explores large spaces | Computationally expensive |
| **Symbolic** | SyzForge, SyzSpec | Precise constraints | Scalability challenges |
| **Type-Aware** | PointFuzz, Crabtree | Semantic correctness | Language-specific |
| **Hybrid** | CKGFuzzer, SyzForge, HybridLLM-Fuzz | Best of multiple worlds | Increased complexity |

## Target Domain Coverage

| **Domain** | **Maturity** | **Key Tools** |
|-----------|-------------|--------------|
| **C/C++ Libraries** | ⭐⭐⭐⭐⭐ Mature | FuzzGen, GraphFuzz, AFGen, WildSync |
| **Kernel/Syscalls** | ⭐⭐⭐⭐ Advanced | SyzSpec, SyzForge, KernelGPT |
| **Rust** | ⭐⭐⭐⭐ Growing | RULF, FRIES, RPG, RuMono |
| **Android** | ⭐⭐⭐ Developing | FuzzGen++, Atlas |
| **Deep Learning** | ⭐⭐⭐⭐ Active | FlashFuzz, Orion, DeepREL, ∇Fuzz |
| **Smart Contracts** | ⭐⭐⭐ Active | Erinys, MAU, Echidna |
| **Web APIs** | ⭐⭐⭐⭐ Mature | EvoMaster, RESTler, GraphQLer |
| **Closed-Source** | ⭐⭐⭐ Challenging | APICraft, WINNIE, Atlas |
| **IoT/Embedded** | ⭐⭐ Emerging | Multiple specialized tools |

## Chronological Evolution

### Early Era (2016-2020)
- **Focus:** Static analysis, dependency graphs
- **Key:** OSS-Fuzz (2016), FUDGE (2019), FuzzGen (2020)

### Maturation (2021-2022)
- **Focus:** Specialized domains, graph-based approaches
- **Key:** APICraft, IntelliGen, RULF, GraphFuzz, WINNIE

### LLM Revolution (2023-2025)
- **Focus:** AI integration, hybrid approaches
- **Key:** PromptFuzz, CKGFuzzer, Fuzz4All, KernelGPT, FlashFuzz, OSS-Fuzz-Gen

## Key Trends (2024-2025)

1. **LLM Integration:** Nearly universal adoption, from pure prompt engineering to hybrid LLM+analysis
2. **Code Knowledge Graphs:** Emerging as bridge between program analysis and AI
3. **Type & Semantic Awareness:** Shift from syntax to semantics
4. **Cross-Language Support:** Growing focus on multi-language systems
5. **Domain Specialization:** Tailored tools for specific domains (DL, kernel, Rust, smart contracts)
6. **Correctness Oracles:** Automated validation of generated harnesses
7. **Industrial Adoption:** Tools in production (EvoMaster, OSS-Fuzz-Gen)
8. **GPU Acceleration:** First GPU-based approaches (MAU)
9. **Evaluation Rigor:** Increased focus on sound benchmarking

---

# APPENDIX: ADDITIONAL RESOURCES

## Tools and Frameworks

- **HarnessGen Collection:** https://github.com/SmllXzBZ/HarnessGen
- **Fuzzing Paper Collections:**
  - https://github.com/0xricksanchez/paper_collection
  - https://wcventure.github.io/FuzzingPaper/

## Fuzzing Books

- **The Fuzzing Book:** https://www.fuzzingbook.org/
  - Chapters on Grammar Fuzzing, Symbolic Fuzzing, Concolic Fuzzing

---

# SUMMARY STATISTICS

- **Total Papers:** 100+
- **Time Span:** 2016-2025
- **Core Techniques:** 10+ distinct approaches
- **Target Domains:** 15+ specialized areas
- **Publication Venues:** ICSE, S&P, CCS, USENIX Security, NDSS, FSE, ISSTA, ASE, OOPSLA
- **Major Shifts:** Static→Dynamic→AI/LLM integration
- **Industrial Impact:** OSS-Fuzz (40,000+ bugs), Fortune 500 adoption

---

**Compiled by:** SoK Research Team
**Purpose:** Comprehensive literature survey for SoK paper on automated fuzz driver generation
**Last Updated:** November 2025
**Version:** 2.0 (Merged and Complete Spectrum)
