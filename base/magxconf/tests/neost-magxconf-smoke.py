#!/usr/bin/env python3
"""Compare MAGXCONF PRG/ACC dialogs, configuration effects and self-save on MagiC."""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import struct
import sys
import tempfile

SOURCE = Path(__file__).resolve().parents[1]
ROOT = SOURCE.parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from neost_common import acquire_headless, create_output, require, run
from neost_host import NeoSTHost
from neost_image import MagiCImage
from neost_screen import diff_stats

BUILD = ROOT / "build/atari"
APPS = BUILD / "apps/magxconf"
ARCHIVES = {"de": SOURCE / "tests/GER/MAGXCONF.ACC",
            "en": SOURCE / "tests/US/MAGXCONF.ACC",
            "fr": SOURCE / "tests/FRA/MAGXCONF.PRG"}
# Captions (including padding) differ between archive and source revisions.
# Compare switch state margins, button borders and numeric version/date fields;
# full captures retain the text differences instead of normalizing them away.
TOGGLE_REGIONS = ((403, 143, 461, 160), (403, 177, 461, 244))
COMPARISON_REGIONS = (
    (403, 143, 418, 160), (445, 143, 461, 160),
    (403, 177, 418, 227), (445, 177, 461, 227), (403, 228, 461, 244),
    (192, 289, 447, 292), (192, 307, 447, 310),
    (266, 257, 317, 276), (354, 257, 436, 276),
)
STEPS = (("CANCEL", (431, 151), (411, 299)),
         ("OK", (431, 151), (228, 299)),
         ("SAVE", (431, 202), (322, 299)))


def sample(path: Path) -> int:
    values = path.read_text(encoding="ascii").split()
    require(len(values) == 2 and values[1] == "0", f"invalid probe result: {path}")
    return int(values[0], 16)


def session(headless: Path, rom: Path, image: MagiCImage,
            output: Path) -> NeoSTHost:
    host = NeoSTHost(headless, rom, output, image=image,
                      options=["--machine", "megaste", "--mem", "4m", "--mono", "--no-disk"],
                      auto=r"C:\AUTO\MAGXBOOT.TOS", timeout=240)
    (host.guest_root / "E").mkdir()
    return host


def open_accessory(host: NeoSTHost) -> None:
    # Pinned fixture: MAGXCONF is the first entry, followed by Control Panels.
    host.move_to(40, 8)
    host.run_frames(30)
    host.click(70, 59)
    host.run_frames(80)


def lifecycle(headless: Path, rom: Path, source: Path, app: Path, mode: str,
              output: Path, offset: int) -> tuple[dict[str, Path], list[int]]:
    output.mkdir()
    image = MagiCImage.clone(source, output / "magic.img", partition_offset=offset)
    image.verify_magic()
    # COPS' MAGXCONF.CPX restores a competing saved mask during boot.
    # Isolate this consumer, keeping COPS and its menu entry available.
    cpx = "::/AUTO/ACCS/CPX/MAGXCONF.CPX"
    if run(["mdir", "-i", image.image_spec, cpx], timeout=30).returncode == 0:
        renamed = run(["mren", "-o", "-i", image.image_spec, cpx,
                       "::/AUTO/ACCS/CPX/MAGXCONF.CPZ"], timeout=30)
        require(renamed.returncode == 0, f"cannot isolate competing CPX: {renamed.stdout}")
    mode_file = output / "CNFMODE.TXT"
    mode_file.write_text(mode)
    guest_app = "::/MAGXCONF.PRG" if mode == "p" else "::/AUTO/ACCS/MAGXCONF.ACC"
    image.install(((app, guest_app),
                   (APPS / "NEOSTART.PRG", "::/GEMSYS/MAGIC/START/ZCNFTEST.PRG"),
                   (mode_file, "::/CNFMODE.TXT")))
    shots = {}
    values = []
    host = session(headless, rom, image, output / "lifecycle")
    with host:
        host.boot(15000)
        host.calibrate_mouse()
        values.append(sample(host.guest_root / "E/BEFORE.RES"))
        for index, (step, toggle, button) in enumerate(STEPS):
            if mode == "a":
                open_accessory(host)
            host.move_to(500, 350)
            host.run_frames(20)
            shots[f"{step}-before"] = host.screenshot(f"{step}-before.ppm")
            host.click(*toggle)
            host.run_frames(30)  # Let AES finish tracking the released button.
            host.move_to(500, 350)
            host.run_frames(30)
            shots[f"{step}-toggle"] = host.screenshot(f"{step}-toggle.ppm")
            changed, _ = diff_stats(shots[f"{step}-before"], shots[f"{step}-toggle"],
                                    region=TOGGLE_REGIONS[0] if index < 2 else TOGGLE_REGIONS[1])
            require(changed > 50, f"{output}: {step} did not visibly toggle a switch")
            host.click(*button)
            host.run_frames(250)
            probe = host.guest_root / "E" / (f"{step}.RES" if mode == "p" else "CURRENT.RES")
            values.append(sample(probe))
            if mode == "a":
                # Keep a snapshot of the guest's live query, not a synthesized result.
                (host.output / f"{step}.RES").write_bytes(probe.read_bytes())
        expected = [values[0], values[0], values[0] ^ 0x10, values[0] ^ 0x90]
        require(values == expected, f"{output}: configuration transitions {values} != {expected}")
    original = app.read_bytes()
    saved_file = output / ("saved.prg" if mode == "p" else "saved.acc")
    image.copy_out(guest_app, saved_file)
    saved = saved_file.read_bytes()
    require(original[28:30] == b"\x60\x04", "self-save layout lacks the short entry branch")
    require(saved[30:34] == struct.pack(">I", values[-1]), "saved configuration value differs")
    require(original[:30] + original[34:] == saved[:30] + saved[34:],
            "Save changed bytes outside the four-byte default value")
    if mode == "a":
        # Reboot the same cloned image: the accessory must restore its saved default.
        mode_file.write_text("r")
        image.copy_in(mode_file, "::/CNFMODE.TXT")
        reboot = session(headless, rom, image, output / "reboot")
        with reboot:
            reboot.boot(15000)
            restored = sample(reboot.guest_root / "E/BEFORE.RES")
            require(restored == values[-1],
                    f"{output}: accessory boot restored {restored:08x}, expected {values[-1]:08x}")
            reboot.calibrate_mouse()
            open_accessory(reboot)
            reboot.move_to(500, 350)
            shots["restored"] = reboot.screenshot("restored.ppm")
            reboot.click(411, 299)
            reboot.run_frames(80)
    return shots, values


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--magic-image", type=Path, default=os.environ.get("MAGIC_IMAGE"))
    parser.add_argument("--tos", type=Path, default=os.environ.get("TOS206_ROM"))
    parser.add_argument("--neost", type=Path, default=Path.home() / ".local/bin/neost")
    parser.add_argument("--neost-headless", type=Path)
    parser.add_argument("--partition-offset", type=int, default=1024)
    parser.add_argument("--languages", nargs="+", choices=("de", "en", "fr"),
                        default=["de", "en", "fr"])
    parser.add_argument("--modes", nargs="+", choices=("p", "a"), default=["p", "a"])
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.magic_image is None or args.tos is None:
        parser.error("supply --magic-image/--tos or MAGIC_IMAGE/TOS206_ROM")
    source, rom = args.magic_image.expanduser().resolve(), args.tos.expanduser().resolve()
    for path in (source, rom):
        if not path.is_file():
            parser.error(f"input not found: {path}")
    build = run([str(ROOT / "scripts/atari-container"), "run", "make", "-f", "Makefile.apps",
                 "magxconf-languages", "magxconf-test-driver"], cwd=ROOT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-magxconf").resolve()
        print(f"Evidence: {output}", flush=True)
        with tempfile.TemporaryDirectory(prefix="neost-magxconf-") as temporary:
            headless, _ = acquire_headless(args.neost_headless, args.neost, Path(temporary))
            for language in args.languages:
                for mode in args.modes:
                    directory = output / f"{language}-{mode}"
                    directory.mkdir()
                    suffix = "PRG" if mode == "p" else "ACC"
                    archive, archive_values = lifecycle(headless, rom, source, ARCHIVES[language], mode,
                                                         directory / "archive", args.partition_offset)
                    rebuilt, rebuilt_values = lifecycle(headless, rom, source,
                                                         APPS / language / f"MAGXCONF.{suffix}", mode,
                                                         directory / "rebuilt", args.partition_offset)
                    require(archive_values == rebuilt_values,
                            f"{language}-{mode}: archived/rebuilt configuration values differ")
                    for state in archive:
                        for region in COMPARISON_REGIONS:
                            count, bbox = diff_stats(archive[state], rebuilt[state], region=region)
                            require(count == 0,
                                    f"{language}-{mode} {state}: controls differ at {bbox}")
                    print(f"{language}-{mode}: Cancel/OK/Save and {len(archive)} control states match" +
                          ("; accessory boot restores saved configuration" if mode == "a" else ""),
                          flush=True)
        (output / "comparison.txt").write_text("PASS\n")
    except (OSError, RuntimeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
