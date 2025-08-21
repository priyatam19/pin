#!/usr/bin/env python3
"""
PIN Protocol Buffer Generator

This module parses C source files and generates corresponding Protocol Buffer (.proto) 
schemas for struct normalization. It supports both pycparser and libclang parsing backends.

Key Features:
- Extracts struct definitions from C code
- Maps C types to Protocol Buffer equivalents  
- Handles nested structs, arrays, and anonymous structs
- Supports function parameter analysis for input struct detection
- Generates random test data for validation

Author: PIN Development Team
"""

import sys
from pycparser import c_parser, c_ast
import re
import os
from clang.cindex import Index, CursorKind

# Mapping from C primitive types to Protocol Buffer types
TYPE_MAP = {
    'int': 'int32',
    'int32_t': 'int32', 
    'long': 'int64',
    'float': 'float',
    'double': 'double',
    'bool': 'bool',
    'char': 'string',  # Single char becomes string for simplicity
    'uint8_t': 'uint32',
    'uint16_t': 'uint32', 
    'uint32_t': 'uint32',
    'size_t': 'uint64',
}

# Standard system types that should be treated as opaque bytes rather than parsed
STANDARD_TYPE_NAMES = set([
    '__int8_t', 'atomic_bool', 'atomic_flag', 'timespec', 'epoll_event',
    'MirConnection', 'MirSurface', 'MirSurfaceSpec', 'MirScreencast',
    'MirPromptSession', 'MirBufferStream', 'MirPersistentId', 'MirBlob',
    'MirDisplayConfig', 'xcb_connection_t', 'mg_mgr', 'mg_connection',
    'mg_addr', 'mg_iobuf', 'mg_dns', 'mg_timer', 'mg_dns_header',
    'mg_dns_rr', 'mg_tcpip_driver_imxrt1020_data', 'mg_tcpip_driver_stm32_data',
    'mg_tcpip_driver_stm32h_data', 'mg_tcpip_driver_tm4c_data'
])

def map_type(decl, structs, depth=0, parent_field=''):
    """
    Map C type declarations to Protocol Buffer field types.
    
    This function handles the complex conversion between C type system and Protocol Buffer
    type system, including special cases for arrays, pointers, and nested structs.
    
    Args:
        decl: Type declaration (pycparser AST node or libclang tuple)
        structs: List to collect nested struct definitions
        depth: Recursion depth for nested struct handling
        parent_field: Parent field name for context in nested structs
        
    Returns:
        str: Protocol Buffer field type (e.g., 'int32', 'string', 'repeated double')
    """
    print(f"DEBUG: Mapping type for decl: {decl}")
    if isinstance(decl, tuple):  # libclang: (type_name, field_name)
        type_name = decl[0].strip()
        if type_name.startswith('struct '):
            struct_name = type_name[7:].split('[')[0].split('*')[0].strip()
            if struct_name in STANDARD_TYPE_NAMES:
                return 'bytes'
            structs.append((struct_name, []))
            return struct_name
        if '[]' in type_name:
            base = type_name.replace('[]', '').strip()
            if base == 'char':
                return 'string'
            return f'repeated {TYPE_MAP.get(base, base)}'
        if '*' in type_name:
            base = type_name.replace('*', '').strip()
            if base == 'char':
                return 'string'
            if base in TYPE_MAP:
                return f'optional {TYPE_MAP[base]}'
            return 'bytes'
        return TYPE_MAP.get(type_name, type_name)
    # pycparser logic
    if isinstance(decl.type, c_ast.ArrayDecl):
        t = decl.type
        base = t.type.type.names[0] if hasattr(t.type.type, 'names') else None
        if base == 'char':
            return 'string'
        elif base in TYPE_MAP:
            return f'repeated {TYPE_MAP[base]}'
        else:
            return f'repeated {base}'
    elif isinstance(decl.type, c_ast.PtrDecl):
        t = decl.type
        if hasattr(t.type, 'type') and hasattr(t.type.type, 'names'):
            base = t.type.type.names[0]
            if base == 'char':
                return 'string'
            elif base in TYPE_MAP:
                return f'optional {TYPE_MAP[base]}'
        return 'bytes'
    elif isinstance(decl.type, c_ast.TypeDecl):
        t = decl.type
        if isinstance(t.type, c_ast.IdentifierType):
            name = t.type.names[0]
            return TYPE_MAP.get(name, name)
        elif isinstance(t.type, c_ast.Struct):
            structname = t.type.name or f'{parent_field.capitalize()}Struct{depth}' if parent_field else f'AnonymousStruct{depth}'
            structs.append((structname, t.type))
            return structname
    elif isinstance(decl.type, c_ast.Struct):
        structname = decl.type.name or f'{parent_field.capitalize()}Struct{depth}' if parent_field else f'AnonymousStruct{depth}'
        structs.append((structname, t.type))
        return structname
    else:
        return 'bytes'

def get_fields(decls, structs, depth=0, parent_field=''):
    fields = []
    for i, decl in enumerate(decls):
        if isinstance(decl, tuple):
            tname = map_type(decl, structs, depth+1, decl[1] or parent_field)
            fname = decl[1] or f"param_{i}"
            fields.append((tname, fname))
        else:
            tname = map_type(decl, structs, depth+1, decl.name or parent_field)
            fname = decl.name or f"param_{i}"
            fields.append((tname, fname))
    return fields

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

def struct_to_proto(structname, fields):
    """
    Convert a C struct definition to Protocol Buffer message syntax.
    
    Args:
        structname (str): Name of the struct/message
        fields (list): List of (type, field_name) tuples
        
    Returns:
        str: Protocol Buffer message definition
    """
    lines = [f"message {structname} "+"{"]
    for i, (t, name) in enumerate(fields, 1):
        t_clean = t.replace('struct ', '')  # Remove 'struct' prefix for proto fields
        lines.append(f"  {t_clean} {name} = {i};")
    lines.append("}\n")
    return "\n".join(lines)

class FuncFinder(c_ast.NodeVisitor):
    def __init__(self, func_name):
        self.func_name = func_name
        self.params = None
        self.referenced_structs = set()
        self.called_funcs = set()
    def visit_FuncDef(self, node):
        if node.decl.name == self.func_name:
            self.params = node.decl.type.args.params if node.decl.type.args else []
            print(f"DEBUG: Found function {self.func_name} with params: {[p.name for p in self.params]}")
            for param in self.params:
                if isinstance(param.type, c_ast.TypeDecl) and isinstance(param.type.type, c_ast.Struct):
                    self.referenced_structs.add(param.type.type.name)
                elif isinstance(param.type, c_ast.PtrDecl) and isinstance(param.type.type.type, c_ast.Struct):
                    self.referenced_structs.add(param.type.type.type.name)
        if node.body:
            for item in node.body.block_items or []:
                if isinstance(item, c_ast.FuncCall) and item.name.name:
                    self.called_funcs.add(item.name.name)
                    print(f"DEBUG: Found called function {item.name.name}")
    def visit_Decl(self, node):
        if isinstance(node.type, c_ast.TypeDecl) and isinstance(node.type.type, c_ast.Struct):
            self.referenced_structs.add(node.type.type.name)
            print(f"DEBUG: Found struct decl {node.type.type.name}")
        elif isinstance(node.type, c_ast.PtrDecl) and isinstance(node.type.type.type, c_ast.Struct):
            self.referenced_structs.add(node.type.type.type.name)
            print(f"DEBUG: Found struct ptr decl {node.type.type.type.name}")

def map_libclang_type(type_spelling):
    """
    Map libclang type spelling to Protocol Buffer type.
    
    This is a specialized version of map_type() for libclang parsed types.
    It handles the string format that libclang provides for type information.
    
    Args:
        type_spelling (str): Raw type string from libclang (e.g., 'char[32]', 'int*')
        
    Returns:
        str: Protocol Buffer field type
    """
    type_str = type_spelling.strip()
    
    # Handle arrays - convert C arrays to Protocol Buffer repeated fields
    if '[' in type_str and ']' in type_str:
        base_type = type_str.split('[')[0].strip()
        if base_type == 'char':
            return 'string'  # char arrays become strings
        elif base_type in TYPE_MAP:
            return f'repeated {TYPE_MAP[base_type]}'
        else:
            return f'repeated {base_type}'
    
    # Handle pointers - special case for char* as strings
    if type_str.endswith('*'):
        base_type = type_str.rstrip('*').strip()
        if base_type == 'char':
            return 'string'  # char* becomes string
        elif base_type in TYPE_MAP:
            return TYPE_MAP[base_type]
        else:
            return base_type
    
    # Handle struct types
    if type_str.startswith('struct '):
        struct_name = type_str[7:].strip()
        # Clean up anonymous struct names
        if '(unnamed' in struct_name or 'anonymous' in struct_name:
            return 'AnonymousStruct'
        return struct_name
    
    # Handle basic types
    if type_str in TYPE_MAP:
        return TYPE_MAP[type_str]
    
    return type_str

def sanitize_struct_name(name):
    """Clean up struct names to be valid protobuf identifiers"""
    if not name or '(unnamed' in name or 'anonymous' in name:
        return 'AnonymousStruct'
    # Remove invalid characters for protobuf
    name = re.sub(r'[^a-zA-Z0-9_]', '_', name)
    if name and not name[0].isalpha():
        name = 'Struct_' + name
    return name

def parse_with_libclang(filename, headers_dir="", target_func="main"):
    print(f"DEBUG: Parsing file {filename} with libclang")
    index = Index.create()
    args = ['-I' + headers_dir] if headers_dir else []
    print(f"DEBUG: Libclang args: {args}")
    tu = index.parse(filename, args=args)
    if not tu:
        print(f"DEBUG: Failed to parse translation unit for {filename}")
        return [], []
    structs = []
    referenced_structs = set()
    called_funcs = set()
    
    # Find structs used in target function or called functions
    print(f"DEBUG: Scanning for function {target_func}")
    for cursor in tu.cursor.walk_preorder():
        if cursor.kind == CursorKind.FUNCTION_DECL:
            if cursor.spelling == target_func:
                print(f"DEBUG: Found target function {cursor.spelling}")
                for param in cursor.get_arguments():
                    type_name = param.type.spelling
                    print(f"DEBUG: Param type: {type_name}")
                    if type_name.startswith('struct '):
                        struct_name = type_name[7:].split('[')[0].split('*')[0].strip()
                        if struct_name not in STANDARD_TYPE_NAMES and not struct_name.startswith('__'):
                            referenced_structs.add(struct_name)
                            print(f"DEBUG: Added referenced struct {struct_name}")
                for child in cursor.walk_preorder():
                    if child.kind == CursorKind.CALL_EXPR:
                        called_funcs.add(child.spelling)
                        print(f"DEBUG: Found called function {child.spelling}")
            elif cursor.spelling in called_funcs:
                for param in cursor.get_arguments():
                    type_name = param.type.spelling
                    print(f"DEBUG: Called func {cursor.spelling} param type: {type_name}")
                    if type_name.startswith('struct '):
                        struct_name = type_name[7:].split('[')[0].split('*')[0].strip()
                        if struct_name not in STANDARD_TYPE_NAMES and not struct_name.startswith('__'):
                            referenced_structs.add(struct_name)
                            print(f"DEBUG: Added referenced struct {struct_name} from called func")
    
    # Collect structs
    print(f"DEBUG: Collecting structs from {filename}")
    for cursor in tu.cursor.walk_preorder():
        if cursor.kind == CursorKind.STRUCT_DECL:
            name = cursor.spelling
            print(f"DEBUG: Found struct {name}")
            clean_name = sanitize_struct_name(name)
            if clean_name and clean_name not in STANDARD_TYPE_NAMES and not clean_name.startswith('__'):
                fields = [(map_libclang_type(field.type.spelling), field.spelling) for field in cursor.get_children() if field.kind == CursorKind.FIELD_DECL]
                structs.append((clean_name, fields))
                print(f"DEBUG: Added struct {clean_name} with fields {fields}")
    
    # Parse headers
    if headers_dir:
        print(f"DEBUG: Parsing headers in {headers_dir}")
        for header in os.listdir(headers_dir):
            if header.endswith('.h'):
                h_path = os.path.join(headers_dir, header)
                print(f"DEBUG: Parsing header {h_path}")
                h_tu = index.parse(h_path, args=args)
                if not h_tu:
                    print(f"DEBUG: Failed to parse header {h_path}")
                    continue
                for cursor in h_tu.cursor.walk_preorder():
                    if cursor.kind == CursorKind.STRUCT_DECL:
                        name = cursor.spelling
                        print(f"DEBUG: Found header struct {name}")
                        clean_name = sanitize_struct_name(name)
                        if clean_name and clean_name not in STANDARD_TYPE_NAMES and not clean_name.startswith('__'):
                            fields = [(map_libclang_type(field.type.spelling), field.spelling) for field in cursor.get_children() if field.kind == CursorKind.FIELD_DECL]
                            structs.append((clean_name, fields))
                            print(f"DEBUG: Added header struct {clean_name} with fields {fields}")
    
    # Collect function parameters
    params = []
    for cursor in tu.cursor.walk_preorder():
        if cursor.kind == CursorKind.FUNCTION_DECL and cursor.spelling == target_func:
            params = [(param.type.spelling, param.spelling or f"param_{i}") for i, param in enumerate(cursor.get_arguments())]
            print(f"DEBUG: Function {target_func} params: {params}")
            break
    
    return structs, params

def main(filename, logic_func, parser="pycparser", headers_dir=""):
    print(f"DEBUG: Starting main with file {filename}, func {logic_func}, parser {parser}, headers {headers_dir}")
    structs = []
    ast = None
    referenced_structs = set()
    called_funcs = set()
    primary_input_struct = None
    func_params = []
    
    if parser == "pycparser":
        print(f"DEBUG: Parsing file {filename} with pycparser")
        parser_obj = c_parser.CParser()
        with open(filename) as f:
            src = f.read()
        try:
            ast = parser_obj.parse(src)
        except Exception as e:
            print(f"DEBUG: Pycparser parsing failed: {e}")
            sys.exit(1)
        
        # Parse included headers
        includes = re.findall(r'#include\s*"(.*?)"', src)
        for inc in includes:
            inc_path = os.path.join(headers_dir, inc) if headers_dir else inc
            if os.path.exists(inc_path):
                print(f"DEBUG: Parsing header {inc_path}")
                with open(inc_path) as f:
                    inc_ast = parser_obj.parse(f.read())
                ast.ext.extend(inc_ast.ext)
        
        # Collect referenced structs and called functions
        finder = FuncFinder(logic_func)
        finder.visit(ast)
        referenced_structs = finder.referenced_structs
        called_funcs = finder.called_funcs
        print(f"DEBUG: Pycparser referenced structs: {referenced_structs}, called funcs: {called_funcs}")
        
        # Check for structs
        for ext in ast.ext:
            if isinstance(ext, c_ast.Typedef) and isinstance(ext.type.type, c_ast.Struct):
                name = ext.name
                if name not in STANDARD_TYPE_NAMES and not name.startswith('__'):
                    structs.append((name, ext.type.type))
                    print(f"DEBUG: Added pycparser struct {name}")
        
        # Find primary input struct
        if finder.params:
            for param in finder.params:
                if isinstance(param.type, c_ast.PtrDecl) and isinstance(param.type.type.type, c_ast.Struct):
                    primary_input_struct = param.type.type.type.name
                    print(f"DEBUG: Primary input struct from params: {primary_input_struct}")
                    break
        if not primary_input_struct and called_funcs:
            for func in called_funcs:
                for ext in ast.ext:
                    if isinstance(ext, c_ast.FuncDef) and ext.decl.name == func:
                        for param in ext.decl.type.args.params if ext.decl.type.args else []:
                            if isinstance(param.type, c_ast.PtrDecl) and isinstance(param.type.type.type, c_ast.Struct):
                                primary_input_struct = param.type.type.type.name
                                print(f"DEBUG: Primary input struct from called func {func}: {primary_input_struct}")
                                break
                    if primary_input_struct:
                        break
        func_params = finder.params or []
        print(f"DEBUG: Pycparser function params: {[(get_type_str(p.type, 'pycparser'), p.name) for p in func_params if p.name]}")
    
    elif parser == "libclang":
        structs, func_params = parse_with_libclang(filename, headers_dir, logic_func)
        # Find primary input struct
        index = Index.create()
        args = ['-I' + headers_dir] if headers_dir else []
        tu = index.parse(filename, args=args)
        for cursor in tu.cursor.walk_preorder():
            if cursor.kind == CursorKind.FUNCTION_DECL and cursor.spelling == logic_func:
                for param in cursor.get_arguments():
                    type_name = param.type.spelling
                    if type_name.startswith('struct '):
                        struct_name = type_name[7:].split('[')[0].split('*')[0].strip()
                        if struct_name not in STANDARD_TYPE_NAMES and not struct_name.startswith('__'):
                            primary_input_struct = struct_name
                            print(f"DEBUG: Primary input struct from libclang params: {primary_input_struct}")
                            break
            elif cursor.kind == CursorKind.FUNCTION_DECL and cursor.spelling in called_funcs:
                for param in cursor.get_arguments():
                    type_name = param.type.spelling
                    if type_name.startswith('struct '):
                        struct_name = type_name[7:].split('[')[0].split('*')[0].strip()
                        if struct_name not in STANDARD_TYPE_NAMES and not struct_name.startswith('__'):
                            primary_input_struct = struct_name
                            print(f"DEBUG: Primary input struct from libclang called func: {primary_input_struct}")
                            break
    
    all_structs = []
    emitted = set()
    top_structname = "Input"
    
    if structs:
        referenced_structs = set([name for name, _ in structs])
        filtered_structs = [(name, s) for name, s in structs if name in referenced_structs]
        collected_structs = filtered_structs[:]
        while collected_structs:
            sname, s = collected_structs.pop(0)
            if sname in emitted:
                continue
            if isinstance(s, list):
                fields = get_fields(s, collected_structs, 0)
            else:
                fields = get_fields(s.decls or [], collected_structs, 0)
            all_structs.append((sname, fields))
            emitted.add(sname)
        if primary_input_struct and primary_input_struct in emitted:
            all_structs.append(('Input', [(primary_input_struct, 'input')]))
            print(f"DEBUG: Added Input wrapper for {primary_input_struct}")
        else:
            top_structname = filtered_structs[-1][0] if filtered_structs else "Input"
            print(f"DEBUG: No primary input struct, using {top_structname}")
    else:
        if parser == "pycparser":
            finder = FuncFinder(logic_func)
            finder.visit(ast)
            if finder.params is None:
                print(f"DEBUG: No function '{logic_func}' found, generating empty Input message")
                all_structs.append(('Input', []))  # Empty Input for parameter-less functions
            else:
                collected_structs = []
                top_fields = get_fields(finder.params, collected_structs, 0)
                all_structs.append((top_structname, top_fields))
                while collected_structs:
                    sname, s = collected_structs.pop(0)
                    if sname in emitted:
                        continue
                    fields = get_fields(s.decls or [], collected_structs, 0)
                    all_structs.append((sname, fields))
                    emitted.add(sname)
        elif func_params:
            top_fields = get_fields([(t, n) for t, n in func_params], [], 0)
            all_structs.append((top_structname, top_fields))
            print(f"DEBUG: Using function params for Input: {top_fields}")
        else:
            print(f"DEBUG: No structs or params found, generating empty Input message")
            all_structs.append(('Input', []))  # Empty Input for parameter-less functions
    
    # Proto output
    lines = ['syntax = "proto3";\n']
    for sname, fields in all_structs:
        lines.append(struct_to_proto(sname, fields))
    proto_str = "\n".join(lines)
    
    out_file = top_structname.lower() + ".proto"
    with open(out_file, "w") as f:
        f.write(proto_str)
    print(f"Wrote proto to {out_file}:\n")
    print(proto_str)

if __name__ == "__main__":
    parser = "pycparser"
    headers_dir = ""
    if len(sys.argv) > 3:
        for arg in sys.argv[3:]:
            if arg.startswith("--parser="):
                parser = arg.split("=")[1]
            elif arg.startswith("--headers-dir="):
                headers_dir = arg.split("=")[1]
    if len(sys.argv) < 3:
        print("Usage: python pycparser_generate_proto.py <cfile> <logic_func> [--parser=<pycparser|libclang>] [--headers-dir=<dir>]")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2], parser, headers_dir)