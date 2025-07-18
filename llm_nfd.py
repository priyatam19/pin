#!/usr/bin/env python3
import openai
import os
import re

raw_key = os.getenv("OPENAI_API_KEY", "")
if not raw_key:
    print("Error: OPENAI_API_KEY environment variable not set.")
    exit(1)

# Strip out any non-ASCII (including U+2028) from your key:
clean_key = re.sub(r'[^\x00-\x7F]', '', raw_key).strip()
if not clean_key:
    print("Error: API key contained no ASCII characters after cleaning!")
    exit(1)

openai.api_key = clean_key

# openai.api_key = os.getenv("OPENAI_API_KEY")

# Prompt construction function
def construct_libfuzzer_conversion_prompt(afl_harness_code):
    prompt = (
        "You are an expert in fuzz testing, specifically with AFL++ and libFuzzer. "
        "Your task is to convert the following AFL++ fuzzing harness into an equivalent harness compatible with libFuzzer. "
        "The libFuzzer harness must follow this signature: \n\n"
        "```c\nint LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {\n    // your implementation here\n}\n```\n\n"
        "Below is the original AFL++ harness:\n\n"
        "```c\n" + afl_harness_code + "\n```\n\n"
        "**Code‐preservation instructions:**"
        "Do not remove or abstract away any global definitions or helper functions: `Entry` struct, `parser_version()`, `add_entry()`, `find_entry()`, `free_entries()`, and `process_command()`. If the original AFL harness showed “(Removed for brevity)”, re-insert the full body from the source file."
        "Ensure all required headers are present"
        "Preserve the guard `if (size == 0 || size >= BUFFER_SIZE) return 0;` exactly as written."
        "After your loop, always call `free_entries();` and `current_id = 1;`."
        "**Conversion instructions:**\n"
        "1. Change the `main` function into a `LLVMFuzzerTestOneInput` function with the required libFuzzer signature above.\n"
        "2. Replace the input handling (fread from stdin) with direct use of the `data` and `size` parameters.\n"
        "3. Ensure proper handling of byte buffers (uint8_t*) input, including null termination only if required by the target program.\n"
        "4. Remove AFL-specific macros such as `__AFL_LOOP` and `__AFL_INIT`.\n"
        "5. Maintain the logic for splitting and processing input commands (e.g., strtok_r for splitting on newlines).\n"
        "6. Keep the rest of the code logic intact, including error checks, memory allocation/deallocation, and resetting global state after processing each input.\n"
        "7. Ensure there is no undefined behavior and proper handling of edge cases (empty inputs, oversized inputs).\n"
        "8. Output **only** valid C code compatible with libFuzzer, without any comments or markdown outside the C code."
    )
    return prompt

# Call LLM
# def call_llm(prompt, model="gpt-4.1"):
#     # Ensure prompt contains only valid ASCII or UTF-8 characters
#     prompt = prompt.encode("utf-8", "replace").decode("utf-8")
    
#     response = openai.ChatCompletion.create(
#         model=model,
#         messages=[
#             {"role": "system", "content": "You are a C programming and fuzzing expert."},
#             {"role": "user", "content": prompt}
#         ],
#         max_tokens=3000
#     )
#     return response.choices[0].message.content.strip()
# def call_llm(prompt, model="gpt-4.1"):
#     prompt = prompt.replace('\u2028', ' ').replace('\u2029', ' ')
#     response = openai.ChatCompletion.create(
#         model=model,
#         messages=[
#             {"role": "system", "content": "You are a C programming and fuzzing expert."},
#             {"role": "user", "content": prompt}
#         ],
#         max_tokens=3000
#     )
#     return response.choices[0].message.content.strip()
# def call_llm(prompt, model="gpt-4.1"):
#     # Explicitly remove problematic Unicode line/paragraph separators
#     sanitized_prompt = prompt.replace('\u2028', ' ').replace('\u2029', ' ')
#     sanitized_prompt = sanitized_prompt.encode('ascii', 'ignore').decode('ascii')

#     response = openai.ChatCompletion.create(
#         model=model,
#         messages=[
#             {"role": "system", "content": "You are a C programming and fuzzing expert."},
#             {"role": "user", "content": sanitized_prompt}
#         ],
#         max_tokens=3000
#     )
#     return response.choices[0].message.content.strip()
def call_llm(prompt, model="gpt-4"):
    # Helper: collapse to pure ASCII (0–127), dropping everything else
    def to_ascii(s: str) -> str:
        return ''.join(ch for ch in s if 0 <= ord(ch) < 128)

    system_content = to_ascii("You are a C programming and fuzzing expert.")
    user_content   = to_ascii(prompt)

    # (Optional) inspect to make sure no weird chars remain:
    # print(repr(system_content))
    # print(repr(user_content))

    response = openai.ChatCompletion.create(
        model=model,
        messages=[
            {"role": "system", "content": system_content},
            {"role": "user",   "content": user_content},
        ],
        max_tokens=3000,
    )
    return response.choices[0].message.content.strip()

# Clean LLM output
def clean_llm_output(code):
    code = re.sub(r'^\s*```c\s*\n|\s*```\s*$', '', code, flags=re.MULTILINE).strip()
    return code

# Save output
def save_libfuzzer_harness(code, path):
    with open(path, 'w') as f:
        f.write(code)
    print(f"LibFuzzer harness saved to {path}")

# Main execution
def convert_afl_to_libfuzzer(afl_harness, output_path):
    prompt = construct_libfuzzer_conversion_prompt(afl_harness)
    raw_output = call_llm(prompt)
    cleaned_output = clean_llm_output(raw_output)
    save_libfuzzer_harness(cleaned_output, output_path)

# Usage example
if __name__ == "__main__":
    afl_harness_path = "./fuzz_driver_json.c"
    libfuzzer_output_path = "./libfuzzer_harness_json.c"

    # Load AFL harness code
    with open(afl_harness_path, 'r') as f:
        afl_harness_code = f.read()

    convert_afl_to_libfuzzer(afl_harness_code, libfuzzer_output_path)
