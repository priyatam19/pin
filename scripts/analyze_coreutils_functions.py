#!/usr/bin/env python3
"""
Analyze GNU Coreutils programs to identify fuzzable functions.

This script:
1. Parses all .c files in coreutils/src/
2. Extracts function declarations using libclang
3. Filters out main(), usage(), and other non-fuzzable functions
4. Scores each function by parameter fuzzability
5. Outputs ranked CSV for automated testing

Usage:
    python3 analyze_coreutils_functions.py /home/priyatam/ws/coreutils/src
"""

import sys
import os
import re
from pathlib import Path
from typing import List, Dict, Tuple, Optional
import json

# Try to import libclang for proper C parsing
try:
    import clang.cindex
    HAVE_LIBCLANG = True
except ImportError:
    HAVE_LIBCLANG = False
    print("[WARNING] libclang not available, using regex fallback")


class FunctionInfo:
    def __init__(self, program: str, name: str, return_type: str,
                 params: List[str], is_static: bool, line_num: int):
        self.program = program
        self.name = name
        self.return_type = return_type
        self.params = params
        self.is_static = is_static
        self.line_num = line_num
        self.score = 0.0
        self.reason = ""
        self.extraction_difficulty = "unknown"


def score_parameter(param: str) -> int:
    """Score a single parameter by fuzzability (1-5)."""
    param_lower = param.lower()

    # Variadic or va_list = lowest
    if 'va_list' in param_lower or '...' in param_lower:
        return 1

    # Function pointers = low
    if '(' in param and '*' in param:
        return 1

    # void* = low (needs context)
    if 'void' in param_lower and '*' in param:
        return 2

    # FILE* = low (I/O complexity)
    if 'file' in param_lower and '*' in param:
        return 2

    # Struct pointers = medium-low
    if 'struct' in param_lower and '*' in param:
        return 2

    # Callback-like names
    if any(word in param_lower for word in ['callback', 'handler', 'fn', 'func']):
        return 2

    # Simple struct values (not pointers) = medium
    if 'struct' in param_lower and '*' not in param:
        return 3

    # String pointers = medium-high
    if 'char' in param_lower and '*' in param:
        return 4

    # Pure scalars = highest
    scalar_types = ['int', 'bool', 'size_t', 'uint', 'unsigned', 'long',
                    'short', 'double', 'float', 'mode_t', 'off_t', 'uid_t',
                    'gid_t', 'pid_t']
    if any(t in param_lower for t in scalar_types):
        return 5

    # Unknown = medium
    return 3


def score_function(func: FunctionInfo) -> Tuple[float, str, str]:
    """
    Score function by fuzzability and determine extraction difficulty.
    Returns: (score, reason, extraction_difficulty)
    """
    if not func.params:
        return (0.0, "No parameters to fuzz", "skip")

    param_scores = [score_parameter(p) for p in func.params]
    avg_score = sum(param_scores) / len(param_scores)

    # Penalize many parameters
    param_count_penalty = max(0, len(func.params) - 5) * 0.5
    final_score = avg_score - param_count_penalty

    # Determine reason
    reasons = []
    if avg_score >= 4.5:
        reasons.append("Mostly scalars")
    elif avg_score >= 3.5:
        reasons.append("Mixed scalars/strings")

    if any('struct' in p for p in func.params):
        reasons.append("Has structs")
    if any('*' in p and 'const' not in p for p in func.params):
        reasons.append("Has mutable pointers")
    if len(func.params) > 5:
        reasons.append(f"Many params ({len(func.params)})")
    if any('va_list' in p or '...' in p for p in func.params):
        reasons.append("Variadic")
    if any('FILE' in p and '*' in p for p in func.params):
        reasons.append("FILE I/O")

    reason = "; ".join(reasons) if reasons else "Standard function"

    # Determine extraction difficulty
    if func.is_static:
        extraction = "easy"  # Just remove static keyword
    elif any('FILE' in p for p in func.params):
        extraction = "hard"  # I/O complexity
    elif any('struct' in p and '*' in p for p in func.params):
        extraction = "medium"  # May need struct definitions
    elif len(func.params) <= 3 and avg_score >= 4.0:
        extraction = "easy"
    else:
        extraction = "medium"

    return (round(final_score, 2), reason, extraction)


def should_skip_function(name: str) -> bool:
    """Check if function should be skipped from fuzzing."""
    skip_patterns = [
        'main',
        'usage',
        'emit_',
        'version_',
        'print_version',
        'print_help',
        'proper_name',
        'xalloc_die',
        'close_stdout',
        '__',  # Internal/system functions
    ]

    name_lower = name.lower()
    return any(pattern in name_lower for pattern in skip_patterns)


def parse_with_libclang(source_file: Path) -> List[FunctionInfo]:
    """Parse C file using libclang for accurate extraction."""
    if not HAVE_LIBCLANG:
        return []

    program_name = source_file.stem
    functions = []

    try:
        index = clang.cindex.Index.create()
        # Parse with common coreutils defines
        args = [
            '-I/usr/include',
            '-DHAVE_CONFIG_H',
            '-D_GNU_SOURCE',
        ]
        tu = index.parse(str(source_file), args=args)

        for cursor in tu.cursor.walk_preorder():
            if cursor.kind != clang.cindex.CursorKind.FUNCTION_DECL:
                continue

            # Skip if not in this file
            if not cursor.location.file or cursor.location.file.name != str(source_file):
                continue

            func_name = cursor.spelling
            if should_skip_function(func_name):
                continue

            # Get return type
            return_type = cursor.result_type.spelling

            # Get parameters
            params = []
            for arg in cursor.get_arguments():
                param_type = arg.type.spelling
                param_name = arg.spelling
                params.append(f"{param_type} {param_name}".strip())

            # Check if static
            is_static = cursor.storage_class == clang.cindex.StorageClass.STATIC

            func = FunctionInfo(
                program=program_name,
                name=func_name,
                return_type=return_type,
                params=params,
                is_static=is_static,
                line_num=cursor.location.line
            )

            functions.append(func)

    except Exception as e:
        print(f"[WARNING] libclang parsing failed for {source_file}: {e}")
        return []

    return functions


def parse_with_regex(source_file: Path) -> List[FunctionInfo]:
    """Fallback: Parse C file using regex (less accurate but works without libclang)."""
    program_name = source_file.stem
    functions = []

    try:
        with open(source_file, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        # Match function definitions (simplified regex)
        # Pattern: [static] return_type function_name(params)
        pattern = r'(?:^|\n)(static\s+)?([a-zA-Z_][a-zA-Z0-9_*\s]+)\s+([a-zA-Z_][a-zA-Z0-9_]+)\s*\(([^)]*)\)\s*(?:\{|;)'

        for match in re.finditer(pattern, content, re.MULTILINE):
            is_static = match.group(1) is not None
            return_type = match.group(2).strip()
            func_name = match.group(3).strip()
            params_str = match.group(4).strip()

            if should_skip_function(func_name):
                continue

            # Parse parameters
            params = []
            if params_str and params_str != 'void':
                # Split by comma, but be careful with nested commas
                param_parts = re.split(r',\s*(?![^()]*\))', params_str)
                params = [p.strip() for p in param_parts if p.strip()]

            # Approximate line number
            line_num = content[:match.start()].count('\n') + 1

            func = FunctionInfo(
                program=program_name,
                name=func_name,
                return_type=return_type,
                params=params,
                is_static=is_static,
                line_num=line_num
            )

            functions.append(func)

    except Exception as e:
        print(f"[WARNING] Regex parsing failed for {source_file}: {e}")
        return []

    return functions


def analyze_coreutils(src_dir: Path) -> List[FunctionInfo]:
    """Analyze all coreutils source files."""
    all_functions = []

    c_files = sorted(src_dir.glob("*.c"))
    print(f"[*] Found {len(c_files)} C source files in {src_dir}")

    parse_func = parse_with_libclang if HAVE_LIBCLANG else parse_with_regex
    parser_name = "libclang" if HAVE_LIBCLANG else "regex"

    for i, c_file in enumerate(c_files, 1):
        print(f"[{i}/{len(c_files)}] Parsing {c_file.name} with {parser_name}...", end=' ')

        functions = parse_func(c_file)

        # Score each function
        for func in functions:
            score, reason, extraction = score_function(func)
            func.score = score
            func.reason = reason
            func.extraction_difficulty = extraction

        all_functions.extend(functions)
        print(f"found {len(functions)} functions")

    return all_functions


def main():
    if len(sys.argv) != 2:
        print("Usage: analyze_coreutils_functions.py <coreutils_src_dir>")
        print("Example: analyze_coreutils_functions.py /home/priyatam/ws/coreutils/src")
        sys.exit(1)

    src_dir = Path(sys.argv[1])
    if not src_dir.exists():
        print(f"[ERROR] Directory not found: {src_dir}")
        sys.exit(1)

    print(f"[*] Analyzing coreutils source directory: {src_dir}")
    print(f"[*] Parser: {'libclang' if HAVE_LIBCLANG else 'regex fallback'}")
    print()

    # Analyze all functions
    all_functions = analyze_coreutils(src_dir)

    print()
    print(f"[*] Total functions found: {len(all_functions)}")

    # Filter out zero-score functions
    fuzzable = [f for f in all_functions if f.score > 0]
    print(f"[*] Fuzzable functions (score > 0): {len(fuzzable)}")

    # Sort by score descending
    fuzzable.sort(key=lambda x: (-x.score, x.program, x.name))

    # Generate output files
    output_dir = Path("/home/priyatam/pin/examples/coreutils")
    output_dir.mkdir(parents=True, exist_ok=True)

    csv_file = output_dir / "coreutils_functions_ranked.csv"
    json_file = output_dir / "coreutils_functions_ranked.json"
    top100_file = output_dir / "coreutils_top100_targets.txt"

    # Write CSV
    with open(csv_file, 'w') as f:
        f.write("rank,score,program,function,params,param_count,return_type,is_static,line_num,reason,extraction_difficulty\n")

        for rank, func in enumerate(fuzzable, 1):
            params_str = "; ".join(func.params).replace('"', '""')
            f.write(f'{rank},{func.score},{func.program},{func.name},"{params_str}",{len(func.params)},{func.return_type},{func.is_static},{func.line_num},"{func.reason}",{func.extraction_difficulty}\n')

    print(f"[+] Ranked CSV saved to: {csv_file}")

    # Write JSON (for programmatic access)
    json_data = []
    for rank, func in enumerate(fuzzable, 1):
        json_data.append({
            "rank": rank,
            "score": func.score,
            "program": func.program,
            "function": func.name,
            "params": func.params,
            "param_count": len(func.params),
            "return_type": func.return_type,
            "is_static": func.is_static,
            "line_num": func.line_num,
            "reason": func.reason,
            "extraction_difficulty": func.extraction_difficulty
        })

    with open(json_file, 'w') as f:
        json.dump(json_data, f, indent=2)

    print(f"[+] JSON data saved to: {json_file}")

    # Write top 100 targets
    top100 = fuzzable[:min(100, len(fuzzable))]
    with open(top100_file, 'w') as f:
        for func in top100:
            f.write(f"{func.program}:{func.name}\n")

    print(f"[+] Top 100 targets saved to: {top100_file}")

    # Print statistics
    print()
    print("=" * 80)
    print("STATISTICS")
    print("=" * 80)

    by_difficulty = {}
    for func in fuzzable:
        by_difficulty[func.extraction_difficulty] = by_difficulty.get(func.extraction_difficulty, 0) + 1

    print(f"\nTotal fuzzable functions: {len(fuzzable)}")
    print(f"Extraction difficulty distribution:")
    for difficulty in ['easy', 'medium', 'hard', 'skip']:
        count = by_difficulty.get(difficulty, 0)
        pct = 100 * count / len(fuzzable) if fuzzable else 0
        print(f"  {difficulty:8s}: {count:4d} ({pct:5.1f}%)")

    # Score distribution
    tier5 = len([f for f in fuzzable if f.score >= 4.5])
    tier4 = len([f for f in fuzzable if 3.5 <= f.score < 4.5])
    tier3 = len([f for f in fuzzable if 2.5 <= f.score < 3.5])
    tier2 = len([f for f in fuzzable if 1.5 <= f.score < 2.5])
    tier1 = len([f for f in fuzzable if f.score < 1.5])

    print(f"\nFuzzability tiers:")
    print(f"  Tier 5 (≥4.5): {tier5:4d} - EXCELLENT")
    print(f"  Tier 4 (3.5-4.5): {tier4:4d} - GOOD")
    print(f"  Tier 3 (2.5-3.5): {tier3:4d} - MEDIUM")
    print(f"  Tier 2 (1.5-2.5): {tier2:4d} - LOW")
    print(f"  Tier 1 (<1.5): {tier1:4d} - POOR")

    # Top 20 preview
    print()
    print("=" * 80)
    print("TOP 20 FUNCTIONS")
    print("=" * 80)
    print(f"{'Rank':<5} {'Score':<6} {'Program':<15} {'Function':<30} {'Params':<5}")
    print("-" * 80)

    for i, func in enumerate(fuzzable[:20], 1):
        print(f"{i:<5} {func.score:<6} {func.program:<15} {func.name:<30} {len(func.params):<5}")

    print()
    print(f"[+] Analysis complete! Found {len(fuzzable)} fuzzable functions")
    print(f"[+] Top 100 ready for automated testing")


if __name__ == '__main__':
    main()
