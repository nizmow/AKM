#!/usr/bin/env python3
"""Compare archived and rebuilt MagiC console tools under NeoST.

Each case runs both binaries through the generic Pexec driver, with identical
fixtures, and compares console output and guest file hashes. Every case declares
the child status and generated guest files it expects; fixture inputs are not
counted as outputs. Missing argument files are intentional errors, so the tools
exit through ``Pterm`` with ``ENOENT`` (0x0000ffdf = 65503) rather than failing
to load.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import sys
import tempfile

from neost_common import create_output, run

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
COMPARE = ROOT / "scripts/neost-console-compare.py"

TOOLS = {
    "mac2st": (ROOT / "base/mac2st/BIN/MAC2ST.TTP",
               BUILD / "apps/mac2st/MAC2ST.TTP"),
    "dump": (ROOT / "base/dump/BIN/DUMP.TTP",
             BUILD / "apps/dump/DUMP.TTP"),
    "fc": (ROOT / "base/fc/BIN/FC.TTP",
           BUILD / "apps/fc/FC.TTP"),
    "split": (ROOT / "base/split/BIN/SPLIT.TTP",
              BUILD / "apps/split/SPLIT.TTP"),
}


@dataclass(frozen=True)
class Case:
    name: str
    tool: str
    tail: str
    status: int
    outputs: tuple[str, ...]
    fixtures: tuple[str, ...]


CASES = (
    Case("mac2st", "mac2st", "MAC2ST.TXT", 0, (), ("MAC2ST.TXT",)),
    Case("dump", "dump", "DUMP.TXT", 0, (), ("DUMP.TXT",)),
    Case("dump-c", "dump", "-c DUMP.TXT", 0, (), ("DUMP.TXT",)),
    Case("fc-equal", "fc", "EQUAL-A.TXT EQUAL-B.TXT", 0, (),
         ("EQUAL-A.TXT", "EQUAL-B.TXT")),
    Case("fc-diff", "fc", "DIFF-A.TXT DIFF-B.TXT", 0, (),
         ("DIFF-A.TXT", "DIFF-B.TXT")),
    Case("fc-word", "fc", "-w DIFF-A.TXT DIFF-B.TXT", 0, (),
         ("DIFF-A.TXT", "DIFF-B.TXT")),
    Case("fc-long", "fc", "-l DIFF-A.TXT DIFF-B.TXT", 0, (),
         ("DIFF-A.TXT", "DIFF-B.TXT")),
    Case("split", "split", "-10 SPLIT.TXT OUT1.TXT OUT2.TXT", 0,
         ("OUT1.TXT", "OUT2.TXT"), ("SPLIT.TXT",)),
    # Missing argument files are intentional Pterm(-33) exits.
    Case("mac2st-missing", "mac2st", "MISSING.TXT", 65503, (), ()),
    Case("dump-missing", "dump", "MISSING.TXT", 65503, (), ()),
    Case("fc-missing", "fc", "MISSING-A.TXT MISSING-B.TXT", 65503, (), ()),
    Case("split-missing", "split", "MISSING.TXT OUT1.TXT OUT2.TXT", 65503, (), ()),
)


def make_fixtures(directory: Path) -> None:
    directory.mkdir(parents=True)
    files = {
        "MAC2ST.TXT": b"Alpha line 1\r\nBeta line 2\r\nGamma line 3\r\n",
        "DUMP.TXT": b"Hello, DUMP!\n0123456789abcdef\n",
        "EQUAL-A.TXT": b"same\n",
        "EQUAL-B.TXT": b"same\n",
        "DIFF-A.TXT": b"left\n",
        "DIFF-B.TXT": b"right\n",
        "SPLIT.TXT": b"0123456789abcdefghijklmnopqrstuvwxyz\n",
    }
    for name, content in files.items():
        (directory / name).write_bytes(content)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--neost", type=Path,
                        default=Path.home() / ".local/bin/neost",
                        help="NeoST AppImage (default: ~/.local/bin/neost)")
    parser.add_argument("--neost-headless", type=Path,
                        help="direct path to the bundled neost-headless executable")
    parser.add_argument("--rom", type=Path,
                        help="TOS ROM (default: NeoST's bundled EmuTOS)")
    parser.add_argument("--frames", type=int, default=1800)
    parser.add_argument("--timeout", type=int, default=120)
    parser.add_argument("--output", type=Path,
                        help="new output directory (default: timestamped build/atari path)")
    parser.add_argument("--skip-build", action="store_true",
                        help="use existing tool binaries and console driver")
    args = parser.parse_args()
    if args.frames <= 0 or args.timeout <= 0:
        parser.error("frames and timeout must be positive")

    try:
        output = create_output(args.output, parent=BUILD,
                               prefix="neost-console-tools")
    except OSError as error:
        parser.error(str(error))

    if not args.skip_build:
        build = run([str(ROOT / "scripts/atari-container"), "run", "make", "-f",
                     "Makefile.apps", "mac2st", "dump", "fc", "split",
                     "console-test-driver"], cwd=ROOT, timeout=900)
        if build.returncode:
            print(build.stdout, file=sys.stderr)
            return build.returncode

    missing = [str(path) for pair in TOOLS.values() for path in pair
               if not path.is_file()]
    driver = BUILD / "apps/console/RUNARG.PRG"
    if not driver.is_file():
        missing.append(str(driver))
    if missing:
        print("missing build output: " + ", ".join(missing), file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory(prefix="neost-console-tools-") as temporary:
        fixtures = Path(temporary) / "fixtures"
        make_fixtures(fixtures)
        for case in CASES:
            archived, rebuilt = TOOLS[case.tool]
            command = [sys.executable, str(COMPARE),
                       "--archived", str(archived),
                       "--rebuilt", str(rebuilt),
                       "--tail", case.tail,
                       "--expect-status", str(case.status),
                       "--compare", "all",
                       "--frames", str(args.frames),
                       "--timeout", str(args.timeout),
                       "--output", str(output / case.name)]
            for expected in case.outputs:
                command.extend(["--expect-file", expected])
            command.extend(["--neost", str(args.neost)])
            if args.neost_headless:
                command.extend(["--neost-headless", str(args.neost_headless)])
            if args.rom:
                command.extend(["--rom", str(args.rom)])
            for fixture in case.fixtures:
                command.extend(["--fixture", str(fixtures / fixture)])
            result = run(command, cwd=ROOT, timeout=args.timeout + 120)
            print(f"### {case.name}\n{result.stdout}", end="")
            if result.returncode:
                print(f"FAIL: {case.name}; evidence: {output / case.name}",
                      file=sys.stderr)
                return result.returncode

    print(f"All console-tool comparisons passed. Evidence: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
