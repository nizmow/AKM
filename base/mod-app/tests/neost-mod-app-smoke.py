#!/usr/bin/env python3
"""Run the rebuilt MOD_APP and check explicit per-case file effects.

Each case runs through the guest Pexec driver, which snapshots APPLICAT.INF,
APPLICAT.BAK and APPLICAT.DAT immediately after the child exits. The host then
requires the declared child status and the exact INF/BAK/DAT contents for that
step: insertion text and position, backup rotation, cache invalidation, and
unchanged files for validation/duplicate/missing-application errors.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import sys
import tempfile

COMPONENT = Path(__file__).resolve().parents[1]
ROOT = COMPONENT.parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from neost_common import acquire_headless, create_output, resolve_rom, run
from neost_server import NeoSTServer

BUILD = ROOT / "build/atari"
APP_BUILD = BUILD / "apps/mod-app"
GCC_APP = APP_BUILD / "MOD_APP.TTP"
DRIVER = APP_BUILD / "NEOSTEST.PRG"
SEED_INF = COMPONENT / "tests/APPLICAT.IN"
SEED_DAT = COMPONENT / "tests/APPLICAT.DAT"

CRLF = b"\r\n"
CASE_LINE = re.compile(r"^CASE\s+(\d{2})\s+(-?\d+)\s*$", re.MULTILINE)
LONG_PATH = "C:\\" + "A" * 80

# (kind, expected child status, arguments); statuses are MOD_APP's documented
# return values: 0 written, 1 bad argument, 3 duplicate, 4 application missing.
CASES = [
    ("app", 0, {"pgm": "papillon.prg", "rsc": "-", "icon": -1, "path": "-", "code": 1}),
    ("app", 3, {"pgm": "papillon.prg", "rsc": "-", "icon": -1, "path": "-", "code": 1}),
    ("id", 0, {"pgm": "papillon.prg", "ftyp": "*.x", "rsc": "-", "icon": -1}),
    ("id", 3, {"pgm": "papillon.prg", "ftyp": "*.x", "rsc": "-", "icon": -1}),
    ("id", 4, {"pgm": "missing.prg", "ftyp": "*.z", "rsc": "-", "icon": -1}),
    ("is", 0, {"key": "ABCD", "name": "sample", "rsc": "-", "icon": 12}),
    ("is", 3, {"key": "ABCD", "name": "sample", "rsc": "-", "icon": 12}),
    ("app", 0, {"pgm": "space.prg", "rsc": "-", "icon": -1,
                "path": "C:\\MY APPS", "code": 1}),
    ("app", 0, {"pgm": "empty.prg", "rsc": "-", "icon": -1, "path": "", "code": 1}),
    ("app", 0, {"pgm": "longpath.prg", "rsc": "-", "icon": -1,
                "path": LONG_PATH, "code": 1}),
    ("app", 1, {"pgm": "tool.prg", "rsc": "RESOURCE-NAME-TOO-LONG", "icon": 0,
                "path": "-", "code": 1}),
    ("id", 1, {"pgm": "papillon.prg", "ftyp": "123456789012345678901234567890123",
               "rsc": "-", "icon": -1}),
    ("app", 0, {"pgm": "code.prg", "rsc": "-", "icon": 0, "path": "-", "code": 7}),
    ("app", 3, {"pgm": "code.prg", "rsc": "-", "icon": -1, "path": "-", "code": 1}),
    ("id", 0, {"pgm": "papillon.prg", "ftyp": "*.y", "rsc": "-", "icon": -1}),
    ("app", 0, {"pgm": "quoted.prg", "rsc": "-", "icon": -1,
                "path": "D:\\APP FILES\\PROGRAMS", "code": 1}),
    ("id", 3, {"pgm": "papillon.prg", "ftyp": "*.x", "rsc": "-", "icon": -1}),
    ("id", 0, {"pgm": "papillon.prg", "ftyp": "*.a", "rsc": "-", "icon": -1}),
]
EXPECTED = [status for _, status, _ in CASES]
RESTORE_CACHE_BEFORE = {2, 5, 11}

LABELS = {
    "english": {"free": "<free>", "special": "<Special>"},
    "german": {"free": "<frei>", "special": "<Spezial>"},
}


def section(pgm: str) -> bytes:
    """name_to_section: '[' + up to eight characters before '.' + ']'."""
    name = ""
    for char in pgm:
        if char == "." or len(name) == 8:
            break
        name += char.upper()
    return f"[{name}]".encode("ascii")


def cstrncmp(left: bytes, right: bytes, length: int) -> int:
    for index in range(length):
        a = left[index] if index < len(left) else 0
        b = right[index] if index < len(right) else 0
        if a != b:
            return -1 if a < b else 1
        if a == 0:
            return 0
    return 0


def app_insert_index(inf: bytes, name: bytes) -> int:
    """Replay do_mode_ia's section scan: first section sorting after `name`."""
    at = 0
    while True:
        at = inf.find(CRLF + b"[", at)
        if at < 0:
            raise AssertionError(f"no section after {name!r}")
        if inf[at + 3:at + 4] == b"<":
            order = 1
        else:
            order = cstrncmp(inf[at + 2:], name, len(name))
        if order == 0:
            raise AssertionError(f"unexpected duplicate {name!r}")
        if order > 0:
            return at + 2
        at += 3


def section_block(inf: bytes, name: str) -> bytes:
    start = inf.index(b"[" + name.encode("ascii") + b"]")
    end = inf.index(CRLF + b"[", start + 1) + 2
    return inf[start:end]


def rscinfo(rsc: str, icon: int) -> bytes:
    return f'{rsc} {icon} "-"\r\n'.encode("ascii")


def app_block(args: dict) -> bytes:
    path = args["path"]
    body = "" if path == "-" else (f"'{path}'" if " " in path else path)
    return (section(args["pgm"]) + CRLF + rscinfo(args["rsc"], args["icon"])
            + body.encode("ascii") + CRLF + str(args["code"]).encode("ascii") + CRLF)


def type_position(block: bytes, prefix: bytes) -> int:
    """Position in an application block where do_mode_id inserts a type line."""
    position = 0
    for _ in range(4):  # name, resource, path, code lines
        position = block.index(CRLF, position) + 2
    while position < len(block):
        end = block.index(CRLF, position)
        if cstrncmp(block[position:end], prefix, len(prefix)) > 0:
            return position
        position = end + 2
    return position


def expected_after(inf: bytes, kind: str, args: dict, labels: dict) -> bytes:
    if kind == "app":
        index = app_insert_index(inf, section(args["pgm"]))
        return inf[:index] + app_block(args) + inf[index:]
    if kind == "id":
        prefix = args["ftyp"].upper().encode("ascii") + b" "
        line = prefix + rscinfo(args["rsc"], args["icon"])
        block = section_block(inf, section(args["pgm"]).decode("ascii")[1:-1])
        position = type_position(block, prefix)
        return inf.replace(block, block[:position] + line + block[position:], 1)
    if kind == "is":
        line = (f'{args["key"]} {args["name"]} '.encode("ascii")
                + rscinfo(args["rsc"], args["icon"]))
        start = inf.index(CRLF + b"[" + labels["special"].encode("ascii") + b"]")
        end = inf.index(CRLF + b"[", start + 1) + 2
        return inf[:end] + line + inf[end:]
    raise AssertionError(f"unknown case kind {kind!r}")


def validate(label: str, seed_inf: bytes, seed_dat: bytes,
             snapshots: dict, statuses: list[int]) -> None:
    inf, bak, dat = seed_inf, None, seed_dat
    if snapshots[0] != (seed_inf, None, seed_dat):
        raise RuntimeError(f"{label}: initial snapshot differs from the seed fixture")
    for number, ((kind, status, args), got) in enumerate(zip(CASES, statuses), 1):
        current = snapshots[number]
        if got != status:
            raise RuntimeError(f"{label} case {number:02d}: status {got} != {status}")
        if number in RESTORE_CACHE_BEFORE:
            dat = seed_dat
        if status == 0:
            previous = inf
            inf = expected_after(inf, kind, args, LABELS[label])
            dat = b"BnKr" + dat[4:] if dat.startswith(b"AnKr") else dat
            if current != (inf, previous, dat):
                raise RuntimeError(
                    f"{label} case {number:02d}: INF/BAK/DAT differ from expectation")
            bak = previous
        elif current != (inf, bak, dat):
            raise RuntimeError(
                f"{label} case {number:02d}: error case changed INF/BAK/DAT")


def prepare(root: Path, inf: bytes, dat: bytes) -> None:
    gemdos = root / "C"
    desk = gemdos / "GEMSYS/GEMDESK"
    desk.mkdir(parents=True)
    (desk / "APPLICAT.INF").write_bytes(inf)
    (desk / "APPLICAT.DAT").write_bytes(dat)
    shutil.copy2(GCC_APP, gemdos / "MOD_APP.TTP")
    shutil.copy2(DRIVER, gemdos / "NEOSTEST.PRG")
    (gemdos / "RESULTS").mkdir()


def invoke(headless: Path, rom: Path, root: Path, output: Path,
           label: str, frames: int) -> tuple[list[int], dict]:
    gemdos = root / "C"
    console = output / f"{label}.console.log"
    screenshot = output / f"{label}.ppm"
    options = ["--machine", "st", "--mem", "4m", "--no-disk",
               "--gemdos", str(gemdos), "--auto", r"C:\NEOSTEST.PRG",
               "--console-log", str(console)]
    server_dir = output / f"{label}-server"
    server_dir.mkdir()
    with NeoSTServer(headless, rom, server_dir, options=options,
                     timeout=240) as server:
        server.run_frames(frames)
        server.screenshot(screenshot)
    if not console.is_file():
        raise RuntimeError(f"NeoST did not create {console}")

    text = console.read_text(encoding="latin-1")
    cases = [(int(number), int(status) & 0xFFFF)
             for number, status in CASE_LINE.findall(text)]
    expected_cases = list(enumerate(EXPECTED, start=1))
    if cases != expected_cases or f"DONE {len(CASES)}" not in text:
        raise RuntimeError(f"{label}: missing/mismatched guest results {cases}; see {console}")

    snapshots = {}
    for number in range(len(CASES) + 1):
        entry = []
        for suffix in ("INF", "BAK", "DAT"):
            path = gemdos / "RESULTS" / f"{number:02d}.{suffix}"
            entry.append(path.read_bytes() if path.is_file() else None)
        snapshots[number] = tuple(entry)
    return [status for _, status in cases], snapshots


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
    if not SEED_INF.is_file() or not SEED_DAT.is_file():
        parser.error(f"MOD_APP seed fixtures missing: {SEED_INF}, {SEED_DAT}")

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

    seed_dat = SEED_DAT.read_bytes()
    if seed_dat[:4] != b"AnKr":
        seed_dat = b"AnKr" + seed_dat[4:]
    seed_inf = SEED_INF.read_bytes()
    german_inf = seed_inf
    english_inf = (seed_inf.replace(b"[<frei>]", b"[<free>]")
                   .replace(b"[<Spezial>]", b"[<Special>]"))
    seeds = {"english": english_inf, "german": german_inf}

    try:
        with tempfile.TemporaryDirectory(prefix="neost-mod-app-") as temporary:
            headless, app_root = acquire_headless(args.neost_headless, args.neost,
                                                   Path(temporary))
            rom = resolve_rom(args.rom, app_root)
            outcomes = {}
            snapshots = {}
            for label, seed in seeds.items():
                guest = output / f"guest-{label}"
                prepare(guest, seed, seed_dat)
                statuses, steps = invoke(headless, rom, guest, output, label, args.frames)
                validate(label, seed, seed_dat, steps, statuses)
                outcomes[label] = statuses
                snapshots[label] = {
                    f"{number:02d}.{suffix}":
                        "MISSING" if entry is None
                        else hashlib.sha256(entry).hexdigest()
                    for number, step in steps.items()
                    for suffix, entry in zip(("INF", "BAK", "DAT"), step)
                }
                (output / f"{label}.results.txt").write_text(
                    "".join(f"{i:02d} {status}\n" for i, status in enumerate(statuses, 1)),
                    encoding="ascii")

            report = {"emulator": str(headless), "rom": str(rom), "frames": args.frames,
                      "cases": len(CASES), "expected_statuses": EXPECTED,
                      "outcomes": outcomes, "snapshots_sha256": snapshots}
            (output / "report.json").write_text(json.dumps(report, indent=2) + "\n",
                                                encoding="utf-8")
            print(f"NeoST MOD_APP smoke passed: {len(CASES)} cases against English and "
                  f"German section seeds; INF/BAK/DAT match explicit expectations. "
                  f"Results: {output}")
    except (OSError, RuntimeError) as error:
        print(f"NeoST MOD_APP smoke failed: {error}\nArtifacts: {output}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
