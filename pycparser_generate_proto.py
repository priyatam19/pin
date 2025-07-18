import sys
from pycparser import c_parser, c_ast, parse_file

TYPE_MAP = {
    'int': 'int32',
    'int32_t': 'int32',
    'float': 'float',
    'double': 'double',
    'char': 'string',  # arrays of char → string
}
import re

def extract_typedef_structs(c_code):
    # Find all 'typedef struct { ... } Name;' blocks, multiline
    # This will NOT match weird nested/anonymous cases, but is robust for almost all practical typedefs
    pattern = re.compile(r'typedef\s+struct\s*{[^}]*}\s*\w+\s*;', re.DOTALL)
    structs = pattern.findall(c_code)
    return '\n\n'.join(structs)

def map_type(decl, structs, depth=0):
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
    # Structs
    elif isinstance(decl.type, c_ast.TypeDecl):
        t = decl.type
        if isinstance(t.type, c_ast.IdentifierType):
            name = t.type.names[0]
            return TYPE_MAP.get(name, name)
        elif isinstance(t.type, c_ast.Struct):
            structname = t.type.name or f'AnonymousStruct{depth}'
            structs.append((structname, t.type))
            return structname
    elif isinstance(decl.type, c_ast.PtrDecl):
        t = decl.type
        if hasattr(t.type, 'type') and hasattr(t.type.type, 'names'):
            if t.type.type.names[0] == 'char':
                return 'string'
        return 'bytes'
    elif isinstance(decl.type, c_ast.Struct):
        structname = decl.type.name or f'AnonymousStruct{depth}'
        structs.append((structname, decl.type))
        return structname
    else:
        return 'bytes'

def get_struct_fields(struct, structs, depth=0):
    fields = []
    if struct.decls:
        for i, decl in enumerate(struct.decls):
            tname = map_type(decl, structs, depth+1)
            fname = decl.name or f"field_{i}"
            fields.append((tname, fname))
    return fields

def struct_to_proto(structname, fields):
    lines = [f"message {structname} "+"{"]
    for i, (t, name) in enumerate(fields, 1):
        lines.append(f"  {t} {name} = {i};")
    lines.append("}\n")
    return "\n".join(lines)

def main(filename):
    with open(filename) as f:
        src = f.read()
    src = extract_typedef_structs(src)
    if not src.strip():
        print("No typedef structs found in input file. Exiting.")
        sys.exit(1)
    parser = c_parser.CParser()
    ast = parser.parse(src)
    structs = []
    # Collect all struct typedefs
    for ext in ast.ext:
        if isinstance(ext, c_ast.Typedef):
            if isinstance(ext.type.type, c_ast.Struct):
                structs.append((ext.name, ext.type.type))

    all_structs = []
    emitted = set()
    while structs:
        structname, struct = structs.pop(0)
        if structname in emitted:
            continue
        fields = get_struct_fields(struct, structs)
        all_structs.append((structname, fields))
        emitted.add(structname)

    # Proto output
    lines = ['syntax = "proto3";\n']
    for structname, fields in all_structs:
        lines.append(struct_to_proto(structname, fields))
    proto_str = "\n".join(lines)

    out_file = structname.lower() + ".proto"
    with open(out_file, "w") as f:
        f.write(proto_str)
    print(f"Wrote proto to {out_file}:\n")
    print(proto_str)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python pycparser_generate_proto.py <cfile>")
        sys.exit(1)
    main(sys.argv[1])