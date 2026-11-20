#!/usr/bin/env python3
"""Compare archived and rebuilt MGSEARCH on identical MagiC fixtures.

Both binaries run in separate clones of the same image with the same German
resource, delayed launcher and C: fixture tree.  The scenario opens the search
dialog, types "*.ZZZ", submits it, and compares the dialog and result screens.
The fixture includes a nested directory so the result also covers recursive
search; only the defined screens are compared.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys
import tempfile

from neost_common import acquire_headless, create_output, resolve_rom, run
from neost_server import NeoSTServer

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
ARCHIVE_APP = ROOT / "MagiC/APPS/MGSEARCH/BIN/MGSEARCH.APP"
RESOURCE = ROOT / "MagiC/APPS/MGSEARCH/SRC/MGSEARCH.RSC"
REBUILT_DIR = BUILD / "apps/mgsearch"
REBUILT_APP = REBUILT_DIR / "MGSEARCH.APP"
DRIVER = REBUILT_DIR / "NEOSTART.PRG"
START_DIR = r"::/GEMSYS/MAGIC/START"
GEMDESK_DIR = r"::/GEMSYS/GEMDESK"
FIXTURE_DIR = r"::/SEARCHFIX"
BOOT_FRAMES = 15000
SEARCH_FRAMES = 3000


def read_ppm(path: Path) -> tuple[int, int, bytes]:
    data = path.read_bytes()
    position = 0

    def token() -> bytes:
        nonlocal position
        while position < len(data) and data[position] in b" \t\r\n":
            position += 1
        start = position
        while position < len(data) and data[position] not in b" \t\r\n#":
            position += 1
        return data[start:position]

    magic, width, height, maximum = token(), int(token()), int(token()), int(token())
    if magic != b"P6" or maximum != 255:
        raise RuntimeError(f"unsupported NeoST screenshot format: {path}")
    while position < len(data) and data[position] in b" \t\r\n":
        position += 1
    pixels = data[position:]
    if len(pixels) != width * height * 3:
        raise RuntimeError(f"truncated NeoST screenshot: {path}")
    return width, height, pixels


def diff_stats(first: Path, second: Path) -> int:
    first_width, first_height, first_pixels = read_ppm(first)
    second_width, second_height, second_pixels = read_ppm(second)
    if (first_width, first_height) != (second_width, second_height):
        raise RuntimeError(f"screenshot sizes differ: {first} and {second}")
    return sum(first_pixels[index:index + 3] != second_pixels[index:index + 3]
               for index in range(0, len(first_pixels), 3))


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def clone_image(source: Path, destination: Path) -> None:
    result = run(["cp", "--reflink=auto", str(source), str(destination)], timeout=120)
    if result.returncode:
        raise RuntimeError(f"could not clone {source}: {result.stdout}")


def verify_magic_image(image: Path, partition_offset: int) -> None:
    result = run(["mdir", "-i", f"{image}@@{partition_offset}",
                  "::/MAGX.INF"], timeout=30)
    if result.returncode:
        raise RuntimeError(f"{image} does not contain a MagiC MAGX.INF: "
                           f"{result.stdout}")


def mcopy(image: Path, partition_offset: int, source: Path, guest: str) -> None:
    result = run(["mcopy", "-o", "-i", f"{image}@@{partition_offset}",
                  str(source), guest], timeout=60)
    if result.returncode:
        raise RuntimeError(f"could not install {source.name}: {result.stdout}")


def mmd(image: Path, partition_offset: int, guest: str) -> None:
    result = run(["mmd", "-i", f"{image}@@{partition_offset}", guest], timeout=30)
    if result.returncode and "exists" not in result.stdout.lower():
        raise RuntimeError(f"could not create {guest}: {result.stdout}")


def prepare(image: Path, source: Path, partition_offset: int, app: Path,
            scratch: Path) -> None:
    clone_image(source, image)
    verify_magic_image(image, partition_offset)
    for local, name in ((app, "MGSEARCH.APP"), (RESOURCE, "MGSEARCH.RSC"),
                        (DRIVER, "NEOSTART.PRG")):
        require(local.is_file(), f"missing build input: {local}")
        mcopy(image, partition_offset, local, f"{GEMDESK_DIR}/{name}")
    mcopy(image, partition_offset, DRIVER, f"{START_DIR}/NEOSTART.PRG")

    mmd(image, partition_offset, FIXTURE_DIR)
    mmd(image, partition_offset, f"{FIXTURE_DIR}/SUB")
    alpha = scratch / "ALPHA.ZZZ"
    beta = scratch / "BETA.ZZZ"
    alpha.write_text("alpha\n", encoding="ascii")
    beta.write_text("beta\n", encoding="ascii")
    mcopy(image, partition_offset, alpha, f"{FIXTURE_DIR}/ALPHA.ZZZ")
    mcopy(image, partition_offset, beta, f"{FIXTURE_DIR}/SUB/BETA.ZZZ")


def start_server(headless: Path, rom: Path, image: Path, output: Path,
                 guest_root: Path) -> NeoSTServer:
    output.mkdir(parents=True)
    options = [
        "--machine", "megaste", "--mem", "4m", "--mono", "--no-disk",
        "--acsi", str(image), "--gemdos", str(guest_root),
        "--auto", r"C:\AUTO\MAGXBOOT.TOS",
    ]
    return NeoSTServer(headless, rom, output, options=options, timeout=180.0)


def run_search(headless: Path, rom: Path, image: Path, output: Path) -> dict[str, Path]:
    guest_root = output / "GEMDRIVE"
    guest_root.mkdir(parents=True)
    shots = {name: output / f"{name}.ppm"
             for name in ("initial", "typed", "result")}
    with start_server(headless, rom, image, output / "server", guest_root) as server:
        server.run_frames(BOOT_FRAMES)
        server.screenshot(shots["initial"])
        server.chord([0x2A, 0x09], frames=2)  # *
        server.press_key(0x34, frames=2)      # .
        for code in (0x2C, 0x2C, 0x2C):       # ZZZ
            server.press_key(code, frames=2)
        server.run_frames(100)
        server.screenshot(shots["typed"])
        server.press_key(0x1C, frames=4)      # Enter submits the dialog
        server.run_frames(SEARCH_FRAMES)
        server.screenshot(shots["result"])
    return shots


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--neost", type=Path,
                        default=Path.home() / ".local/bin/neost",
                        help="NeoST AppImage (default: ~/.local/bin/neost)")
    parser.add_argument("--neost-headless", type=Path,
                        help="direct path to the bundled neost-headless executable")
    environment_image = os.environ.get("MAGIC_IMAGE")
    environment_tos = os.environ.get("TOS206_ROM")
    parser.add_argument("--magic-image", type=Path,
                        default=Path(environment_image) if environment_image else None,
                        help="known-bootable MagiC image (or MAGIC_IMAGE)")
    parser.add_argument("--tos", type=Path,
                        default=Path(environment_tos) if environment_tos else None,
                        help="TOS 2.06 ROM (or TOS206_ROM)")
    parser.add_argument("--partition-offset", type=int, default=1024)
    parser.add_argument("--output", type=Path,
                        help="new output directory (default: timestamped build/atari path)")
    args = parser.parse_args()
    if args.magic_image is None or args.tos is None:
        parser.error("pass --magic-image and --tos (or set MAGIC_IMAGE and TOS206_ROM)")
    source = args.magic_image.expanduser().resolve()
    rom = args.tos.expanduser().resolve()
    for path in (source, rom):
        if not path.is_file():
            parser.error(f"input not found: {path}")

    build = run([str(ROOT / "scripts/atari-container"), "run", "make", "-f",
                 "Makefile.apps", "mgsearch", "mgsearch-test-driver"],
                cwd=ROOT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    for path in (ARCHIVE_APP, RESOURCE, REBUILT_APP, DRIVER):
        if not path.is_file():
            print(f"build completed without producing {path}", file=sys.stderr)
            return 1

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-mgsearch")
    except OSError as error:
        parser.error(str(error))

    screenshots: dict[str, dict[str, Path]] = {}
    try:
        with tempfile.TemporaryDirectory(prefix="neost-mgsearch-") as temporary:
            scratch = Path(temporary)
            headless, _ = acquire_headless(args.neost_headless, args.neost, scratch)
            for name, app in (("archive", ARCHIVE_APP), ("rebuilt", REBUILT_APP)):
                directory = output / name
                directory.mkdir()
                image = directory / "magic.img"
                prepare(image, source, args.partition_offset, app, scratch)
                screenshots[name] = run_search(headless, rom, image,
                                               directory / "search")
        for state in ("initial", "typed", "result"):
            count = diff_stats(screenshots["archive"][state],
                               screenshots["rebuilt"][state])
            require(count == 0, f"{state}: archived/rebuilt screens differ "
                                f"in {count} pixels")
            print(f"{state}: screens identical")
        dialog = diff_stats(screenshots["rebuilt"]["initial"],
                            screenshots["rebuilt"]["typed"])
        result = diff_stats(screenshots["rebuilt"]["typed"],
                            screenshots["rebuilt"]["result"])
        require(dialog > 0, "typing did not change the search dialog")
        require(result > 0, "submitting did not open the result window")
        print(f"state changes: typed={dialog} pixels, result={result} pixels")
        (output / "comparison.txt").write_text(
            "PASS\n"
            f"initial={screenshots['rebuilt']['initial']}\n"
            f"typed={screenshots['rebuilt']['typed']}\n"
            f"result={screenshots['rebuilt']['result']}\n",
            encoding="utf-8")
        print(f"Evidence: {output}")
    except RuntimeError as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
