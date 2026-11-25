#!/usr/bin/env python3
"""Compare rebuilt image SLBs with a selected legacy baseline under NeoST.

Each pair runs the same GCC probe against a fresh image clone and the same
PAT6 fixture. The probe checks the known XP conversion, then exercises
LOAD_IMG exports #0, #2, #3 and #1 and hashes rendered colours rather than
descriptor or framebuffer storage.

The probe names a dedicated guest directory explicitly, while inherited
same-name libraries are removed from MagiC's default search directory and
SLBPATH/#_SLB overrides are stripped. Control runs with one library absent
must report a guest Slbopen failure instead of silently loading a copy.
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import sys
import tempfile

from neost_common import acquire_headless, create_output, require, run
from neost_image import MagiCImage
from neost_host import NeoSTHost

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
ARCHIVE = ROOT / "MagiC/Releases/MAGIC.A/GEMSYS/MAGIC/XTENSION"
ARCHIVE_XP = ROOT / "MagiC/EXTENSIO/XP_RASTR.SLB/BIN/XP_RASTR.SLB"
ARCHIVE_LOAD = ARCHIVE / "LOAD_IMG.SLB"
SOURCE_LOAD = ROOT / "MagiC/TOOLS/LOAD_IMG.SLB/BIN/LOAD_IMG.SLB"
PAT6 = ROOT / "MagiC/Releases/MAGIC.A/GEMSYS/GEMDESK/PAT/16/PAT6.IMG"
XP = BUILD / "apps/xp-rastr/XP_RASTR.SLB"
LOAD = BUILD / "apps/load-img/LOAD_IMG.SLB"
PROBE = BUILD / "apps/slbtest/SLBTEST.PRG"

XP_NAME = "XP_RASTR.SLB"
LOAD_NAME = "LOAD_IMG.SLB"
SLB_NAMES = (XP_NAME, LOAD_NAME)
# Slbopen searches the loaded list, SLBPATH/the default XTENSION directory and
# any explicit path.  Test copies live in a dedicated directory named by the
# probe; inherited same-name copies are removed from the default directory.
SLB_AUDIT_DIR = "::/SLBAUDIT"
INHERITED_SLB_DIR = "::/GEMSYS/MAGIC/XTENSION"


def configure_inf(image: MagiCImage, output: Path) -> None:
    """Remove every way the image could load an inherited same-name library."""
    inf = output / "MAGX.INF"
    image.copy_out("::/MAGX.INF", inf)
    data = inf.read_bytes()
    data = data.replace(b"#_DEV 1 0", b"#_DEV 2 0")
    require(b"#_DEV 2 0" in data, "MAGX.INF does not select a colour mode")
    # Slbopen always also searches SLBPATH (or the default XTENSION directory)
    # in addition to an explicit path, so drop any override that would point at
    # an inherited copy.
    data = re.sub(rb"(?im)^[ \t]*#_ENV[ \t]+SLBPATH=.*(?:\r?\n|$)", b"", data)
    require(not re.search(rb"(?im)^[ \t]*#_ENV[ \t]+SLBPATH=", data),
            "MAGX.INF still declares SLBPATH")
    # #_SLB entries load a library before the probe runs; Slbopen returns an
    # already-loaded library by name regardless of the path it is given.
    for library in SLB_NAMES:
        active = re.compile(rb"(?im)^[ \t]*#_SLB[ \t]+\d+[ \t]+" +
                            re.escape(library.encode()) + rb"[ \t]*\r?\n?")
        data = active.sub(b"", data)
        require(not active.search(data),
                f"MAGX.INF still preloads {library}")
    inf.write_bytes(data)
    image.copy_in(inf, "::/MAGX.INF")


def prepare_case(name: str, pair: tuple[Path, Path], source: Path,
                 output: Path, *, missing: frozenset[str] = frozenset()
                 ) -> MagiCImage:
    directory = output / name
    directory.mkdir()
    image = MagiCImage.clone(source, directory / "magic.img", partition_offset=1024)
    image.verify_magic()
    configure_inf(image, directory)
    for library in SLB_NAMES:
        inherited = f"{INHERITED_SLB_DIR}/{library}"
        image.remove(inherited)
        require(not image.exists(inherited),
                f"{name}: inherited {inherited} survived removal")
    image.remove(SLB_AUDIT_DIR, tree=True)
    image.mkdir(SLB_AUDIT_DIR)
    for local, library in ((pair[0], XP_NAME), (pair[1], LOAD_NAME)):
        target = f"{SLB_AUDIT_DIR}/{library}"
        if library in missing:
            require(not image.exists(target),
                    f"{name}: missing {target} is present in the clone")
            continue
        image.copy_in(local, target)
        require(image.exists(target), f"{name}: {target} was not installed")
    image.install(((PAT6, "::/GEMSYS/GEMDESK/PAT/16/PAT6.IMG"),
                   (PROBE, "::/GEMSYS/MAGIC/START/ZCNFTEST.PRG")))
    # Never accept a marker left in the input image as proof of execution.
    image.remove("::/SLBTEST.TXT")
    require(not image.exists("::/SLBTEST.TXT"),
            f"{name}: stale SLBTEST.TXT marker survived removal")
    return image


def run_probe(name: str, pair: tuple[Path, Path], source: Path,
              output: Path, headless: Path, rom: Path, *,
              missing: frozenset[str] = frozenset()) -> str:
    directory = output / name
    image = prepare_case(name, pair, source, output, missing=missing)

    host = NeoSTHost(headless, rom, directory / "run", image=image,
                     options=["--machine", "megaste", "--mem", "4m", "--no-disk"],
                     auto=r"C:\AUTO\MAGXBOOT.TOS", timeout=240)
    with host:
        host.boot(15000)
        host.screenshot(directory / "screen.ppm")
    require(image.exists("::/SLBTEST.TXT"),
            f"{name}: probe did not write SLBTEST.TXT")
    result = directory / "SLBTEST.TXT"
    image.copy_out("::/SLBTEST.TXT", result)
    text = result.read_text(errors="replace").strip()
    print(f"{name} SLB consumer result: {text}")
    return text


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--magic-image", type=Path,
                        default=os.environ.get("MAGIC_IMAGE"))
    parser.add_argument("--tos", type=Path, default=os.environ.get("TOS206_ROM"))
    parser.add_argument("--neost-headless", type=Path)
    parser.add_argument("--baseline", choices=("source", "release"), default="source",
                        help="compare with the source-tree archive or the shipped Release")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.magic_image is None or args.tos is None:
        parser.error("supply --magic-image/--tos or MAGIC_IMAGE/TOS206_ROM")
    source, rom = args.magic_image.expanduser().resolve(), args.tos.expanduser().resolve()
    archive_load = SOURCE_LOAD if args.baseline == "source" else ARCHIVE_LOAD
    for path in (source, rom, ARCHIVE_XP, archive_load, PAT6):
        require(path.is_file(), f"required input not found: {path}")

    build = run(["scripts/atari-container", "run", "make", "-f", "Makefile.apps",
                 "xp-rastr", "load-img", "slbtest"], cwd=ROOT, timeout=300)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        print(f"ERROR: SLB consumer build exited {build.returncode}", file=sys.stderr)
        return build.returncode
    for path in (XP, LOAD, PROBE):
        require(path.is_file(), f"build completed without producing {path}")

    output = create_output(args.output, parent=BUILD, prefix="neost-slb-consumer")
    print(f"Evidence: {output}", flush=True)
    with tempfile.TemporaryDirectory(prefix="neost-slb-") as temporary:
        headless, _ = acquire_headless(args.neost_headless,
                                       Path.home() / ".local/bin/neost",
                                       Path(temporary))
        if args.baseline == "source":
            # Slbopen needs space for its own bookkeeping after Pexec. The
            # source archive predates the minimal-allocation flag used by
            # the shipped SLB. Adjust only the flag in a disposable copy.
            archive_data = bytearray(archive_load.read_bytes())
            require(archive_data[22:26] == b"\x00\x00\x00\x07",
                    "unexpected source archive TOS program flags")
            archive_data[25] |= 8
            archive_load = output / SOURCE_LOAD.name
            archive_load.write_bytes(archive_data)
        archived = run_probe(args.baseline, (ARCHIVE_XP, archive_load), source,
                             output, headless, rom)
        rebuilt = run_probe("rebuilt", (XP, LOAD), source, output, headless, rom)
        # Controls: with one library absent from the clone (and no inherited
        # copy anywhere on the search paths) Slbopen must report a guest load
        # failure instead of silently reusing another same-name library.
        missing_xp = run_probe("missing-xp", (XP, LOAD), source, output,
                               headless, rom, missing=frozenset({XP_NAME}))
        missing_load = run_probe("missing-load", (XP, LOAD), source, output,
                                 headless, rom, missing=frozenset({LOAD_NAME}))
    expected_suffix = "72246d45"
    for name, text in ((args.baseline, archived), ("rebuilt", rebuilt)):
        require(text.startswith("ok 64 64 ") and text.endswith(expected_suffix),
                f"{name}: probe failed or XP conversion differed: {text}")
    for name, text, prefix in (("missing-xp", missing_xp, "xp open"),
                               ("missing-load", missing_load, "load open")):
        fields = text.split()
        require(text.startswith(prefix + " "),
                f"{name}: expected {prefix} failure, got: {text}")
        try:
            code = int(fields[2])
        except (IndexError, ValueError):
            raise RuntimeError(
                f"{name}: malformed guest failure report: {text}") from None
        require(code < 0,
                f"{name}: missing library did not report a load failure: {text}")
    (output / "comparison.txt").write_text(
        f"{args.baseline}={archived}\nrebuilt={rebuilt}\n"
        f"missing-xp={missing_xp}\nmissing-load={missing_load}\n",
        encoding="utf-8")
    require(archived == rebuilt,
            f"rendered IMG colours differ from {args.baseline}: {archived} != {rebuilt}")
    print(f"{args.baseline} and rebuilt IMG colours: identical")
    print("missing-library controls: guest Slbopen failure observed for each")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise SystemExit(1)
