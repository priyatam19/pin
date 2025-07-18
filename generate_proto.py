# import re
# import sys

# def parse_struct(file_content):
#     """
#     Parse the first struct found in the file.
#     Returns (struct_name, [(type, name), ...])
#     """
#     struct_regex = re.compile(r"typedef\s+struct\s*\{([^}]+)\}\s*(\w+);", re.MULTILINE | re.DOTALL)
#     field_regex = re.compile(r"(\w[\w\s\*]*)\s+([a-zA-Z_]\w*(?:\[\d*\])?);")

#     match = struct_regex.search(file_content)
#     if not match:
#         print("No struct found.")
#         sys.exit(1)
#     fields_block, struct_name = match.groups()
#     fields = []
#     for line in fields_block.split(';'):
#         line = line.strip()
#         if not line: continue
#         m = field_regex.match(line + ';')
#         if m:
#             typ = m.group(1).strip()
#             name = m.group(2).strip()
#             fields.append((typ, name))
#     return struct_name, fields

# def struct_to_proto(struct_name, fields):
#     proto = []
#     proto.append('syntax = "proto3";\n')
#     proto.append(f"message {struct_name} "+"{")
#     field_num = 1
#     for typ, name in fields:
#         # Handle arrays as repeated or string
#         array_match = re.match(r'(\w+)\[(\d*)\]', name)
#         if typ in ["int", "int32_t"]:
#             proto_type = "int32"
#         elif typ in ["float"]:
#             proto_type = "float"
#         elif typ in ["double"]:
#             proto_type = "double"
#         elif typ == "char" and array_match:
#             proto_type = "string"
#             name = array_match.group(1)
#         elif typ == "char*":
#             proto_type = "string"
#         else:
#             proto_type = "bytes"  # fallback for unsupported types
#         proto.append(f"  {proto_type} {name} = {field_num};")
#         field_num += 1
#     proto.append("}\n")
#     return '\n'.join(proto)

# if __name__ == "__main__":
#     if len(sys.argv) != 2:
#         print("Usage: python generate_proto.py <cfile>")
#         sys.exit(1)

#     with open(sys.argv[1], "r") as f:
#         file_content = f.read()
#     struct_name, fields = parse_struct(file_content)
#     proto_str = struct_to_proto(struct_name, fields)
#     out_file = struct_name.lower() + ".proto"
#     with open(out_file, "w") as f:
#         f.write(proto_str)
#     print(f"Wrote proto to {out_file}:\n")
#     print(proto_str)
import re
import sys

def parse_structs(file_content):
    # Regex for top-level and nested structs
    struct_regex = re.compile(
        r"typedef\s+struct\s*{([^}]*)}\s*(\w+)\s*;", re.MULTILINE | re.DOTALL)
    nested_struct_regex = re.compile(
        r"struct\s*{([^}]*)}\s*(\w+)\s*;", re.MULTILINE | re.DOTALL)

    structs = []

    # Parse nested structs first
    for n_match in nested_struct_regex.finditer(file_content):
        fields_block, struct_name = n_match.groups()
        fields = parse_fields(fields_block)
        structs.append({'name': struct_name, 'fields': fields})

    # Parse top-level structs
    for match in struct_regex.finditer(file_content):
        fields_block, struct_name = match.groups()
        fields = parse_fields(fields_block)
        structs.append({'name': struct_name, 'fields': fields})

    return structs

def parse_fields(fields_block):
    # Split fields, ignore empty lines
    lines = [l.strip() for l in fields_block.split(';') if l.strip()]
    fields = []
    for line in lines:
        m = re.match(r"(.*?)([a-zA-Z_]\w*(?:\[[^\]]+\])?)$", line)
        if not m:
            continue
        typ, name = m.groups()
        typ = typ.strip()
        name = name.strip()
        fields.append((typ, name))
    return fields

def map_field(typ, name, struct_names):
    # Check for array
    array_match = re.match(r"([a-zA-Z_]\w*)\[(\d+)\]", name)
    if array_match:
        fname, alen = array_match.groups()
        # String: char field[N] --> string field
        if typ == "char":
            return f"string {fname}"
        # Numeric array: int arr[N] --> repeated int32 arr
        elif typ in ["int", "int32_t"]:
            return f"repeated int32 {fname}"
        elif typ == "float":
            return f"repeated float {fname}"
        elif typ == "double":
            return f"repeated double {fname}"
        else:
            return f"repeated bytes {fname}"
    # Char * or char buf[...]
    if typ.startswith("char") and "*" in typ:
        return f"string {name}"
    # Single primitive
    if typ in ["int", "int32_t"]:
        return f"int32 {name}"
    elif typ == "float":
        return f"float {name}"
    elif typ == "double":
        return f"double {name}"
    # Nested struct
    if typ in struct_names:
        return f"{typ} {name}"
    return f"bytes {name}"

def struct_to_proto(structs):
    struct_names = {s['name'] for s in structs}
    proto = ['syntax = "proto3";\n']
    for struct in structs:
        proto.append(f"message {struct['name']} " + "{")
        field_num = 1
        for typ, name in struct['fields']:
            field_def = map_field(typ, name, struct_names)
            proto.append(f"  {field_def} = {field_num};")
            field_num += 1
        proto.append("}\n")
    return "\n".join(proto)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python generate_proto.py <cfile>")
        sys.exit(1)

    with open(sys.argv[1], "r") as f:
        file_content = f.read()

    structs = parse_structs(file_content)
    proto_str = struct_to_proto(structs)
    # Write the top-level struct as filename
    out_file = structs[-1]['name'].lower() + ".proto"
    with open(out_file, "w") as f:
        f.write(proto_str)
    print(f"Wrote proto to {out_file}:\n")
    print(proto_str)