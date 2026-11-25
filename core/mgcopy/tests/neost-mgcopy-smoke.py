#!/usr/bin/env python3
"""Verify rebuilt MGCOPY file operations on a disposable MagiC image."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys
import tempfile

COMPONENT = Path(__file__).resolve().parents[1]
ROOT = COMPONENT.parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from neost_common import acquire_headless, create_output, require, resolve_rom, run, sha256
from neost_image import MagiCImage
from neost_server import NeoSTServer

BUILD = ROOT / "build/atari"
REBUILT_APP = BUILD / "apps/mgcopy/MGCOPY.APP"
REBUILT_RSC = BUILD / "apps/mgcopy/MGCOPY.RSC"
DRIVER = BUILD / "apps/mgcopy/NEOSTART.PRG"
START_DIR = r"::/GEMSYS/MAGIC/START"
DEST_DIR = r"::/DEST"
BOOT_FRAMES = 15000
RUN_FRAMES = 9000
RESULT = "::/MGCOPY.RES"
ALIAS_TARGET = b"C:\\ALIAS.TXT"
D_PARTITION_OFFSET = 34200576
# Guest labels the suite must leave byte-identical to the installed seed.
SEEDED_GUESTS = {
    "src": "SRC.TXT",
    "alias-source": "ALIAS.TXT",
    "dest-src": "SRC.TXT",
    "dest-move": "MOVE.TXT",
    "tree-source-one": "TREE/ONE.TXT",
    "tree-source-sub-two": "TREE/SUB/TWO.TXT",
    "dest-tree-one": "TREE/ONE.TXT",
    "dest-tree-sub-two": "TREE/SUB/TWO.TXT",
    "dest-mtree-one": "MTREE/ONE.TXT",
    "dest-mtree-sub-two": "MTREE/SUB/TWO.TXT",
    "dest-xmove": "XMOVE.TXT",
}


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
    if not image.exists(guest):
        return None
    image.copy_out(guest, destination)
    return sha256(destination)


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
    for path in (REBUILT_APP, REBUILT_RSC, DRIVER):
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
            seed_hashes = {name: sha256(path) for name, path in fixtures.items()}
            seed_hashes["XMOVE.TXT"] = sha256(scratch / "fixtures" / "XMOVE.TXT")
            headless, _ = acquire_headless(args.neost_headless, args.neost, scratch)
            image = MagiCImage.clone(source, output / "rebuilt.img",
                                     partition_offset=args.partition_offset)
            image.verify_magic()
            # Remove inherited outputs so only this run can satisfy the checks.
            image.remove(RESULT)
            image.remove(DEST_DIR, tree=True)
            guest_root = output / "GEMDRIVE"
            guest_root.mkdir()
            image.install(((DRIVER, f"{START_DIR}/NEOSTART.PRG"),
                           (REBUILT_APP, "::/MGCOPY.APP"),
                           (REBUILT_RSC, "::/MGCOPY.RSC")))
            image.mkdir(DEST_DIR)
            install_fixtures(image, fixtures)
            d_image = MagiCImage(image.path, args.d_partition_offset)
            d_image.copy_in(scratch / "fixtures" / "XMOVE.TXT", "::/XMOVE.TXT")
            with start_server(headless, rom, image.path, output / "run", guest_root) as server:
                server.run_frames(BOOT_FRAMES + RUN_FRAMES)
                server.screenshot(output / "final.ppm")

            result_path = output / "MGCOPY.RES"
            require(image.exists(RESULT), f"driver did not produce {RESULT}")
            image.copy_out(RESULT, result_path)
            statuses = result_path.read_text(encoding="latin-1").splitlines()
            require(len(statuses) == 7,
                    f"expected seven numbered cases, got {statuses!r}")
            for number, line in enumerate(statuses, 1):
                require(line.split() == ["CASE", f"{number:02d}", "0"],
                        f"case {number:02d} did not report zero: {line!r}")

            for guest, label in (
                ("::/SRC.TXT", "src"),
                ("::/ALIAS.TXT", "alias-source"),
                ("::/DEST/SRC.TXT", "dest-src"),
                ("::/DEST/MOVE.TXT", "dest-move"),
                ("::/TREE/ONE.TXT", "tree-source-one"),
                ("::/TREE/SUB/TWO.TXT", "tree-source-sub-two"),
                ("::/DEST/TREE/ONE.TXT", "dest-tree-one"),
                ("::/DEST/TREE/SUB/TWO.TXT", "dest-tree-sub-two"),
                ("::/DEST/MTREE/ONE.TXT", "dest-mtree-one"),
                ("::/DEST/MTREE/SUB/TWO.TXT", "dest-mtree-sub-two"),
                ("::/DEST/XMOVE.TXT", "dest-xmove"),
            ):
                seed = SEEDED_GUESTS[label]
                actual = guest_hash(image, guest, output / f"{label}.out")
                require(actual == seed_hashes[seed],
                        f"{guest} is missing or differs from seed {seed}")

            for guest in ("::/MOVE.TXT", "::/DEL.TXT",
                          "::/MTREE/ONE.TXT", "::/MTREE/SUB/TWO.TXT"):
                require(not image.exists(guest), f"source left behind: {guest}")
            require(not d_image.exists("::/XMOVE.TXT"),
                    "cross-drive move left D:/XMOVE.TXT behind")

            # Fsymlink stores a big-endian payload length and a NUL-padded target.
            alias_path = output / "dest-alias.out"
            require(image.exists("::/DEST/ALIAS.TXT"), "alias destination is missing")
            image.copy_out("::/DEST/ALIAS.TXT", alias_path)
            record = alias_path.read_bytes()
            payload = ALIAS_TARGET + b"\0\0"
            require(record == len(payload).to_bytes(2, "big") + payload,
                    f"alias record is not a link to {ALIAS_TARGET!r}: {record!r}")
        print(f"MGCOPY file-operation suite passed: {output}")
    except (OSError, RuntimeError) as error:
        print(f"NeoST MGCOPY smoke failed: {error}\nArtifacts: {output}",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
