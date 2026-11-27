"""Shared NeoST Pexec setup and result capture for console regressions."""

import re
import shutil
import sys
from pathlib import Path

from neost_common import sha256
from neost_server import NeoSTServer

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
DEFAULT_DRIVER = BUILD / "apps/console/RUNARG.PRG"
STATUS_RE = re.compile(r"^STATUS\s+(-?\d+)\s*$", re.MULTILINE)

sys.path.insert(0, str(ROOT / "scripts/atari-text"))
import atari_text


def atari_bytes(text):
    return atari_text.convert(text.encode("utf-8"), "encode")


def guest_name(name):
    return name.replace("\\", "/").upper()


def copy_fixture(source, destination):
    source = source.expanduser().resolve()
    copied = set()
    if source.is_dir():
        for child in source.iterdir():
            copied |= copy_fixture(child, destination)
    else:
        target = destination / source.name
        if target.exists():
            raise RuntimeError(f"fixture collides with {target}")
        shutil.copy2(source, target)
        copied.add(guest_name(source.name))
    return copied


def prepare(guest, executable, tail, driver):
    guest.mkdir(parents=True)
    suffix = executable.suffix.upper() if executable.suffix.upper() in (".TTP", ".PRG") else ".PRG"
    target_name = "TARGET" + suffix
    shutil.copy2(executable, guest / target_name)
    shutil.copy2(driver, guest / "RUNARG.PRG")
    config = f"C:\\{target_name}\r\n{tail}\r\n".encode("utf-8")
    (guest / "RUNARG.TXT").write_bytes(atari_bytes(config.decode("utf-8")))
    return target_name


def invoke(headless, rom, guest, output, label, frames, timeout,
           expected_status, expected_files, inputs):
    console = output / f"{label}.console.log"
    screenshot = output / f"{label}.ppm"
    options = ["--machine", "st", "--mem", "4m", "--no-disk",
               "--gemdos", str(guest), "--auto", r"C:\RUNARG.PRG",
               "--console-log", str(console)]
    server_dir = output / f"{label}-server"
    server_dir.mkdir()
    with NeoSTServer(headless, rom, server_dir, options=options,
                     timeout=timeout) as server:
        server.run_frames(frames)
        server.screenshot(screenshot)
    if not console.is_file():
        raise RuntimeError(f"NeoST did not create {console}")
    text = console.read_text(encoding="latin-1")
    match = STATUS_RE.search(text)
    if not match:
        raise RuntimeError(f"{label}: missing STATUS line; see {console}")
    status = int(match.group(1))
    # GEMDOS: negative 32-bit Pexec return means the child never loaded;
    # 0..0xffff is the child's own 16-bit Pterm/Pterm0 exit value.
    if not 0 <= status <= 0xFFFF:
        raise RuntimeError(
            f"{label}: Pexec loader failure (status {status}); see {console}")
    if status != expected_status:
        raise RuntimeError(
            f"{label}: status {status}, expected {expected_status}; see {console}")
    files = {}
    reserved = {"RUNARG.PRG", "RUNARG.TXT"}
    for path in sorted(guest.rglob("*")):
        if not path.is_file():
            continue
        name = guest_name(path.relative_to(guest).as_posix())
        if name in reserved or name.startswith("TARGET.") or name in inputs:
            continue
        files[name] = sha256(path)
    missing = expected_files.difference(files)
    unexpected = set(files).difference(expected_files)
    if missing or unexpected:
        details = []
        if missing:
            details.append("missing " + ", ".join(sorted(missing)))
        if unexpected:
            details.append("unexpected " + ", ".join(sorted(unexpected)))
        raise RuntimeError(
            f"{label}: guest outputs differ ({'; '.join(details)}); see {guest}")
    return status, text, files
