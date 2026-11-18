#!/usr/bin/env python3
"""Verify the MGClock window move and close-box lifecycle under NeoST/EmuTOS."""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from neost_common import acquire_headless, create_output, resolve_rom, run, run_logged

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
APP_BUILD = BUILD / "apps/mgclock"
APP = APP_BUILD / "MGCLOCK.PRG"
CLOSE_FRAME = 600
MOVE_FRAME = 640
FRAMES = 800
MOUSE_SCRIPT = "LU" + "r" * 8 + "." * 30 + "L" * 10 + "1"


def ppm_pixels(path: Path) -> tuple[int, int, bytes]:
    data = path.read_bytes()
    position = 0

    def token() -> bytes:
        nonlocal position
        while position < len(data):
            if data[position] in b" \t\r\n":
                position += 1
            elif data[position] == ord("#"):
                while position < len(data) and data[position] not in b"\r\n":
                    position += 1
            else:
                break
        start = position
        while position < len(data) and data[position] not in b" \t\r\n#":
            position += 1
        return data[start:position]

    magic, width, height, maximum = token(), int(token()), int(token()), int(token())
    if magic != b"P6" or maximum != 255:
        raise RuntimeError(f"unsupported NeoST screenshot format: {path}")
    if position >= len(data) or data[position] not in b" \t\r\n":
        raise RuntimeError(f"malformed NeoST screenshot header: {path}")
    if data[position:position + 2] == b"\r\n":
        position += 2
    else:
        position += 1
    pixels = data[position:]
    if len(pixels) != width * height * 3:
        raise RuntimeError(f"truncated NeoST screenshot: {path}")
    return width, height, pixels


def window_pixels(path: Path) -> tuple[int, int]:
    width, height, pixels = ppm_pixels(path)
    left, top = min(180, width), min(118, height)
    right, bottom = min(350, width), min(155, height)
    dark = green = 0
    for y in range(top, bottom):
        for x in range(left, right):
            offset = (y * width + x) * 3
            red, green_channel, blue = pixels[offset:offset + 3]
            dark += max(red, green_channel, blue) < 40
            green += (green_channel > red * 1.25 and
                      green_channel > blue * 1.1)
    return dark, green


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--neost", type=Path, default=Path.home() / ".local/bin/neost",
                        help="NeoST AppImage (default: ~/.local/bin/neost)")
    parser.add_argument("--neost-headless", type=Path,
                        help="direct path to the bundled neost-headless executable")
    parser.add_argument("--rom", type=Path,
                        help="TOS ROM; default is bundled EmuTOS 1.04")
    parser.add_argument("--output", type=Path,
                        help="new output directory (default: timestamped build/atari path)")
    args = parser.parse_args()

    build = run([str(ROOT / "scripts/atari-container"), "run", "make", "-f",
                 "Makefile.apps", "mgclock"], cwd=ROOT, timeout=600)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    if not APP.is_file():
        print(f"build completed without producing {APP}", file=sys.stderr)
        return 1

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-mgclock")
    except OSError as error:
        parser.error(str(error))
    guest = output / "guest/C"
    guest.mkdir(parents=True)
    shutil.copy2(APP, guest / "MGCLOCK.PRG")
    try:
        with tempfile.TemporaryDirectory(prefix="neost-mgclock-") as temporary:
            headless, app_root = acquire_headless(args.neost_headless, args.neost,
                                                   Path(temporary))
            rom = resolve_rom(args.rom, app_root)

            active = output / f"clock.ppm{MOVE_FRAME:05d}.ppm"
            closed = output / "closed.ppm"
            hostlog = output / "headless.log"
            command = [str(headless), "--machine", "st", "--mem", "4m", "--no-disk",
                       "--frames", str(FRAMES), "--gemdos", str(guest),
                       "--auto", r"C:\MGCLOCK.PRG", "--mouse-at", str(CLOSE_FRAME),
                       MOUSE_SCRIPT, "--mouse-step", "8", "--shot-every", str(MOVE_FRAME),
                       str(output / "clock.ppm"), "--screenshot", str(closed), str(rom)]
            run_logged(command, hostlog, cwd=ROOT, timeout=240)
            if not active.is_file() or not closed.is_file():
                raise RuntimeError(f"NeoST screenshots missing; see {hostlog}")

            active_dark, active_green = window_pixels(active)
            closed_dark, closed_green = window_pixels(closed)
            if active_dark < 1000 or active_green > 1000:
                raise RuntimeError(f"MGClock window did not remain visible after moving: {active}")
            if closed_dark > 1000 or closed_green < 2000:
                raise RuntimeError(f"MGClock close box did not return to desktop in {closed}")
            print(f"MGClock move and close-box smoke passed; screenshots: {output}")
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"NeoST MGClock smoke failed: {error}\nArtifacts: {output}",
            file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
