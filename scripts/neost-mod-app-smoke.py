#!/usr/bin/env python3
"""Run original and GCC MOD_APP binaries through NeoST's headless TOS boot."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

from neost_common import acquire_headless, create_output, resolve_rom, run, run_logged, sha256

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
APP_BUILD = BUILD / "apps/mod-app"
FIXTURES = BUILD / "mod-app-smoke"
ORIGINAL = ROOT / "MagiC/Releases/MAGIC.A/GEMSYS/GEMDESK/MOD_APP.TTP"
GCC_APP = APP_BUILD / "MOD_APP.TTP"
DRIVER = APP_BUILD / "NEOSTEST.PRG"
EXPECTED = [0, 3, 0, 3, 4, 0, 3, 0, 0, 0, 1, 1, 0, 3, 0, 0, 3]
CASE_LINE = re.compile(r"^CASE\s+(\d{2})\s+(-?\d+)\s*$", re.MULTILINE)


def prepare_case(root: Path, executable: Path) -> None:
    gemdos = root / "C"
    desk = gemdos / "GEMSYS/GEMDESK"
    desk.mkdir(parents=True)
    shutil.copy2(FIXTURES / "ENGLISH.INF", desk / "APPLICAT.INF")
    shutil.copy2(FIXTURES / "BASE.DAT", desk / "APPLICAT.DAT")
    shutil.copy2(executable, gemdos / "MOD_APP.TTP")
    shutil.copy2(DRIVER, gemdos / "NEOSTEST.PRG")


def invoke(headless: Path, rom: Path, root: Path, output: Path, label: str, frames: int) -> tuple[list[int], dict[str, str]]:
    gemdos = root / "C"
    console = output / f"{label}.console.log"
    hostlog = output / f"{label}.host.log"
    screenshot = output / f"{label}.ppm"
    command = [str(headless), "--machine", "st", "--mem", "4m", "--no-disk",
               "--frames", str(frames), "--gemdos", str(gemdos),
               "--auto", r"C:\NEOSTEST.PRG", "--console-log", str(console),
               "--screenshot", str(screenshot), str(rom)]
    run_logged(command, hostlog, cwd=ROOT, timeout=240)
    if not console.is_file():
        raise RuntimeError(f"NeoST did not create {console}; see {hostlog}")

    text = console.read_text(encoding="latin-1")
    cases = [(int(number), int(status) & 0xFFFF)
             for number, status in CASE_LINE.findall(text)]
    expected_cases = list(enumerate(EXPECTED, start=1))
    if cases != expected_cases or "DONE 17" not in text:
        raise RuntimeError(f"{label}: missing/mismatched guest results {cases}; see {console}")
    artifacts = {}
    for name in ("APPLICAT.INF", "APPLICAT.BAK", "APPLICAT.DAT"):
        path = gemdos / "GEMSYS/GEMDESK" / name
        artifacts[name] = sha256(path) if path.is_file() else "MISSING"
    return [status for _, status in cases], artifacts


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--neost", type=Path, default=Path.home() / ".local/bin/neost",
                        help="NeoST AppImage (default: ~/.local/bin/neost)")
    parser.add_argument("--neost-headless", type=Path,
                        help="direct path to the bundled neost-headless executable")
    parser.add_argument("--rom", type=Path, help="TOS ROM; default is bundled EmuTOS 1.04")
    parser.add_argument("--frames", type=int, default=1200,
                        help="frames per boot (default: 1200)")
    parser.add_argument("--output", type=Path,
                        help="new output directory (default: timestamped build/atari path)")
    args = parser.parse_args()

    if args.frames < 1:
        parser.error("--frames must be positive")
    if not ORIGINAL.is_file():
        parser.error(f"archived original MOD_APP missing: {ORIGINAL}")
    if not (FIXTURES / "ENGLISH.INF").is_file() or not (FIXTURES / "BASE.DAT").is_file():
        parser.error(f"MOD_APP fixtures missing under {FIXTURES}")

    build = run([str(ROOT / "scripts/atari-container"), "run", "make", "-f",
                 "Makefile.apps", "mod-app", "mod-app-test-driver"], cwd=ROOT, timeout=600)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    if not GCC_APP.is_file() or not DRIVER.is_file():
        print("build completed without producing MOD_APP.TTP and NEOSTEST.PRG", file=sys.stderr)
        return 1

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-mod-app-smoke")
    except OSError as error:
        parser.error(str(error))
    try:
        with tempfile.TemporaryDirectory(prefix="neost-mod-app-") as temporary:
            headless, app_root = acquire_headless(args.neost_headless, args.neost,
                                                   Path(temporary))
            rom = resolve_rom(args.rom, app_root)
            outcomes = {}
            snapshots = {}
            for label, binary in (("original", ORIGINAL), ("gcc", GCC_APP)):
                guest = output / f"guest-{label}"
                prepare_case(guest, binary)
                statuses, artifacts = invoke(headless, rom, guest, output, label, args.frames)
                outcomes[label] = statuses
                snapshots[label] = artifacts
                (output / f"{label}.results.txt").write_text(
                    "".join(f"{i:02d} {status}\n" for i, status in enumerate(statuses, 1)),
                    encoding="ascii")

            report = {"emulator": str(headless), "rom": str(rom), "frames": args.frames,
                      "cases": len(EXPECTED), "expected_statuses": EXPECTED,
                      "outcomes": outcomes, "snapshots_sha256": snapshots}
            (output / "report.json").write_text(json.dumps(report, indent=2) + "\n",
                                                encoding="utf-8")
            if outcomes["original"] != outcomes["gcc"]:
                raise RuntimeError("original/GCC exit statuses differ; see report.json")
            if snapshots["original"] != snapshots["gcc"]:
                raise RuntimeError("original/GCC APPLICAT files differ; see report.json")
            print(f"NeoST MOD_APP smoke passed: {len(EXPECTED)} cases per binary; "
                  f"INF/BAK/DAT snapshots match. Results: {output}")
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"NeoST MOD_APP smoke failed: {error}\nArtifacts: {output}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
