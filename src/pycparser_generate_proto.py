import sys
from pycparser import c_parser, c_ast
import re
import json  # Optional for debugging

TYPE_MAP = {
    'int': 'int32',
    'int32_t': 'int32',
    'long': 'int64',
    'float': 'float',
    'double': 'double',
    'bool': 'bool',
    'char': 'string',  # arrays of char or char* → string
}

def map_type(decl, structs, depth=0, parent_field=''):
    # Arrays
    if isinstance(decl.type, c_ast.ArrayDecl):
        t = decl.type
        base = t.type.type.names[0] if hasattr(t.type.type, 'names') else None
        if base == 'char':
            return 'string'
        elif base in TYPE_MAP:
            return f'repeated {TYPE_MAP[base]}'
        else:
            return f'repeated {base}'
    # Pointers
    elif isinstance(decl.type, c_ast.PtrDecl):
        t = decl.type
        if hasattr(t.type, 'type') and hasattr(t.type.type, 'names'):
            base = t.type.type.names[0]
            if base == 'char':
                return 'string'
            elif base in TYPE_MAP:
                return TYPE_MAP[base]  # Treat as value for now (extend for optional/repeated)
        return 'bytes'  # Fallback for other pointers
    # Primitives or typedefs
    elif isinstance(decl.type, c_ast.TypeDecl):
        t = decl.type
        if isinstance(t.type, c_ast.IdentifierType):
            name = t.type.names[0]
            return TYPE_MAP.get(name, name)
        elif isinstance(t.type, c_ast.Struct):
            structname = t.type.name or f'{parent_field.capitalize()}Struct{depth}' if parent_field else f'AnonymousStruct{depth}'
            structs.append((structname, t.type))
            return structname
    # Direct structs
    elif isinstance(decl.type, c_ast.Struct):
        structname = decl.type.name or f'{parent_field.capitalize()}Struct{depth}' if parent_field else f'AnonymousStruct{depth}'
        structs.append((structname, decl.type))
        return structname
    else:
        return 'bytes'  # Fallback

def get_fields(decls, structs, depth=0, parent_field=''):
    fields = []
    for i, decl in enumerate(decls):
        tname = map_type(decl, structs, depth+1, decl.name or parent_field)
        fname = decl.name or f"field_{i}"
        fields.append((tname, fname))
    return fields

def struct_to_proto(structname, fields):
    lines = [f"message {structname} "+"{"]
    for i, (t, name) in enumerate(fields, 1):
        lines.append(f"  {t} {name} = {i};")
    lines.append("}\n")
    return "\n".join(lines)

class FuncFinder(c_ast.NodeVisitor):
    def __init__(self, func_name):
        self.func_name = func_name
        self.params = None
    def visit_FuncDef(self, node):
        if node.decl.name == self.func_name:
            self.params = node.decl.type.args.params if node.decl.type.args else []

def main(filename, logic_func):
    with open(filename) as f:
        src = f.read()
    parser = c_parser.CParser()
    ast = parser.parse(src)
    
    # Check for structs (typedef struct)
    structs = []
    for ext in ast.ext:
        if isinstance(ext, c_ast.Typedef) and isinstance(ext.type.type, c_ast.Struct):
            structs.append((ext.name, ext.type.type))
    
    all_structs = []
    emitted = set()
    top_structname = None
    top_fields = []
    
    if structs:
        # Struct mode: Process structs as before
        collected_structs = structs[:]
        while collected_structs:
            sname, s = collected_structs.pop(0)
            if sname in emitted:
                continue
            fields = get_fields(s.decls or [], collected_structs, 0)
            all_structs.append((sname, fields))
            emitted.add(sname)
        top_structname = structs[-1][0]  # Last struct as top
    else:
        # Param mode: Find function and use params as fields
        finder = FuncFinder(logic_func)
        finder.visit(ast)
        if finder.params is None:
            print(f"No function '{logic_func}' or structs found. Exiting.")
            sys.exit(1)
        collected_structs = []
        top_fields = get_fields(finder.params, collected_structs, 0)
        top_structname = "Input"
        all_structs.append((top_structname, top_fields))
        # Add any nested structs
        while collected_structs:
            sname, s = collected_structs.pop(0)
            if sname in emitted:
                continue
            fields = get_fields(s.decls or [], collected_structs, 0)
            all_structs.append((sname, fields))
            emitted.add(sname)
    
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
    if len(sys.argv) != 3:
        print("Usage: python pycparser_generate_proto.py <cfile> <logic_func>")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2])