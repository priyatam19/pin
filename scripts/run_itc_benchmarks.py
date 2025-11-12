#!/usr/bin/env python3
"""
Batch runner for the ITC benchmark corpus.

Invokes pin_diff.sh for every `.c` file under the ITC suite (excluding the
suite's own harness `main.c` files) and records success / failure metadata.
"""

from __future__ import annotations

import json
import re
import subprocess
import time
from pathlib import Path
from typing import Dict, List


ROOT = Path(__file__).resolve().parent.parent
PIN_DIFF = ROOT / "src" / "pin_diff.sh"
BENCH_ROOT = ROOT / "examples" / "itc-benchmarks"
HEADER_DIR = "examples/itc-benchmarks/include"
EXTRA_SOURCES = "examples/itc-benchmarks/common_stubs.c"
DEFAULT_LIBS = "-lm -lpthread"
RESULTS_DIR = ROOT / "results" / "itc_benchmarks"
LOG_DIR = RESULTS_DIR / "logs"
SUMMARY_PATH = RESULTS_DIR / "summary.json"
REFERENCE_DECODER = "nanopb"

TARGET_DIRS = [
    BENCH_ROOT / "01.w_Defects",
    BENCH_ROOT / "02.wo_Defects",
]

MAIN_REGEX = re.compile(r"\bvoid\s+([a-zA-Z0-9_]+_main)\s*\(")


def discover_targets() -> List[Dict[str, str]]:
    targets: List[Dict[str, str]] = []
    for directory in TARGET_DIRS:
        for c_path in sorted(directory.glob("*.c")):
            if c_path.name == "main.c":
                continue
            contents = c_path.read_text(errors="ignore")
            match = MAIN_REGEX.search(contents)
            if not match:
                targets.append(
                    {
                        "source": str(c_path.relative_to(ROOT)),
                        "function": "",
                        "error": "no *_main entry point found",
                    }
                )
                continue
            targets.append(
                {
                    "source": str(c_path.relative_to(ROOT)),
                    "function": match.group(1),
                    "error": "",
                }
            )
    return targets


def run_target(source: str, function: str) -> Dict[str, object]:
    if not function:
        return {
            "source": source,
            "function": function,
            "status": "skip",
            "exit_code": None,
            "duration_sec": 0.0,
            "note": "missing entry point",
        }

    cmd = [
        str(PIN_DIFF),
        source,
        function,
        f"--headers-dir={HEADER_DIR}",
        f"--extra-sources={EXTRA_SOURCES}",
        f"--libs={DEFAULT_LIBS}",
        f"--reference-decoder={REFERENCE_DECODER}",
    ]

    started = time.time()
    proc = subprocess.run(
        cmd,
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    duration = time.time() - started

    log_name = source.replace("/", "_").replace(".c", "") + ".log"
    log_path = LOG_DIR / log_name
    log_path.write_text(proc.stdout)

    output_lines = [line for line in proc.stdout.strip().splitlines() if line.strip()]
    tail = output_lines[-1] if output_lines else ""

    status = "ok" if proc.returncode == 0 else "fail"

    return {
        "source": source,
        "function": function,
        "status": status,
        "exit_code": proc.returncode,
        "duration_sec": round(duration, 3),
        "note": tail,
        "log_path": str(log_path.relative_to(ROOT)),
    }


def main() -> None:
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)
    LOG_DIR.mkdir(parents=True, exist_ok=True)

    summary: List[Dict[str, object]] = []
    for target in discover_targets():
        result = run_target(target["source"], target["function"])
        if target["error"]:
            result["status"] = "skip"
            result["note"] = target["error"]
        summary.append(result)

    SUMMARY_PATH.write_text(json.dumps(summary, indent=2))

    total = len(summary)
    passed = sum(1 for item in summary if item["status"] == "ok")
    skipped = sum(1 for item in summary if item["status"] == "skip")
    failed = total - passed - skipped

    print(f"[itc-benchmarks] total={total} passed={passed} failed={failed} skipped={skipped}")
    if failed:
        print("First few failures:")
        for item in summary:
            if item["status"] == "ok" or item["status"] == "skip":
                continue
            print(f"  - {item['source']} ({item['function']}): {item['note']}")
            failed -= 1
            if failed <= 0:
                break


if __name__ == "__main__":
    main()
