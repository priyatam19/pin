🚀 C Program Interface Normalization & Protobuf Wrapping Pipeline

Overview

This project provides a fully-automated pipeline to normalize the input interface of arbitrary C programs using Google Protocol Buffers (protobuf), enabling robust, schema-driven fuzzing and testing.

Key features:
	•	Automatically extracts input structs from C programs.
	•	Generates a matching .proto schema for input types (including nested/anonymous structs).
	•	Generates nanopb serialization/deserialization code for the C side.
	•	Auto-generates a main.c wrapper, deserializing protobuf input and calling your logic function.
	•	Supports both:
	•	Struct input (typedef struct { ... } MyStruct;)
	•	Single-field struct input (int, float, string) — calls your logic as P(x) or P(s).
	•	Fully automates the build and execution process for new C programs, with no manual editing needed!

⸻

Pipeline Usage

1. Prepare Your C Program
	•	Use a typedef struct for your input, e.g.:

typedef struct {
    int id;
    float score;
    char name[32];
} MyStruct;

int P(const MyStruct* input, const char* name_buf);


	•	OR for single-value input:

typedef struct { int x; } Input;
int P(int x);


	•	Place your logic in a function named P(...) (the function name can be parameterized).

2. Run the Pipeline
	•	Place your C file in the repo (e.g. myprog.c).
	•	Run:

./full_pipeline.sh


	•	This will:
	•	Strip #include lines for parsing.
	•	Auto-generate .proto file(s), main.c wrapper, and input serialization script.
	•	Generate nanopb C code for serialization/deserialization.
	•	Compile everything.
	•	Generate a sample input.bin and run the program.

3. Adding/Testing New Programs
	•	Just replace the input C file and rerun the pipeline.
	•	The wrapper, proto, build, and input scripts are all regenerated from scratch!

⸻

What’s Supported
	•	✅ Arbitrary nested/anonymous structs
	•	✅ Multiple primitive field types: int, float, double, arrays, strings (char arr[N])
	•	✅ Single-field struct inputs: string/int/float, with minimal wrapper logic
	•	✅ Automatic buffer/callback handling for string fields (using nanopb)
	•	✅ Full build pipeline: proto → wrapper → nanopb → compile → run

⸻

What’s Not Yet Fully Supported (Work in Progress)
	•	❌ C programs that do not use typedef struct for input (e.g. raw int P(int) or int P(const char* s)).
	•	Current workaround: wrap your function input in a struct typedef.
	•	❌ Multiple input arguments in P(...) that are not part of a single struct.
	•	❌ Support for pointers, enums, or unions in input struct.
	•	❌ C programs with global input variables (no input struct/function parameter).

⸻

Next Steps & Planned Enhancements
	•	Automatic support for function signatures with no struct
- Detects and handles int P(int) and int P(const char*) directly.
	•	Auto-generate .proto and wrapper for any C function signature, even if not using a struct typedef.
	•	Support more complex input types: pointers, arrays of structs, enums, unions.
	•	Smarter buffer length handling (auto-detect field lengths for string/arrays).
	•	Seamless integration with popular fuzzers (e.g., AFL, libFuzzer) for input coverage maximization.
	•	Better error handling and logging throughout the toolchain.
	•	Python-based end-to-end orchestrator for all steps (optional, for cross-platform/CI use).
	•	User-friendly CLI tool and config system for easy adoption.

⸻

How to Contribute
	•	Feel free to file issues or pull requests for:
	•	Edge cases not currently handled.
	•	Bug fixes or improvements.
	•	New feature suggestions.

⸻

References
	•	Google Protocol Buffers
	•	nanopb (Protocol Buffers for Embedded C)
	•	pycparser

⸻

License

MIT License (see LICENSE)

⸻

Questions or suggestions?
Contact Priyatam Annambhotla or open an issue!

⸻

Acknowledgements

Thanks to the SSRG/Virginia Tech research group and all open source contributors.

