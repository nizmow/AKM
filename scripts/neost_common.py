"""Host-side helpers shared by the standalone NeoST smoke runners.

Machine profiles, input timing, guest fixtures and success assertions belong to
individual runners, not this module.
"""

from __future__ import annotations

import datetime
import hashlib
from pathlib import Path
import shutil
import subprocess


def run(command: list[str], *, cwd: Path | None = None,
        timeout: int = 240) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, cwd=cwd, text=True, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, timeout=timeout, check=False)


def run_logged(command: list[str], log: Path, *, cwd: Path | None = None,
               timeout: int = 240) -> None:
    """Keep emulator diagnostics on disk, including partial output on timeout."""
    try:
        result = run(command, cwd=cwd, timeout=timeout)
    except subprocess.TimeoutExpired as error:
        output = error.output or b""
        text = output.decode("utf-8", errors="replace") if isinstance(output, bytes) else output
        log.write_text(text + f"\nTimed out after {timeout} seconds.\n", encoding="utf-8")
        raise RuntimeError(f"NeoST timed out after {timeout} seconds; see {log}") from error
    log.write_text(result.stdout, encoding="utf-8")
    if result.returncode:
        raise RuntimeError(f"NeoST exited {result.returncode}; see {log}")


def require(condition: bool, message: str) -> None:
    """Raise a scenario failure when an assertion fails."""
    if not condition:
        raise RuntimeError(message)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def create_output(requested: Path | None, *, parent: Path, prefix: str) -> Path:
    """Create a fresh artifact directory; never reuse an existing guest tree."""
    output = (requested.expanduser().resolve() if requested else
              parent / f"{prefix}-{datetime.datetime.now():%Y%m%d-%H%M%S}")
    try:
        output.mkdir(parents=True)
    except FileExistsError as error:
        raise FileExistsError(f"output already exists; choose a new path: {output}") from error
    return output


def acquire_headless(explicit: Path | None, appimage: Path,
                     scratch: Path) -> tuple[Path, Path | None]:
    """Prefer an explicit executable, then PATH, then scratch AppImage extraction."""
    found = str(explicit.expanduser()) if explicit is not None else shutil.which("neost-headless")
    if found:
        headless = Path(found).resolve()
        if not headless.is_file():
            raise RuntimeError(f"NeoST headless executable not found: {headless}")
        root = headless.parent.parent.parent
        return headless, root if (root / "usr/roms/etos192us.img").is_file() else None

    appimage = appimage.expanduser().resolve()
    if not appimage.is_file():
        raise RuntimeError(f"NeoST AppImage not found: {appimage}; pass --neost-headless")
    extraction = scratch / "neost-appimage"
    extraction.mkdir()
    result = run([str(appimage), "--appimage-extract"], cwd=extraction, timeout=120)
    if result.returncode:
        raise RuntimeError(f"NeoST AppImage extraction failed:\n{result.stdout}")
    root = extraction / "squashfs-root"
    headless = root / "usr/bin/neost-headless"
    if not headless.is_file():
        raise RuntimeError(f"AppImage has no headless executable: {headless}")
    return headless, root


def resolve_rom(explicit: Path | None, app_root: Path | None) -> Path:
    rom = explicit if explicit is not None else (
        app_root / "usr/roms/etos192us.img" if app_root else None)
    if rom is None or not rom.expanduser().is_file():
        raise RuntimeError("TOS ROM unavailable; pass --rom or use the NeoST AppImage")
    return rom.expanduser().resolve()
