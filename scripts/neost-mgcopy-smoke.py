#!/usr/bin/env python3
"""Compare archived and rebuilt MGCOPY file operations under MagiC.

Both binaries run in separate clones of the same MagiC image.  A delayed
startup driver invokes MGCOPY through Pexec with the same fixtures and writes
the child statuses to C:\\MGCOPY.RES.  The host compares statuses and guest
file effects, including nested-tree and cross-drive moves.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys
import tempfile

from neost_common import acquire_headless, create_output, require, resolve_rom, run, sha256
from neost_image import MagiCImage
from neost_server import NeoSTServer

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
ARCHIVE_APP = ROOT / "MagiC/APPS/MGCOPY/BIN/MGCOPY.APP"
ARCHIVE_RSC = ROOT / "MagiC/APPS/MGCOPY/SRC/MGCOPY.RSC"
REBUILT_APP = BUILD / "apps/mgcopy/MGCOPY.APP"
REBUILT_RSC = BUILD / "apps/mgcopy/MGCOPY.RSC"
DRIVER = BUILD / "apps/mgcopy/NEOSTART.PRG"
START_DIR = r"::/GEMSYS/MAGIC/START"
DEST_DIR = r"::/DEST"
BOOT_FRAMES = 15000
RUN_FRAMES = 9000
RESULT = "::/MGCOPY.RES"
D_PARTITION_OFFSET = 34200576


def start_server(headless: Path, rom: Path, image: Path, output: Path,
                 guest_root: Path) -> NeoSTServer:
    output.mkdir(parents=True)
    options = [
        "--machine", "megaste", "--mem", "4m", "--mono", "--no-disk",
        "--acsi", str(image), "--gemdos", str(guest_root),
        "--auto", r"C:\AUTO\MAGXBOOT.TOS",
    ]
    return NeoSTServer(headless, rom, output, options=options, timeout=240.0)


def make_fixtures(directory: Path) -> dict[str, Path]:
    directory.mkdir(parents=True)
    content = {
        "SRC.TXT": b"copy source\n",
        "MOVE.TXT": b"move source\n",
        "ALIAS.TXT": b"alias source\n",
        "DEL.TXT": b"delete source\n",
        "TREE/ONE.TXT": b"tree one\n",
        "TREE/SUB/TWO.TXT": b"tree two\n",
        "MTREE/ONE.TXT": b"mtree one\n",
        "MTREE/SUB/TWO.TXT": b"mtree two\n",
    }
    paths = {}
    for name, data in content.items():
        path = directory / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        paths[name] = path
    (directory / "XMOVE.TXT").write_bytes(b"cross drive move\n")
    return paths


def install_fixtures(image: MagiCImage, fixtures: dict[str, Path]) -> None:
    for name, path in fixtures.items():
        guest = f"::/{name}"
        parent = guest.rsplit("/", 1)[0]
        if parent != "::":
            image.mkdir(parent)
        image.copy_in(path, guest)


def guest_hash(image: MagiCImage, guest: str, destination: Path) -> str | None:
    try:
        image.copy_out(guest, destination)
    except RuntimeError:
        return None
    return sha256(destination)


def host_hash(path: Path) -> str | None:
    return sha256(path) if path.is_file() else None


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
    parser.add_argument("--d-partition-offset", type=int, default=D_PARTITION_OFFSET,
                        help="offset of the image's D: partition (default: 34200576)")
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
                 "Makefile.apps", "mgcopy", "mgcopy-test-driver"],
                cwd=ROOT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    for path in (ARCHIVE_APP, ARCHIVE_RSC, REBUILT_APP, REBUILT_RSC, DRIVER):
        if not path.is_file():
            print(f"build completed without producing {path}", file=sys.stderr)
            return 1

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-mgcopy")
    except OSError as error:
        parser.error(str(error))

    try:
        with tempfile.TemporaryDirectory(prefix="neost-mgcopy-") as temporary:
            scratch = Path(temporary)
            fixtures = make_fixtures(scratch / "fixtures")
            headless, _ = acquire_headless(args.neost_headless, args.neost, scratch)
            outcomes = {}
            for name in ("archive", "rebuilt"):
                directory = output / name
                directory.mkdir()
                image = MagiCImage.clone(source, directory / f"{name}.img",
                                         partition_offset=args.partition_offset)
                image.verify_magic()
                app = ARCHIVE_APP if name == "archive" else REBUILT_APP
                rsc = ARCHIVE_RSC if name == "archive" else REBUILT_RSC
                guest_root = directory / "GEMDRIVE"
                guest_root.mkdir(parents=True)
                image.install(((DRIVER, f"{START_DIR}/NEOSTART.PRG"),
                               (app, "::/MGCOPY.APP"),
                               (rsc, "::/MGCOPY.RSC")))
                image.mkdir(DEST_DIR)
                install_fixtures(image, fixtures)
                d_image = MagiCImage(image.path, args.d_partition_offset)
                d_image.copy_in(scratch / "fixtures" / "XMOVE.TXT", "::/XMOVE.TXT")
                with start_server(headless, rom, image.path,
                                  directory / "run", guest_root) as server:
                    server.run_frames(BOOT_FRAMES + RUN_FRAMES)
                    server.screenshot(directory / "final.ppm")
                result_path = directory / "MGCOPY.RES"
                result_hash = guest_hash(image, RESULT, result_path)
                if result_hash is None:
                    raise RuntimeError(f"{name}: driver did not produce {RESULT}")
                checks = {}
                for guest, label in (
                    ("::/SRC.TXT", "src"),
                    ("::/MOVE.TXT", "moved-source"),
                    ("::/ALIAS.TXT", "alias-source"),
                    ("::/DEL.TXT", "deleted-source"),
                    ("::/DEST/SRC.TXT", "dest-src"),
                    ("::/DEST/MOVE.TXT", "dest-move"),
                    ("::/DEST/ALIAS.TXT", "dest-alias"),
                    ("::/TREE/ONE.TXT", "tree-source-one"),
                    ("::/TREE/SUB/TWO.TXT", "tree-source-sub-two"),
                    ("::/DEST/TREE/ONE.TXT", "dest-tree-one"),
                    ("::/DEST/TREE/SUB/TWO.TXT", "dest-tree-sub-two"),
                    ("::/MTREE/ONE.TXT", "mtree-source-one"),
                    ("::/MTREE/SUB/TWO.TXT", "mtree-source-sub-two"),
                    ("::/DEST/MTREE/ONE.TXT", "dest-mtree-one"),
                    ("::/DEST/MTREE/SUB/TWO.TXT", "dest-mtree-sub-two"),
                    ("::/DEST/XMOVE.TXT", "dest-xmove"),
                ):
                    checks[label] = guest_hash(image, guest, directory / f"{label}.out")
                checks["xmove-source"] = guest_hash(
                    d_image, "::/XMOVE.TXT", directory / "xmove-source.out")
                outcomes[name] = (result_path.read_text(encoding="latin-1"), checks)
                print(f"{name}: {checks}")

        archive_text, archive_files = outcomes["archive"]
        rebuilt_text, rebuilt_files = outcomes["rebuilt"]
        require(archive_text == rebuilt_text,
                "archived/rebuilt driver statuses differ")
        require(archive_files == rebuilt_files,
                "archived/rebuilt guest file effects differ")
        require(archive_files["dest-src"] is not None, "copy did not create DEST/SRC.TXT")
        require(archive_files["moved-source"] is None, "move left MOVE.TXT behind")
        require(archive_files["dest-move"] is not None, "move did not create DEST/MOVE.TXT")
        require(archive_files["alias-source"] is not None, "alias removed ALIAS.TXT")
        require(archive_files["dest-alias"] is not None, "alias did not create DEST/ALIAS.TXT")
        require(archive_files["deleted-source"] is None, "delete left DEL.TXT behind")
        require(archive_files["tree-source-one"] is not None, "copy removed TREE/ONE.TXT")
        require(archive_files["dest-tree-one"] is not None, "nested copy missed TREE/ONE.TXT")
        require(archive_files["dest-tree-sub-two"] is not None, "nested copy missed TREE/SUB/TWO.TXT")
        require(archive_files["mtree-source-one"] is None, "nested move left MTREE/ONE.TXT")
        require(archive_files["mtree-source-sub-two"] is None, "nested move left MTREE/SUB/TWO.TXT")
        require(archive_files["dest-mtree-one"] is not None, "nested move missed DEST/MTREE/ONE.TXT")
        require(archive_files["dest-mtree-sub-two"] is not None, "nested move missed DEST/MTREE/SUB/TWO.TXT")
        require(archive_files["xmove-source"] is None, "cross-drive move left D:/XMOVE.TXT")
        require(archive_files["dest-xmove"] is not None, "cross-drive move missed DEST/XMOVE.TXT")
        print(f"MGCOPY archived/rebuilt file-operation suite passed: {output}")
    except (OSError, RuntimeError) as error:
        print(f"NeoST MGCOPY smoke failed: {error}\nArtifacts: {output}",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
