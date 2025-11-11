#!/usr/bin/env bash
# Automatically extract a coreutils function to standalone file for PIN testing
#
# Usage: extract_coreutils_function.sh <program> <function_name>
# Example: extract_coreutils_function.sh basename remove_suffix

set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 <program> <function_name>"
  echo "Example: $0 basename remove_suffix"
  exit 1
fi

PROGRAM=$1
FUNCTION=$2

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PIN_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
COREUTILS_SRC="/home/priyatam/ws/coreutils/src"
OUTPUT_DIR="$PIN_ROOT/examples/coreutils"
OUTPUT_FILE="$OUTPUT_DIR/${PROGRAM}_${FUNCTION}.c"

# Check if source file exists
SRC_FILE="$COREUTILS_SRC/${PROGRAM}.c"
if [[ ! -f "$SRC_FILE" ]]; then
  echo "[ERROR] Source file not found: $SRC_FILE"
  exit 1
fi

echo "[*] Extracting $FUNCTION from $PROGRAM.c"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Extract function using Python script
python3 - "$SRC_FILE" "$FUNCTION" "$OUTPUT_FILE" <<'PYTHON_SCRIPT'
import sys
import re

def extract_function(source_file, function_name, output_file):
    """Extract a function from C source file."""
    with open(source_file, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # Find function definition
    # Match: [static] return_type function_name(params) { ... }
    pattern = rf'(?:^|\n)((?:static\s+)?[a-zA-Z_][a-zA-Z0-9_*\s]+\s+{re.escape(function_name)}\s*\([^)]*\)\s*)\{{'

    match = re.search(pattern, content, re.MULTILINE)
    if not match:
        print(f"[ERROR] Function '{function_name}' not found in {source_file}")
        sys.exit(1)

    func_start = match.start(1)
    brace_start = content.find('{', match.end(1))

    # Find matching closing brace
    brace_count = 1
    pos = brace_start + 1
    while pos < len(content) and brace_count > 0:
        if content[pos] == '{':
            brace_count += 1
        elif content[pos] == '}':
            brace_count -= 1
        pos += 1

    if brace_count != 0:
        print(f"[ERROR] Could not find matching braces for function")
        sys.exit(1)

    # Extract function text
    func_signature = content[func_start:brace_start].strip()
    func_body = content[brace_start:pos]

    # Remove 'static' keyword if present
    func_signature = func_signature.replace('static ', '')

    # Collect includes from original file
    includes = []
    for line in content[:func_start].split('\n'):
        if line.strip().startswith('#include'):
            # Filter out unnecessary includes
            if any(inc in line for inc in ['<string.h>', '<stdlib.h>', '<stdio.h>',
                                            '<stdbool.h>', '<stdint.h>', '<unistd.h>',
                                            '<ctype.h>', '<limits.h>', '<errno.h>']):
                if line not in includes:
                    includes.append(line)

    # Write extracted function
    with open(output_file, 'w') as f:
        f.write(f"/* Extracted from GNU coreutils {sys.argv[1].split('/')[-1]} */\n")
        f.write(f"/* Function: {function_name} */\n\n")

        # Add minimal includes
        if not includes:
            f.write("#include <string.h>\n")
            f.write("#include <stdio.h>\n")
            f.write("#include <stdlib.h>\n")
        else:
            for inc in includes:
                f.write(inc + '\n')

        f.write("\n")
        f.write(func_signature)
        f.write(func_body)
        f.write("\n")

    print(f"[+] Extracted to: {output_file}")
    return True

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print("Usage: extract_function.py <source> <function> <output>")
        sys.exit(1)

    extract_function(sys.argv[1], sys.argv[2], sys.argv[3])
PYTHON_SCRIPT

# Check if extraction succeeded
if [[ ! -f "$OUTPUT_FILE" ]]; then
  echo "[ERROR] Extraction failed"
  exit 1
fi

# Test compilation
echo "[*] Testing compilation..."
if gcc -c -I"$PIN_ROOT/utils/coreutils_headers" "$OUTPUT_FILE" -o /tmp/test_${FUNCTION}.o 2>/dev/null; then
  echo "[+] Compilation successful"
  rm -f /tmp/test_${FUNCTION}.o
else
  echo "[WARNING] Compilation failed, but file was extracted"
fi

echo "[+] Extraction complete: $OUTPUT_FILE"
