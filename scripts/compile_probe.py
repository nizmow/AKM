#!/usr/bin/env python3
"""Run syntax-only probes over remaining source trees and classify failures."""

import argparse
import json
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

from port_scan import ROOT, scan, source_files

DEFAULT_COMPILER = "m68k-atari-mintelf-gcc"


def compiler_command(explicit):
    if explicit:
        return shlex.split(explicit)
    if shutil.which(DEFAULT_COMPILER):
        return [DEFAULT_COMPILER]
    wrapper = ROOT / "scripts/atari-container"
    if wrapper.is_file():
        return [str(wrapper), "run", DEFAULT_COMPILER]
    return [DEFAULT_COMPILER]


def include_dirs(source_root):
    root = ROOT / source_root
    directories = [root]
    directories.extend(sorted({path.parent for path in root.rglob("*.h")}))
    # Match the demo's Make rule: its SDK header belongs to production WINFRAME.
    if source_root == "base/winframe-demo":
        directories.append(ROOT / "base/winframe")
    directories.append(ROOT / "kernel/INC")
    return [path.relative_to(ROOT) for path in directories if path.is_dir()]


def classify(output):
    lower = output.lower()
    if "no such file or directory" in lower or "fatal error:" in lower:
        return "missing-header"
    if "unknown type name" in lower or "implicit declaration" in lower:
        return "legacy-api"
    if "conflicting types" in lower or "incompatible" in lower:
        return "abi"
    return "error"


def probe(compiler, path, includes):
    command = [*compiler, "-std=gnu11", "-m68000", "-fsyntax-only", "-x", "c"]
    command.extend(f"-I{directory}" for directory in includes)
    command.append(str(path.relative_to(ROOT)))
    try:
        result = subprocess.run(command, cwd=ROOT, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                timeout=60)
    except subprocess.TimeoutExpired:
        return "timeout", "timed out"
    output = result.stdout.strip()
    if result.returncode == 0:
        return "ok", ""
    first = next((line for line in output.splitlines() if "error:" in line), output.splitlines()[0] if output else "")
    return classify(output), first


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--only", help="limit to source paths containing this text")
    parser.add_argument("--compiler", help="compiler command; defaults to the container wrapper")
    parser.add_argument("--limit", type=int, default=25, help="maximum files to probe")
    parser.add_argument("--json", action="store_true")
    parser.add_argument("-o", "--output")
    args = parser.parse_args()
    if args.limit <= 0:
        parser.error("--limit must be positive")
    rows, _counts = scan()
    compiler = compiler_command(args.compiler)
    results = []
    for row in rows:
        if row["kind"] != "remaining":
            continue
        if args.only and args.only.lower() not in row["source"].lower():
            continue
        includes = include_dirs(row["source"])
        for path in source_files(row["source"]):
            if path.suffix.lower() != ".c":
                continue
            if len(results) >= args.limit:
                break
            status, detail = probe(compiler, path, includes)
            results.append({"source": row["source"], "file": str(path.relative_to(ROOT)),
                            "status": status, "detail": detail})
        if len(results) >= args.limit:
            break
    counts = {}
    for result in results:
        counts[result["status"]] = counts.get(result["status"], 0) + 1
    if args.json:
        text = json.dumps({"compiler": compiler, "counts": counts, "results": results},
                          indent=2) + "\n"
    else:
        lines = ["# Compile probe", "", f"- Compiler: `{' '.join(compiler)}`",
                 f"- Files: **{len(results)}**", ""]
        for status in ("ok", "missing-header", "legacy-api", "abi", "error", "timeout"):
            if counts.get(status):
                lines.append(f"- {status}: **{counts[status]}**")
        lines.extend(["", "| Status | Source | File | First error |", "|---|---|---|---|"])
        for result in results:
            detail = result["detail"].replace("|", "\\|")
            lines.append(f"| {result['status']} | `{result['source']}` | "
                         f"`{result['file']}` | {detail} |")
        text = "\n".join(lines) + "\n"
    if args.output:
        Path(args.output).write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
