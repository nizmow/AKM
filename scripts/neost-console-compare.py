#!/usr/bin/env python3
"""Compare archived and rebuilt console binaries through a generic Pexec driver.

Each case declares the child status and the generated guest files it expects.
Fixture inputs are excluded from the output comparison, so a missing or extra
guest output fails the case. Pexec reports loader errors as negative 32-bit
values and child exits as 16-bit values, so an out-of-range status is a loader
failure and is never accepted as a matching child status.
"""

import argparse
import sys
import tempfile
from pathlib import Path

from neost_common import acquire_headless, create_output, resolve_rom
from neost_console import DEFAULT_DRIVER, copy_fixture, guest_name, invoke, prepare

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archived", type=Path, required=True)
    parser.add_argument("--rebuilt", type=Path, required=True)
    parser.add_argument("--tail", default="", help="child command tail")
    parser.add_argument("--fixture", type=Path, action="append", default=[],
                        help="file or directory copied into each guest C: root")
    parser.add_argument("--driver", type=Path, default=DEFAULT_DRIVER)
    parser.add_argument("--expect-status", type=int, required=True,
                        help="child status both binaries must report")
    parser.add_argument("--expect-file", action="append", default=[],
                        metavar="NAME",
                        help="generated guest file both binaries must create; "
                             "repeat for each output (driver, target and fixtures excluded)")
    parser.add_argument("--compare",
                        choices=("status", "console", "files", "status-files", "all"),
                        default="all",
                        help="console comparison mode; status and output files "
                             "are always checked against the declared expectations")
    parser.add_argument("--frames", type=int, default=1800)
    parser.add_argument("--timeout", type=int, default=120)
    parser.add_argument("--neost", type=Path, default=Path.home() / ".local/bin/neost")
    parser.add_argument("--neost-headless", type=Path)
    parser.add_argument("--rom", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    for path in (args.archived, args.rebuilt, args.driver):
        if not path.is_file():
            parser.error(f"missing file: {path}")
    if args.frames <= 0 or args.timeout <= 0:
        parser.error("frames and timeout must be positive")
    if not 0 <= args.expect_status <= 0xFFFF:
        parser.error("--expect-status must be a 16-bit child status")
    expected_files = {guest_name(name) for name in args.expect_file}
    if len(expected_files) != len(args.expect_file):
        parser.error("duplicate --expect-file values")
    for name in expected_files:
        if name in ("RUNARG.PRG", "RUNARG.TXT") or name.startswith("TARGET."):
            parser.error(f"--expect-file names a driver file: {name}")
        if name.startswith("/") or ".." in name.split("/"):
            parser.error(f"invalid --expect-file name: {name}")
    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-console")
    except OSError as error:
        parser.error(str(error))
    with tempfile.TemporaryDirectory(prefix="neost-console-") as temporary:
        headless, app_root = acquire_headless(args.neost_headless, args.neost, Path(temporary))
        rom = resolve_rom(args.rom, app_root)
        results = {}
        guest_inputs = None
        for label, executable in (("archived", args.archived), ("rebuilt", args.rebuilt)):
            guest = output / f"guest-{label}" / "C"
            prepare(guest, executable, args.tail, args.driver)
            copied = set()
            for fixture in args.fixture:
                copied |= copy_fixture(fixture, guest)
            if guest_inputs is None:
                guest_inputs = copied
                overlap = expected_files & copied
                if overlap:
                    parser.error("--expect-file names a fixture input: "
                                 + ", ".join(sorted(overlap)))
            elif copied != guest_inputs:
                raise RuntimeError("fixture names differ between guest trees")
            results[label] = invoke(headless, rom, guest, output, label,
                                    args.frames, args.timeout,
                                    args.expect_status, expected_files,
                                    guest_inputs)
    a_status, a_console, a_files = results["archived"]
    r_status, r_console, r_files = results["rebuilt"]
    failures = []
    if args.compare in ("console", "all") and a_console != r_console:
        failures.append("console output differs")
    if args.compare in ("files", "status-files", "all") and a_files != r_files:
        failures.append("guest file output differs")
    summary = [
        f"expected status: {args.expect_status}",
        f"archived status: {a_status}",
        f"rebuilt status:  {r_status}",
        f"expected files:  {sorted(expected_files)}",
        f"archived files:  {a_files}",
        f"rebuilt files:   {r_files}",
        "result: " + ("PASS" if not failures else "FAIL"),
    ]
    if a_console != r_console:
        (output / "console.diff").write_text(
            "--- archived ---\n" + a_console + "\n--- rebuilt ---\n" + r_console,
            encoding="utf-8")
    (output / "comparison.txt").write_text("\n".join(summary) + "\n", encoding="utf-8")
    print("\n".join(summary))
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"Evidence: {output}", file=sys.stderr)
        return 1
    print(f"Evidence: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
