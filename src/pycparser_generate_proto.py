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
import re
import os
import json
from dataclasses import dataclass, field
from typing import List, Optional

# Conditional imports based on parser selection
try:
    from pycparser import c_parser, c_ast
    PYCPARSER_AVAILABLE = True
except ImportError:
    PYCPARSER_AVAILABLE = False

try:
    from clang.cindex import Index, CursorKind, TypeKind
    LIBCLANG_AVAILABLE = True
except ImportError:
    LIBCLANG_AVAILABLE = False

# Mapping from C primitive types to Protocol Buffer types
FORCE_INTERNAL_TYPEDEFS = set(
    filter(
        None,
        [name.strip() for name in os.environ.get("PIN_FORCE_INTERNAL_TYPES", "").split(",")]
    )
)

TYPE_MAP = {
    'int': 'int32',
    'int32_t': 'int32', 
    'long': 'int64',
    'long long': 'int64',
    'long long int': 'int64',
    'short': 'int32',
    'short int': 'int32',
    'unsigned': 'uint32',
    'unsigned int': 'uint32',
    'unsigned long': 'uint64',
    'unsigned long long': 'uint64',
    'unsigned short': 'uint32',
    'signed': 'int32',
    'signed int': 'int32',
    'signed long': 'int64',
    'signed short': 'int32',
    'int32': 'int32',
    'int64': 'int64',
    'uint8': 'uint32',
    'uint16': 'uint32',
    'uint32': 'uint32',
    'uint64': 'uint64',
    'float': 'float',
    'double': 'double',
    'long double': 'double',
    'bool': 'bool',
    '_Bool': 'bool',
    'char': 'string',  # Single char becomes string for simplicity
    'signed char': 'int32',
    'unsigned char': 'uint32',
    'uint8_t': 'uint32',
    'uint16_t': 'uint32', 
    'uint32_t': 'uint32',
    'uint64_t': 'uint64',
    'int8_t': 'int32',
    'int16_t': 'int32',
    'int64_t': 'int64',
    'size_t': 'uint64',
    'ssize_t': 'int64',
    'ptrdiff_t': 'int64',
    'intptr_t': 'int64',
    'uintptr_t': 'uint64',
    'tmsize_t': 'int64',
    'off_t': 'int64',
    'time_t': 'int64',
    'clock_t': 'int64',
    'pid_t': 'int32',
    'uid_t': 'uint32',
    'gid_t': 'uint32',
    'mode_t': 'uint32',
    'dev_t': 'uint64',
    'ino_t': 'uint64',
    'nlink_t': 'uint64',
    'blksize_t': 'int64',
    'blkcnt_t': 'int64',
    'void': 'bytes',
    'FILE': 'bytes',
    'DIR': 'bytes',
    'string': 'string',
    'bytes': 'bytes',
}

# Standard system types that should be treated as opaque bytes rather than parsed
STANDARD_TYPE_NAMES = set([
    '__int8_t', 'atomic_bool', 'atomic_flag', 'timespec', 'epoll_event',
    'MirConnection', 'MirSurface', 'MirSurfaceSpec', 'MirScreencast',
    'MirPromptSession', 'MirBufferStream', 'MirPersistentId', 'MirBlob',
    'MirDisplayConfig', 'xcb_connection_t', 'mg_mgr', 'mg_connection',
    'mg_addr', 'mg_iobuf', 'mg_dns', 'mg_timer', 'mg_dns_header',
    'mg_dns_rr', 'mg_tcpip_driver_imxrt1020_data', 'mg_tcpip_driver_stm32_data',
    'mg_tcpip_driver_stm32h_data', 'mg_tcpip_driver_tm4c_data',
    'stat', 'option', 'timeval', 'timezone', 'rusage', 'rlimit',
    'sigaction', 'sigset_t', 'stack_t', 'ucontext_t', 'sigevent',
    'pthread_t', 'pthread_attr_t', 'pthread_mutex_t', 'pthread_cond_t',
    'sem_t', 'regex_t', 'regmatch_t', 'wordexp_t', 'glob_t',
    'div_t', 'ldiv_t', 'lldiv_t', 'imaxdiv_t', 'mbstate_t', 'wctrans_t', 'wctype_t'
])

POINTER_FIELD_METADATA = {}
FUNC_PARAM_METADATA = {}
KNOWN_STRUCTS = set()
TYPEDEF_ALIAS_MAP = {}
CURRENT_HEADERS_DIR = ""
POINTER_HELPERS = {}
CURRENT_FUNCTION = None
OPAQUE_STRUCTS = set()


@dataclass
class PointerMetadata:
    depth: int
    base_type: str
    qualifiers: List[str] = field(default_factory=list)
    kind: str = "opaque_ptr"
    proto_hint: Optional[str] = None
    raw: str = ""
    clean_base: Optional[str] = None
    wrapper_name: Optional[str] = None
    length_param: Optional[str] = None
    length_type: Optional[str] = None
    length_proto: Optional[str] = None
    typedef_target: Optional[str] = None
    include_header: Optional[str] = None
    external: bool = False

    @property
    def is_pointer(self):
        return self.depth > 0


@dataclass
class TypeMetadata:
    proto_type: str
    pointer: Optional[PointerMetadata] = None


def strip_qualifiers(type_name):
    """Remove common C qualifiers and normalize whitespace."""
    if not type_name:
        return type_name
    cleaned = re.sub(r'\b(const|volatile|restrict)\b', '', type_name)
    return ' '.join(cleaned.split())


def extract_qualifiers(type_name):
    if not type_name:
        return []
    qualifiers = []
    for qualifier in ("const", "volatile", "restrict"):
        if re.search(rf"\b{qualifier}\b", type_name):
            qualifiers.append(qualifier)
    return qualifiers


def classify_pointer_kind(base_type, depth):
    if depth > 1:
        return 'pointer_chain'
    if base_type == 'char':
        return 'string_ptr'
    if base_type.startswith('struct '):
        return 'struct_ptr'
    if base_type in TYPE_MAP:
        return 'scalar_ptr'
    if base_type == 'void':
        return 'void_ptr'
    return 'opaque_ptr'


def normalize_pointer_proto(base_type, kind):
    if kind == 'string_ptr':
        return 'string'
    if kind == 'scalar_ptr' and base_type in TYPE_MAP:
        return TYPE_MAP[base_type]
    if kind == 'void_ptr':
        return 'bytes'
    if kind == 'struct_ptr':
        return base_type
    if kind == 'pointer_chain':
        return 'bytes'
    return 'bytes'


def to_pascal_case(name):
    tokens = re.split(r'[^0-9a-zA-Z]+', name)
    tokens = [t for t in tokens if t]
    if not tokens:
        return 'Value'
    joined = ''.join(token[:1].upper() + token[1:] for token in tokens)
    if joined and joined[0].isdigit():
        joined = f'N{joined}'
    return joined


def is_integral_proto(proto_name):
    if not proto_name:
        return False
    return proto_name in {
        'int32', 'int64', 'sint32', 'sint64', 'uint32', 'uint64',
        'fixed32', 'fixed64', 'sfixed32', 'sfixed64'
    }


def ensure_pointer_helper(pointer_meta):
    if not pointer_meta or not pointer_meta.is_pointer:
        return None
    if pointer_meta.depth != 1:
        return None
    if getattr(pointer_meta, 'external', False):
        return None

    if pointer_meta.length_param and pointer_meta.kind == 'scalar_ptr':
        base_proto = TYPE_MAP.get(pointer_meta.base_type)
        if not base_proto:
            return None
        length_proto = pointer_meta.length_proto or 'uint32'
        helper_key = ('scalar_slice', base_proto, length_proto)
        if helper_key not in POINTER_HELPERS:
            base_token = to_pascal_case(base_proto)
            helper_name = f'{base_token}Slice'
            POINTER_HELPERS[helper_key] = (
                helper_name,
                [(f'repeated {base_proto}', 'data'), (length_proto, 'length')]
            )
        else:
            helper_name = POINTER_HELPERS[helper_key][0]
        pointer_meta.wrapper_name = helper_name
        pointer_meta.proto_hint = helper_name
        return helper_name

    if pointer_meta.kind == 'scalar_ptr':
        base_proto = TYPE_MAP.get(pointer_meta.base_type)
        if not base_proto:
            return None
        helper_key = ('scalar', base_proto)
        if helper_key not in POINTER_HELPERS:
            base_token = to_pascal_case(base_proto)
            helper_name = f'{base_token}ScalarPtr'
            POINTER_HELPERS[helper_key] = (
                helper_name,
                [('bool', 'has_value'), (base_proto, 'value')]
            )
        else:
            helper_name = POINTER_HELPERS[helper_key][0]
        pointer_meta.wrapper_name = helper_name
        pointer_meta.proto_hint = helper_name
        return helper_name

    if pointer_meta.kind == 'struct_ptr':
        clean_base = pointer_meta.clean_base
        if not clean_base:
            raw_base = pointer_meta.base_type[7:].strip() if pointer_meta.base_type.startswith('struct ') else pointer_meta.base_type
            clean_base = sanitize_struct_name(raw_base)
        if not clean_base or clean_base in STANDARD_TYPE_NAMES:
            return None
        if clean_base.startswith('AnonymousStruct'):
            return None
        if clean_base not in KNOWN_STRUCTS:
            OPAQUE_STRUCTS.add(clean_base)
        helper_key = ('struct', clean_base)
        if helper_key not in POINTER_HELPERS:
            base_token = to_pascal_case(clean_base)
            helper_name = f'{base_token}Ptr'
            value_type = clean_base if clean_base in KNOWN_STRUCTS else 'bytes'
            POINTER_HELPERS[helper_key] = (
                helper_name,
                [('bool', 'present'), (value_type, 'value')]
            )
        else:
            helper_name = POINTER_HELPERS[helper_key][0]
            if clean_base in KNOWN_STRUCTS:
                name, fields = POINTER_HELPERS[helper_key]
                if fields[1][0] == 'bytes':
                    POINTER_HELPERS[helper_key] = (
                        name,
                        [('bool', 'present'), (clean_base, 'value')]
                    )
        pointer_meta.wrapper_name = helper_name
        pointer_meta.clean_base = clean_base
        pointer_meta.proto_hint = helper_name
        return helper_name

    return None


def analyze_pointer_spelling(type_spelling):
    raw = type_spelling.strip()
    qualifiers = extract_qualifiers(raw)
    stripped = strip_qualifiers(raw)
    depth = 0
    base = stripped
    while base.endswith('*'):
        base = base[:-1]
        depth += 1
    base = base.strip()
    if depth == 0:
        return PointerMetadata(depth=0, base_type=base or stripped, qualifiers=qualifiers, kind='none', proto_hint=None, raw=raw)
    kind = classify_pointer_kind(base, depth)
    proto_hint = normalize_pointer_proto(base, kind)
    return PointerMetadata(
        depth=depth,
        base_type=base,
        qualifiers=qualifiers,
        kind=kind,
        proto_hint=proto_hint,
        raw=raw,
    )


def map_libclang_metadata(type_spelling):
    type_str = strip_qualifiers(type_spelling.strip())

    # Handle union types - convert to bytes
    if type_str.startswith('union '):
        print(f"DEBUG: Converting union {type_str} to bytes")
        return TypeMetadata(proto_type='bytes')

    # Handle arrays - convert C arrays to Protocol Buffer repeated fields
    if '[' in type_str and ']' in type_str:
        base_type = strip_qualifiers(type_str.split('[')[0].strip())
        if base_type == 'char':
            return TypeMetadata(proto_type='string')
        mapped_base = TYPE_MAP.get(base_type, 'bytes')
        if mapped_base == 'bytes':
            return TypeMetadata(proto_type='bytes')
        return TypeMetadata(proto_type=f'repeated {mapped_base}')

    # Handle pointers with structured metadata
    if type_str.endswith('*'):
        pointer_meta = analyze_pointer_spelling(type_spelling)
        alias_info = TYPEDEF_ALIAS_MAP.get(pointer_meta.base_type)
        if not alias_info and pointer_meta.base_type.startswith('struct '):
            alias_key = pointer_meta.base_type[7:].strip()
            alias_info = TYPEDEF_ALIAS_MAP.get(alias_key)
        if alias_info:
            pointer_meta.typedef_target = pointer_meta.base_type
            pointer_meta.base_type = f"struct {alias_info['struct']}"
            pointer_meta.kind = 'struct_ptr'
            pointer_meta.clean_base = alias_info['struct']
            pointer_meta.include_header = alias_info.get('header')
            pointer_meta.external = alias_info.get('external', False)
        proto_hint = pointer_meta.proto_hint or 'bytes'
        if pointer_meta.kind == 'struct_ptr':
            struct_name = pointer_meta.base_type[7:].strip() if pointer_meta.base_type.startswith('struct ') else pointer_meta.base_type
            clean_name = sanitize_struct_name(struct_name) if struct_name else struct_name
            pointer_meta.clean_base = clean_name
            proto_hint = 'bytes'
            if clean_name in STANDARD_TYPE_NAMES:
                proto_hint = 'bytes'
        elif pointer_meta.kind == 'scalar_ptr':
            proto_hint = TYPE_MAP.get(pointer_meta.base_type, 'bytes')
        elif pointer_meta.kind == 'string_ptr':
            proto_hint = 'string'
        elif pointer_meta.kind == 'void_ptr':
            proto_hint = 'bytes'
        pointer_meta.proto_hint = proto_hint
        return TypeMetadata(proto_type=proto_hint, pointer=pointer_meta)

    # Handle struct types
    if type_str.startswith('struct '):
        struct_name = type_str[7:].strip()
        if '(unnamed' in struct_name or 'anonymous' in struct_name:
            return TypeMetadata(proto_type='AnonymousStruct')
        if struct_name in STANDARD_TYPE_NAMES:
            return TypeMetadata(proto_type='bytes')
        return TypeMetadata(proto_type=struct_name)

    clean_name = sanitize_struct_name(type_str)
    if type_str in KNOWN_STRUCTS:
        return TypeMetadata(proto_type=type_str)
    if clean_name in KNOWN_STRUCTS:
        return TypeMetadata(proto_type=clean_name)

    # Handle basic types - check TYPE_MAP first
    mapped_type = TYPE_MAP.get(type_str)
    if mapped_type:
        return TypeMetadata(proto_type=mapped_type)

    print(f"DEBUG: Unmapped type {type_str}, using bytes")
    return TypeMetadata(proto_type='bytes')


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
        global CURRENT_FUNCTION
        if CURRENT_FUNCTION and len(decl) > 1:
            ptr_override = FUNC_PARAM_METADATA.get((CURRENT_FUNCTION, decl[1]))
            if ptr_override and ptr_override.is_pointer:
                helper_name = ensure_pointer_helper(ptr_override)
                if helper_name:
                    return helper_name
                if ptr_override.proto_hint:
                    return ptr_override.proto_hint
        type_meta = map_libclang_metadata(decl[0])
        resolved_type = type_meta.proto_type
        if type_meta.pointer and type_meta.pointer.is_pointer:
            helper_name = ensure_pointer_helper(type_meta.pointer)
            if helper_name:
                resolved_type = helper_name
        return resolved_type
        
    # pycparser logic
    if isinstance(decl.type, c_ast.ArrayDecl):
        t = decl.type
        if hasattr(t.type, 'type') and hasattr(t.type.type, 'names'):
            base = ' '.join(t.type.type.names)
        else:
            base = 'unknown'
        if base == 'char':
            return 'string'
        mapped_base = TYPE_MAP.get(base, 'bytes')
        if mapped_base == 'bytes':
            return 'bytes'
        return f'repeated {mapped_base}'
    elif isinstance(decl.type, c_ast.PtrDecl):
        t = decl.type
        if hasattr(t.type, 'type') and hasattr(t.type.type, 'names'):
            base = ' '.join(t.type.type.names)
            if base == 'char':
                return 'string'
            mapped_base = TYPE_MAP.get(base)
            if mapped_base:
                return f'optional {mapped_base}'
        return 'bytes'
    elif isinstance(decl.type, c_ast.TypeDecl):
        t = decl.type
        if isinstance(t.type, c_ast.IdentifierType):
            name = ' '.join(t.type.names)
            mapped_type = TYPE_MAP.get(name)
            if mapped_type:
                return mapped_type
            print(f"DEBUG: Unmapped identifier type {name}, using bytes")
            return 'bytes'
        elif isinstance(t.type, c_ast.Struct):
            structname = t.type.name or f'{parent_field.capitalize()}Struct{depth}' if parent_field else f'AnonymousStruct{depth}'
            KNOWN_STRUCTS.add(structname)
            structs.append((structname, t.type))
            return structname
        elif isinstance(t.type, c_ast.Union):
            print(f"DEBUG: Converting union to bytes")
            return 'bytes'
    elif isinstance(decl.type, c_ast.Struct):
        structname = decl.type.name or f'{parent_field.capitalize()}Struct{depth}' if parent_field else f'AnonymousStruct{depth}'
        KNOWN_STRUCTS.add(structname)
        structs.append((structname, decl.type))
        return structname
    elif isinstance(decl.type, c_ast.Union):
        print(f"DEBUG: Converting union to bytes")
        return 'bytes'
    else:
        print(f"DEBUG: Unknown type {type(decl.type)}, using bytes")
        return 'bytes'

def get_fields(decls, structs, depth=0, parent_field=''):
    fields = []
    for i, decl in enumerate(decls):
        if isinstance(decl, tuple):
            fname = decl[1] or f"param_{i}"
            if CURRENT_FUNCTION:
                ptr_override = FUNC_PARAM_METADATA.get((CURRENT_FUNCTION, fname))
                if ptr_override and getattr(ptr_override, 'include_header', None):
                    print(f"DEBUG: Skipping external pointer param {fname} in proto fields")
                    continue
            tname = map_type(decl, structs, depth+1, decl[1] or parent_field)
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

# Pycparser-specific classes and functions
if PYCPARSER_AVAILABLE:
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
    """Backward-compatible wrapper that returns the proto type string."""
    return map_libclang_metadata(type_spelling).proto_type

def sanitize_struct_name(name):
    """Clean up struct names to be valid protobuf identifiers"""
    if not name or '(unnamed' in name or 'anonymous' in name:
        return 'AnonymousStruct'
    # Remove invalid characters for protobuf
    name = re.sub(r'[^a-zA-Z0-9_]', '_', name)
    if name and not name[0].isalpha():
        name = 'Struct_' + name
    return name

def is_cli_main_function(func_params, parser="pycparser"):
    """
    Detect if function parameters match CLI main signature: int main(int argc, char **argv)
    
    Args:
        func_params: Function parameters (list of AST nodes or tuples)
        parser: Parser type ("pycparser" or "libclang")
        
    Returns:
        bool: True if this looks like a CLI main function
    """
    if len(func_params) != 2:
        return False
        
    if parser == "pycparser":
        # Check for: int argc, char **argv or char *argv[]
        param1, param2 = func_params
        
        # First param should be int argc
        if (isinstance(param1.type, c_ast.TypeDecl) and 
            hasattr(param1.type.type, 'names') and 
            param1.type.type.names == ['int'] and
            param1.name == 'argc'):
            
            # Second param should be char **argv or char *argv[]
            if (param2.name == 'argv' and 
                isinstance(param2.type, c_ast.PtrDecl)):
                # Check for char **argv (ptr to ptr to char)
                if (isinstance(param2.type.type, c_ast.PtrDecl) and
                    isinstance(param2.type.type.type, c_ast.TypeDecl) and
                    hasattr(param2.type.type.type.type, 'names') and
                    param2.type.type.type.type.names == ['char']):
                    return True
                # Check for char *argv[] (array of ptr to char)  
                if (isinstance(param2.type.type, c_ast.ArrayDecl) and
                    isinstance(param2.type.type.type, c_ast.TypeDecl) and
                    hasattr(param2.type.type.type.type, 'names') and
                    param2.type.type.type.type.names == ['char']):
                    return True
                    
    else:  # libclang
        # Check parameter type strings
        param1_type, param1_name = func_params[0]
        param2_type, param2_name = func_params[1]
        
        if (param1_name == 'argc' and param1_type.strip() == 'int' and
            param2_name == 'argv' and 
            (param2_type.strip() == 'char **' or param2_type.strip() == 'char *[]')):
            return True
            
    return False

def generate_cli_proto():
    """
    Generate Protocol Buffer schema for CLI arguments (argc/argv).
    
    Returns:
        str: CLI protobuf schema content
    """
    return """syntax = "proto3";

message CliArgs {
  repeated string args = 1;  // argv[0], argv[1], ..., argv[argc-1]
}
"""

def record_typedef_alias(cursor):
    if cursor.kind != CursorKind.TYPEDEF_DECL:
        return
    try:
        underlying = cursor.underlying_typedef_type
    except AttributeError:
        return
    if not underlying:
        return
    canonical = underlying.get_canonical()
    if not canonical or canonical.kind != TypeKind.RECORD:
        return
    spelling = canonical.spelling or ""
    if not spelling.startswith('struct '):
        return
    struct_name = spelling[7:].strip()
    struct_name = sanitize_struct_name(struct_name)
    if not struct_name or struct_name.startswith('__'):
        return
    header = None
    external = False
    if cursor.location and cursor.location.file:
        header_path = os.path.abspath(str(cursor.location.file))
        headers_root = os.path.abspath(CURRENT_HEADERS_DIR) if CURRENT_HEADERS_DIR else None
        if headers_root and header_path.startswith(headers_root):
            external = True
            header = os.path.relpath(header_path, headers_root)
        else:
            header = os.path.basename(header_path)
    if cursor.spelling in FORCE_INTERNAL_TYPEDEFS:
        external = False
    TYPEDEF_ALIAS_MAP[cursor.spelling] = {
        "struct": struct_name,
        "header": header,
        "external": external,
    }
    KNOWN_STRUCTS.add(struct_name)

def parse_with_libclang(filename, headers_dir="", target_func="main"):
    print(f"DEBUG: Parsing file {filename} with libclang")
    global CURRENT_HEADERS_DIR
    CURRENT_HEADERS_DIR = headers_dir or ""
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
        record_typedef_alias(cursor)
        if cursor.kind == CursorKind.FUNCTION_DECL:
            if cursor.spelling == target_func:
                if not cursor.is_definition():
                    continue
                print(f"DEBUG: Found target function {cursor.spelling}")
                func_args = list(cursor.get_arguments())
                print(f"DEBUG: Function has {len(func_args)} arguments")
                
                # If function has no arguments or only void, return empty structs
                if len(func_args) == 0:
                    print(f"DEBUG: Function {target_func} has no parameters, returning empty structs")
                    return [], []
                
                for param in func_args:
                    type_name = param.type.spelling
                    clean_type = strip_qualifiers(type_name)
                    print(f"DEBUG: Param type: {type_name}")
                    if clean_type.startswith('struct '):
                        struct_name = clean_type[7:].split('[')[0].split('*')[0].strip()
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
                    clean_type = strip_qualifiers(type_name)
                    print(f"DEBUG: Called func {cursor.spelling} param type: {type_name}")
                    if clean_type.startswith('struct '):
                        struct_name = clean_type[7:].split('[')[0].split('*')[0].strip()
                        if struct_name not in STANDARD_TYPE_NAMES and not struct_name.startswith('__'):
                            referenced_structs.add(struct_name)
                            print(f"DEBUG: Added referenced struct {struct_name} from called func")
    
    # Collect structs - only from main file and headers_dir
    print(f"DEBUG: Collecting structs from {filename}")
    import os
    main_file = os.path.abspath(filename)
    for cursor in tu.cursor.walk_preorder():
        if cursor.kind == CursorKind.STRUCT_DECL and cursor.location.file:
            cursor_file = str(cursor.location.file)
            # Include structs from main file, headers_dir, or tmp_structs.c files
            is_main_file = cursor_file == main_file or cursor_file.endswith(os.path.basename(filename))
            is_header_file = headers_dir and headers_dir in cursor_file
            is_temp_file = 'tmp_structs.c' in cursor_file or 'temp_no_pp.c' in cursor_file

            name = cursor.spelling
            clean_name = sanitize_struct_name(name)

            if is_header_file and not is_main_file and not is_temp_file and (clean_name not in referenced_structs):
                print(f"DEBUG: Skipping header struct {cursor.spelling} from {cursor_file}")
                continue
            if (is_main_file or is_temp_file or (is_header_file and clean_name in referenced_structs)):
                print(f"DEBUG: Found struct {name} in {cursor_file}")
                if clean_name and clean_name not in STANDARD_TYPE_NAMES and not clean_name.startswith('__'):
                    alias_info = next((info for info in TYPEDEF_ALIAS_MAP.values() if info.get('struct') == clean_name and info.get('external')), None)
                    if alias_info:
                        print(f"DEBUG: Skipping external struct {clean_name}")
                        continue
                    fields = []
                    for field in cursor.get_children():
                        if field.kind != CursorKind.FIELD_DECL:
                            continue
                        field_meta = map_libclang_metadata(field.type.spelling)
                        resolved_type = field_meta.proto_type
                        if field_meta.pointer and field_meta.pointer.is_pointer:
                            helper_name = ensure_pointer_helper(field_meta.pointer)
                            if helper_name:
                                resolved_type = helper_name
                        fields.append((resolved_type, field.spelling))
                        if field_meta.pointer and field_meta.pointer.is_pointer:
                            POINTER_FIELD_METADATA[(clean_name, field.spelling)] = field_meta.pointer
                            print(f"DEBUG: Recorded pointer metadata for {clean_name}.{field.spelling}: {field_meta.pointer}")
                    KNOWN_STRUCTS.add(clean_name)
                    structs.append((clean_name, fields))
                    print(f"DEBUG: Added struct {clean_name} with fields {fields}")
                else:
                    print(f"DEBUG: Skipped struct {name} (filtered out)")
            else:
                print(f"DEBUG: Skipped struct {cursor.spelling} from {cursor_file} (not in main file or headers dir)")
    
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
                    record_typedef_alias(cursor)
                    if cursor.kind == CursorKind.STRUCT_DECL:
                        name = cursor.spelling
                        print(f"DEBUG: Found header struct {name}")
                        clean_name = sanitize_struct_name(name)
                        if clean_name and clean_name not in STANDARD_TYPE_NAMES and not clean_name.startswith('__'):
                            alias_info = next((info for info in TYPEDEF_ALIAS_MAP.values() if info.get('struct') == clean_name and info.get('external')), None)
                            if alias_info:
                                print(f"DEBUG: Skipping external header struct {clean_name}")
                                continue
                            print(f"DEBUG: Skipping header struct {clean_name} to avoid duplication")
    
    # Collect function parameters
    params = []
    param_entries = []
    for cursor in tu.cursor.walk_preorder():
        if cursor.kind == CursorKind.FUNCTION_DECL and cursor.spelling == target_func:
            if not cursor.is_definition():
                continue
            for i, param in enumerate(cursor.get_arguments()):
                param_name = param.spelling or f"param_{i}"
                param_meta = map_libclang_metadata(param.type.spelling)
                params.append((param.type.spelling, param_name))
                param_entries.append((param.type.spelling, param_name, param_meta))
            print(f"DEBUG: Function {target_func} params: {params}")
            break

    # Detect pointer-length pairs using simple heuristics
    size_like_names = {'len', 'length', 'size', 'count', 'num', 'n'}

    def looks_like_length(name):
        if not name:
            return False
        lowered = name.lower()
        if lowered in size_like_names:
            return True
        tokens = []
        for part in re.split(r'[_]', name):
            tokens.extend([tok.lower() for tok in re.findall(r'[A-Za-z]+', part)])
        if tokens:
            if tokens[0] in size_like_names or tokens[-1] in size_like_names:
                return True
        if any(lowered.endswith('_' + suffix) for suffix in size_like_names):
            return True
        return False

    for idx, (type_str, name, meta) in enumerate(param_entries):
        ptr_meta = meta.pointer
        if not ptr_meta or not ptr_meta.is_pointer:
            continue
        if ptr_meta.kind not in ('scalar_ptr', 'struct_ptr') or ptr_meta.depth != 1:
            continue
        if idx + 1 >= len(param_entries):
            continue
        next_type, next_name, next_meta = param_entries[idx + 1]
        if next_meta.pointer and next_meta.pointer.is_pointer:
            continue
        if not looks_like_length(next_name):
            continue
        if not is_integral_proto(next_meta.proto_type):
            continue
        ptr_meta.length_param = next_name
        ptr_meta.length_type = next_type
        ptr_meta.length_proto = next_meta.proto_type
        print(f"DEBUG: Detected length companion {next_name} for pointer {name}")

    for type_str, name, meta in param_entries:
        ptr_meta = meta.pointer
        if ptr_meta and ptr_meta.is_pointer:
            ensure_pointer_helper(ptr_meta)
            FUNC_PARAM_METADATA[(target_func, name)] = ptr_meta
            print(f"DEBUG: Recorded pointer metadata for param {target_func}.{name}: {ptr_meta}")

    return structs, params

def main(filename, logic_func, parser="pycparser", headers_dir=""):
    print(f"DEBUG: Starting main with file {filename}, func {logic_func}, parser {parser}, headers {headers_dir}")
    global CURRENT_FUNCTION
    CURRENT_FUNCTION = logic_func
    POINTER_FIELD_METADATA.clear()
    FUNC_PARAM_METADATA.clear()
    POINTER_HELPERS.clear()
    TYPEDEF_ALIAS_MAP.clear()
    OPAQUE_STRUCTS.clear()
    structs = []
    global CURRENT_HEADERS_DIR
    CURRENT_HEADERS_DIR = headers_dir or ""
    ast = None
    referenced_structs = set()
    called_funcs = set()
    primary_input_struct = None
    func_params = []
    is_cli_main = False  # Initialize CLI detection flag
    
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
                    KNOWN_STRUCTS.add(name)
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
        
        # Check if this is a CLI main function
        is_cli_main = is_cli_main_function(func_params, parser)
        print(f"DEBUG: CLI main function detected: {is_cli_main}")
    
    elif parser == "libclang":
        structs, func_params = parse_with_libclang(filename, headers_dir, logic_func)
        
        # Check if this is a CLI main function
        is_cli_main = is_cli_main_function(func_params, parser)
        print(f"DEBUG: CLI main function detected: {is_cli_main}")
        
        # Find primary input struct
        index = Index.create()
        args = ['-I' + headers_dir] if headers_dir else []
        tu = index.parse(filename, args=args)
        for cursor in tu.cursor.walk_preorder():
            if cursor.kind == CursorKind.FUNCTION_DECL and cursor.spelling == logic_func:
                for param in cursor.get_arguments():
                    type_name = param.type.spelling
                    clean_type = strip_qualifiers(type_name)
                    if clean_type.startswith('struct '):
                        struct_name = clean_type[7:].split('[')[0].split('*')[0].strip()
                        if struct_name not in STANDARD_TYPE_NAMES and not struct_name.startswith('__'):
                            primary_input_struct = struct_name
                            print(f"DEBUG: Primary input struct from libclang params: {primary_input_struct}")
                            break
            elif cursor.kind == CursorKind.FUNCTION_DECL and cursor.spelling in called_funcs:
                for param in cursor.get_arguments():
                    type_name = param.type.spelling
                    clean_type = strip_qualifiers(type_name)
                    if clean_type.startswith('struct '):
                        struct_name = clean_type[7:].split('[')[0].split('*')[0].strip()
                        if struct_name not in STANDARD_TYPE_NAMES and not struct_name.startswith('__'):
                            primary_input_struct = struct_name
                            print(f"DEBUG: Primary input struct from libclang called func: {primary_input_struct}")
                            break
    
    all_structs = []
    emitted = set()
    top_structname = "Input"
    
    # Handle CLI main functions specially
    if is_cli_main:
        print("DEBUG: Generating CLI protobuf schema")
        proto_content = generate_cli_proto()
        proto_filename = "cliargs.proto"
        print(f"Wrote proto to {proto_filename}:")
        print(proto_content)
        
        with open(proto_filename, 'w') as f:
            f.write(proto_content)
            
        # Generate CLI test input
        test_args = ["./program", "--help", "test.txt"]  # Example CLI args
        print(f"Generated CLI args: {test_args}")
        
        # Create input generation script
        gen_script = f'''#!/usr/bin/env python3
import cliargs_pb2
import sys

# Create CLI args message
cli_args = cliargs_pb2.CliArgs()
cli_args.args.extend({test_args})

# Serialize to binary
with open("input.bin", "wb") as f:
    f.write(cli_args.SerializeToString())

print("Generated input.bin with CLI args:", {test_args})
'''
        
        with open('gen_input.py', 'w') as f:
            f.write(gen_script)
            
        return  # Early return for CLI case
    
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
            print(f"DEBUG: Primary input struct {primary_input_struct} available for wrapper")
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
    
    # Prepend pointer helper messages so they are available to referencing structs
    input_fields = []
    if primary_input_struct and any(sname == primary_input_struct for sname, _ in all_structs):
        input_fields = [(primary_input_struct, 'input')]
    elif func_params:
        input_fields = get_fields([(t, n) for t, n in func_params], [], 0)
    all_structs = [(top_structname, input_fields)] + [item for item in all_structs if item[0] != top_structname]

    helper_structs = [(name, fields) for name, fields in POINTER_HELPERS.values()]
    if helper_structs:
        if all_structs:
            all_structs = [all_structs[0]] + helper_structs + all_structs[1:]
        else:
            all_structs = helper_structs

    existing_names = {name for name, _ in all_structs}
    for opaque_name in sorted(OPAQUE_STRUCTS):
        if opaque_name not in existing_names and opaque_name not in STANDARD_TYPE_NAMES:
            all_structs.append((opaque_name, [('bytes', 'raw')]))

    # Proto output
    lines = ['syntax = "proto3";\n']
    for sname, fields in all_structs:
        lines.append(struct_to_proto(sname, fields))
    proto_str = "\n".join(lines)
    
    out_file = top_structname.lower() + ".proto"
    with open(out_file, "w") as f:
        f.write(proto_str)
    metadata = {
        "typedef_aliases": TYPEDEF_ALIAS_MAP,
    }
    with open("pin_pointer_metadata.json", "w") as meta_file:
        json.dump(metadata, meta_file, indent=2)
    print(f"Wrote proto to {out_file}:\n")
    print(proto_str)
    CURRENT_FUNCTION = None

if __name__ == "__main__":
    # Default to libclang for broader C/C++ support
    parser = "libclang"
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
    
    # Check parser availability; fall back if needed
    if parser == "pycparser" and not PYCPARSER_AVAILABLE:
        print("Error: pycparser is not installed. Please install with: pip install pycparser")
        sys.exit(1)
    elif parser == "libclang" and not LIBCLANG_AVAILABLE:
        print("Warning: libclang Python bindings not available; falling back to pycparser")
        parser = "pycparser"
        if not PYCPARSER_AVAILABLE:
            print("Error: Neither libclang nor pycparser available. Install one: pip install libclang or pip install pycparser")
            sys.exit(1)
    
    main(sys.argv[1], sys.argv[2], parser, headers_dir)
