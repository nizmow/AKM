#!/usr/bin/env python3
"""Check rebuilt console tools through Pexec against explicit guest results."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from hashlib import sha256
from pathlib import Path
import sys
import tempfile

from neost_common import acquire_headless, create_output, resolve_rom, run
from neost_console import DEFAULT_DRIVER, copy_fixture, invoke, prepare

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"

TOOLS = {name: BUILD / f"apps/{name}/{name.upper()}.TTP"
         for name in ("mac2st", "dump", "fc", "split")}


@dataclass(frozen=True)
class Case:
    name: str
    tool: str
    tail: str
    status: int
    console: str
    fixtures: tuple[str, ...]
    outputs: tuple[tuple[str, bytes], ...] = ()


# The harness normalizes console line endings and decodes Atari bytes as Latin-1.
# MAC2ST expands CR even in CRLF input, leaving the original LF in place.
FC_DIFFERENT_LENGTHS = ("Datei1:  DIFF-A.TXT\n"
                        "                Datei2: DIFF-B.TXT\n\n"
                        "Dateil\x84nge unterschiedlich\n")
CASES = (
    Case("mac2st", "mac2st", "MAC2ST.TXT", 0,
         "Alpha line 1\n\nBeta line 2\n\nGamma line 3\n\n", ("MAC2ST.TXT",)),
    Case("dump", "dump", "DUMP.TXT", 0,
         " Datei: DUMP.TXT\n\n"
         "000000  4865 6c6c 6f2c 2044 554d 5021 0a30 3132  Hello, DUMP!.012\n"
         "000010  3334 3536 3738 3961 6263 6465 660a       3456789abcdef.\n",
         ("DUMP.TXT",)),
    Case("dump-c", "dump", "-c DUMP.TXT", 0,
         "int  dump[] = {\n"
         "0x4865,0x6c6c,0x6f2c,0x2044,0x554d,0x5021,0x0a30,0x3132,\n"
         "0x3334,0x3536,0x3738,0x3961,0x6263,0x6465,0x660a\n};\n",
         ("DUMP.TXT",)),
    Case("fc-equal", "fc", "EQUAL-A.TXT EQUAL-B.TXT", 0,
         "Datei1:  EQUAL-A.TXT\n                Datei2: EQUAL-B.TXT\n\n",
         ("EQUAL-A.TXT", "EQUAL-B.TXT")),
    Case("fc-diff", "fc", "DIFF-A.TXT DIFF-B.TXT", 0,
         FC_DIFFERENT_LENGTHS, ("DIFF-A.TXT", "DIFF-B.TXT")),
    Case("fc-word", "fc", "-w DIFF-A.TXT DIFF-B.TXT", 0,
         FC_DIFFERENT_LENGTHS, ("DIFF-A.TXT", "DIFF-B.TXT")),
    Case("fc-long", "fc", "-l DIFF-A.TXT DIFF-B.TXT", 0,
         FC_DIFFERENT_LENGTHS, ("DIFF-A.TXT", "DIFF-B.TXT")),
    Case("split", "split", "-10 SPLIT.TXT OUT1.TXT OUT2.TXT", 0, "",
         ("SPLIT.TXT",), (("OUT1.TXT", b"0123456789abcdef"),
                          ("OUT2.TXT", b"ghijklmnopqrstuvwxyz\n"))),
    # Missing argument files exit through Pterm(-33), with no console output.
    Case("mac2st-missing", "mac2st", "MISSING.TXT", 65503, "", ()),
    Case("dump-missing", "dump", "MISSING.TXT", 65503, "", ()),
    Case("fc-missing", "fc", "MISSING-A.TXT MISSING-B.TXT", 65503, "", ()),
    Case("split-missing", "split", "MISSING.TXT OUT1.TXT OUT2.TXT", 65503, "", ()),
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

    missing = [str(path) for path in (*TOOLS.values(), DEFAULT_DRIVER)
               if not path.is_file()]
    if missing:
        print("missing build output: " + ", ".join(missing), file=sys.stderr)
        return 1

    try:
        with tempfile.TemporaryDirectory(prefix="neost-console-tools-") as temporary:
            fixtures = Path(temporary) / "fixtures"
            make_fixtures(fixtures)
            headless, app_root = acquire_headless(args.neost_headless, args.neost,
                                                  Path(temporary))
            rom = resolve_rom(args.rom, app_root)
            for case in CASES:
                case_output = output / case.name
                guest = case_output / "guest-rebuilt" / "C"
                prepare(guest, TOOLS[case.tool], case.tail, DEFAULT_DRIVER)
                inputs = set()
                for fixture in case.fixtures:
                    inputs.update(copy_fixture(fixtures / fixture, guest))
                expected_files = {name: sha256(data).hexdigest()
                                  for name, data in case.outputs}
                status, console, files = invoke(
                    headless, rom, guest, case_output, "rebuilt", args.frames,
                    args.timeout, case.status, set(expected_files), inputs)
                if console != case.console + f"STATUS {status}\n":
                    raise RuntimeError(f"{case.name}: unexpected console output {console!r}")
                if files != expected_files:
                    raise RuntimeError(f"{case.name}: unexpected output bytes in {guest}")
                print(f"{case.name}: PASS", flush=True)
    except (OSError, RuntimeError) as error:
        print(f"FAIL: {error}; evidence: {output}", file=sys.stderr)
        return 1

    print(f"All console-tool cases passed. Evidence: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
