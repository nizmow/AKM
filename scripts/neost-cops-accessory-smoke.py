#!/usr/bin/env python3
"""Compare the COPS desk accessory lifecycle on disposable MagiC images."""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import tempfile
from concurrent.futures import ThreadPoolExecutor

from neost_common import acquire_headless, create_output, require, run, sha256
from neost_host import NeoSTHost
from neost_image import MagiCImage
from neost_screen import bbox_inside, diff_stats

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build/atari"
GUEST = "::/AUTO/ACCS/COPS.ACC"
ARCHIVE = ROOT / "MagiC/Releases/MAGIC.A/AUTO/ACCS/COPS.ACC"
REBUILT = BUILD / "apps/cops/COPS.APP"
TSLICE = ROOT / "MagiC/APPS/CPX/BIN/TSLICE.CPX"


def run_case(headless, rom, source, offset, folder, accessory):
    image = MagiCImage.clone(source, folder / "magic.img", partition_offset=offset)
    image.verify_magic()
    settings = run(["mtype", "-i", image.image_spec, "::/MAGX.INF"])
    require(settings.returncode == 0, settings.stdout)
    lines = [line.upper().split() for line in settings.stdout.splitlines()]
    for key, path in (("#_ACC", r"C:\AUTO\ACCS"), ("#_APP", r"C:\GEMSYS\MAGIC\START")):
        require([line[1].rstrip("\\") for line in lines if line[:1] == [key]] == [path],
                f"{key} must point to {path}")
    for name in ("::/GEMSYS/MAGIC/START/COPS.*", GUEST):
        result = run(["mdel", "-i", image.image_spec, name])
        require(result.returncode == 0 or "not found" in result.stdout.lower(), result.stdout)
    if accessory is not None:
        image.copy_in(accessory, GUEST)
        image.copy_out(GUEST, folder / "installed.acc")
        require(sha256(folder / "installed.acc") == sha256(accessory), "wrong COPS installed")
    image.copy_in(TSLICE, "::/AUTO/ACCS/CPX/TSLICE.CPX")
    host = NeoSTHost(headless, rom, folder / "run", image=image,
                     options=["--machine", "megaste", "--mem", "4m", "--mono", "--no-disk"],
                     auto=r"C:\AUTO\MAGXBOOT.TOS", timeout=240)
    (host.guest_root / "E").mkdir()
    shots = {}
    with host:
        host.boot(10000)
        host.calibrate_mouse()

        def snap(state, *, park=True):
            if park:
                host.move_to(408, 384)
                host.run_frames(10)
            shots[state] = host.screenshot(folder / f"{state}.ppm")

        def menu():
            host.move_to(40, 8)
            host.run_frames(30)
            host.click()
            host.run_frames(60)

        def select():
            host.move_to(70, 65)
            host.run_frames(20)
            host.click()
            host.run_frames(180)

        snap("desktop")
        menu()
        snap("menu", park=False)
        if accessory is None:
            return shots
        select()
        snap("panel")
        host.move_to(113, 188)
        host.run_frames(20)
        host.click()
        host.click()
        host.run_frames(180)
        snap("dialog")
        host.click(406, 281)
        host.run_frames(90)
        snap("cancelled")
        host.click(65, 118)
        host.run_frames(90)
        snap("closed")
        menu()
        select()
        snap("reopened")
    return shots


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--magic-image", type=Path, default=os.environ.get("MAGIC_IMAGE"))
    parser.add_argument("--tos", type=Path, default=os.environ.get("TOS206_ROM"))
    parser.add_argument("--neost", type=Path, default=Path.home() / ".local/bin/neost")
    parser.add_argument("--neost-headless", type=Path)
    parser.add_argument("--partition-offset", type=int, default=1024)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.magic_image is None or args.tos is None:
        parser.error("supply --magic-image/--tos or MAGIC_IMAGE/TOS206_ROM")
    source, rom = (path.expanduser().resolve() for path in (args.magic_image, args.tos))
    try:
        build = run([str(ROOT / "scripts/atari-container"), "run", "make", "-f",
                     "Makefile.apps", "cops"], cwd=ROOT, timeout=900)
        require(build.returncode == 0, build.stdout)
        output = create_output(args.output, parent=BUILD, prefix="neost-cops-accessory").resolve()
        print(f"Evidence: {output}", flush=True)
        with tempfile.TemporaryDirectory(prefix="neost-cops-acc-") as scratch:
            headless, _ = acquire_headless(args.neost_headless, args.neost, Path(scratch))
            (output / "inputs.txt").write_text(f"{sha256(source)}  {source}\n{sha256(rom)}  {rom}\n")
            with ThreadPoolExecutor() as pool:
                archive, rebuilt, missing = [job.result() for job in [
                    pool.submit(run_case, headless, rom, source, args.partition_offset, output / name, acc)
                    for name, acc in (("archive", ARCHIVE), ("rebuilt", REBUILT), ("missing", None))
                ]]
        changed, box = diff_stats(archive["menu"], missing["menu"], region=(15, 42, 152, 69))
        require(changed > 100, f"accessory menu item not distinguished from control ({box})")
        for state in archive:
            count, box = diff_stats(archive[state], rebuilt[state])
            require(count == 0, f"{state}: archived/rebuilt differ at {box} ({count} pixels)")
        require(diff_stats(archive["desktop"], archive["panel"])[0] > 1000, "panel did not open")
        require(diff_stats(archive["panel"], archive["dialog"])[0] > 1000, "CPX did not open")
        require(diff_stats(archive["panel"], archive["cancelled"], region=(192, 105, 450, 232))[0] == 0,
                "Cancel did not dismiss the CPX")
        closed, box = diff_stats(archive["desktop"], archive["closed"])
        require(closed <= 8 and (box is None or bbox_inside(box, (60, 210, 90, 235))),
                f"panel did not close ({closed} pixels at {box})")
        count, box = diff_stats(archive["panel"], archive["reopened"])
        require(count < 2000 and (box is None or bbox_inside(box, (56, 169, 170, 215))),
                f"panel did not reopen at the original position ({box}, {count} pixels)")
        (output / "comparison.txt").write_text("PASS\n")
    except (OSError, RuntimeError) as error:
        print(f"FAIL: {error}")
        return 1
    print("COPS accessory menu, panel lifecycle and TSLICE dialog match the archive")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
