#!/usr/bin/env python3
"""Compare all six KEYB_SYS generators with their archived binaries under NeoST.

Each generator writes a single `KEYTABLS.SYS`. The runner reuses the generic
Pexec comparator: archive and rebuild run in separate disposable guest trees
and must report child status 0, create exactly `KEYTABLS.SYS`, and match console
output. Independent checks then require the nine 128-byte tables plus the
locale's dead-key trailer and six distinct layouts.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import sys
import tempfile

from neost_common import create_output, run, sha256

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
COMPARE = ROOT / "scripts/neost-console-compare.py"
ARCHIVE = ROOT / "base/keyb-sys/tests"
REBUILT = BUILD / "apps/keyb-sys"
# Nine 128-byte tables are always written; the dead-key trailer is per locale.
TABLE_BYTES = 9 * 128
DEAD_BYTES = {"CZ": 116, "DEI": 42, "FRA": 42, "SWG": 42, "UK": 42, "US": 42}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--neost", type=Path,
                        default=Path.home() / ".local/bin/neost")
    parser.add_argument("--neost-headless", type=Path)
    parser.add_argument("--rom", type=Path,
                        help="TOS ROM (default: NeoST's bundled EmuTOS)")
    parser.add_argument("--frames", type=int, default=1800)
    parser.add_argument("--timeout", type=int, default=120)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--skip-build", action="store_true")
    args = parser.parse_args()
    if args.frames <= 0 or args.timeout <= 0:
        parser.error("frames and timeout must be positive")

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-keyb-sys")
    except OSError as error:
        parser.error(str(error))

    if not args.skip_build:
        build = run([str(ROOT / "scripts/atari-container"), "run", "make", "-f",
                     "Makefile.apps", "keyb-sys", "console-test-driver"],
                    cwd=ROOT, timeout=900)
        if build.returncode:
            print(build.stdout, file=sys.stderr)
            return build.returncode

    missing = [str(ARCHIVE / f"KEYB_{name}.PRG") for name in DEAD_BYTES
               if not (ARCHIVE / f"KEYB_{name}.PRG").is_file()]
    missing += [str(REBUILT / f"KEYB_{name}.PRG") for name in DEAD_BYTES
                if not (REBUILT / f"KEYB_{name}.PRG").is_file()]
    driver = BUILD / "apps/console/RUNARG.PRG"
    if not driver.is_file():
        missing.append(str(driver))
    if missing:
        print("missing build output: " + ", ".join(missing), file=sys.stderr)
        return 1

    layouts: dict[str, str] = {}
    with tempfile.TemporaryDirectory(prefix="neost-keyb-sys-") as temporary:
        for name in DEAD_BYTES:
            case = output / name
            command = [sys.executable, str(COMPARE),
                       "--archived", str(ARCHIVE / f"KEYB_{name}.PRG"),
                       "--rebuilt", str(REBUILT / f"KEYB_{name}.PRG"),
                       "--expect-status", "0",
                       "--expect-file", "KEYTABLS.SYS",
                       "--compare", "all",
                       "--frames", str(args.frames),
                       "--timeout", str(args.timeout),
                       "--output", str(case)]
            command.extend(["--neost", str(args.neost)])
            if args.neost_headless:
                command.extend(["--neost-headless", str(args.neost_headless)])
            if args.rom:
                command.extend(["--rom", str(args.rom)])
            # The comparator runs archive and rebuild as two sequential NeoST
            # sessions, each bounded by --timeout, plus fixture preparation.
            result = run(command, cwd=ROOT, timeout=2 * args.timeout + 120)
            print(f"### KEYB_{name}\n{result.stdout}", end="")
            if result.returncode:
                print(f"FAIL: KEYB_{name}; evidence: {case}", file=sys.stderr)
                return result.returncode

            archived = case / "guest-archived" / "C" / "KEYTABLS.SYS"
            rebuilt = case / "guest-rebuilt" / "C" / "KEYTABLS.SYS"
            for path in (archived, rebuilt):
                data = path.read_bytes()
                expected = TABLE_BYTES + DEAD_BYTES[name]
                if len(data) != expected:
                    print(f"FAIL: {name} wrote {len(data)} bytes, "
                          f"expected {expected}", file=sys.stderr)
                    return 1
            if archived.read_bytes() != rebuilt.read_bytes():
                print(f"FAIL: {name} tables differ", file=sys.stderr)
                return 1
            layouts[name] = sha256(rebuilt)

    if len(set(layouts.values())) != len(layouts):
        print("FAIL: generators did not produce six distinct layouts:\n"
              f"{layouts}", file=sys.stderr)
        return 1
    print(f"All six KEYB_SYS generators match their archives. Evidence: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
