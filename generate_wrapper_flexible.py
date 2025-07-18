import sys
from pycparser import c_parser, c_ast, parse_file

MAXLEN_DEFAULT = 128

TYPE_MAP = {
    'int': 'int32',
    'int32_t': 'int32',
    'float': 'float',
    'double': 'double',
    'char': 'string',
}

def detect_single_primitive_struct(struct):
    """Return (field_type, field_name) if it's a single primitive, else None"""
    if struct.decls and len(struct.decls) == 1:
        decl = struct.decls[0]
        # Single field, primitive
        if isinstance(decl.type, c_ast.ArrayDecl):
            # Only char array maps to string, rest are arrays (not single primitive)
            basetype = decl.type.type.type.names[0]
            if basetype == 'char':
                return ('string', decl.name)
        elif isinstance(decl.type, c_ast.TypeDecl):
            if hasattr(decl.type.type, 'names'):
                typename = decl.type.type.names[0]
                if typename in TYPE_MAP:
                    return (TYPE_MAP[typename], decl.name)
        # For now, skip pointer/struct/array
    return None

def generate_string_wrapper(logic_func, pb_base, field_name):
    # field_name is e.g. 's'
    return f"""\
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "{pb_base}.pb.h"
#include "pb_decode.h"

#define MAXLEN {MAXLEN_DEFAULT}

bool decode_{field_name}(pb_istream_t *stream, const pb_field_t *field, void **arg) {{
    char *buffer = (char *)(*arg);
    size_t len = stream->bytes_left < (MAXLEN - 1) ? stream->bytes_left : (MAXLEN - 1);
    if (!pb_read(stream, (pb_byte_t*)buffer, len)) return false;
    buffer[len] = '\\0';
    return true;
}}

// User logic function
extern int {logic_func}(const char* {field_name});

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

    {pb_base.capitalize()} input = {pb_base.capitalize()}_init_zero;
    char {field_name}_buf[MAXLEN] = {{0}};
    input.{field_name}.arg = {field_name}_buf;
    input.{field_name}.funcs.decode = &decode_{field_name};

    pb_istream_t stream = pb_istream_from_buffer(buf, len);
    if (!pb_decode(&stream, {pb_base.capitalize()}_fields, &input)) {{
        fprintf(stderr, "pb_decode failed\\n");
        free(buf);
        return 1;
    }}
    free(buf);

    int result = {logic_func}({field_name}_buf);
    printf("Output: %d\\n", result);
    return 0;
}}
"""

def generate_primitive_wrapper(logic_func, pb_base, field_type, field_name):
    # For int/float/double (NOT string)
    ctype = {'int32': 'int', 'float': 'float', 'double': 'double'}.get(field_type, 'int')
    return f"""\
#include <stdio.h>
#include <stdlib.h>
#include "{pb_base}.pb.h"
#include "pb_decode.h"

// User logic function
extern int {logic_func}({ctype} {field_name});

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

    {pb_base.capitalize()} input = {pb_base.capitalize()}_init_zero;

    pb_istream_t stream = pb_istream_from_buffer(buf, len);
    if (!pb_decode(&stream, {pb_base.capitalize()}_fields, &input)) {{
        fprintf(stderr, "pb_decode failed\\n");
        free(buf);
        return 1;
    }}
    free(buf);

    int result = {logic_func}(input.{field_name});
    printf("Output: %d\\n", result);
    return 0;
}}
"""

def walk_struct(struct, prefix, callbacks, decls, structs, ast, cpath='input', depth=0):
    buf_assignments = ""
    arglist = []
    for field in struct.decls:
        fieldname = prefix + (field.name or "field")
        fieldcpath = f"{cpath}.{field.name}"
        if isinstance(field.type, c_ast.ArrayDecl) and hasattr(field.type.type, 'type'):
            basetype = field.type.type.type.names[0]
            if basetype == "char":
                buflen = MAXLEN_DEFAULT
                callbacks.append((fieldname, buflen, fieldcpath))
                buf_assignments += f"""
    {fieldname}_buf[0] = '\\0';
    {fieldcpath}.arg = {fieldname}_buf;
    {fieldcpath}.funcs.decode = &decode_{fieldname};
"""
                arglist.append(f"{fieldname}_buf")
            else:
                arglist.append(fieldcpath)
        elif (isinstance(field.type, c_ast.TypeDecl) and hasattr(field.type.type, 'declname')
              and isinstance(field.type.type, c_ast.Struct)):
            nested_struct = field.type.type
            nested_assign, nested_args = walk_struct(
                nested_struct, fieldname + "_", callbacks, decls, structs, ast, cpath=fieldcpath, depth=depth+1)
            buf_assignments += nested_assign
            arglist.extend(nested_args)
        elif isinstance(field.type, c_ast.Struct):
            nested_struct = field.type
            nested_assign, nested_args = walk_struct(
                nested_struct, fieldname + "_", callbacks, decls, structs, ast, cpath=fieldcpath, depth=depth+1)
            buf_assignments += nested_assign
            arglist.extend(nested_args)
        else:
            arglist.append(fieldcpath)
    return buf_assignments, arglist

def generate_struct_wrapper(structname, struct, logic_func, pb_base):
    callbacks = []
    decls = []
    buf_assignments, arglist = walk_struct(struct, "", callbacks, decls, [], None)
    buf_args = [a for a in arglist if a.endswith('_buf')]

    cb_code = ""
    buf_decls = ""
    for bufname, buflen, fieldcpath in callbacks:
        cb_code += f"""
bool decode_{bufname}(pb_istream_t *stream, const pb_field_t *field, void **arg) {{
    char *buffer = (char *)(*arg);
    size_t len = stream->bytes_left < ({buflen} - 1) ? stream->bytes_left : ({buflen} - 1);
    if (!pb_read(stream, (pb_byte_t*)buffer, len)) return false;
    buffer[len] = '\\0';
    return true;
}}
"""
        buf_decls += f"    char {bufname}_buf[{buflen}];\n"

    return f"""\
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

def generate_wrapper(filename, logic_func, pb_base, mainstruct=None):
    ast = parse_file(filename, use_cpp=True)
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

    # Check for single primitive field struct
    single = detect_single_primitive_struct(struct)
    if single:
        ftype, fname = single
        if ftype == 'string':
            code = generate_string_wrapper(logic_func, pb_base, fname)
        elif ftype in ('int32', 'float', 'double'):
            code = generate_primitive_wrapper(logic_func, pb_base, ftype, fname)
        else:
            print(f"Unsupported single field type: {ftype}")
            sys.exit(1)
    else:
        code = generate_struct_wrapper(structname, struct, logic_func, pb_base)

    with open("main.c", "w") as f:
        f.write(code)
    print("Generated main.c for wrapper.")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python generate_wrapper_flexible.py <cfile> <logic_func_name> <pb_base> [MainStruct]")
        sys.exit(1)
    generate_wrapper(
        sys.argv[1],
        sys.argv[2],
        sys.argv[3],
        sys.argv[4] if len(sys.argv) > 4 else None
    )