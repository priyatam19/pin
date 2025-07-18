import re
import sys

def fd_to_nfd(fd_c_path, nfd_c_path):
    with open(fd_c_path, 'r') as f:
        code = f.read()
    
    # A. Signature and Entry Point
    # 1. Replace 'int main(...)' (any variant) with NFD function signature
    code = re.sub(
        r'int\s+main\s*\([^)]*\)\s*\{',
        'int NFD(const uint8_t *input, size_t input_len) {',
        code,
        flags=re.DOTALL
    )
    # 2. Remove all references to argc, argv, or input filename checks
    code = re.sub(
        r'if\s*\(\s*argc\s*[!<>=]=?\s*\d+\s*\)\s*\{[^}]*\}',
        '',
        code,
        flags=re.DOTALL
    )
    code = re.sub(
        r'char\s+\*\w+\s*=\s*argv\s*\[.*?\]\s*;',
        '',
        code
    )
    code = re.sub(r'\bargc\b', '', code)
    code = re.sub(r'\bargv\b', '', code)

    # --- B. Input Handling ---

    # 1. Remove 'char input[...]' buffer declaration(s)
    code = re.sub(r'char\s+input\s*\[[^\]]*\]\s*;\s*', '', code)

    # 2. Remove fread/fgets blocks
    code = re.sub(
        r'size_t\s+\w+\s*=\s*fread\s*\([^)]+\);\s*'
        r'if\s*\([^)]+\)\s*{[^}]*}',
        '// Input is now provided as function arguments.\n',
        code,
        flags=re.DOTALL
    )
    code = re.sub(
        r'if\s*\(\s*fgets\s*\([^)]+\)\s*==\s*NULL\s*\)\s*{[^}]*}',
        '// Input is now provided as function arguments.\n',
        code,
        flags=re.DOTALL
    )

    # 3. Remove "No input from AFL++" and related return/exit
    code = re.sub(
        r'printf\("No input from AFL\+\+"\);\s*delete db;\s*return 1;\s*',
        '',
        code
    )

    # 4. Replace 'input_size' with 'input_len'
    code = re.sub(r'\binput_size\b', 'input_len', code)

    # 5. Fix process_filter_route to cast input as needed (example)
    code = re.sub(
        r'process_filter_route\(([^,]+),\s*input\s*,\s*input_len\)',
        r'process_filter_route(\1, (const char*)input, input_len)',
        code
    )

    # # 2. Remove stdin reading code
    # # Covers common fread, fgets, and buffer allocation patterns
    # code = re.sub(
    #     r'(char\s+input\s*\[\s*\d+\s*\]\s*;\s*)?(size_t\s+)?input_size\s*=\s*fread\s*\(\s*input\s*,\s*1\s*,\s*[^,]+,\s*stdin\s*\)\s*;\s*'
    #     r'if\s*\(\s*input_size\s*<=\s*0\s*\)\s*\{[^\}]*\}',
    #     '// Input is now provided as function arguments.\n',
    #     code,
    #     flags=re.DOTALL
    # )
    # # Remove potential FILE *f and fclose
    # code = re.sub(r'FILE\s*\*f\s*=\s*fopen\(.*?;\s*fseek\(.*?;\s*long\s+len\s*=\s*ftell\(.*?;\s*rewind\(.*?;\s*uint8_t\s*\*buf\s*=\s*malloc\(len\);.*?fread\(.*?;\s*fclose\(.*?;', '', code, flags=re.DOTALL)
    # # Remove printf("No input from AFL++") blocks
    # code = re.sub(r'printf\("No input from AFL\+\+"\);\s*delete db;\s*return 1;\s*', '', code)
    # # Remove any check on argc/argv for input file
    # code = re.sub(r'if\s*\(argc\s*!=\s*\d+\)\s*\{[^}]*\}', '', code, flags=re.DOTALL)

    # # 3. Replace any use of input_size with input_len
    # code = re.sub(r'\binput_size\b', 'input_len', code)
    # # 4. Replace any use of the old buffer variable (if needed)
    # # Here we assume 'input' is the buffer, adjust if your original uses different name

    # # 5. Replace call to process function if it uses input/input_size
    # # For process_filter_route(db, input, input_size) => process_filter_route(db, (const char*)input, input_len)
    # code = re.sub(
    #     r'process_filter_route\(([^,]+),\s*input\s*,\s*input_len\)',
    #     r'process_filter_route(\1, (const char*)input, input_len)',
    #     code
    # )
    # code = re.sub(
    #     r'process_filter_route\(([^,]+),\s*input\s*,\s*input_size\)',
    #     r'process_filter_route(\1, (const char*)input, input_len)',
    #     code
    # )

    # --- C. Looping, memory cleanups, and further corner cases ---

    # 1. Remove or adjust __AFL_LOOP reading from stdin
    #   - If loop reads stdin, refactor so it only processes 'input' and 'input_len'
    code = re.sub(
        r'while\s*\(__AFL_LOOP\([^)]+\)\)\s*\{[^\{]*ssize_t\s+\w+\s*=\s*fread\s*\([^)]+\);\s*[^\}]*}',
        'while (__AFL_LOOP(10000)) {\n    // Process input buffer here using input/input_len\n}\n',
        code,
        flags=re.DOTALL
    )
    # 2. Remove or move free_entries()/cleanup to right place
    # (You may need to do this by hand if the code is complex, but in most cases, just keep cleanup after processing input.)

    # 3. Reset global state at end of loop, if needed
    # E.g., ensure any "current_id = 1;" or "head = NULL;" is *after* the call to cleanup in the loop

    # 4. Ensure input buffer is null-terminated for string processing
    # ---- Null-Termination Patch ----
    # Find the NFD function signature to locate where to insert null-termination logic
    nfd_func_pattern = re.compile(
        r'(int\s+NFD\s*\(\s*const\s+uint8_t\s*\*\s*input\s*,\s*size_t\s*input_len\s*\)\s*\{)', re.MULTILINE
    )
    match = nfd_func_pattern.search(code)
    if match:
        insert_at = match.end()
        nullterm_snippet = '''
        // [fd_to_nfd] Make a safe, null-terminated buffer for string ops
        size_t safe_len = (input_len < BUFFER_SIZE - 1) ? input_len : (BUFFER_SIZE - 1);
        char safe_buf[BUFFER_SIZE];
        memcpy(safe_buf, input, safe_len);
        safe_buf[safe_len] = '\\0';
        '''
        code = code[:insert_at] + nullterm_snippet + code[insert_at:]
        # Optionally: replace all uses of `input` in string-processing with `safe_buf`
        code = re.sub(r'\binput\b', 'safe_buf', code)

    # ---- State Cleanup Patch ----
    # Move or ensure cleanup (free_entries and global resets) at the end of NFD
    cleanup_snippet = '''
        // [fd_to_nfd] Cleanup persistent state after each call
        free_entries();
        current_id = 1;
    '''
    # Place before final return in NFD
    code = re.sub(r'return\s+0\s*;', cleanup_snippet + '\n    return 0;', code)
    # (Manually check, or add after buffer usage if the code needs it.)

    # 5. Remove exit()/return for CLI errors (but keep for real program failures)
    code = re.sub(r'exit\s*\(\d+\)\s*;', '', code)
    code = re.sub(r'return\s+\d+\s*;\s*//\s*CLI error', '', code)
    
    # 6. Remove or adjust return/exit for input checks (caller will decide validity)

    # 7. (Optional) Add #include <stdint.h> if not present
    if "#include <stdint.h>" not in code:
        code = "#include <stdint.h>\n" + code

    # 8. Add a comment at the top for provenance
    code = "// --- Auto-generated by fd_to_nfd.py: Normalized Fuzz Driver ---\n" + code

    with open(nfd_c_path, 'w') as f:
        f.write(code)
    print(f"[+] Generated normalized fuzz driver: {nfd_c_path}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 fd_to_nfd.py <input_fd.c> <output_nfd.c>")
        sys.exit(1)
    fd_to_nfd(sys.argv[1], sys.argv[2])