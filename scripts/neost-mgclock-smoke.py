#!/usr/bin/env python3
"""Verify MGCLOCK desktop drawing, timer updates and MagiC coexistence.

The restored MGCLOCK draws directly on the desktop; it does not own a GEM
window.  The EmuTOS check therefore looks for changing pixels in the desktop's
top-right clock area, not for a movable window or a close box.

The MagiC check compares three disposable boots of the same image:

* MGCLOCK only: the clock must update.
* APPLICAT only: the same desktop area must stay static.
* MGCLOCK plus APPLICAT: the clock must still update, and APPLICAT's windows
  must otherwise be pixel-identical to the control run.

This proves timer behavior and that the desktop clock coexists with a normal
multitasking application.  Every run is driven through the shared
`neost_server.NeoSTServer` client, which owns protocol framing and cleanup.
The check does not measure CPU load or long-term stability.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import sys
import tempfile

from neost_common import acquire_headless, create_output, require, resolve_rom, run
from neost_image import MagiCImage
from neost_screen import bbox_inside, diff_stats
from neost_server import NeoSTServer

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
APP_DIR = BUILD / "apps/mgclock"
APP = APP_DIR / "MGCLOCK.PRG"
APPLICAT_DIR = BUILD / "apps/applicat"
APPLICAT = APPLICAT_DIR / "APPLICAT.APP"
APPLICAT_FILES = (
    (APPLICAT, "APPLICAT.APP"),
    (APPLICAT_DIR / "applicat.rsc", "applicat.rsc"),
    (APPLICAT_DIR / "applicat.inf", "applicat.inf"),
)

EMUTOS_FRAMES = 1800
EMUTOS_INTERVAL = 300
EMUTOS_STABLE_FRAME = 600
EMUTOS_CLOCK_REGION = (300, 0, 416, 60)

MAGIC_FRAMES = 14000
MAGIC_INTERVAL = 1000
MAGIC_STABLE_FRAME = 8000
MAGIC_CLOCK_REGION = (480, 0, 640, 24)


def capture_frames(headless: Path, rom: Path, output: Path, options: list[str],
                   start: int, stop: int, interval: int) -> list[Path]:
    """Run an empty server artifact directory and capture clock samples."""
    output.mkdir(parents=True)
    frames = []
    current = 0
    with NeoSTServer(headless, rom, output, options=options, timeout=180.0) as server:
        for frame in range(start, stop + 1, interval):
            server.run_frames(frame - current)
            current = frame
            screenshot = output / f"frame{frame:05d}.ppm"
            server.screenshot(screenshot)
            frames.append(screenshot)
    missing = [str(frame) for frame in frames if not frame.is_file()]
    if missing:
        raise RuntimeError(f"NeoST did not produce expected screenshots: {missing}")
    return frames


def check_clock_updates(frames: list[Path], region: tuple[int, int, int, int],
                        label: str) -> None:
    changes = []
    for first, second in zip(frames, frames[1:]):
        count, bbox = diff_stats(first, second)
        if not count:
            continue
        require(bbox is not None and bbox_inside(bbox, region),
                f"{label}: changes outside the clock area: {bbox}")
        require(10 <= count <= 5000,
                f"{label}: unexpected change count {count} in {first.name}")
        changes.append(count)
    require(len(changes) >= 4,
            f"{label}: the clock area changed in only {len(changes)} frame pairs")
    print(f"{label}: clock updated in {len(changes)} frame pairs "
          f"(counts {changes[0]}..{changes[-1]})")


def check_static(frames: list[Path], label: str) -> None:
    for first, second in zip(frames, frames[1:]):
        count, bbox = diff_stats(first, second)
        require(not count, f"{label}: unexpected change {bbox} between "
                           f"{first.name} and {second.name}")
    print(f"{label}: clock area remained static across "
          f"{len(frames) - 1} frame pairs")


def check_coexistence(clock_only: Path, control: Path, both: Path,
                      region: tuple[int, int, int, int]) -> None:
    # APPLICAT's windows must explain the difference between the clock-only
    # and APPLICAT-only runs, not the clock.
    window_count, window_bbox = diff_stats(clock_only, control)
    clock_overlap = diff_stats(clock_only, control, region=region)[0]
    require(window_count - clock_overlap > 10_000,
            "APPLICAT changed too little outside the clock area")
    print(f"APPLICAT window evidence: {window_count - clock_overlap} pixels "
          f"outside the clock area (bbox {window_bbox})")

    # With APPLICAT running, MGCLOCK must add only its own clock pixels.
    count, bbox = diff_stats(both, control)
    require(count > 0, "MGCLOCK drew no clock text while APPLICAT was running")
    require(bbox is not None and bbox_inside(bbox, region),
            f"MGCLOCK altered APPLICAT's windows: {bbox}")
    clock_only_count = diff_stats(both, control, region=region)[0]
    require(count - clock_only_count == 0,
            "MGCLOCK changed pixels outside the clock area while APPLICAT ran")
    print(f"coexistence: MGCLOCK added {count} pixels, all within {bbox}")


def run_emutos(headless: Path, rom: Path, output: Path) -> list[Path]:
    guest_root = output / "GEMDRIVE"
    guest = guest_root / "C"
    guest.mkdir(parents=True)
    shutil.copy2(APP, guest / "MGCLOCK.PRG")
    options = [
        "--machine", "st", "--mem", "4m", "--no-disk",
        "--gemdos", str(guest_root), "--auto", r"C:\MGCLOCK.PRG",
    ]
    return capture_frames(headless, rom, output / "server", options,
                          EMUTOS_STABLE_FRAME, EMUTOS_FRAMES, EMUTOS_INTERVAL)


def run_magic(headless: Path, rom: Path, image: Path, output: Path) -> list[Path]:
    guest_root = output / "GEMDRIVE"
    (guest_root / "E").mkdir(parents=True)
    options = [
        "--machine", "megaste", "--mem", "4m", "--mono", "--no-disk",
        "--acsi", str(image), "--gemdos", str(guest_root),
        "--auto", r"C:\AUTO\MAGXBOOT.TOS",
    ]
    return capture_frames(headless, rom, output / "server", options,
                          MAGIC_STABLE_FRAME, MAGIC_FRAMES, MAGIC_INTERVAL)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--neost", type=Path,
                        default=Path.home() / ".local/bin/neost",
                        help="NeoST AppImage (default: ~/.local/bin/neost)")
    parser.add_argument("--neost-headless", type=Path,
                        help="direct path to the bundled neost-headless executable")
    parser.add_argument("--rom", type=Path,
                        help="EmuTOS ROM; default is bundled EmuTOS 1.04")
    environment_image = os.environ.get("MAGIC_IMAGE")
    environment_tos = os.environ.get("TOS206_ROM")
    parser.add_argument("--magic-image", type=Path,
                        default=Path(environment_image) if environment_image else None,
                        help="known-bootable MagiC disk image (or MAGIC_IMAGE)")
    parser.add_argument("--tos", type=Path,
                        default=Path(environment_tos) if environment_tos else None,
                        help="TOS 2.06 ROM for the MagiC run (or TOS206_ROM)")
    parser.add_argument("--partition-offset", type=int, default=1024,
                        help="byte offset of MagiC's C: partition (default: 1024)")
    parser.add_argument("--output", type=Path,
                        help="new output directory (default: timestamped build/atari path)")
    parser.add_argument("--skip-emutos", action="store_true",
                        help="skip the EmuTOS desktop-clock check")
    parser.add_argument("--skip-magic", action="store_true",
                        help="skip the MagiC coexistence check")
    args = parser.parse_args()

    if not args.skip_magic and (args.magic_image is None or args.tos is None):
        parser.error("pass --magic-image and --tos (or set MAGIC_IMAGE and "
                     "TOS206_ROM) for the MagiC coexistence check")

    build = run([str(ROOT / "scripts/atari-container"), "run", "make", "-f",
                 "Makefile.apps", "mgclock", "applicat"], cwd=ROOT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    for output in (APP, APPLICAT, APPLICAT_DIR / "applicat.rsc",
                   APPLICAT_DIR / "applicat.inf"):
        if not output.is_file():
            print(f"build completed without producing {output}", file=sys.stderr)
            return 1

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-mgclock")
    except OSError as error:
        parser.error(str(error))

    try:
        with tempfile.TemporaryDirectory(prefix="neost-mgclock-") as temporary:
            headless, app_root = acquire_headless(args.neost_headless, args.neost,
                                                  Path(temporary))
            if not args.skip_emutos:
                emutos_rom = resolve_rom(args.rom, app_root)
                emutos = output / "emutos"
                emutos.mkdir()
                frames = run_emutos(headless, emutos_rom, emutos)
                check_clock_updates(frames, EMUTOS_CLOCK_REGION, "EmuTOS MGCLOCK")

            if not args.skip_magic:
                assert args.magic_image is not None and args.tos is not None
                magic_rom = args.tos.expanduser().resolve()
                source = args.magic_image.expanduser().resolve()
                require(source.is_file(), f"MagiC image not found: {source}")
                require(magic_rom.is_file(), f"TOS ROM not found: {magic_rom}")

                scenarios = {
                    "clock-only": [
                        (APP, r"::/GEMSYS/MAGIC/START/MGCLOCK.PRG"),
                    ],
                    "control": [
                        (local, rf"::/GEMSYS/MAGIC/START/{guest}")
                        for local, guest in APPLICAT_FILES
                    ],
                    "coexist": [
                        (APP, r"::/GEMSYS/MAGIC/START/MGCLOCK.PRG"),
                        *[(local, rf"::/GEMSYS/MAGIC/START/{guest}")
                          for local, guest in APPLICAT_FILES],
                    ],
                }
                results: dict[str, list[Path]] = {}
                for name, files in scenarios.items():
                    directory = output / name
                    directory.mkdir()
                    image = MagiCImage.clone(source, directory / "magic.img",
                                             partition_offset=args.partition_offset)
                    image.verify_magic()
                    image.install(files)
                    results[name] = run_magic(headless, magic_rom, image.path, directory)
                    print(f"MagiC {name}: {len(results[name])} screenshots")

                check_clock_updates(results["clock-only"], MAGIC_CLOCK_REGION,
                                    "MagiC MGCLOCK")
                check_static(results["control"], "MagiC APPLICAT control")
                check_clock_updates(results["coexist"], MAGIC_CLOCK_REGION,
                                    "MagiC MGCLOCK + APPLICAT")
                check_coexistence(results["clock-only"][-1],
                                  results["control"][-1],
                                  results["coexist"][-1], MAGIC_CLOCK_REGION)

        print(f"MGCLOCK desktop timer and coexistence checks passed; "
              f"artifacts: {output}")
    except (OSError, RuntimeError) as error:
        print(f"NeoST MGCLOCK smoke failed: {error}\nArtifacts: {output}",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
