#!/usr/bin/env python3
"""
PIN Wrapper Code Generator

This module generates C wrapper code that uses nanopb to deserialize Protocol Buffer
input data and call the original normalized functions. It handles complex struct
deserialization with callback functions for string fields.

Key Features:
- Generates nanopb-based deserialization code
- Creates callback functions for string field handling
- Supports both pycparser and libclang parsed struct data
- Handles nested structs and complex field mappings
- Generates proper function calls with correct argument passing

Author: PIN Development Team
"""

import sys
from pycparser import c_parser, c_ast
import os
from clang.cindex import Index, CursorKind

# Default buffer size for string fields when size cannot be determined
MAXLEN_DEFAULT = 128

def get_string_buf_size(decl, parser="pycparser"):
    """
    Extract buffer size for string fields from C array declarations.
    
    For char arrays like char[32], this returns 32. Falls back to default
    size if the array size cannot be determined.
    
    Args:
        decl: Field declaration (pycparser AST or libclang tuple)
        parser: Parser type ("pycparser" or "libclang")
        
    Returns:
        int: Buffer size for the string field
    """
    if parser == "pycparser":
        if isinstance(decl.type, c_ast.ArrayDecl):
            try:
                return int(decl.type.dim.value)
            except:
                return MAXLEN_DEFAULT
        return MAXLEN_DEFAULT
    # libclang case - extract size from type string like "char[32]"
    if '[]' in decl[0]:
        try:
            return int(decl[0].split('[')[1].split(']')[0])
        except:
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

def get_type_str(type_node, parser="pycparser"):
    if parser == "pycparser":
        quals = ' '.join(type_node.quals) + ' ' if hasattr(type_node, 'quals') and type_node.quals else ''
        if isinstance(type_node, c_ast.PtrDecl):
            return quals + get_type_str(type_node.type, parser) + '*'
        elif isinstance(type_node, c_ast.TypeDecl):
            if isinstance(type_node.type, c_ast.IdentifierType):
                return quals + ' '.join(type_node.type.names)
            elif isinstance(type_node.type, c_ast.Struct):
                return quals + 'struct ' + (type_node.type.name or '')
        elif isinstance(type_node, c_ast.ArrayDecl):
            return get_type_str(type_node.type, parser) + '[]'
        return 'void'
    return type_node.spelling.replace('struct ', '')

def is_ptr_decl(decl, parser="pycparser"):
    if parser == "pycparser":
        return isinstance(decl.type, c_ast.PtrDecl)
    return '*' in decl[0]

def is_struct_type(decl, parser="pycparser"):
    if parser == "pycparser":
        t = decl.type if not is_ptr_decl(decl) else decl.type.type
        return isinstance(t, c_ast.TypeDecl) and isinstance(t.type, c_ast.Struct)
    # For libclang, check if it's a struct type or one of our cleaned struct names
    type_str = decl[0].strip()
    return (type_str.startswith('struct ') or 
            type_str.startswith('AnonymousStruct') or 
            type_str in ['MyStruct', 'SubStruct1', 'SubStruct2'])

def walk_decls(decls, prefix, callbacks, structs, ast, cpath='input', depth=0, is_param_mode=False, parser="pycparser"):
    buf_assignments = ""
    call_args = []
    buf_call_args = []
    print(f"DEBUG: walk_decls called with {len(decls)} fields, prefix='{prefix}', depth={depth}, parser={parser}")
    for field in decls:
        fieldname = prefix + (field[1] if isinstance(field, tuple) else field.name or "field")
        fieldcpath = f"{cpath}.{field[1] if isinstance(field, tuple) else field.name}"
        is_ptr = is_ptr_decl(field, parser)
        is_struct = is_struct_type(field, parser)
        print(f"DEBUG: Processing field {field}, name={fieldname}, is_struct={is_struct}, is_ptr={is_ptr}")
        if (parser == "pycparser" and (
            (isinstance(field.type, c_ast.ArrayDecl) and getattr(field.type.type.type, 'names', [None])[0] == 'char') or
            (is_ptr and getattr(field.type.type.type, 'names', [None])[0] == 'char')
        )) or (parser == "libclang" and (
            'char[' in field[0] or field[0] == 'char *'
        )):
            buflen = get_string_buf_size(field, parser)
            callbacks.append((fieldname, buflen, fieldcpath))
            buf_assignments += f"""
    {fieldname}_buf[0] = '\\0';
    {fieldcpath}.arg = {fieldname}_buf;
    {fieldcpath}.funcs.decode = &decode_{fieldname};
"""
            buf_call_args.append(f"{fieldname}_buf")
            if is_param_mode:
                call_args.append(f"{fieldname}_buf")
        elif is_struct:
            if parser == "pycparser":
                nested_struct = field.type.type.type if is_ptr else field.type.type
                nested_decls = nested_struct.decls or []
            else:
                # Handle libclang struct field types
                struct_type = field[0].replace('struct ', '').strip()
                # Try exact match first, then fuzzy match for anonymous structs
                nested_decls = [f for n, f in structs if n == struct_type]
                if not nested_decls:
                    # For anonymous structs, find any AnonymousStruct variant
                    nested_decls = [f for n, f in structs if n.startswith('AnonymousStruct')]
                nested_decls = nested_decls[0] if nested_decls else []
                print(f"DEBUG: Looking for struct type '{struct_type}', found {len(nested_decls)} nested fields")
            nested_assign, nested_call_args, nested_buf_call_args = walk_decls(
                nested_decls, fieldname + "_", callbacks, structs, ast, fieldcpath, depth+1, is_param_mode, parser
            )
            buf_assignments += nested_assign
            if is_param_mode:
                call_args.append(f"&{fieldcpath}" if is_ptr else fieldcpath)
            buf_call_args.extend(nested_buf_call_args)
        else:
            if is_param_mode:
                call_args.append(f"{fieldcpath}")
    return buf_assignments, call_args, buf_call_args

def parse_with_libclang(filename, headers_dir=""):
    index = Index.create()
    args = ['-I' + headers_dir] if headers_dir else []
    tu = index.parse(filename, args=args)
    structs = []
    anonymous_counter = 0
    for cursor in tu.cursor.walk_preorder():
        if cursor.kind == CursorKind.STRUCT_DECL:
            name = cursor.spelling
            if not name:  # Handle anonymous structs
                anonymous_counter += 1
                name = f"AnonymousStruct{anonymous_counter}"
            elif '(unnamed' in name or 'anonymous' in name:
                anonymous_counter += 1 
                name = f"AnonymousStruct{anonymous_counter}"
            if name and name not in ['mg_mgr', 'mg_connection', 'mg_addr', 'mg_iobuf', 'mg_dns', 'mg_timer']:
                fields = [(field.type.spelling, field.spelling) for field in cursor.get_children() if field.kind == CursorKind.FIELD_DECL]
                structs.append((name, fields))
                print(f"DEBUG: Wrapper parse found struct {name} with fields {fields}")
    return structs

class FuncFinder(c_ast.NodeVisitor):
    def __init__(self, func_name):
        self.func_name = func_name
        self.params = None
        self.called_funcs = set()
        self.handler_func = None
        self.handler_struct = None
    def visit_FuncDef(self, node):
        if node.decl.name == self.func_name:
            self.params = node.decl.type.args.params if node.decl.type.args else []
        if node.body:
            for item in node.body.block_items or []:
                if isinstance(item, c_ast.FuncCall) and item.name.name:
                    self.called_funcs.add(item.name.name)

def generate_wrapper(filename, logic_func, pb_base, mainstruct=None, parser="pycparser", headers_dir=""):
    if parser == "pycparser":
        with open(filename) as f:
            src = f.read()
        parser_obj = c_parser.CParser()
        ast = parser_obj.parse(src)
        
        finder = FuncFinder(logic_func)
        finder.visit(ast)
        if finder.params is None and logic_func != "main":
            print(f"No function '{logic_func}' found.")
            sys.exit(1)
        params = finder.params or []
        return_type = 'int'  # Default, as we can't reliably get return type
        param_sig = ", ".join(f"{get_type_str(p.type, parser)} {p.name}" for p in params) if params else ""
        structs = []
        for ext in ast.ext:
            if isinstance(ext, c_ast.Typedef) and isinstance(ext.type.type, c_ast.Struct):
                structs.append((ext.name, ext.type.type))
        # Find handler function with struct param
        handler_func = None
        handler_struct = None
        for func in finder.called_funcs:
            for ext in ast.ext:
                if isinstance(ext, c_ast.FuncDef) and ext.decl.name == func:
                    params = ext.decl.type.args.params if ext.decl.type.args else []
                    for param in params:
                        if isinstance(param.type, c_ast.PtrDecl) and isinstance(param.type.type.type, c_ast.Struct):
                            handler_func = ext.decl.name
                            handler_struct = param.type.type.type.name
                            break
                    if handler_struct:
                        break
    else:
        index = Index.create()
        args = ['-I' + headers_dir] if headers_dir else []
        tu = index.parse(filename, args=args)
        params = []
        return_type = "int"
        param_sig = ""
        structs = parse_with_libclang(filename, headers_dir)
        handler_func = None
        handler_struct = None
        for cursor in tu.cursor.walk_preorder():
            if cursor.kind == CursorKind.FUNCTION_DECL and cursor.spelling == logic_func:
                params = [(param.type.spelling, param.spelling) for param in cursor.get_arguments()]
                return_type = cursor.result_type.spelling
                param_sig = ", ".join(f"{t} {n}" for t, n in params)
                for child in cursor.walk_preorder():
                    if child.kind == CursorKind.CALL_EXPR:
                        for c in tu.cursor.walk_preorder():
                            if c.kind == CursorKind.FUNCTION_DECL and c.spelling == child.spelling:
                                for param in c.get_arguments():
                                    type_name = param.type.spelling
                                    if type_name.startswith('struct '):
                                        handler_struct = type_name[7:].split('[')[0].split('*')[0].strip()
                                        handler_func = c.spelling
                                        break
                        if handler_struct:
                            break
    
    callbacks = []
    buf_assignments = ""
    call_args = []
    buf_call_args = []
    structname = pb_base
    
    is_main = logic_func == "main"
    call_func_name = "pin_original_main" if is_main else logic_func
    
    if is_main and not params and handler_func and handler_struct:
        # Server-like main: call handler function with struct param
        matches = [s for n, s in structs if n == handler_struct]
        if not matches:
            print(f"No struct '{handler_struct}' found for handler '{handler_func}'.")
            sys.exit(1)
        struct = matches[0]
        buf_assignments, _, buf_call_args = walk_decls(struct if isinstance(struct, list) else struct.decls or [], "", callbacks, structs, None, parser=parser)
        call_args = ["&dummy", "&input.input"] + buf_call_args
        call_str = f"{handler_func}({', '.join(call_args)})"
        
        cb_code = ""
        buf_decls = ""
        for bufname, buflen, _ in callbacks:
            cb_code += generate_decode_callback(bufname, buflen)
            buf_decls += f"    char {bufname}_buf[{buflen}];\n"
        
        wrapper = f"""\
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pb.h>
#include <pb_decode.h>
#include "{pb_base.lower()}.pb.h"

#define MAXLEN {MAXLEN_DEFAULT}
{cb_code}
extern void {handler_func}(struct {handler_struct} *, struct {handler_struct} *);

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

    struct {handler_struct} dummy = {{0}};
    {handler_func}(&dummy, &input.input);
    return 0;
}}
"""
    else:
        if structs and mainstruct:
            matches = [s for n, s in structs if n == mainstruct]
            if not matches:
                print(f"No struct '{mainstruct}' found.")
                sys.exit(1)
            struct = matches[0]
            buf_assignments, _, buf_call_args = walk_decls(struct if isinstance(struct, list) else struct.decls or [], "", callbacks, structs, None, parser=parser)
            call_args = ["&input"] + buf_call_args
            call_str = f"{call_func_name}({', '.join(call_args)})"
        else:
            if is_main and not params:
                call_str = f"{call_func_name}()"
            else:
                buf_assignments, call_args, buf_call_args = walk_decls(params, "", callbacks, structs, None, is_param_mode=True, parser=parser)
                call_str = f"{call_func_name}({', '.join(call_args)})"
        
        cb_code = ""
        buf_decls = ""
        for bufname, buflen, _ in callbacks:
            cb_code += generate_decode_callback(bufname, buflen)
            buf_decls += f"    char {bufname}_buf[{buflen}];\n"
        
        if return_type != 'void':
            call_line = f"{return_type} result = {call_str};\n    printf(\"Output: %d\\n\", result);\n    return result;"
        else:
            call_line = f"{call_str};\n    return 0;"
        
        extern_decl = f"extern {return_type} {logic_func}({param_sig});" if not is_main else f"extern {return_type} pin_original_main({param_sig});"
        
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
    parser = "pycparser"
    headers_dir = ""
    if len(sys.argv) > 3:
        for arg in sys.argv[3:]:
            if arg.startswith("--parser="):
                parser = arg.split("=")[1]
            elif arg.startswith("--headers-dir="):
                headers_dir = arg.split("=")[1]
    if len(sys.argv) < 4:
        print("Usage: python generate_wrapper_ast.py <cfile> <logic_func> <pb_base> [--parser=<pycparser|libclang>] [--headers-dir=<dir>]")
        sys.exit(1)
    generate_wrapper(
        sys.argv[1],
        sys.argv[2],
        sys.argv[3],
        sys.argv[4] if len(sys.argv) > 4 else None,
        parser,
        headers_dir
    )