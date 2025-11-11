#!/usr/bin/env python3
"""
Analyze libtiff public APIs and rank them by PIN fuzzability.

Input:  Path to tiffio.h (or any header containing extern prototypes)
Output: Text summary + CSV + top-N list (mirrors mongoose workflow)
"""

from __future__ import annotations

import argparse
import csv
import pathlib
import re
from dataclasses import dataclass, field
from typing import List

SCALAR_KEYWORDS = {
    "int",
    "uint",
    "uint8",
    "uint16",
    "uint32",
    "uint64",
    "int8",
    "int16",
    "int32",
    "int64",
    "float",
    "double",
    "long",
    "short",
    "char",
    "size_t",
    "tmsize_t",
    "ttag_t",
    "tsample_t",
    "tdir_t",
    "toff_t",
    "uintmax_t",
    "TIFFDataType",
    "TIFFRGBValue",
    "TIFFColormap",
    "TIFFCodec",
    "TIFFStreamLengthProc",
    "TIFFStreamSeekProc",
}

CALLBACK_TYPES = {
    "TIFFReadWriteProc",
    "TIFFSeekProc",
    "TIFFCloseProc",
    "TIFFSizeProc",
    "TIFFMapFileProc",
    "TIFFUnmapFileProc",
    "TIFFErrorHandler",
    "TIFFErrorHandlerExt",
    "TIFFExtendProc",
    "tileContigRoutine",
    "tileSeparateRoutine",
    "TIFFDisplay",
    "TIFFCIELabToRGB",
    "TIFFRGBAImage",
    "TIFFYCbCrToRGB",
}

STRUCT_POINTER_KEYWORDS = {
    "TIFFRGBAImage",
    "TIFFDisplay",
    "TIFFCIELabToRGB",
    "TIFFYCbCrToRGB",
    "TIFFDirectory",
}


@dataclass
class FunctionInfo:
    prototype: str
    return_type: str
    name: str
    params: List[str]
    score: float
    reasons: List[str] = field(default_factory=list)

    @property
    def param_count(self) -> int:
        return len(self.params)

    @property
    def tier(self) -> int:
        if self.score >= 4.5:
            return 5
        if self.score >= 3.5:
            return 4
        if self.score >= 2.5:
            return 3
        if self.score >= 1.5:
            return 2
        return 1


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//.*", "", text)
    return text


def normalize_statement(stmt: str) -> str:
    stmt = stmt.strip()
    stmt = stmt.replace("\n", " ")
    stmt = re.sub(r"\s+", " ", stmt)
    stmt = re.sub(r"__attribute__\s*\(\([^)]*\)\)", "", stmt)
    return stmt


def remove_default_args(params: str) -> str:
    # Remove simple C++-style defaults (e.g., "= 0" or "= ORIENTATION_BOTLEFT")
    return re.sub(r"=\s*[^,\)]+", "", params)


def split_statements(text: str) -> List[str]:
    statements: List[str] = []
    buffer = ""
    for line in text.splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        buffer += " " + stripped
        while ";" in buffer:
            before, after = buffer.split(";", 1)
            statements.append(before.strip())
            buffer = after
    return [normalize_statement(stmt) for stmt in statements if stmt.strip()]


def parse_functions(statements: List[str]) -> List[FunctionInfo]:
    functions: List[FunctionInfo] = []
    seen: set[tuple[str, tuple[str, ...]]] = set()
    for stmt in statements:
        if not stmt.startswith("extern"):
            continue

        # Drop trailing comments or macros that survived.
        stmt = stmt.rstrip()
        match = re.match(r"extern\s+(.+?)\s+([A-Za-z_]\w*)\s*\((.*)\)", stmt)
        if not match:
            continue
        return_type = match.group(1).strip()
        name = match.group(2).strip()
        params_raw = remove_default_args(match.group(3).strip())
        params: List[str] = []
        if params_raw and params_raw.lower() != "void":
            params = [p.strip() for p in params_raw.split(",") if p.strip()]
        key = (name, tuple(params))
        if key in seen:
            continue
        seen.add(key)
        # Score + reasons
        score, reasons = score_function(return_type, name, params)
        functions.append(
            FunctionInfo(
                prototype=stmt,
                return_type=return_type,
                name=name,
                params=params,
                score=round(score, 2),
                reasons=reasons,
            )
        )
    return functions


def param_has_keyword(param: str, keyword: str) -> bool:
    return keyword in param or keyword.replace(" ", "") in param


def is_tiff_handle(param: str) -> bool:
    normalized = param.replace("const", "")
    return "TIFF*" in normalized or "TIFF *" in normalized


def is_char_pointer(param: str) -> bool:
    return "*" in param and any(token in param for token in ["char", "uint8"])


def is_struct_pointer(param: str) -> bool:
    return any(keyword in param for keyword in STRUCT_POINTER_KEYWORDS)


def is_callback(param: str) -> bool:
    if "(*" in param:
        return True
    tokens = re.split(r"\s+", param)
    return any(token.strip(" *") in CALLBACK_TYPES for token in tokens)


def is_variadic(param: str) -> bool:
    return "va_list" in param or "..." in param


def score_function(return_type: str, name: str, params: List[str]) -> tuple[float, List[str]]:
    if not params:
        return 4.0, ["No parameters (easy harness)"]

    score = 5.0
    reasons: List[str] = []

    for param in params:
        lower = param.lower()
        if is_variadic(param):
            reasons.append("Variadic parameter")
            return 1.0, reasons
        if is_callback(param):
            score -= 2.5
            reasons.append("Callback parameter")
            continue
        if is_tiff_handle(param):
            score -= 2.0
            reasons.append("Requires TIFF* handle")
            continue
        if "*" in param:
            if is_char_pointer(param):
                score -= 0.5
                reasons.append("String/buffer pointer")
            elif is_struct_pointer(param):
                score -= 1.5
                reasons.append("Struct pointer")
            else:
                score -= 1.0
                reasons.append("Raw pointer")
        else:
            if not any(keyword in param for keyword in SCALAR_KEYWORDS):
                score -= 0.5
                reasons.append("Custom scalar type")

    if len(params) >= 6:
        score -= 0.5
        reasons.append(f"Many params ({len(params)})")

    return max(score, 0.0), reasons or ["Scalar-heavy signature"]


def summarize(functions: List[FunctionInfo], top_n: int) -> None:
    print(f"Total functions analyzed: {len(functions)}\n")
    print("=" * 100)
    print(f"TOP {top_n} MOST FUZZABLE LIBTIFF FUNCTIONS")
    print("=" * 100)
    print(f"{'Rank':<5} {'Score':<7} {'Name':<32} {'Params':<6} {'Reasons'}")
    print("-" * 100)
    for rank, func in enumerate(functions[:top_n], 1):
        reason = "; ".join(dict.fromkeys(func.reasons))[:60]
        print(f"{rank:<5} {func.score:<7} {func.name:<32} {func.param_count:<6} {reason}")

    tiers = {tier: 0 for tier in range(1, 6)}
    for func in functions:
        tiers[func.tier] += 1
    print("\nTier distribution:")
    for tier in range(5, 0, -1):
        print(f"  Tier {tier}: {tiers[tier]} functions")


def write_csv(functions: List[FunctionInfo], output: pathlib.Path) -> None:
    with output.open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["rank", "name", "score", "return_type", "param_count", "params", "reasons"])
        for rank, func in enumerate(functions, 1):
            writer.writerow(
                [
                    rank,
                    func.name,
                    func.score,
                    func.return_type,
                    func.param_count,
                    " | ".join(func.params),
                    "; ".join(dict.fromkeys(func.reasons)),
                ]
            )


def write_targets(functions: List[FunctionInfo], output: pathlib.Path, top_n: int) -> None:
    with output.open("w") as f:
        for func in functions[:top_n]:
            f.write(f"{func.name}\n")


def main() -> None:
    parser = argparse.ArgumentParser(description="Rank libtiff functions by PIN fuzzability.")
    parser.add_argument("header", type=pathlib.Path, help="Path to tiffio.h")
    parser.add_argument("--top-n", type=int, default=25, help="Number of top functions to print/save")
    parser.add_argument("--output-prefix", type=pathlib.Path, default=pathlib.Path("/tmp/libtiff_functions"))
    args = parser.parse_args()

    header_text = strip_comments(args.header.read_text())
    statements = split_statements(header_text)
    functions = parse_functions(statements)
    functions.sort(key=lambda f: (-f.score, f.param_count, f.name))

    summarize(functions, args.top_n)

    csv_path = args.output_prefix.with_suffix(".csv")
    targets_path = args.output_prefix.with_suffix(".top.txt")
    write_csv(functions, csv_path)
    write_targets(functions, targets_path, args.top_n)

    print(f"\n[+] CSV written to {csv_path}")
    print(f"[+] Top {args.top_n} target list written to {targets_path}")


if __name__ == "__main__":
    main()
