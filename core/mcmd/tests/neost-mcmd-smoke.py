#!/usr/bin/env python3
"""Compare localized archived/rebuilt MCMD batch execution on cloned MagiC disks."""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys
import tempfile

COMPONENT = Path(__file__).resolve().parents[1]
ROOT = COMPONENT.parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from neost_common import acquire_headless, create_output, require, run
from neost_image import MagiCImage
from neost_server import NeoSTServer

BUILD = ROOT / "build/atari"
APPS = BUILD / "apps/mcmd"
SOURCE = ROOT / "MagiC/APPS/CMD"
BATCH = r"""REM MCMD source-build regression fixture
SET VALUE=expanded
ECHO "alpha beta" > C:\QUOTE.TXT
ECHO 'literal %VALUE%' > C:\LITERAL.TXT
ECHO %VALUE% > C:\ENV.TXT
ECHO %1 > C:\ARG.TXT
TYPE C:\INPUT.TXT > C:\REDIR.TXT
ECHO first > C:\APPEND.TXT
ECHO second >> C:\APPEND.TXT
MD C:\WORK
COPY C:\INPUT.TXT C:\WORK\COPY.TXT
REN C:\WORK\COPY.TXT RENAMED.TXT
IF EXIST C:\WORK\RENAMED.TXT ECHO copied > C:\COPY.OK
IF EXIST C:\WORK ECHO directory > C:\DIR.OK
IF EXIST C:\WORK\ ECHO directory > C:\DIRSLASH.OK
C:\CHILD.TTP "two words" plain < C:\INPUT.TXT > C:\CHILD.OUT
IF ERRORLEVEL 7 ECHO child-seven > C:\STATUS.TXT
IF NOT ERRORLEVEL 8 ECHO exact >> C:\STATUS.TXT
GOTO SKIP
ECHO unreachable > C:\BAD.TXT
:SKIP
NO_SUCH_COMMAND
IF ERRORLEVEL 1 ECHO missing-command > C:\ERROR.TXT
DEL -N C:\WORK\RENAMED.TXT
RD C:\WORK
ECHO done > C:\DONE.TXT
EXIT
"""
EXPECTED = {
    "QUOTE.TXT": b"alpha beta\r\n",
    "LITERAL.TXT": b"literal %VALUE%\r\n",
    "ENV.TXT": b"expanded\r\n",
    "ARG.TXT": b"batch argument\r\n",
    "REDIR.TXT": b"input bytes\r\n",
    "APPEND.TXT": b"first\r\nsecond\r\n",
    "COPY.OK": b"copied\r\n",
    "DIR.OK": b"directory\r\n",
    "CHILD.OUT": b"child output\r\n",
    "DIRSLASH.OK": b"",
    "STATUS.TXT": b"child-seven\r\nexact\r\n",
    "ERROR.TXT": b"missing-command\r\n",
    "DONE.TXT": b"done\r\n",
    "INPUT.TXT": b"input bytes\r\n",
}

# Outputs and absence probes owned by this batch.  INPUT.TXT is a seeded input
# and is installed fresh, so it is not removed from the clone.
INHERITED_RESULTS = tuple(name for name in EXPECTED if name != "INPUT.TXT") + (
    "CHILD.RES", "MCMD.RES", "BAD.TXT")


def capture(headless: Path, rom: Path, source: Path, app: Path,
            output: Path, partition_offset: int) -> dict[str, bytes]:
    output.mkdir()
    image = MagiCImage.clone(source, output / "magic.img",
                             partition_offset=partition_offset)
    image.verify_magic()
    # A working base image can already hold batch outputs; clear them so every
    # result and absence probe describes this run.
    for name in INHERITED_RESULTS:
        image.remove(f"::/{name}")
    image.remove("::/WORK", tree=True)
    fixture = output / "fixture"
    fixture.mkdir()
    (fixture / "CASE.BAT").write_bytes(BATCH.replace("\n", "\r\n").encode("ascii"))
    (fixture / "INPUT.TXT").write_bytes(EXPECTED["INPUT.TXT"])
    image.install(((app, "::/MCMD.TOS"),
                   (APPS / "NEOSTART.PRG", "::/GEMSYS/MAGIC/START/MCMDTEST.PRG"),
                   (APPS / "CHILD.TTP", "::/CHILD.TTP"),
                   (fixture / "CASE.BAT", "::/CASE.BAT"),
                   (fixture / "INPUT.TXT", "::/INPUT.TXT")))
    guest = output / "GEMDRIVE"
    guest.mkdir()
    server_dir = output / "server"
    server_dir.mkdir()
    options = ["--machine", "megaste", "--mem", "4m", "--mono", "--no-disk",
               "--acsi", str(image.path), "--gemdos", str(guest),
               "--auto", r"C:\AUTO\MAGXBOOT.TOS",
               "--console-log", str(output / "console.log")]
    with NeoSTServer(headless, rom, server_dir, options=options, timeout=240) as server:
        server.run_frames(16000)
        server.screenshot(output / "desktop.ppm")
    results = output / "results"
    results.mkdir()
    for name in ("CHILD.RES", "MCMD.RES"):
        require(image.exists(f"::/{name}"),
                f"{output.name}: run did not create fresh {name}")
    contents = {}
    for name in (*EXPECTED, "CHILD.RES", "MCMD.RES"):
        destination = results / name
        image.copy_out(f"::/{name}", destination)
        contents[name] = destination.read_bytes()
    for name, expected in EXPECTED.items():
        require(contents[name] == expected,
                f"{output.name}: {name}: expected {expected!r}, got {contents[name]!r}")
    require(contents["MCMD.RES"] == b"STATUS 0\r\n",
            f"{output.name}: MCMD did not return status zero: {contents['MCMD.RES']!r}")
    child = contents["CHILD.RES"].decode("ascii").strip()
    require(child == "argc=4 arg1=two arg2=words arg3=plain env=expanded input=13 valid=1",
            f"{output.name}: child arguments/environment/stdin differ: {child}")
    for absent in ("BAD.TXT", "WORK"):
        require(not image.exists(f"::/{absent}"),
                f"{output.name}: unexpected {absent}")
    return contents


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--magic-image", type=Path, default=os.environ.get("MAGIC_IMAGE"))
    parser.add_argument("--tos", type=Path, default=os.environ.get("TOS206_ROM"))
    parser.add_argument("--neost", type=Path, default=Path.home() / ".local/bin/neost")
    parser.add_argument("--neost-headless", type=Path)
    parser.add_argument("--partition-offset", type=int, default=1024)
    parser.add_argument("--languages", nargs="+", choices=("de", "en", "fr"),
                        default=["de", "en", "fr"])
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.magic_image is None or args.tos is None:
        parser.error("supply --magic-image/--tos or MAGIC_IMAGE/TOS206_ROM")
    source, rom = args.magic_image.expanduser().resolve(), args.tos.expanduser().resolve()
    for path in (source, rom):
        if not path.is_file():
            parser.error(f"input not found: {path}")
    build = run([str(ROOT / "scripts/atari-container"), "run", "make", "-f",
                 "Makefile.apps", "mcmd-languages", "mcmd-test-driver"], cwd=ROOT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-mcmd")
        print(f"Evidence: {output}", flush=True)
        with tempfile.TemporaryDirectory(prefix="neost-mcmd-") as temporary:
            headless, _ = acquire_headless(args.neost_headless, args.neost, Path(temporary))
            for language in args.languages:
                directory = output / language
                directory.mkdir()
                archive = capture(headless, rom, source,
                                  SOURCE / f"BIN/MCMD_{language.upper()}.TOS",
                                  directory / "archive", args.partition_offset)
                rebuilt = capture(headless, rom, source, APPS / language / "MCMD.TOS",
                                  directory / "rebuilt", args.partition_offset)
                require(archive == rebuilt, f"{language}: archived/rebuilt guest results differ")
                print(f"{language}: child status and {len(rebuilt)} guest files match", flush=True)
        (output / "comparison.txt").write_text("PASS " + " ".join(args.languages) + "\n")
    except (RuntimeError, OSError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
