#!/usr/bin/env python3
"""
generate_intelligent_seeds.py
================================

Generate a structured seed corpus directly from the protobuf schema that PIN
already emits for each target. The goal is to boost the valid-input rate before
Stage A fuzzing starts, without sharing any source code with external LLMs.

Features:
 - Type-driven boundary enumeration for every scalar / string / bytes field.
 - Optional constraint hints (enums, ranges, string constants, length pairs)
   consumed from the JSON file emitted by future analysis passes.
 - Basic handling for repeated fields and nested messages so that seeds cover
   empty / minimal / maximal layouts.

Usage example (run inside build/<target>_diff):

    python3 $ROOT/src/generate_intelligent_seeds.py \
        --py-proto-dir py_proto \
        --module input_pb2 \
        --message Input \
        --output seed_corpus \
        --max-seeds 500

"""

import argparse
import importlib.util
import json
import os
import random
import shutil
import sys
from pathlib import Path
from typing import Iterable, List, Optional, Sequence

from google.protobuf.descriptor import FieldDescriptor


def load_proto_module(py_proto_dir: str, module_name: str):
    module_path = Path(py_proto_dir) / f"{module_name}.py"
    if not module_path.exists():
        raise FileNotFoundError(f"Cannot import proto module {module_path}")
    spec = importlib.util.spec_from_file_location(module_name, module_path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def default_value(field: FieldDescriptor):
    """Return a neutral default for the given field."""
    cpp_type = field.cpp_type
    if cpp_type in (
        FieldDescriptor.CPPTYPE_INT32,
        FieldDescriptor.CPPTYPE_INT64,
        FieldDescriptor.CPPTYPE_UINT32,
        FieldDescriptor.CPPTYPE_UINT64,
        FieldDescriptor.CPPTYPE_ENUM,
    ):
        return 0
    if cpp_type == FieldDescriptor.CPPTYPE_BOOL:
        return False
    if cpp_type == FieldDescriptor.CPPTYPE_FLOAT:
        return 0.0
    if cpp_type == FieldDescriptor.CPPTYPE_DOUBLE:
        return 0.0
    if cpp_type == FieldDescriptor.CPPTYPE_STRING:
        return ""
    if cpp_type == FieldDescriptor.CPPTYPE_MESSAGE:
        return None
    raise NotImplementedError(f"Unhandled field type: {cpp_type}")


def type_driven_values(field: FieldDescriptor) -> Sequence:
    """Boundary values derived purely from the protobuf field type."""
    cpp_type = field.cpp_type
    if cpp_type in (FieldDescriptor.CPPTYPE_INT32, FieldDescriptor.CPPTYPE_INT64):
        return [
            0,
            1,
            -1,
            127,
            -128,
            255,
            -255,
            32767,
            -32768,
            2147483647,
            -2147483648,
        ]
    if cpp_type in (FieldDescriptor.CPPTYPE_UINT32, FieldDescriptor.CPPTYPE_UINT64):
        return [0, 1, 2, 10, 255, 256, 65535, 65536, 2**32 - 1]
    if cpp_type == FieldDescriptor.CPPTYPE_ENUM:
        return [value.number for value in field.enum_type.values]
    if cpp_type == FieldDescriptor.CPPTYPE_BOOL:
        return [False, True]
    if cpp_type == FieldDescriptor.CPPTYPE_STRING:
        if field.type == FieldDescriptor.TYPE_BYTES:
            return [
                b"",
                b"\x00",
                b"\xff",
                bytes(range(16)),
                b"A" * 32,
                b"B" * 128,
                b"C" * 256,
            ]
        return [
            "",
            "a",
            "test",
            "A" * 32,
            "B" * 128,
            "C" * 256,
            "../../../../etc/passwd",
            "\x00\x01\xff",
        ]
    if cpp_type == FieldDescriptor.CPPTYPE_FLOAT or cpp_type == FieldDescriptor.CPPTYPE_DOUBLE:
        return [0.0, 1.0, -1.0, 0.5, -0.5, float("inf"), float("-inf")]
    if cpp_type == FieldDescriptor.CPPTYPE_MESSAGE:
        return [None]
    raise NotImplementedError(f"Unhandled field type: {cpp_type}")


def load_constraints(hints_path: Optional[str]):
    if not hints_path:
        return {}
    hint_file = Path(hints_path)
    if not hint_file.exists():
        raise FileNotFoundError(f"Constraint hints file not found: {hints_path}")
    with open(hint_file, "r", encoding="utf-8") as fp:
        data = json.load(fp)
    return data.get("constraints", {})


def ensure_output_dir(path: Path):
    path.mkdir(parents=True, exist_ok=True)


def clone_message(msg):
    clone = msg.__class__()
    clone.CopyFrom(msg)
    return clone


def set_field_value(message, field: FieldDescriptor, value):
    if field.label == FieldDescriptor.LABEL_REPEATED:
        repeated_field = getattr(message, field.name)
        repeated_field.clear()
        repeated_field.extend(value)
    elif field.cpp_type == FieldDescriptor.CPPTYPE_MESSAGE:
        nested = getattr(message, field.name)
        if value is None:
            nested.Clear()
        else:
            nested.CopyFrom(value)
    else:
        setattr(message, field.name, value)


def generate_repeated_variants(field: FieldDescriptor):
    elem_default = default_value(field)
    base_values = [elem_default]
    if field.cpp_type != FieldDescriptor.CPPTYPE_MESSAGE:
        base_values.extend(type_driven_values(field))
    variants = [
        [],
        [elem_default],
        base_values[:3],
    ]
    if field.cpp_type != FieldDescriptor.CPPTYPE_MESSAGE:
        variants.append(base_values[:5])
    return variants


def select_candidate_values(
    field: FieldDescriptor, constraints: dict
) -> Sequence:
    values = list(type_driven_values(field))

    enums = constraints.get("enums", {}).get(field.name)
    if enums:
        values.extend(enums)

    ranges = constraints.get("ranges", {}).get(field.name)
    if ranges:
        min_v = ranges.get("min")
        max_v = ranges.get("max")
        extra = []
        for delta in (-1, 0, 1):
            if min_v is not None:
                extra.append(min_v + delta)
            if max_v is not None:
                extra.append(max_v + delta)
        values.extend(extra)

    strings = constraints.get("strings", {}).get(field.name)
    if strings and field.cpp_type == FieldDescriptor.CPPTYPE_STRING:
        values.extend(strings)

    # Deduplicate while preserving order
    seen = set()
    deduped = []
    for val in values:
        key = val if isinstance(val, (int, str, float, bool)) else id(val)
        if key in seen:
            continue
        seen.add(key)
        deduped.append(val)
    return deduped


def generate_nested_default(field: FieldDescriptor, depth: int = 1):
    if depth <= 0:
        return None
    nested_cls = field.message_type._concrete_class
    nested_msg = nested_cls()
    for nested_field in field.message_type.fields:
        if nested_field.label == FieldDescriptor.LABEL_REPEATED:
            getattr(nested_msg, nested_field.name).extend([])
        elif nested_field.cpp_type == FieldDescriptor.CPPTYPE_MESSAGE:
            setattr(
                nested_msg,
                nested_field.name,
                generate_nested_default(nested_field, depth - 1),
            )
        else:
            setattr(nested_msg, nested_field.name, default_value(nested_field))
    return nested_msg


def generate_seeds(
    message_cls,
    output_dir: Path,
    max_seeds: int,
    constraints: dict,
) -> int:
    """Generate up to max_seeds seeds and write them to output_dir."""
    ensure_output_dir(output_dir)
    seeds_written = 0
    base_message = message_cls()

    fields = message_cls.DESCRIPTOR.fields

    # Single-field variation
    for field in fields:
        if seeds_written >= max_seeds:
            break

        candidates: Iterable
        if field.label == FieldDescriptor.LABEL_REPEATED:
            candidates = generate_repeated_variants(field)
        elif field.cpp_type == FieldDescriptor.CPPTYPE_MESSAGE:
            candidates = [generate_nested_default(field), None]
        else:
            candidates = select_candidate_values(field, constraints)

        for value in candidates:
            msg = clone_message(base_message)
            set_field_value(msg, field, value)
            seed_path = output_dir / f"{field.name}_{seeds_written:04d}.bin"
            with open(seed_path, "wb") as fp:
                fp.write(msg.SerializeToString())
            seeds_written += 1
            if seeds_written >= max_seeds:
                break

    return seeds_written


def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate intelligent seed corpus from proto schema"
    )
    parser.add_argument(
        "--py-proto-dir",
        required=True,
        help="Directory containing *_pb2.py modules (e.g., build/.../py_proto)",
    )
    parser.add_argument(
        "--module",
        required=True,
        help="Python module name for the generated proto (e.g., input_pb2)",
    )
    parser.add_argument(
        "--message",
        required=True,
        help="Top-level message/class name to instantiate (e.g., Input)",
    )
    parser.add_argument(
        "--output",
        required=True,
        help="Directory to write generated seeds (will be created)",
    )
    parser.add_argument(
        "--max-seeds",
        type=int,
        default=1000,
        help="Maximum seeds to emit (default: 1000)",
    )
    parser.add_argument(
        "--constraints",
        default=None,
        help="Optional JSON file containing constraint hints",
    )
    parser.add_argument(
        "--clean-output",
        action="store_true",
        help="If set, remove existing output directory before generating seeds",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    constraints = load_constraints(args.constraints)

    py_proto_dir = Path(args.py_proto_dir)
    output_dir = Path(args.output)
    if args.clean_output and output_dir.exists():
        shutil.rmtree(output_dir)

    module = load_proto_module(py_proto_dir, args.module)
    if not hasattr(module, args.message):
        raise AttributeError(
            f"Message {args.message} not found in module {args.module}"
        )
    message_cls = getattr(module, args.message)

    seeds_written = generate_seeds(
        message_cls=message_cls,
        output_dir=output_dir,
        max_seeds=args.max_seeds,
        constraints=constraints,
    )
    print(f"[+] Generated {seeds_written} intelligent seeds in {output_dir}")


if __name__ == "__main__":
    sys.exit(main())
