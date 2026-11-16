#!/usr/bin/env python3
"""Compare archived and GCC LIMITMEM on disposable PRG copies under NeoST."""

from __future__ import annotations

import argparse
import datetime
import hashlib
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
ORIGINAL = ROOT / "MagiC/APPS/LIMITMEM/BIN/LIMITMEM.TTP"
GCC_APP = BUILD / "apps/limitmem/LIMITMEM.TTP"
DRIVER = BUILD / "apps/limitmem/NEOSTEST.PRG"
CASE_LINE = re.compile(r"^CASE\s+(\d+)\s+(-?\d+)\s*$", re.MULTILINE)
CASES = ("SET.PRG", "SAME.PRG", "REMOVE.PRG", "BADHDR.PRG", "BADREL.PRG")


def run(command: list[str], *, cwd: Path | None = None, timeout: int = 240) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, cwd=cwd, text=True, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, timeout=timeout, check=False)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def make_prg(*, branch: int = 0x601A, relocation: int = 1, limit: int | None = None) -> bytes:
    header = struct.pack(">H6IH", branch, 2, 0, 0, 0, 0, 0, 0)
    data = header + b"\x4e\x75" + struct.pack(">I", relocation) + b"\0"
    if limit is not None:
        data += b"MAGX" + struct.pack(">I", limit)
    return data


def fixtures(gemdos: Path) -> dict[str, bytes]:
    initial = {
        "SET.PRG": make_prg(),
        "SAME.PRG": make_prg(limit=64 * 1024),
        "REMOVE.PRG": make_prg(limit=32 * 1024),
        "BADHDR.PRG": make_prg(branch=0x1234),
        "BADREL.PRG": make_prg(relocation=0),
    }
    for name, data in initial.items():
        (gemdos / name).write_bytes(data)
    return initial


def acquire_headless(appimage: Path, scratch: Path) -> tuple[Path, Path, Path]:
    extraction = scratch / "appimage"
    extraction.mkdir()
    result = run([str(appimage.resolve()), "--appimage-extract"], cwd=extraction)
    if result.returncode:
        raise RuntimeError(f"NeoST AppImage extraction failed:\n{result.stdout}")
    root = extraction / "squashfs-root"
    headless = root / "usr/bin/neost-headless"
    rom = root / "usr/roms/etos192us.img"
    if not headless.is_file() or not rom.is_file():
        raise RuntimeError("NeoST AppImage is missing neost-headless or bundled EmuTOS ROM")
    return headless, rom, root


def invoke(headless: Path, rom: Path, gemdos: Path, out: Path, label: str) -> tuple[list[int], dict[str, str], str]:
    shutil.copy2(DRIVER, gemdos / "NEOSTEST.PRG")
    console = out / f"{label}.console.log"
    host = out / f"{label}.host.log"
    screenshot = out / f"{label}.ppm"
    result = run([str(headless), "--machine", "st", "--mem", "4m", "--no-disk",
                  "--frames", "1800", "--gemdos", str(gemdos), "--auto",
                  r"C:\NEOSTEST.PRG", "--console-log", str(console),
                  "--screenshot", str(screenshot), str(rom)])
    host.write_text(result.stdout, encoding="utf-8")
    if result.returncode or not console.is_file():
        raise RuntimeError(f"NeoST {label} failed; see {host}")
    text = console.read_text(encoding="latin-1")
    cases = [(int(number), int(status) & 0xFFFF) for number, status in CASE_LINE.findall(text)]
    if [case for case, _ in cases] != list(range(1, 9)) or "DONE 8" not in text:
        raise RuntimeError(f"{label}: missing guest cases {cases}; see {console}")
    hashes = {name: sha256(gemdos / name) for name in CASES if (gemdos / name).is_file()}
    return [status for _, status in cases], hashes, text


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--neost", type=Path, default=Path.home() / ".local/bin/neost")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    output = args.output.resolve() if args.output else BUILD / f"neost-limitmem-{datetime.datetime.now():%Y%m%d-%H%M%S}"
    if output.exists():
        parser.error(f"output already exists: {output}")
    if not ORIGINAL.is_file() or not GCC_APP.is_file() or not DRIVER.is_file():
        parser.error("archived LIMITMEM, GCC LIMITMEM, and test driver must exist")
    output.mkdir(parents=True)
    try:
        with tempfile.TemporaryDirectory(prefix="neost-limitmem-") as temporary:
            headless, rom, app_root = acquire_headless(args.neost, Path(temporary))
            results = {}
            for label, executable in (("archived", ORIGINAL), ("gcc", GCC_APP)):
                guest = output / f"guest-{label}" / "C"
                guest.mkdir(parents=True)
                before = fixtures(guest)
                shutil.copy2(executable, guest / "LIMITMEM.TTP")
                statuses, hashes, text = invoke(headless, rom, guest, output, label)
                results[label] = (statuses, hashes, before, text)
            a_status, a_hashes, a_before, a_text = results["archived"]
            g_status, g_hashes, g_before, g_text = results["gcc"]
            if a_status != g_status or a_hashes != g_hashes:
                raise RuntimeError("archived/GCC status or edited fixture bytes differ")
            if a_text != g_text:
                raise RuntimeError("archived/GCC console text differs")
            # EmuTOS reports -ERANGE from the legacy Fshrink extension after
            # the header bytes are changed; both binaries must match this.
            expected_statuses = [65472, 0, 65472, 65470, 65470, 65503, 1, 1]
            if a_status != expected_statuses:
                raise RuntimeError(f"unexpected LIMITMEM child statuses: {a_status}")
            expected_set = a_before["SET.PRG"] + b"MAGX" + struct.pack(">I", 64 * 1024)
            if (output / "guest-gcc/C/SET.PRG").read_bytes() != expected_set:
                raise RuntimeError("LIMITMEM did not append the expected MAGX/64KiB marker")
            # Under EmuTOS the legacy Fshrink trap surrogate reports -ERANGE
            # and leaves the existing marker intact, matching the archive.
            if (output / "guest-archived/C/REMOVE.PRG").read_bytes() != a_before["REMOVE.PRG"]:
                raise RuntimeError("archived LIMITMEM unexpectedly changed REMOVE.PRG under EmuTOS")
            if (output / "guest-gcc/C/REMOVE.PRG").read_bytes() != a_before["REMOVE.PRG"]:
                raise RuntimeError("GCC LIMITMEM differs from archived REMOVE.PRG behavior under EmuTOS")
            if a_hashes["SAME.PRG"] != hashlib.sha256(a_before["SAME.PRG"]).hexdigest():
                raise RuntimeError("already-installed marker was not left unchanged")
            (output / "comparison.txt").write_text(
                "archived statuses: " + repr(a_status) + "\n"
                "GCC statuses:      " + repr(g_status) + "\n"
                "fixture hashes:    " + repr(g_hashes) + "\n",
                encoding="utf-8")
            print(f"LIMITMEM comparison passed: statuses {a_status}; disposable PRG copies only")
            print(f"Evidence: {output}")
    finally:
        pass
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
