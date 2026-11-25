#!/usr/bin/env python3
"""Compare archived and rebuilt COPS panel and CPX dialogs on MagiC."""
from __future__ import annotations

import argparse
import os
import sys
import tempfile
from pathlib import Path

COMPONENT = Path(__file__).resolve().parents[1]
ROOT = COMPONENT.parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from neost_common import acquire_headless, create_output, require, run
from neost_host import NeoSTHost
from neost_image import MagiCImage
from neost_screen import bbox_inside, diff_stats

BUILD = ROOT / "build/atari"
ARCHIVE = COMPONENT / "SRC/COPS.APP"
REBUILT = BUILD / "apps/cops/COPS.APP"
CPX_SOURCE = ROOT / "MagiC/APPS/CPX/BIN"
CPX_MODULES = ("MAGXCONF", "TSLICE")
GUEST_APP = "::/GEMSYS/MAGIC/START/COPS.APP"
GUEST_CPX = "::/CPX"
ACC_ARCHIVE = "::/AUTO/ACCS/COPS.ACC"
ACC_DISABLED = "::/AUTO/ACCS/COPS.ACX"

PANEL_AT = (428, 117)
MAGXCONF_AT = (112, 187)
TSLICE_AT = (207, 187)
CANCEL_AT = (406, 281)
NEUTRAL_AT = (408, 384)
# Double-clicking an icon leaves its selection highlight behind. Icons sit left
# of the CPX windows, so Cancel only has to restore everything to their right.
ICON_STRIP = (0, 0, 170, 400)


def verify_launch_paths(image: MagiCImage) -> None:
    """Reject images whose MagiC startup paths differ from the fixture."""
    result = run(["mtype", "-i", image.image_spec, "::/MAGX.INF"])
    require(result.returncode == 0, f"cannot read MAGX.INF: {result.stdout}")
    lines = [line.strip().upper() for line in result.stdout.splitlines()]
    app = [line for line in lines if line.startswith("#_APP")]
    acc = [line for line in lines if line.startswith("#_ACC")]
    require(any("GEMSYS\\MAGIC\\START" in line for line in app),
            "MAGX.INF #_APP is not C:\\GEMSYS\\MAGIC\\START")
    require(any("AUTO\\ACCS" in line for line in acc),
            "MAGX.INF #_ACC is not C:\\AUTO\\ACCS")


def reset_cpx_fixture(image: MagiCImage) -> None:
    """Remove inherited modules and COPS settings from this clone."""
    for pattern in ("::/CPX/*.CPX", "::/CPX/*.CPZ"):
        run(["mdel", "-i", image.image_spec, pattern])
    for name in ("::/COPS.INF", "::/DEFAULTS/COPS.INF"):
        run(["mdel", "-i", image.image_spec, name])


def settled(host: NeoSTHost, path: Path) -> Path:
    """Capture with the pointer parked away from the panel and dialogs."""
    host.move_to(*NEUTRAL_AT)
    host.run_frames(10)
    return host.screenshot(path)


def session(headless: Path, rom: Path, image: MagiCImage, output: Path) -> NeoSTHost:
    host = NeoSTHost(headless, rom, output, image=image,
                     options=["--machine", "megaste", "--mem", "4m", "--mono",
                              "--no-disk"],
                     auto=r"C:\AUTO\MAGXBOOT.TOS", timeout=240)
    (host.guest_root / "E").mkdir()
    return host


def run_case(headless: Path, rom: Path, source: Path, app: Path, output: Path,
             offset: int) -> dict[str, Path]:
    output.mkdir()
    image = MagiCImage.clone(source, output / "magic.img", partition_offset=offset)
    image.verify_magic()
    verify_launch_paths(image)
    if run(["mdir", "-i", image.image_spec, ACC_ARCHIVE]).returncode == 0:
        moved = run(["mren", "-o", "-i", image.image_spec,
                     ACC_ARCHIVE, ACC_DISABLED])
        require(moved.returncode == 0,
                f"cannot disable the COPS accessory: {moved.stdout}")
    image.mkdir(GUEST_CPX)
    reset_cpx_fixture(image)
    for name in CPX_MODULES:
        image.copy_in(CPX_SOURCE / f"{name}.CPX", f"{GUEST_CPX}/{name}.CPX")
    image.copy_in(app, GUEST_APP)

    host = session(headless, rom, image, output / "run")
    shots: dict[str, Path] = {}
    with host:
        host.boot(15000)
        host.calibrate_mouse()
        host.click(*PANEL_AT)
        host.run_frames(90)
        shots["panel"] = settled(host, output / "panel.ppm")
        host.move_to(*MAGXCONF_AT)
        host.run_frames(20)
        host.click()
        host.click()
        host.run_frames(180)
        shots["magxconf"] = settled(host, output / "magxconf.ppm")
        host.click(*CANCEL_AT)
        host.run_frames(90)
        shots["panel_closed"] = settled(host, output / "panel_closed.ppm")
        host.move_to(*TSLICE_AT)
        host.run_frames(20)
        host.click()
        host.click()
        host.run_frames(180)
        shots["tslice"] = settled(host, output / "tslice.ppm")
        return shots


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--magic-image", type=Path,
                        default=os.environ.get("MAGIC_IMAGE"))
    parser.add_argument("--tos", type=Path, default=os.environ.get("TOS206_ROM"))
    parser.add_argument("--neost", type=Path, default=Path.home() / ".local/bin/neost")
    parser.add_argument("--neost-headless", type=Path)
    parser.add_argument("--partition-offset", type=int, default=1024)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.magic_image is None or args.tos is None:
        parser.error("supply --magic-image/--tos or MAGIC_IMAGE/TOS206_ROM")
    source = args.magic_image.expanduser().resolve()
    rom = args.tos.expanduser().resolve()
    for path in (source, rom):
        if not path.is_file():
            parser.error(f"input not found: {path}")

    build = run([str(ROOT / "scripts/atari-container"), "run", "make", "-f",
                 "Makefile.apps", "cops"],
                cwd=ROOT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    for path in (ARCHIVE, REBUILT):
        if not path.is_file():
            print(f"build completed without producing {path}", file=sys.stderr)
            return 1

    try:
        output = create_output(args.output, parent=BUILD,
                               prefix="neost-cops").resolve()
        print(f"Evidence: {output}", flush=True)
        with tempfile.TemporaryDirectory(prefix="neost-cops-") as temporary:
            headless, _ = acquire_headless(args.neost_headless, args.neost,
                                           Path(temporary))
            cases: dict[str, dict[str, Path]] = {}
            for name, app in (("archive", ARCHIVE), ("rebuilt", REBUILT)):
                cases[name] = run_case(headless, rom, source, app,
                                       output / name, args.partition_offset)
        for name, shots in cases.items():
            closed, bbox = diff_stats(shots["panel"], shots["panel_closed"])
            require(closed == 0 or (bbox is not None and bbox_inside(bbox, ICON_STRIP)),
                    f"{name}: Cancel left non-icon changes ({closed} pixels "
                    f"at {bbox})")
            for dialog in ("magxconf", "tslice"):
                changed, bbox = diff_stats(shots["panel"], shots[dialog])
                require(changed > 0,
                        f"{name}: {dialog} dialog did not open "
                        f"(panel/dialog match)")
            distinct, bbox = diff_stats(shots["magxconf"], shots["tslice"])
            require(distinct > 0,
                    f"{name}: both dialogs render identically at {bbox}")
        for state in ("panel", "panel_closed", "magxconf", "tslice"):
            count, bbox = diff_stats(cases["archive"][state],
                                     cases["rebuilt"][state])
            require(count == 0,
                    f"{state}: archived/rebuilt screens differ at {bbox} "
                    f"({count} pixels)")
            print(f"{state}: screens identical")
        (output / "comparison.txt").write_text("PASS\n")
        print("COPS panel and both CPX dialogs match the archive", flush=True)
    except (OSError, RuntimeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
