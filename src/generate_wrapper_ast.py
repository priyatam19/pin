import sys
from pycparser import c_parser, c_ast

MAXLEN_DEFAULT = 128

def get_string_buf_size(decl):
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

def get_type_str(type_node):
    quals = ' '.join(type_node.quals) + ' ' if hasattr(type_node, 'quals') and type_node.quals else ''
    if isinstance(type_node, c_ast.PtrDecl):
        return quals + get_type_str(type_node.type) + '*'
    elif isinstance(type_node, c_ast.TypeDecl):
        if isinstance(type_node.type, c_ast.IdentifierType):
            return quals + ' '.join(type_node.type.names)
        elif isinstance(type_node.type, c_ast.Struct):
            return quals + 'struct ' + (type_node.type.name or '')
    elif isinstance(type_node, c_ast.ArrayDecl):
        return get_type_str(type_node.type) + '[]'
    return 'void'

class FuncFinder(c_ast.NodeVisitor):
    def __init__(self, func_name):
        self.func_name = func_name
        self.params = None
        self.return_type = 'int'
        self.param_sigs = []
    def visit_FuncDef(self, node):
        if node.decl.name == self.func_name:
            self.params = node.decl.type.args.params if node.decl.type.args else []
            ret_type_node = node.decl.type.type
            self.return_type = get_type_str(ret_type_node)
            # Collect full param signatures
            for param in self.params:
                param_type = get_type_str(param.type)
                self.param_sigs.append(f"{param_type} {param.name}")

def is_ptr_decl(decl):
    return isinstance(decl.type, c_ast.PtrDecl)

def is_struct_type(decl):
    t = decl.type if not is_ptr_decl(decl) else decl.type.type
    return isinstance(t, c_ast.TypeDecl) and isinstance(t.type, c_ast.Struct)

def walk_decls(decls, prefix, callbacks, structs, ast, cpath='input', depth=0, is_param_mode=False):
    buf_assignments = ""
    call_args = []
    buf_call_args = []
    for field in decls:
        fieldname = prefix + (field.name or "field")
        fieldcpath = f"{cpath}.{field.name}"
        is_ptr = is_ptr_decl(field)
        is_struct = is_struct_type(field)
        # String field
        if (isinstance(field.type, c_ast.ArrayDecl) and getattr(field.type.type.type, 'names', [None])[0] == 'char') or (is_ptr and getattr(field.type.type.type, 'names', [None])[0] == 'char'):
            buflen = get_string_buf_size(field) if not is_ptr else MAXLEN_DEFAULT
            callbacks.append((fieldname, buflen, fieldcpath))
            buf_assignments += f"""
    {fieldname}_buf[0] = '\\0';
    {fieldcpath}.arg = {fieldname}_buf;
    {fieldcpath}.funcs.decode = &decode_{fieldname};
"""
            buf_call_args.append(f"{fieldname}_buf")
            if is_param_mode:
                call_args.append(f"{fieldname}_buf")
        # Nested struct
        elif is_struct:
            nested_struct = field.type.type.type if is_ptr else field.type.type
            nested_assign, nested_call_args, nested_buf_call_args = walk_decls(
                nested_struct.decls or [], fieldname + "_", callbacks, structs, ast, fieldcpath, depth+1, is_param_mode
            )
            buf_assignments += nested_assign
            if is_param_mode:
                call_args.append(f"&{fieldcpath}" if is_ptr else fieldcpath)
            buf_call_args.extend(nested_buf_call_args)
        else:
            # Primitive
            if is_param_mode:
                call_args.append(f"{fieldcpath}")
    return buf_assignments, call_args, buf_call_args

def generate_wrapper(filename, logic_func, pb_base, mainstruct=None):
    with open(filename) as f:
        src = f.read()
    parser = c_parser.CParser()
    ast = parser.parse(src)
    
    # Find function details
    finder = FuncFinder(logic_func)
    finder.visit(ast)
    if finder.params is None:
        print(f"No function '{logic_func}' found.")
        sys.exit(1)
    params = finder.params
    return_type = finder.return_type
    param_sig = ", ".join(finder.param_sigs)
    
    # Check for structs
    structs = []
    for ext in ast.ext:
        if isinstance(ext, c_ast.Typedef) and isinstance(ext.type.type, c_ast.Struct):
            structs.append((ext.name, ext.type.type))
    
    callbacks = []
    buf_assignments = ""
    call_args = []
    buf_call_args = []
    structname = pb_base
    
    is_main = logic_func == "main"
    call_func_name = "original_main" if is_main else logic_func
    # For main, pass argc, argv
    call_str = f"{call_func_name}(argc, argv)" if is_main and len(params) > 0 else f"{call_func_name}({', '.join(call_args)})"
    
    if structs:
        # Struct mode
        if mainstruct:
            matches = [s for n, s in structs if n == mainstruct]
            if not matches:
                print(f"No struct '{mainstruct}' found.")
                sys.exit(1)
            struct = matches[0]
        else:
            struct = structs[-1][1]
        buf_assignments, _, buf_call_args = walk_decls(struct.decls or [], "", callbacks, structs, ast)
        call_args = ["&input"] + buf_call_args
        call_str = f"{call_func_name}({', '.join(call_args)})"
    else:
        # Param mode
        buf_assignments, call_args, buf_call_args = walk_decls(params, "", callbacks, [], ast, is_param_mode=True)
        call_str = f"{call_func_name}({', '.join(call_args)})"
    
    # Generate callback code and buf decls
    cb_code = ""
    buf_decls = ""
    for bufname, buflen, _ in callbacks:
        cb_code += generate_decode_callback(bufname, buflen)
        buf_decls += f"    char {bufname}_buf[{buflen}];\n"
    
    # Handle return
    if return_type != 'void':
        call_line = f"{return_type} result = {call_str};\n    printf(\"Output: %d\\n\", result);\n    return result;"
    else:
        call_line = f"{call_str};\n    return 0;"
    
    # Extern declaration
    extern_decl = f"extern {return_type} {logic_func}({param_sig});"
    
    wrapper = f"""\
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pb.h>  
#include <pb_decode.h>  
#include "{pb_base.lower()}.pb.h"

#define MAXLEN {MAXLEN_DEFAULT}
{cb_code}
{extern_decl}

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
        fprintf(stderr, "pb_decode failed: %s\\n", PB_GET_ERROR(&stream));
        free(buf);
        return 1;
    }}
    free(buf);

    {call_line}
}}
"""
    with open("main.c", "w") as f:
        f.write(wrapper)
    print("Generated main.c for wrapper.")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python generate_wrapper_ast.py <cfile> <logic_func> <pb_base> [MainStruct]")
        sys.exit(1)
    generate_wrapper(
        sys.argv[1],
        sys.argv[2],
        sys.argv[3],
        sys.argv[4] if len(sys.argv) > 4 else None
    )