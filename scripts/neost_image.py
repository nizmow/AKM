"""Clone and populate disposable Atari/MagiC disk images.

This module owns the host-side image operations shared by NeoST scenarios.  It
does not know about particular applications: callers provide the guest paths
and the bundle contents.
"""

from __future__ import annotations

from collections.abc import Iterable, Mapping
import shutil
from pathlib import Path

from neost_common import run


class MagiCImage:
    """A writable image with an mtools partition selector."""

    def __init__(self, path: Path, partition_offset: int = 1024):
        if partition_offset < 0:
            raise ValueError("partition_offset must be non-negative")
        self.path = Path(path)
        self.partition_offset = partition_offset

    @property
    def image_spec(self) -> str:
        """Path in the ``image@@offset`` form accepted by mtools."""
        return f"{self.path}@@{self.partition_offset}"

    @classmethod
    def clone(cls, source: Path, destination: Path, *,
              partition_offset: int = 1024, timeout: int = 120) -> "MagiCImage":
        """Copy an image, preferring a reflink and falling back to a full copy."""
        source = Path(source)
        destination = Path(destination)
        if not source.is_file():
            raise RuntimeError(f"source image not found: {source}")
        destination.parent.mkdir(parents=True, exist_ok=True)
        result = run(["cp", "--reflink=auto", str(source), str(destination)],
                     timeout=timeout)
        if result.returncode:
            destination.unlink(missing_ok=True)
            try:
                shutil.copy2(source, destination)
            except OSError as error:
                raise RuntimeError(
                    f"could not clone {source}: {result.stdout.strip() or error}"
                ) from error
        return cls(destination, partition_offset)

    def verify_magic(self, guest: str = "::/MAGX.INF",
                     timeout: int = 30) -> None:
        """Require the selected partition to contain a MagiC MAGX.INF."""
        result = run(["mdir", "-i", self.image_spec, guest], timeout=timeout)
        if result.returncode:
            raise RuntimeError(
                f"{self.path} does not contain {guest}: {result.stdout}"
            )

    def mkdir(self, guest: str, timeout: int = 30) -> None:
        """Create a guest directory; an already-existing directory is fine."""
        result = run(["mmd", "-i", self.image_spec, guest], timeout=timeout)
        if result.returncode and "exists" not in result.stdout.lower():
            raise RuntimeError(f"could not create {guest}: {result.stdout}")

    def exists(self, guest: str, timeout: int = 30) -> bool:
        """Test a literal guest path without treating media errors as absence."""
        result = run(["env", "LC_ALL=C", "mdir", "-i", self.image_spec, guest],
                     timeout=timeout)
        if result.returncode == 0:
            return True
        if result.returncode == 1 and result.stdout.strip() == f'mdir: File "{guest}" not found':
            return False
        raise RuntimeError(f"could not inspect {guest}: {result.stdout}")

    def remove(self, guest: str, *, tree: bool = False) -> None:
        """Ensure a guest path is absent before installing or running a case."""
        if not self.exists(guest):
            return
        result = run(["mdeltree" if tree else "mdel", "-i", self.image_spec, guest],
                     timeout=60)
        if result.returncode or self.exists(guest):
            raise RuntimeError(f"could not remove {guest}: {result.stdout}")

    def copy_in(self, local: Path, guest: str, timeout: int = 60) -> None:
        """Copy one host file into the image."""
        local = Path(local)
        if not local.is_file():
            raise RuntimeError(f"missing build input: {local}")
        result = run(["mcopy", "-o", "-i", self.image_spec, str(local), guest],
                     timeout=timeout)
        if result.returncode:
            raise RuntimeError(
                f"could not install {local.name} as {guest}: {result.stdout}"
            )

    def copy_out(self, guest: str, local: Path, timeout: int = 60) -> None:
        """Copy one guest file out of the image."""
        local = Path(local)
        local.parent.mkdir(parents=True, exist_ok=True)
        result = run(["mcopy", "-o", "-i", self.image_spec, guest, str(local)],
                     timeout=timeout)
        if result.returncode:
            raise RuntimeError(
                f"could not extract {guest} to {local}: {result.stdout}"
            )

    def install(self, files: Iterable[tuple[Path, str]] |
                Mapping[Path, str]) -> None:
        """Copy an ordered bundle of host files to guest paths."""
        entries = files.items() if isinstance(files, Mapping) else files
        for local, guest in entries:
            self.copy_in(local, guest)
