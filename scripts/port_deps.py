#!/usr/bin/env python3
"""Resolve legacy PRJ module/header references to repository candidates.

This is a report-only companion to port_scan.py. It does not rewrite PRJ files
or claim that a resolved source compiles against the public SDK.
"""

import argparse
import json
import re
import sys
from pathlib import Path

from port_scan import ROOT, scan, source_files

LIBRARY_RE = re.compile(r"(?i)(?:^lib[\\/])?([a-z0-9_.-]+\.(?:lib|o|obj))$")
PATH_TOKEN_RE = re.compile(r"(?i)(?:[a-z]:[\\/]|\.{0,2}[\\/])[^\s()\[\]]+")
EXTERNAL_LIBRARIES = {
    "tcnstart.o", "pcstdlib.lib", "pctoslib.lib", "pcgemlib.lib", "pcextlib.lib",
}
SOURCE_LIBRARIES = {
    "mt_aes.lib": "MagiC/LIB/MT_AES",
    "mt_dos.lib": "MagiC/LIB/MT_DOS",
    "magxlib.lib": "MagiC/LIB/MAGIXLIB",
    "emu_dbg.lib": "MagiC/APPS/EMUDBG.LIB",
    "pcmxstrt.o": "MagiC/LIB/PCMXSTRT",
}


def repository_index():
    index = {}
    for base in (ROOT / "MagiC", ROOT / "KAOS", ROOT / "NON-TOS"):
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.is_file():
                index.setdefault(path.name.lower(), []).append(path)
    return index


def reference_score(reference, candidate):
    reference_parts = {part.lower() for part in re.split(r"[\\/]", reference) if part}
    candidate_parts = {part.lower() for part in candidate.parts}
    score = len(reference_parts & candidate_parts)
    if candidate.suffix.lower() in (".c", ".s", ".asm"):
        score += 0.25
    return score


def resolve(name, index, source, absolute=False):
    cleaned = name.strip().strip("\"'").rstrip(".,\x1a")
    basename = re.split(r"[\\/]", cleaned)[-1]
    lower = basename.lower()
    if lower in EXTERNAL_LIBRARIES:
        return {"reference": name, "status": "external", "candidates": []}
    if lower in SOURCE_LIBRARIES:
        return {"reference": name, "status": "resolved",
                "candidates": [SOURCE_LIBRARIES[lower]]}

    candidates = list(index.get(lower, []))
    if lower.endswith((".o", ".obj")):
        stem = lower.rsplit(".", 1)[0]
        for suffix in (".c", ".s", ".asm"):
            candidates.extend(index.get(stem + suffix, []))
    if not candidates and "." not in lower:
        for suffix in (".c", ".s", ".asm", ".lib", ".o"):
            candidates.extend(index.get(lower + suffix, []))
    if not candidates:
        status = "external" if lower.endswith((".lib", ".o", ".obj")) else "unresolved"
        return {"reference": name, "status": status, "candidates": []}

    # Archived BIN/IMPORT objects are evidence, not source candidates.
    filtered = [path for path in candidates
                if "BIN" not in path.parts and "IMPORT" not in path.parts]
    if filtered:
        candidates = filtered
    # Prefer the project's own directory, then locale-neutral, non-ORIG files.
    source_root = (ROOT / source).resolve()

    def alternative(path):
        return (any(part.upper() in ("FRA", "US", "USA", "EN", "FR", "DE",
                                     "DEUTSCH", "ENGLISCH", "FRANZ")
                    for part in path.parts)
                or "ORIG" in path.parts)

    def relative_depth(path):
        return len(path.relative_to(source_root).parts) if path.is_relative_to(source_root) else 99

    candidates = sorted(set(candidates), key=lambda path: (
        0 if path.is_relative_to(source_root) else 1,
        alternative(path),
        relative_depth(path),
        -reference_score(cleaned, path),
        str(path).lower()))
    best = reference_score(cleaned, candidates[0])
    next_best = reference_score(cleaned, candidates[1]) if len(candidates) > 1 else -1
    status = "resolved" if len(candidates) == 1 or best > next_best else "ambiguous"
    if (candidates[0].is_relative_to(source_root) and not alternative(candidates[0])
            and all(not path.is_relative_to(source_root) or alternative(path)
                    for path in candidates[1:])):
        status = "resolved"
    return {"reference": name,
            "status": status,
            "candidates": [str(path.relative_to(ROOT)) for path in candidates]}


def parse_project(path):
    text = path.read_bytes().decode("cp850", "replace")
    text = re.sub(r";[^\r\n]*", "", text)
    output = None
    refs = []
    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.startswith(("=", ".")):
            continue
        if output is None and not line.lower().endswith((".lib", ".o", ".obj")):
            output = line.split()[0]
            continue
        token = re.split(r"[\s(\[]", line, maxsplit=1)[0].strip("\x1a\r")
        if not token:
            continue
        if token.startswith("lib\\") or token.startswith("lib/"):
            refs.append(("library", token, False))
        elif LIBRARY_RE.search(token):
            refs.append(("library", token, False))
        elif PATH_TOKEN_RE.match(token):
            refs.append(("path", token, True))
        elif token.lower().endswith((".c", ".s", ".asm")):
            refs.append(("module", token, False))
        else:
            refs.append(("module", token, False))
        for headers in re.findall(r"\(([^)]*)\)", line):
            for header in headers.split(","):
                header = header.strip()
                if header:
                    refs.append(("header", header, PATH_TOKEN_RE.match(header) is not None))
    return output, refs


def scan_projects(only=None):
    index = repository_index()
    rows, _counts = scan()
    projects = []
    for row in rows:
        if row["kind"] != "remaining":
            continue
        if only and only.lower() not in row["source"].lower():
            continue
        for path in source_files(row["source"]):
            if path.suffix.lower() != ".prj":
                continue
            if only and only.lower() not in str(path).lower():
                continue
            output, refs = parse_project(path)
            resolved = []
            seen = set()
            for kind, reference, absolute in refs:
                key = (kind, reference)
                if key in seen:
                    continue
                seen.add(key)
                item = resolve(reference, index, row["source"], absolute)
                item.update({"kind": kind})
                resolved.append(item)
            projects.append({
                "project": str(path.relative_to(ROOT)),
                "source": row["source"],
                "output": output,
                "references": resolved,
            })
    return projects


def markdown(projects):
    counts = {"resolved": 0, "ambiguous": 0, "external": 0, "unresolved": 0}
    for project in projects:
        for ref in project["references"]:
            counts[ref["status"]] = counts.get(ref["status"], 0) + 1
    lines = [
        "# PRJ dependency scan",
        "",
        f"- Projects: **{len(projects)}**",
        f"- Resolved: **{counts['resolved']}**",
        f"- Ambiguous: **{counts['ambiguous']}**",
        f"- External libraries/startup: **{counts['external']}**",
        f"- Unresolved: **{counts['unresolved']}**",
        "",
        "`external` means an archived compiler library or startup object, not a source tree.",
        "",
    ]
    for project in projects:
        lines.extend([f"## `{project['project']}`", "",
                      f"- Output: `{project['output'] or 'unknown'}`",
                      f"- Source root: `{project['source']}`", "",
                      "| Reference | Kind | Status | Candidates |",
                      "|---|---|---|---|"])
        for ref in project["references"]:
            candidates = ", ".join(f"`{candidate}`" for candidate in ref["candidates"]) or ""
            lines.append(f"| `{ref['reference']}` | {ref['kind']} | {ref['status']} | {candidates} |")
        lines.append("")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--only", help="limit to source/project paths containing this text")
    parser.add_argument("-o", "--output", help="write Markdown or JSON instead of stdout")
    parser.add_argument("--json", action="store_true", help="emit JSON")
    parser.add_argument("--strict", action="store_true",
                        help="exit nonzero when a non-external reference is unresolved")
    args = parser.parse_args()
    projects = scan_projects(args.only)
    if args.json:
        text = json.dumps(projects, indent=2) + "\n"
    else:
        text = markdown(projects)
    if args.output:
        Path(args.output).write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)
    if args.strict and any(ref["status"] in ("unresolved", "ambiguous")
                           for project in projects for ref in project["references"]):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
