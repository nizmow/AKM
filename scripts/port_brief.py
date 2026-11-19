#!/usr/bin/env python3
"""Compose a per-component porting brief from the inventory and PRJ scanners."""

import argparse
import json
import re
from pathlib import Path

from port_deps import scan_projects
from port_scan import ROOT, decode_text, scan, source_files

MAKEFILE = ROOT / "Makefile.apps"
TARGET_RE = re.compile(r"^([A-Za-z0-9_.-]+):", re.MULTILINE)


def candidate_target(output):
    stem = Path(output).stem.lower().replace("_", "-")
    return stem or output.lower()


def find_rows(query):
    rows, _counts = scan()
    needle = query.lower()
    matches = [row for row in rows
               if needle in row["output"].lower() or needle in row["source"].lower()]
    return matches


def brief(row, query):
    files = source_files(row["source"])
    text_files = [path for path in files if path.suffix.lower() in (".c", ".h", ".s", ".asm")]
    encoding = {str(path.relative_to(ROOT)): decode_text(path)[0] for path in text_files}
    projects = [project for project in scan_projects(row["source"])
                if project["source"] == row["source"]]
    targets = set(TARGET_RE.findall(MAKEFILE.read_text(encoding="utf-8")))
    target = candidate_target(row["output"])
    return {
        "query": query,
        "status": row["status"],
        "kind": row["kind"],
        "output": row["output"],
        "source": row["source"],
        "target": target,
        "target_exists": target in targets,
        "files": {
            "text": len(text_files),
            "projects": sum(path.suffix.lower() == ".prj" for path in files),
            "atari_text": sorted(name for name, state in encoding.items() if state == "atari"),
            "invalid_text": sorted(name for name, state in encoding.items()
                                   if state == "binary/invalid"),
        },
        "projects": projects,
    }


def markdown(items):
    lines = ["# Port brief", ""]
    for item in items:
        lines.extend([
            f"## `{item['output']}`",
            "",
            f"- Source: `{item['source']}`",
            f"- Status: {item['status']}",
            f"- Candidate target: `{item['target']}` "
            f"({'exists' if item['target_exists'] else 'not present'})",
            f"- Text files: {item['files']['text']}; PRJ files: {item['files']['projects']}",
            f"- Atari-encoded files: {len(item['files']['atari_text'])}",
            f"- Invalid text files: {len(item['files']['invalid_text'])}",
            "",
        ])
        if item["files"]["atari_text"]:
            lines.append("Atari text:")
            lines.extend(f"- `{name}`" for name in item["files"]["atari_text"])
            lines.append("")
        if item["files"]["invalid_text"]:
            lines.append("Invalid text:")
            lines.extend(f"- `{name}`" for name in item["files"]["invalid_text"])
            lines.append("")
        for project in item["projects"]:
            lines.extend([f"### `{project['project']}`", "",
                          f"Output: `{project['output'] or 'unknown'}`", "",
                          "| Reference | Kind | Status | Candidates |",
                          "|---|---|---|---|"])
            for ref in project["references"]:
                candidates = ", ".join(f"`{path}`" for path in ref["candidates"][:6])
                lines.append(f"| `{ref['reference']}` | {ref['kind']} | "
                             f"{ref['status']} | {candidates} |")
            lines.append("")
        lines.extend([
            "### Checklist",
            "",
            "- [ ] Convert known Atari text; keep runtime literals as explicit bytes.",
            "- [ ] Resolve every source reference; record deliberate locale/version choices.",
            "- [ ] Add the independent Makefile target and bundle files.",
            "- [ ] Build in a fresh output directory.",
            "- [ ] Add the focused NeoST scenario and compare meaningful outputs.",
            "- [ ] Update `APPLICATION_BUILDS.md`, `PORTING_NOTES.md` and `TODO.md`.",
            "",
        ])
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("component", help="output or source substring, e.g. MGSEARCH")
    parser.add_argument("--json", action="store_true", help="emit JSON")
    parser.add_argument("-o", "--output", help="write to this file instead of stdout")
    args = parser.parse_args()
    rows = find_rows(args.component)
    if not rows:
        parser.error(f"no catalogue row matches {args.component!r}")
    items = [brief(row, args.component) for row in rows]
    text = json.dumps(items, indent=2) + "\n" if args.json else markdown(items)
    if args.output:
        Path(args.output).write_text(text, encoding="utf-8")
    else:
        print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
