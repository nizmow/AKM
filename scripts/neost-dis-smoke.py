#!/usr/bin/env python3
"""Compare archived and rebuilt DIS1/DIS2 against the bundled TST corpus.

DIS1 is compared on its own, then its archived workfiles feed both DIS2
binaries. Each case gets identical guest fixtures and is checked for child
status and guest file hashes. The shared console log contains only the
driver's ``STATUS`` line; child banners, usage and diagnostics are not captured.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import shutil
import sys
import tempfile

from neost_common import create_output, run

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
COMPARE = ROOT / "scripts/neost-console-compare.py"
DIS1_ARCHIVED = ROOT / "MagiC/TOOLS/DIS/BIN/DIS1.TTP"
DIS1_REBUILT = BUILD / "apps/dis1/DIS1.TTP"
DIS2_ARCHIVED = ROOT / "MagiC/TOOLS/DIS/BIN/DIS2.TTP"
DIS2_REBUILT = BUILD / "apps/dis2/DIS2.TTP"
TST = ROOT / "MagiC/TOOLS/DIS/TST"
DRI_OBJECT = ROOT / "KAOS/APPS/KAOSDESK.SRC/ICNEDIT/IMPORT/PCMXSTRT.O"


@dataclass(frozen=True)
class Case:
    name: str
    tool: str
    tail: str
    fixtures: tuple[Path, ...]


CASES = (
    Case("dis1-t", "dis1", "T.PRG", (TST / "T.PRG",)),
    Case("dis1-u", "dis1", "U.PRG", (TST / "U.PRG",)),
    Case("dis1-o", "dis1", "T.O", (TST / "T.O",)),
    Case("dis1-68030", "dis1", "-3 T.PRG", (TST / "T.PRG",)),
    Case("dis1-object", "dis1", "PCMXSTRT.O", (DRI_OBJECT,)),
    Case("dis1-noarg", "dis1", "", ()),
    Case("dis2-raw", "dis2", "-e T.PRG", (TST / "T.PRG",)),
    Case("dis2-raw-info", "dis2", "-e -c -a -z T.PRG", (TST / "T.PRG",)),
    Case("dis2-raw-68030", "dis2", "-e -3 T.PRG", (TST / "T.PRG",)),
    Case("dis2-raw-mas", "dis2", "-e -u -s T.PRG", (TST / "T.PRG",)),
    Case("dis2-range", "dis2", "-e 10-40 T.PRG", (TST / "T.PRG",)),
    Case("dis2-missing", "dis2", "MISSING.PRG", ()),
)


def compare_case(case: Case, output: Path, args: argparse.Namespace) -> int:
    archived, rebuilt = (
        (DIS1_ARCHIVED, DIS1_REBUILT) if case.tool == "dis1"
        else (DIS2_ARCHIVED, DIS2_REBUILT)
    )
    command = [
        sys.executable, str(COMPARE),
        "--archived", str(archived),
        "--rebuilt", str(rebuilt),
        "--tail", case.tail,
        "--compare", "status-files",
        "--frames", str(args.frames),
        "--timeout", str(args.timeout),
        "--output", str(output / case.name),
        "--neost", str(args.neost),
    ]
    if args.neost_headless:
        command.extend(["--neost-headless", str(args.neost_headless)])
    if args.rom:
        command.extend(["--rom", str(args.rom)])
    for fixture in case.fixtures:
        command.extend(["--fixture", str(fixture)])
    result = run(command, cwd=ROOT, timeout=args.timeout + 120)
    print(f"### {case.name}\n{result.stdout}", end="")
    if result.returncode:
        print(f"FAIL: {case.name}; evidence: {output / case.name}",
              file=sys.stderr)
    return result.returncode


def make_workfixture(output: Path, destination: Path) -> None:
    """Reuse archived DIS1 output, with a non-zero symbol length.

    The corpus programs have no symbol table. A four-byte length exercises the
    loader without changing the label or relocation tables.
    """
    guest = output / "dis1-t" / "guest-archived" / "C"
    for name in ("T.PRG", "T.lbl", "T.rlo"):
        source = guest / name
        if not source.is_file():
            raise RuntimeError(f"missing DIS1 fixture: {source}")
        shutil.copy2(source, destination / name)
    rlo = destination / "T.rlo"
    data = bytearray(rlo.read_bytes())
    if len(data) < 20:
        raise RuntimeError(f"short DIS1 relocation file: {rlo}")
    data[12:16] = (4).to_bytes(4, "big")
    rlo.write_bytes(data)


def make_objectfixture(output: Path, destination: Path) -> None:
    guest = output / "dis1-object" / "guest-archived" / "C"
    for name in ("PCMXSTRT.O", "PCMXSTRT.lbl", "PCMXSTRT.rlo", "PCMXSTRT.sym"):
        source = guest / name
        if not source.is_file():
            raise RuntimeError(f"missing DIS1 object fixture: {source}")
        shutil.copy2(source, destination / name)


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
                        help="use existing DIS binaries and console driver")
    args = parser.parse_args()
    if args.frames <= 0 or args.timeout <= 0:
        parser.error("frames and timeout must be positive")

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-dis")
    except OSError as error:
        parser.error(str(error))

    if not args.skip_build:
        build = run([str(ROOT / "scripts/atari-container"), "run", "make", "-f",
                     "Makefile.apps", "dis", "console-test-driver"],
                    cwd=ROOT, timeout=900)
        if build.returncode:
            print(build.stdout, file=sys.stderr)
            return build.returncode

    missing = [str(path) for path in (DIS1_ARCHIVED, DIS1_REBUILT,
                                      DIS2_ARCHIVED, DIS2_REBUILT)
               if not path.is_file()]
    driver = BUILD / "apps/console/RUNARG.PRG"
    if not driver.is_file():
        missing.append(str(driver))
    if missing:
        print("missing build output: " + ", ".join(missing), file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory(prefix="neost-dis-") as temporary:
        work = Path(temporary) / "work"
        work.mkdir()
        objwork = Path(temporary) / "objwork"
        objwork.mkdir()
        # move.w $8000(a0),d0: exercises the sign of a 16-bit displacement.
        neg = Path(temporary) / "NEG.BIN"
        neg.write_bytes(bytes.fromhex("30288000"))
        # movem.l sp,-(sp): mask bit 0 selects A7 under the reversed MOVEM map.
        movem = Path(temporary) / "MOVEM.BIN"
        movem.write_bytes(bytes.fromhex("48e70001"))
        cases = list(CASES) + [
            Case("dis2-neg-word", "dis2", "-e NEG.BIN", (neg,)),
            Case("dis2-movem", "dis2", "-e MOVEM.BIN", (movem,)),
        ]
        for case in cases:
            status = compare_case(case, output, args)
            if status:
                return status
            if case.name == "dis1-t":
                make_workfixture(output, work)
            elif case.name == "dis1-object":
                make_objectfixture(output, objwork)

        work_cases = (
            Case("dis2-work", "dis2", "T.PRG",
                 (work / "T.PRG", work / "T.lbl", work / "T.rlo")),
            Case("dis2-object", "dis2", "PCMXSTRT.O",
                 (objwork / "PCMXSTRT.O", objwork / "PCMXSTRT.lbl",
                  objwork / "PCMXSTRT.rlo", objwork / "PCMXSTRT.sym")),
        )
        for case in work_cases:
            status = compare_case(case, output, args)
            if status:
                return status

    print(f"All DIS comparisons passed. Evidence: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
