#!/usr/bin/env python3
import argparse
import sys
import os
import re
import tempfile
import pycparser
from pycparser import parse_file, c_ast, c_generator

# path to pycparser’s built-in fake-libc headers
fake_libc = os.path.join(os.path.dirname(pycparser.__file__),
                         'utils', 'fake_libc_include')

def make_autostub_dir(src_path):
    """
    Auto-create empty stubs for every #include in src_path.
    """
    text = open(src_path).read()
    hdrs = re.findall(r'^\s*#\s*include\s+[<"]([^>"]+)[>"]',
                      text, flags=re.M)
    tmp = tempfile.mkdtemp(prefix="nfd-stubs-")
    for hdr in set(hdrs):
        stub = os.path.join(tmp, hdr)
        os.makedirs(os.path.dirname(stub), exist_ok=True)
        open(stub, "w").close()
    return tmp

class FD2NFDTransformer(c_ast.NodeVisitor):
    def __init__(self):
        self.stmts_to_inline = []

    def visit_FuncDef(self, node):
        if node.decl.name == 'main':
            for stmt in node.body.block_items or []:
                if (isinstance(stmt, c_ast.While)
                    and hasattr(stmt.cond, 'name')
                    and stmt.cond.name.startswith('__AFL_LOOP')):
                    self.stmts_to_inline.extend(stmt.stmt.block_items or [])
                else:
                    self.stmts_to_inline.append(stmt)
            return None
        self.generic_visit(node)
        return node

def make_foo_r_ast():
    # Build: double foo_r(const uint8_t *buf, uint64_t len) { return 0.0; }
    type_double = c_ast.TypeDecl('foo_r',
                                 c_ast.IdentifierType(['double']))
    ptr_buf = c_ast.PtrDecl(c_ast.TypeDecl('buf',
                       c_ast.IdentifierType(['uint8_t'])))
    param_buf = c_ast.Decl('buf', [], [], [], ptr_buf, None, None)
    param_len= c_ast.Decl('len', [], [], [],
                   c_ast.TypeDecl('len', c_ast.IdentifierType(['uint64_t'])),
                   None, None)
    func_type = c_ast.FuncDecl(c_ast.ParamList([param_buf, param_len]),
                               type_double)
    decl = c_ast.Decl('foo_r', [], [], [], func_type, None, None)
    ret = c_ast.Return(c_ast.Constant('double', '0.0'))
    return c_ast.FuncDef(decl, None, c_ast.Compound([ret]))

def main():
    p = argparse.ArgumentParser()
    p.add_argument("in_c")
    p.add_argument("out_c")
    args = p.parse_args()

    # auto-stub every include so pycparser can parse without error
    stub_dir = make_autostub_dir(args.in_c)

    ast = parse_file(
        args.in_c,
        use_cpp=True,
        cpp_path='cpp',
        cpp_args=[
            '-nostdinc',
            f'-I{fake_libc}',
            f'-I{stub_dir}',
            '-D__AFL_LOOP(x)=1',
            '-D__AFL_INIT()='
        ]
    )

    # extract main’s body
    transformer = FD2NFDTransformer()
    transformer.visit(ast)

    # build foo_r and splice in the statements
    foo_r_node = make_foo_r_ast()
    comp = foo_r_node.body
    default_ret = comp.block_items.pop()
    comp.block_items.extend(transformer.stmts_to_inline)
    comp.block_items.append(default_ret)

    # only emit foo_r in the new AST
    new_ast = c_ast.FileAST([foo_r_node])

    # pretty-print with necessary includes
    gen = c_generator.CGenerator()
    with open(args.out_c, 'w') as f:
        f.write("// === Normalized driver for CoverMe 2.0 ===\n")
        f.write("#include <stdint.h>\n#include <stddef.h>\n\n")
        f.write(gen.visit(new_ast))
        f.write("\n")

if __name__ == '__main__':
    main()