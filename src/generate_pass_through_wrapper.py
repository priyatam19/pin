#!/usr/bin/env python3
"""
Generate a pass-through wrapper that feeds raw fuzzer bytes directly into
parser-style entry points (uint8_t *buf, size_t len, ...).

The wrapper:
  * Reads fuzz input bytes as-is (no protobuf decoding)
  * Initializes additional parameters with safe defaults
  * Calls the target function within pin_wrapper_entry
  * Emits the same standalone main() used by the regular wrappers
"""

import os
import sys
import textwrap
from typing import List, Tuple

try:
    from clang.cindex import Index, CursorKind, TypeKind
except ImportError as exc:
    print("[-] libclang Python bindings are required for pass-through mode.")
    print("    Install with: pip install libclang")
    raise SystemExit(1) from exc


def strip_qualifiers(type_str: str) -> str:
    for qualifier in ("const", "volatile", "restrict"):
        type_str = type_str.replace(qualifier, "")
    return " ".join(type_str.split()).strip()


def is_byte_buffer(param_cursor) -> bool:
    if param_cursor.type.kind != TypeKind.POINTER:
        return False
    pointee = param_cursor.type.get_pointee()
    spelling = strip_qualifiers(pointee.spelling)
    return spelling in ("uint8_t", "unsigned char", "char", "void")


def is_length_param(param_cursor, name: str) -> bool:
    type_name = param_cursor.type.spelling
    lowered = (name or "").lower()
    size_hint = any(token in lowered for token in ("len", "length", "size"))
    if "size_t" in type_name:
        return True
    if size_hint and any(token in type_name for token in ("int", "long", "size")):
        return True
    return False


def mangle_name(name: str, fallback: str) -> str:
    clean = name or fallback
    clean = clean.replace(".", "_").replace("-", "_")
    if not clean:
        clean = fallback
    return clean


def emit_pointer_setup(param_cursor, pname: str, idx: int) -> Tuple[List[str], str]:
    pointee = param_cursor.type.get_pointee()
    pointee_str = pointee.spelling
    base_no_qual = strip_qualifiers(pointee_str)
    storage_name = f"pin_ptr_{pname or idx}_storage"
    pointer_name = f"pin_ptr_{pname or idx}"
    lines = []

    if base_no_qual in ("uint8_t", "unsigned char", "char"):
        lines.append(f"    static unsigned char {storage_name}[1] = {{0}};")
        init_expr = f"{storage_name}"
    elif base_no_qual == "void":
        lines.append(f"    static unsigned char {storage_name}[1] = {{0}};")
        init_expr = f"{storage_name}"
    else:
        lines.append(f"    {pointee_str} {storage_name};")
        lines.append(f"    memset(&{storage_name}, 0, sizeof({storage_name}));")
        init_expr = f"&{storage_name}"

    lines.append(f"    {param_cursor.type.spelling} {pointer_name} = ({param_cursor.type.spelling}){init_expr};")
    return lines, pointer_name


def emit_scalar_setup(param_cursor, pname: str, idx: int) -> Tuple[List[str], str]:
    type_name = param_cursor.type.spelling
    lines = [f"    {type_name} pin_val_{pname or idx} = 0;"]
    return lines, f"pin_val_{pname or idx}"


def parse_function(filename: str, func_name: str, headers_dir: str) -> Tuple:
    index = Index.create()
    args = []
    if headers_dir:
        args.append("-I" + headers_dir)
    tu = index.parse(filename, args=args)
    if not tu:
        print(f"[-] Failed to parse translation unit for {filename}")
        raise SystemExit(1)

    for cursor in tu.cursor.walk_preorder():
        if cursor.kind == CursorKind.FUNCTION_DECL and cursor.spelling == func_name:
            params = list(cursor.get_arguments())
            return cursor.result_type.spelling, params

    print(f"[-] Function {func_name} not found in {filename}")
    raise SystemExit(1)


def build_wrapper(return_type: str,
                  params,
                  func_name: str,
                  include_headers: List[str]) -> str:
    includes = [
        "#include <stdint.h>",
        "#include <stddef.h>",
        "#include <stdio.h>",
        "#include <stdlib.h>",
        "#include <string.h>",
    ]
    need_stdbool = any("bool" in p.type.spelling for p in params)
    if need_stdbool:
        includes.insert(0, "#include <stdbool.h>")
    for header in include_headers:
        includes.append(f'#include "{header}"')

    extern_sig = ""
    if not include_headers:
        extern_sig = f"extern {return_type} {func_name}(" + ", ".join(
            p.type.spelling for p in params
        ) + ");"

    statements: List[str] = []
    call_args: List[str] = []
    pointer_bound = False
    len_bound = False

    for idx, param in enumerate(params):
        pname = mangle_name(param.spelling, f"arg{idx}")
        if (not pointer_bound) and param.type.kind == TypeKind.POINTER and is_byte_buffer(param):
            call_args.append(f"({param.type.spelling})data")
            pointer_bound = True
            continue

        if pointer_bound and (not len_bound) and is_length_param(param, pname):
            call_args.append(f"({param.type.spelling})len")
            len_bound = True
            continue

        if param.type.kind == TypeKind.POINTER:
            setup_lines, arg_expr = emit_pointer_setup(param, pname, idx)
            statements.extend(setup_lines)
            call_args.append(arg_expr)
        else:
            setup_lines, arg_expr = emit_scalar_setup(param, pname, idx)
            statements.extend(setup_lines)
            call_args.append(arg_expr)

    call_line = f"    {func_name}(" + ", ".join(call_args) + ");"
    if not call_args:
        call_line = f"    {func_name}();"

    wrapper = f"""{os.linesep.join(includes)}

#define PIN_EMI_REJECT_RC 86

{extern_sig}

int pin_wrapper_entry(const uint8_t *data, size_t len) {{
    (void)data;
    (void)len;
{os.linesep.join(statements) if statements else ""}
{call_line}
    return 0;
}}

#ifndef PIN_WRAPPER_NO_MAIN
int main(int argc, char *argv[]) {{
    if (argc != 2) {{
        fprintf(stderr, "Usage: %s input.bin\\n", argv[0]);
        return 1;
    }}

    FILE *f = fopen(argv[1], "rb");
    if (!f) {{ perror("fopen"); return 1; }}
    fseek(f, 0, SEEK_END);
    long file_len = ftell(f);
    rewind(f);

    if (file_len < 0) {{
        perror("ftell");
        fclose(f);
        return 1;
    }}

    uint8_t *buf = malloc((size_t)file_len);
    if (!buf) {{ perror("malloc"); fclose(f); return 1; }}
    size_t read_len = fread(buf, 1, (size_t)file_len, f);
    fclose(f);
    if (read_len != (size_t)file_len) {{
        perror("fread");
        free(buf);
        return 1;
    }}

    int rc = pin_wrapper_entry(buf, (size_t)file_len);
    free(buf);
    return rc;
}}
#endif
"""
    return textwrap.dedent(wrapper)


def main():
    if len(sys.argv) < 3:
        print("Usage: python generate_pass_through_wrapper.py <cfile> <function> "
              "[--headers-dir=<dir>] [--include=<header>]")
        raise SystemExit(1)

    filename = sys.argv[1]
    func_name = sys.argv[2]
    headers_dir = ""
    include_headers: List[str] = []

    for arg in sys.argv[3:]:
        if arg.startswith("--headers-dir="):
            headers_dir = arg.split("=", 1)[1]
        elif arg.startswith("--include="):
            include_headers.append(arg.split("=", 1)[1])

    return_type, params = parse_function(filename, func_name, headers_dir)
    wrapper_code = build_wrapper(return_type, params, func_name, include_headers)
    with open("main.c", "w") as outf:
        outf.write(wrapper_code)
    print("Generated pass-through main.c", file=sys.stderr)


if __name__ == "__main__":
    main()
