import sys
from pycparser import c_parser, c_ast, parse_file

MAXLEN_DEFAULT = 128

import re

def extract_typedef_structs(c_code):
    pattern = re.compile(r'typedef\s+struct\s*{[^}]*}\s*\w+\s*;', re.DOTALL)
    return '\n\n'.join(pattern.findall(c_code))

def get_string_buf_size(decl):
    # If field is char arr[LEN], get LEN, else default
    if isinstance(decl.type, c_ast.ArrayDecl):
        try:
            return int(decl.type.dim.value)
        except Exception:
            return MAXLEN_DEFAULT
    return MAXLEN_DEFAULT

def generate_decode_callback(bufname, buflen):
    return f"""
bool decode_{bufname}(pb_istream_t *stream, const pb_field_t *field, void **arg) {{
    char *buffer = (char *)(*arg);
    size_t len = stream->bytes_left < ({buflen} - 1) ? stream->bytes_left : ({buflen} - 1);
    if (!pb_read(stream, (pb_byte_t*)buffer, len)) return false;
    buffer[len] = '\\0';
    return true;
}}
"""

def walk_struct(struct, prefix, callbacks, decls, structs, ast, cpath='input', depth=0):
    """
    Recursively walk the struct definition, accumulating buffer assignments and arg list.
    - prefix: hierarchical field name for buffer/func name, e.g. sub_desc_
    - cpath: C field path for assignments, e.g. input.sub.desc
    """
    buf_assignments = ""
    arglist = []
    for field in struct.decls:
        fieldname = prefix + (field.name or "field")
        fieldcpath = f"{cpath}.{field.name}"

        # String field (char array)
        if isinstance(field.type, c_ast.ArrayDecl) and hasattr(field.type.type, 'type'):
            basetype = field.type.type.type.names[0]
            if basetype == "char":
                buflen = get_string_buf_size(field)
                callbacks.append((fieldname, buflen, fieldcpath))
                buf_assignments += f"""
    {fieldname}_buf[0] = '\\0';
    {fieldcpath}.arg = {fieldname}_buf;
    {fieldcpath}.funcs.decode = &decode_{fieldname};
"""
                arglist.append(f"{fieldname}_buf")
            else:
                arglist.append(fieldcpath)
        # Nested struct by typedef or direct
        elif (isinstance(field.type, c_ast.TypeDecl) and hasattr(field.type.type, 'declname')
              and isinstance(field.type.type, c_ast.Struct)):
            # Named nested struct
            nested_struct = field.type.type
            nested_assign, nested_args = walk_struct(
                nested_struct, fieldname + "_", callbacks, decls, structs, ast, cpath=fieldcpath, depth=depth+1)
            buf_assignments += nested_assign
            arglist.extend(nested_args)
        elif isinstance(field.type, c_ast.Struct):
            # Anonymous/inline nested struct
            nested_struct = field.type
            nested_assign, nested_args = walk_struct(
                nested_struct, fieldname + "_", callbacks, decls, structs, ast, cpath=fieldcpath, depth=depth+1)
            buf_assignments += nested_assign
            arglist.extend(nested_args)
        else:
            # Other primitive fields (int, float, etc.)
            arglist.append(fieldcpath)
    return buf_assignments, arglist

def generate_wrapper(filename, logic_func, pb_base, mainstruct=None):
    # Read the file, extract only typedef struct(s)
    with open(filename) as f:
        src = f.read()
    struct_src = extract_typedef_structs(src)
    if not struct_src.strip():
        print("No typedef structs found in input file. Exiting.")
        sys.exit(1)
    parser = c_parser.CParser()
    ast = parser.parse(struct_src)
    structs = []
    # Find all typedef structs
    for ext in ast.ext:
        if isinstance(ext, c_ast.Typedef) and isinstance(ext.type.type, c_ast.Struct):
            structs.append((ext.name, ext.type.type))
    if not structs:
        print("No structs found.")
        sys.exit(1)
    # Default: use last struct as main struct
    if not mainstruct:
        structname, struct = structs[-1]
    else:
        matches = [(n, s) for n, s in structs if n.lower() == mainstruct.lower()]
        if not matches:
            print(f"No struct named '{mainstruct}' found in typedefs! Found: {[n for n, _ in structs]}")
            sys.exit(1)
        structname, struct = matches[0]
    callbacks = []
    decls = []
    buf_assignments, arglist = walk_struct(struct, "", callbacks, decls, structs, ast)
    buf_args = [a for a in arglist if a.endswith('_buf')]

    # Generate decode callback functions and buffer declarations
    cb_code = ""
    buf_decls = ""
    for bufname, buflen, fieldcpath in callbacks:
        cb_code += generate_decode_callback(bufname, buflen)
        buf_decls += f"    char {bufname}_buf[{buflen}];\n"

    wrapper = f"""\
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "{pb_base}.pb.h"
#include "pb_decode.h"

#define MAXLEN {MAXLEN_DEFAULT}
{cb_code}
// User logic function
extern int {logic_func}(const {structname}* input{''.join([', const char* '+a for a in buf_args])});

int main(int argc, char *argv[]) {{
    if (argc != 2) {{
        fprintf(stderr, "Usage: %s input.bin\\n", argv[0]);
        return 1;
    }}

    FILE *f = fopen(argv[1], "rb");
    if (!f) {{ perror("fopen"); return 1; }}
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    uint8_t *buf = malloc(len);
    if (!buf) {{ perror("malloc"); fclose(f); return 1; }}
    fread(buf, 1, len, f);
    fclose(f);

    {structname} input = {structname}_init_zero;
{buf_decls}
{buf_assignments}
    pb_istream_t stream = pb_istream_from_buffer(buf, len);
    if (!pb_decode(&stream, {structname}_fields, &input)) {{
        fprintf(stderr, "pb_decode failed\\n");
        free(buf);
        return 1;
    }}
    free(buf);

    int result = {logic_func}(&input{''.join([', '+a for a in buf_args])});
    printf("Output: %d\\n", result);
    return 0;
}}
"""
    with open("main.c", "w") as f:
        f.write(wrapper)
    print("Generated main.c for wrapper.")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python generate_wrapper_ast.py <cfile> <logic_func_name> <pb_base> [MainStruct]")
        sys.exit(1)
    generate_wrapper(
        sys.argv[1],
        sys.argv[2],
        sys.argv[3],
        sys.argv[4] if len(sys.argv) > 4 else None
    )