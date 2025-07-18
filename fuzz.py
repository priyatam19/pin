#!/usr/bin/env python3
import os
import subprocess
import openai
import json
import re

# Configuration
API_KEY = os.getenv("OPENAI_API_KEY")
if not API_KEY:
    print("Error: OPENAI_API_KEY environment variable not set.")
    exit(1)
CJSON_SRC = "cJSON/cJSON.c"
CJSON_HEADER = "cJSON/cJSON.h"
# CJSON_SRC = "lib/cJSON/cJSON.c"
# CJSON_HEADER = "lib/cJSON/cJSON.h"  # Path to cJSON header file
FUZZ_DRIVER_PATH = "./fuzz_driver.c"
SEED_DIR = "./afl_in"
OUTPUT_DIR = "./afl_out3"
BINARY_PATH = "./instrumented_binary"
# MAIN_FUNCTION_FILE = "app/main.c"  # File containing the main function code
MAIN_FUNCTION_FILE = "cJSON/test.c"

# Vulnerability description
VULNERABILITY_DESCRIPTION = r"""
CVE-cjson-002
The cJSON library version 1.7.99 (artificially modified for the E-BOSS program) contains a vulnerability in cJSON.c inside the static cJSON_bool replace_item_in_object(cJSON *object, const char *string, cJSON *replacement, cJSON_bool case_sensitive) function. When the function is called with a cJSON *object that has an object->type that equates to cJSON_String (16), a double free occurs resulting in a crash.
"""

# Initialize OpenAI client
openai.api_key = API_KEY

def read_main_function(file_path):
    """Read the main function code from the specified file."""
    try:
        with open(file_path, 'r') as f:
            code = f.read()
        return code
    except FileNotFoundError:
        print(f"Error: {file_path} not found.")
        exit(1)
    except Exception as e:
        print(f"Error reading main function file: {e}")
        exit(1)

def construct_fuzzing_harness_prompt(main_function_code, vuln_description):
    """Construct a generic prompt for the LLM to generate a C fuzzing harness for AFL++."""
    prompt = (
        r"You are an expert in C programming and fuzzing with AFL++. Below is the C code for the target program's main function:\n\n" +
        r"```c\n" + main_function_code + r"\n```\n\n" +
        r"Below is the vulnerability description for the target application:\n\n" +
        r"```text\n" + vuln_description + r"\n```\n\n" +
        r"Your task is to generate an effective C fuzzing harness for the provided main function, suitable for use with AFL++, targeting the described vulnerability. The harness must be complete, valid C code and must not be empty.\n\n" +
        r"**Instructions:**\n\n" +
        r"1. **Analyze Input Mechanism & Pre-processing:**\n" +
        r"   * Examine the `main` function to identify how it receives external input (e.g., `fgets`/`read` from `stdin`, `read`/`fread` from files, `getopt`/`argv` for arguments, `accept`/`recv` for sockets).\n" +
        r"   * Note any loops processing input data (e.g., reading lines, parsing arguments, handling socket chunks).\n" +
        r"   * Identify pre-processing steps applied to the input (e.g., null termination, string splitting with `strtok`, type conversion with `atoi`).\n" +
        r"   * If the input mechanism is unclear, assume a simple input mechanism like reading a buffer from `stdin` with `fread`.\n\n" +
        r"2. **Identify Processing Target(s):**\n" +
        r"   * Determine the primary function call(s) or code block(s) within `main` where input data is processed. Note the expected data format (e.g., buffer, lines, parsed arguments).\n\n" +
        r"3. **Incorporate Vulnerability Targeting:**\n" +
        r"   * Use the vulnerability description to craft inputs that maximize the likelihood of triggering the vulnerability (e.g., malformed data, specific structures).\n" +
        r"   * Focus on input patterns relevant to the vulnerability without assuming library details unless explicitly mentioned in `main` or the description.\n\n" +
        r"   * Take into consideration the description of the vulnerability too while crafting the fuzz driver, i.e. the fuzz driver should be able to replicate the scenario which might trigger the vulnerability "
        r"4. **Generate Harness Code:**\n" +
        r"   * Create a complete C `main` function harness compatible with AFL++.\n" +
        r"   * Read a single test case from `stdin` (provided by AFL++).\n" +
        r"   * Simulate the input delivery and pre-processing steps from Step 1, converting the `stdin` test case into the format expected by the target in Step 2, while targeting the vulnerability from Step 3.\n" +
        r"   * For any functions defined in `main.c` (e.g., helper functions), include their full definitions (signature and body) directly in the harness and use them as needed.\n" +
        r"   * Implement AFL++ persistent mode (`__AFL_LOOP`, `__AFL_INIT`) if feasible, unless complex state dependencies prevent it.\n" +
        r"   * Include necessary headers exactly as they appear in `main.c` (e.g., same path, casing, and quotation style). Do not modify or guess header paths.\n" +
        r"   * Add error handling for critical operations (e.g., check `malloc`, `fread` returns). Return early for invalid inputs to focus on the vulnerability.\n" +
        r"   * Validate input data to prevent crashes (e.g., check buffer sizes, non-null pointers).\n" +
        r"   * Free all allocated memory to prevent leaks, especially in persistent mode.\n" +
        r"   * Handle edge cases (e.g., empty or oversized inputs) gracefully.\n\n" +
        r"5. **State Handling:**\n" +
        r"   * If `main` has persistent state (e.g., via loops or global variables), include a comment in the harness where state reset would occur in persistent mode (e.g., `/* Reset state here */`).\n" +
        r"   * Ensure resources (e.g., memory, file descriptors) are cleaned up across iterations.\n\n" +
        r"6. **Output:**\n" +
        r"   * Output **only** complete, valid C code for the harness, starting with `#include` directives and defining `int main` with a proper signature.\n" +
        r"   * Ensure the code is syntactically correct, compilable, and free of undefined behavior (e.g., no missing declarations).\n" +
        r"   * The output must not be empty. If unable to generate a complex harness, produce a minimal valid harness that reads from `stdin` and calls a processing function.\n" +
        r"   * Do **not** include comments, explanations, or markdown outside the C code.\n\n" +
        r"**Focus on replicating the I/O handling and pre-processing from `main`, targeting the vulnerability without assuming library details unless explicitly used in `main` or the description.**"
    )
    return prompt

def construct_seed_corpus_prompt(main_function_code, vuln_description):
    """Construct a generic prompt for the LLM to generate a seed corpus for AFL++."""
    prompt = (
        r"You are an expert in C programming and fuzzing with AFL++. Below is the C code for the target program:\n\n" +
        r"```c\n" + main_function_code + r"\n```\n\n" +
        r"Below is the vulnerability description for the target application:\n\n" +
        r"```text\n" + vuln_description + r"\n```\n\n" +
        r"Your task is to generate initial seed inputs for AFL++ fuzzing, tailored to the input mechanism and processing logic of the provided code, and designed to maximize the likelihood of triggering the described vulnerability. Do not make assumptions about specific libraries or their functions unless explicitly used in the code.\n\n" +
        r"**Instructions:**\n\n" +
        r"1. **Analyze Input Mechanism and Format:**\n" +
        r"   * If the code contains a `main` function, examine it to identify how it receives external input (e.g., `fgets`/`read` from `stdin`, `read`/`fread` from files, `getopt`/`argv` for arguments, `accept`/`recv` for sockets).\n" +
        r"   * If no `main` function exists, identify the primary function intended for processing input. Analyze its parameters to determine the input mechanism (e.g., buffer, custom I/O struct) and expected data format (e.g., text, binary, structured).\n" +
        r"   * Note that the fuzz driver will read input directly from `stdin` using `fread` for binary/stream data or `fgets` for text, so seeds must match the primary function’s expected format, adjusted for direct `stdin` input.\n" +
        r"   * Identify any pre-processing applied to the input (e.g., null termination, string splitting, type conversion) and any constraints (e.g., buffer sizes, expected syntax).\n" +
        r"   * Determine the expected input format (e.g., null-terminated text, binary data, structured data like key-value pairs or JSON).\n\n" +
        r"2. **Incorporate Vulnerability Targeting:**\n" +
        r"   * Use the vulnerability description to guide seed generation. Focus on generating inputs that align with the conditions or patterns described (e.g., malformed data, oversized metadata, invalid syntax, or boundary values).\n" +
        r"   * Include inputs that test both valid cases (to exercise normal processing paths) and invalid/edge cases (to trigger the vulnerability).\n\n" +
        r"3. **Generate Seed Inputs:**\n" +
        r"   * Create at least 5 distinct seed inputs that match the expected data format identified in Step 1, tailored to trigger the vulnerability described in Step 2.\n" +
        r"   * Ensure diversity in seeds:\n" +
        r"     * **Valid Inputs**: Conform to the expected format (e.g., well-formed text, valid binary data, or structured data).\n" +
        r"     * **Malformed Inputs**: Include inputs with incorrect formats, special characters, invalid syntax, or oversized metadata that could trigger the vulnerability.\n" +
        r"     * **Edge Cases**: Include empty inputs, overly long inputs, inputs with boundary values, or inputs that exploit parsing limits (e.g., maximum buffer sizes or recursion depth).\n" +
        r"   * Represent seeds in the format expected by the primary function:\n" +
        r"     * For text inputs (e.g., strings, JSON), use plain strings, ensuring compatibility with null-termination if required.\n" +
        r"     * For binary inputs (e.g., compressed data), use hex-encoded strings to ensure binary compatibility with AFL++.\n" +
        r"     * For structured inputs (e.g., key-value pairs), use the appropriate delimited format (e.g., newline-separated or comma-separated).\n" +
        r"   * Separate multiple seeds with a newline (`\n`).\n" +
        r"   * Ensure seeds are concise yet effective, avoiding unnecessary complexity.\n\n" +
        r"4. **Output:**\n" +
        r"   * Output **only** the seed inputs in the format determined in Step 3 (e.g., plain strings for text, hex-encoded strings for binary, or structured data).\n" +
        r"   * For multiple seeds, separate them with a newline (`\n`).\n" +
        r"   * Do **not** include any additional text, comments, explanations, or markdown outside the seed inputs themselves.\n"
    )
    return prompt

def clean_llm_output(code):
    """Validate and clean LLM output, returning validity and cleaned code."""
    # Check for empty or whitespace-only output
    if not code or code.isspace():
        print(f"Warning: LLM returned empty or whitespace-only output for fuzz driver.")
        print(f"Raw LLM output: '{code}'")
        return False, ""
    # Remove markdown code block markers
    code = re.sub(r'^\s*```c\s*\n|\s*```\s*$', '', code, flags=re.MULTILINE)
    # Remove any leading/trailing non-C content
    code = code.strip()
    # Validate that the output is valid C code
    if not code.startswith(('#include', 'int main')):
        print(f"Warning: Generated fuzz driver is invalid (does not start with #include or int main).")
        print(f"Raw LLM output:\n{code}")
        return False, code
    return True, code

def call_llm(prompt, max_completion_tokens=3000):
    """Call the LLM API to generate content using o4-mini."""
    try:
        response = openai.ChatCompletion.create(
            model="gpt-4.1",
            messages=[
                {"role": "system", "content": "You are a C programming and fuzzing expert."},
                {"role": "user", "content": prompt}
            ],
            max_completion_tokens=max_completion_tokens
        )
        return response.choices[0].message.content.strip()
    except Exception as e:
        print(f"Error calling LLM API: {e}")
        return ""

def save_fuzz_driver(code, prompt):
    """Save the generated fuzz driver to a file, retrying until valid output."""
    retries = 0
    try:
        while True:
            is_valid, cleaned_code = clean_llm_output(code)
            if is_valid:
                try:
                    with open(FUZZ_DRIVER_PATH, 'w') as f:
                        f.write(cleaned_code)
                    print(f"Saved fuzz driver to {FUZZ_DRIVER_PATH}")
                    return cleaned_code
                except Exception as e:
                    print(f"Error saving fuzz driver: {e}")
                    exit(1)
            print(f"Fuzz driver generation attempt {retries + 1} failed. Retrying...")
            retries += 1
            # Re-query LLM with the same prompt
            code = call_llm(prompt, max_completion_tokens=3000)
    except KeyboardInterrupt:
        print("Fuzz driver generation stopped by user.")
        exit(1)

def create_seed_corpus(seeds_output, prompt):
    """Create a seed corpus from the LLM-generated output, retrying until valid."""
    retries = 0
    try:
        while True:
            # Debug: Print raw LLM output before parsing
            print(f"Raw seed corpus LLM output:\n{seeds_output}")
            # Check for empty or whitespace-only output
            if not seeds_output or seeds_output.isspace():
                print(f"Seed corpus generation attempt {retries + 1} failed: Empty or whitespace-only output. Retrying...")
                retries += 1
                seeds_output = call_llm(prompt, max_completion_tokens=2000)
                continue
            
            # Split seeds by newline (assuming delimiter-based output)
            seeds = [seed.strip() for seed in seeds_output.split('\n') if seed.strip()]
            
            if len(seeds) < 5:
                print(f"Seed corpus generation attempt {retries + 1} failed: Fewer than 5 seeds generated. Got {len(seeds)} seeds.")
                print(f"Raw LLM output:\n{seeds_output}")
                retries += 1
                seeds_output = call_llm(prompt, max_completion_tokens=2000)
                continue
            
            os.makedirs(SEED_DIR, exist_ok=True)
            for i, seed in enumerate(seeds):
                # Save seeds as raw input files without format assumptions
                seed_path = os.path.join(SEED_DIR, f"seed_{i}")
                with open(seed_path, 'w') as f:
                    f.write(seed)
                print(f"Created seed file: {seed_path}")
            
            print(f"Created {len(seeds)} seed files in {SEED_DIR}")
            break
    except KeyboardInterrupt:
        print("Seed corpus generation stopped by user.")
        exit(1)
    except Exception as e:
        print(f"Error creating seed corpus: {e}")
        print(f"Raw LLM output:\n{seeds_output}")
        exit(1)

def construct_fix_compilation_prompt(original_prompt, original_code, error_message):
    """Construct a prompt for the LLM to fix compilation errors in the fuzz driver."""
    prompt = (
        r"You are an expert in C programming and fuzzing with AFL++. You previously generated a C fuzzing harness, but it failed to compile with the following error:\n\n" +
        r"```text\n" + error_message + r"\n```\n\n" +
        r"Below is the original prompt used to generate the harness:\n\n" +
        r"```text\n" + original_prompt + r"\n```\n\n" +
        r"Below is the generated C code that failed to compile:\n\n" +
        r"```c\n" + original_code + r"\n```\n\n" +
        r"Your task is to analyze the compilation error and the original code, then generate a corrected version of the C fuzzing harness that resolves the compilation issues while strictly adhering to the requirements of the original prompt.\n\n" +
        r"**Instructions:**\n\n" +
        r"1. Carefully analyze the compilation error to identify the specific issues (e.g., incorrect header paths, missing declarations).\n" +
        r"2. Fix all compilation errors, ensuring the harness remains a valid C program compatible with AFL++ and targets the described vulnerability.\n" +
        r"3. Maintain the exact input mechanism, pre-processing steps, and vulnerability targeting as specified in the original prompt.\n" +
        r"4. For header includes:\n" +
        r"   * Use the exact header paths and casing as they appear in the original `main` function from the provided code.\n" +
        r"   * Do not guess or modify header paths .\n" +
        r"   * Assume headers are available via the compilation command’s include paths.\n" +
        r"5. Ensure all necessary headers are included to match the dependencies of the original code, without adding unnecessary includes.\n" +
        r"6. Output **only** the corrected, complete, valid C code for the fuzzing harness.\n" +
        r"7. The output must start with `#include` directives or `int main` and contain only valid C syntax.\n" +
        r"8. Do **not** include any additional text, comments, explanations, or markdown outside the C code itself.\n"
    )
    return prompt

def compile_fuzz_driver():
    """Compile the fuzz driver with afl-clang-fast and Address Sanitizer, returning error if compilation fails."""
    if not os.path.exists(CJSON_SRC):
        print(f"Error: {CJSON_SRC} not found.")
        exit(1)
    for var in ['AFL_TESTCASE_MAX_SIZE', 'AFL_MAX_INPUT']:
        if var in os.environ:
            del os.environ[var]
            print(f"Removed invalid AFL environment variable: {var}")
    os.environ["AFL_USE_ASAN"] = "1"  # Enable Address Sanitizer for AFL++
    compile_cmd = [
        "afl-clang-fast",
        FUZZ_DRIVER_PATH,
        CJSON_SRC,
        "-IcJSON",
        "-o", BINARY_PATH,
        "-Wall", "-Wextra", "-g",
        "-fsanitize=address"
    ]
    try:
        result = subprocess.run(compile_cmd, check=True, capture_output=True, text=True)
        print(f"Compiled fuzz driver to {BINARY_PATH} with Address Sanitizer")
        return True, ""
    except subprocess.CalledProcessError as e:
        error_message = f"Compilation failed with error:\n{e.stderr}"
        print(error_message)
        return False, error_message
    except FileNotFoundError:
        print("Error: afl-clang-fast not found. Ensure AFL++ is installed.")
        exit(1)

def run_afl():
    """Run AFL++ with the compiled fuzz driver."""
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    afl_cmd = [
        "afl-fuzz",
        "-i", SEED_DIR,
        "-o", OUTPUT_DIR,
        "-m", "none",
        "--", BINARY_PATH
    ]
    print(f"Starting AFL++ with command: {' '.join(afl_cmd)}")
    try:
        subprocess.run(afl_cmd)
    except subprocess.CalledProcessError as e:
        print(f"Error running AFL++: {e}")
        exit(1)
    except KeyboardInterrupt:
        print("AFL++ stopped by user.")

def main():
    # Read the main function code from main.c
    main_function_code = read_main_function(MAIN_FUNCTION_FILE)
    # Debug: Confirm content read
    print(f"Read {len(main_function_code)} characters from main.c")
    
    # Generate initial fuzz driver
    fuzz_driver_prompt = construct_fuzzing_harness_prompt(main_function_code, VULNERABILITY_DESCRIPTION)
    fuzz_driver_code = call_llm(fuzz_driver_prompt, max_completion_tokens=3000)
    cleaned_fuzz_driver_code = save_fuzz_driver(fuzz_driver_code, fuzz_driver_prompt)
    
    # Attempt compilation with retries until success or user interruption
    retries = 0
    current_code = cleaned_fuzz_driver_code
    current_prompt = fuzz_driver_prompt
    
    try:
        while True:
            success, error_message = compile_fuzz_driver()
            if success:
                break
            print(f"Compilation attempt {retries + 1} failed. Attempting to fix...")
            retries += 1
            # Generate a new prompt to fix the compilation error
            fix_prompt = construct_fix_compilation_prompt(current_prompt, current_code, error_message)
            # Call LLM to get fixed code
            fixed_code = call_llm(fix_prompt, max_completion_tokens=3000)
            current_code = save_fuzz_driver(fixed_code, fix_prompt)
            current_prompt = fix_prompt  # Update prompt for next iteration
    except KeyboardInterrupt:
        print("Compilation stopped by user.")
        exit(1)
    
    # Proceed with seed generation and fuzzing
    seed_corpus_prompt = construct_seed_corpus_prompt(main_function_code, VULNERABILITY_DESCRIPTION)
    seeds_output = call_llm(seed_corpus_prompt, max_completion_tokens=2000)
    create_seed_corpus(seeds_output, seed_corpus_prompt)
    run_afl()

if __name__ == "__main__":
    main()