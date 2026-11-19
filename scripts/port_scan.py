#!/usr/bin/env python3
"""Read-only inventory scan for APPLICATION_BUILDS.md.

Reports source presence, legacy project-file absolute paths and Atari-encoded
text. It never edits sources or claims that a row builds.
"""

import argparse
import codecs
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CATALOGUE = ROOT / "APPLICATION_BUILDS.md"
TEXT_SUFFIXES = {".c", ".h", ".s", ".asm"}
SOURCE_SUFFIXES = TEXT_SUFFIXES | {".prj"}
TICK_RE = re.compile(r"`([^`]+)`")
ABS_PATH_RE = re.compile(r"(?i)(?<![A-Za-z0-9_])([A-Za-z]:\\[^\s,;)\"']+)")

try:
    sys.path.insert(0, str(ROOT / "NON-TOS/atari-utf8-conversion"))
    import atari_text
except (ImportError, OSError, ValueError):
    atari_text = None


def cells_of(line):
    return [cell.strip() for cell in line.strip().strip("|").split("|")]


def is_separator(cells):
    return all(not cell or set(cell) <= set("-: ") for cell in cells)


def status_kind(section, status):
    lower = status.lower()
    if "blocked" in lower:
        return "blocked"
    if "binary" in lower:
        return "binary"
    if "unclassified" in lower:
        return "unclassified"
    if "build infrastructure" in lower:
        return "infrastructure"
    if section.startswith("7."):
        return "historical"
    if "test only" in lower or section.startswith("8."):
        return "test/host"
    if status.startswith("H") or "/ mac" in lower:
        return "historical"
    if any(word in lower for word in ("verified", "built", "builds", "tested", "compared", "launched")):
        return "built"
    return "remaining"


def decode_text(path):
    data = path.read_bytes()
    try:
        data.decode("utf-8")
        return "utf8", None
    except UnicodeDecodeError:
        pass
    if atari_text is not None:
        try:
            table = atari_text.character_table()
            codecs.charmap_decode(data, "strict", table)
            return "atari", None
        except (UnicodeError, ValueError):
            pass
    return "binary/invalid", None


def source_files(source):
    path = ROOT / source
    if not path.exists():
        return []
    if path.is_file():
        return [path]
    return [candidate for candidate in path.rglob("*")
            if candidate.is_file() and candidate.suffix.lower() in SOURCE_SUFFIXES]


def project_paths(files):
    paths = set()
    for path in files:
        if path.suffix.lower() != ".prj":
            continue
        text = path.read_bytes().decode("cp850", "replace")
        paths.update(match.group(1).rstrip(".)") for match in ABS_PATH_RE.finditer(text))
    return sorted(paths)


def scan():
    section = ""
    rows = []
    companions = []
    for line in CATALOGUE.read_text(encoding="utf-8").splitlines():
        if line.startswith("## "):
            section = line[3:].strip()
            continue
        if not line.startswith("|"):
            continue
        cells = cells_of(line)
        if len(cells) < 2 or is_separator(cells) or cells[0].lower().startswith("priority"):
            continue
        if section.startswith(("Scope", "3.")):
            continue
        tokens = [token for cell in cells for token in TICK_RE.findall(cell)]
        source = next((token for token in tokens if (ROOT / token).exists()), None)
        if source is None:
            continue
        output = next((token for token in tokens
                       if not token.startswith(("MagiC/", "KAOS/", "NON-TOS/"))), cells[0])
        description = cells[-1]
        status = cells[0] if section.startswith(("1.", "4.", "5.")) else section
        if section.startswith("2."):
            status = "system"
        elif section.startswith("6."):
            status = "library"
        elif section.startswith("7."):
            status = "historical"
        elif section.startswith("8."):
            status = "host/test"

        files = source_files(source)
        text_files = [path for path in files if path.suffix.lower() in TEXT_SUFFIXES]
        encodings = [decode_text(path)[0] for path in text_files]
        row = {
            "section": section,
            "status": status,
            "kind": status_kind(section, status),
            "output": output,
            "source": source,
            "exists": (ROOT / source).exists(),
            "text_files": len(text_files),
            "projects": sum(path.suffix.lower() == ".prj" for path in files),
            "atari_text_files": encodings.count("atari"),
            "invalid_text_files": encodings.count("binary/invalid"),
            "absolute_project_paths": project_paths(files),
            "description": description,
        }
        rows.append(row)
        if "EDIT.PRG" in description and "remains unported" in description:
            companions.append({
                "section": section, "status": "companion", "kind": "remaining",
                "output": "EDIT.PRG", "source": source, "exists": True,
                "text_files": 0, "projects": 0, "atari_text_files": 0,
                "invalid_text_files": 0, "absolute_project_paths": [],
                "description": "EDITOBJC.SLB companion demo",
            })

    all_rows = rows + companions
    counts = {}
    for row in all_rows:
        counts[row["kind"]] = counts.get(row["kind"], 0) + 1
    counts["total"] = len(all_rows)
    return all_rows, counts


def markdown(rows, counts):
    def table(items):
        lines = ["| Status | Output | Source | C/H/S | PRJ | Atari text | Absolute PRJ paths |",
                 "|---|---|---|---:|---:|---:|---|"]
        for row in items:
            paths = ", ".join(f"`{path}`" for path in row["absolute_project_paths"]) or ""
            lines.append(
                f"| {row['status']} | `{row['output']}` | `{row['source']}` | "
                f"{row['text_files']} | {row['projects']} | {row['atari_text_files']} | {paths} |")
        return lines

    lines = [
        "# Port inventory scan",
        "",
        f"- Rows: **{counts['total']}**",
        f"- Built/verified: **{counts.get('built', 0)}**",
        f"- Remaining core: **{counts.get('remaining', 0)}**",
        f"- Historical: **{counts.get('historical', 0)}**",
        f"- Test/host: **{counts.get('test/host', 0)}**",
        f"- Blocked: **{counts.get('blocked', 0)}**",
        f"- Binary-only: **{counts.get('binary', 0)}**",
        "",
        "`Atari text` counts files that are valid Atari ST text but not UTF-8.",
        "`Absolute PRJ paths` are developer-machine paths that still need mapping.",
        "",
        "## Remaining core",
        "",
    ]
    lines.extend(table([row for row in rows if row["kind"] == "remaining"]))
    for title, kind in (("Historical", "historical"), ("Test/host", "test/host"),
                        ("Blocked", "blocked"), ("Binary-only", "binary"),
                        ("Unclassified", "unclassified"),
                        ("Build infrastructure", "infrastructure")):
        items = [row for row in rows if row["kind"] == kind]
        lines.extend(["", f"## {title}", ""])
        lines.extend(table(items) if items else ["None."])
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-o", "--output", help="write Markdown or JSON instead of stdout")
    parser.add_argument("--json", action="store_true", help="emit JSON")
    args = parser.parse_args()
    rows, counts = scan()
    if args.json:
        text = json.dumps({"counts": counts, "rows": rows}, indent=2) + "\n"
    else:
        text = markdown(rows, counts)
    if args.output:
        Path(args.output).write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
