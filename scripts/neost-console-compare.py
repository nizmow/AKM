#!/usr/bin/env python3
"""Compare archived and rebuilt console binaries through a generic Pexec driver."""

import argparse
import re
import shutil
import sys
import tempfile
from pathlib import Path

from neost_common import acquire_headless, create_output, resolve_rom, sha256
from neost_server import NeoSTServer

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
DEFAULT_DRIVER = BUILD / "apps/console/RUNARG.PRG"
STATUS_RE = re.compile(r"^STATUS\s+(-?\d+)\s*$", re.MULTILINE)

sys.path.insert(0, str(ROOT / "NON-TOS/atari-utf8-conversion"))
import atari_text


def atari_bytes(text):
    return atari_text.convert(text.encode("utf-8"), "encode")


def copy_fixture(source, destination):
    source = source.expanduser().resolve()
    if source.is_dir():
        for child in source.iterdir():
            copy_fixture(child, destination)
    else:
        target = destination / source.name
        if target.exists():
            raise RuntimeError(f"fixture collides with {target}")
        shutil.copy2(source, target)


def prepare(guest, executable, tail, driver):
    guest.mkdir(parents=True)
    suffix = executable.suffix.upper() if executable.suffix.upper() in (".TTP", ".PRG") else ".PRG"
    target_name = "TARGET" + suffix
    shutil.copy2(executable, guest / target_name)
    shutil.copy2(driver, guest / "RUNARG.PRG")
    config = f"C:\\{target_name}\r\n{tail}\r\n".encode("utf-8")
    (guest / "RUNARG.TXT").write_bytes(atari_bytes(config.decode("utf-8")))
    return target_name


def invoke(headless, rom, guest, output, label, frames, timeout):
    console = output / f"{label}.console.log"
    screenshot = output / f"{label}.ppm"
    options = ["--machine", "st", "--mem", "4m", "--no-disk",
               "--gemdos", str(guest), "--auto", r"C:\RUNARG.PRG",
               "--console-log", str(console)]
    server_dir = output / f"{label}-server"
    server_dir.mkdir()
    with NeoSTServer(headless, rom, server_dir, options=options,
                     timeout=timeout) as server:
        server.run_frames(frames)
        server.screenshot(screenshot)
    if not console.is_file():
        raise RuntimeError(f"NeoST did not create {console}")
    text = console.read_text(encoding="latin-1")
    match = STATUS_RE.search(text)
    if not match:
        raise RuntimeError(f"{label}: missing STATUS line; see {console}")
    files = {}
    for path in sorted(guest.rglob("*")):
        if not path.is_file() or path.name in ("RUNARG.PRG", "RUNARG.TXT") or path.name.startswith("TARGET."):
            continue
        files[str(path.relative_to(guest))] = sha256(path)
    return int(match.group(1)), text, files


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archived", type=Path, required=True)
    parser.add_argument("--rebuilt", type=Path, required=True)
    parser.add_argument("--tail", default="", help="child command tail")
    parser.add_argument("--fixture", type=Path, action="append", default=[],
                        help="file or directory copied into each guest C: root")
    parser.add_argument("--driver", type=Path, default=DEFAULT_DRIVER)
    parser.add_argument("--compare",
                        choices=("status", "console", "files", "status-files", "all"),
                        default="all")
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
    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-console")
    except OSError as error:
        parser.error(str(error))
    with tempfile.TemporaryDirectory(prefix="neost-console-") as temporary:
        headless, app_root = acquire_headless(args.neost_headless, args.neost, Path(temporary))
        rom = resolve_rom(args.rom, app_root)
        results = {}
        for label, executable in (("archived", args.archived), ("rebuilt", args.rebuilt)):
            guest = output / f"guest-{label}" / "C"
            prepare(guest, executable, args.tail, args.driver)
            for fixture in args.fixture:
                copy_fixture(fixture, guest)
            results[label] = invoke(headless, rom, guest, output, label,
                                    args.frames, args.timeout)
    a_status, a_console, a_files = results["archived"]
    r_status, r_console, r_files = results["rebuilt"]
    failures = []
    if args.compare in ("status", "status-files", "all") and a_status != r_status:
        failures.append(f"status archived={a_status} rebuilt={r_status}")
    if args.compare in ("console", "all") and a_console != r_console:
        failures.append("console output differs")
    if args.compare in ("files", "status-files", "all") and a_files != r_files:
        failures.append("guest file output differs")
    summary = [
        f"archived status: {a_status}",
        f"rebuilt status:  {r_status}",
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
