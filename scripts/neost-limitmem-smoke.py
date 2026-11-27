#!/usr/bin/env python3
"""Check rebuilt LIMITMEM on disposable PRG copies under NeoST/EmuTOS."""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import struct
import tempfile

from neost_common import acquire_headless, create_output, resolve_rom
from neost_server import NeoSTServer

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
REBUILT = BUILD / "apps/limitmem/LIMITMEM.TTP"
DRIVER = BUILD / "apps/limitmem/NEOSTEST.PRG"
BANNER = b"MagiC- Memory Limitation, \xbd 1990-95 Andreas Kromke\r\n\n"
HEAP = b"\r\nActual heap size: "
SYNTAX = b"Syntax: LIMITMEM fname [kbytes_dec|-]\r\n"
# Ordered as in LIMITMEMTEST.C. Pexec prints the child's unsigned 16-bit status.
EXPECTED_CASES = (
    (65472, HEAP + b"unlimited\r\n"),  # Marker written before Fshrink returns -ERANGE.
    (0, HEAP + b"64 kBytes\r\n Memory limitation already installed\r\n"),
    (65472, HEAP + b"32 kBytes\r\n"),  # Failed shrink leaves the old marker intact.
    (65470, b""),  # Invalid header.
    (65470, b""),  # Invalid relocation.
    (65503, b""),  # Missing file.
    (1, SYNTAX),   # Zero limit.
    (1, SYNTAX),   # Nonnumeric limit.
)


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


def invoke(headless: Path, rom: Path, gemdos: Path, out: Path) -> None:
    shutil.copy2(DRIVER, gemdos / "NEOSTEST.PRG")
    console = out / "rebuilt.console.log"
    screenshot = out / "rebuilt.ppm"
    options = ["--machine", "st", "--mem", "4m", "--no-disk",
               "--gemdos", str(gemdos), "--auto", r"C:\NEOSTEST.PRG",
               "--console-log", str(console)]
    server_dir = out / "rebuilt-server"
    server_dir.mkdir()
    with NeoSTServer(headless, rom, server_dir, options=options,
                     timeout=240) as server:
        server.run_frames(1800)
        server.screenshot(screenshot)
    actual = console.read_bytes()
    expected = b"BEGIN\r\n" + b"".join(
        BANNER + text + f"CASE {number:02d} {status}\r\n".encode("ascii")
        for number, (status, text) in enumerate(EXPECTED_CASES, 1)
    ) + b"DONE 8\r\n"
    if actual != expected:
        (out / "expected.console.log").write_bytes(expected)
        raise RuntimeError(f"unexpected LIMITMEM console or child status; see {console}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--neost", type=Path, default=Path.home() / ".local/bin/neost")
    parser.add_argument("--neost-headless", type=Path)
    parser.add_argument("--rom", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if not REBUILT.is_file() or not DRIVER.is_file():
        parser.error("build limitmem and limitmem-test-driver first")
    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-limitmem")
    except OSError as error:
        parser.error(str(error))
    try:
        with tempfile.TemporaryDirectory(prefix="neost-limitmem-") as temporary:
            headless, app_root = acquire_headless(args.neost_headless, args.neost, Path(temporary))
            rom = resolve_rom(args.rom, app_root)
            guest = output / "guest-rebuilt" / "C"
            guest.mkdir(parents=True)
            expected = fixtures(guest)
            shutil.copy2(REBUILT, guest / "LIMITMEM.TTP")
            invoke(headless, rom, guest, output)

            expected["SET.PRG"] += b"MAGX" + struct.pack(">I", 64 * 1024)
            expected_names = set(expected) | {"LIMITMEM.TTP", "NEOSTEST.PRG"}
            actual_names = {p.relative_to(guest).as_posix() for p in guest.rglob("*")}
            if actual_names != expected_names:
                raise RuntimeError(f"unexpected guest paths: {sorted(actual_names ^ expected_names)}")
            for name, data in expected.items():
                if (guest / name).read_bytes() != data:
                    raise RuntimeError(f"{name}: unexpected file contents")
            print(f"LIMITMEM: {len(EXPECTED_CASES)} cases passed; console, statuses and file bytes match")
            print(f"Evidence: {output}")
    except (OSError, RuntimeError) as error:
        print(f"NeoST LIMITMEM smoke failed: {error}\nArtifacts: {output}",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
