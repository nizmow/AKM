"""Regression checks for disposable image setup helpers."""

from pathlib import Path
from types import SimpleNamespace
from unittest import mock
import tempfile
import unittest

from neost_image import MagiCImage


class MagiCImageTests(unittest.TestCase):
    def test_clone_falls_back_when_reflink_is_unsupported(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source.img"
            destination = root / "clone.img"
            source.write_bytes(b"disk")
            failure = SimpleNamespace(returncode=1, stdout="reflink unsupported")
            with mock.patch("neost_image.run", return_value=failure):
                image = MagiCImage.clone(source, destination)
            self.assertEqual(image.path, destination)
            self.assertEqual(destination.read_bytes(), b"disk")

    def test_copy_in_uses_partition_selector_and_guest_path(self):
        with tempfile.TemporaryDirectory() as directory:
            local = Path(directory) / "APP.PRG"
            local.write_bytes(b"binary")
            success = SimpleNamespace(returncode=0, stdout="")
            with mock.patch("neost_image.run", return_value=success) as invoke:
                MagiCImage(Path("disk.img"), 1024).copy_in(
                    local, "::/GEMSYS/START/APP.PRG")
            command = invoke.call_args.args[0]
            self.assertIn("disk.img@@1024", command)
            self.assertIn(str(local), command)
            self.assertIn("::/GEMSYS/START/APP.PRG", command)
            invoke.assert_called_once()

    def test_mkdir_accepts_existing_directory(self):
        existing = SimpleNamespace(returncode=1, stdout="MMD: directory exists")
        with mock.patch("neost_image.run", return_value=existing):
            MagiCImage(Path("disk.img")).mkdir("::/SEARCHFIX")

    def test_missing_local_file_is_rejected_before_mcopy(self):
        with self.assertRaisesRegex(RuntimeError, "missing build input"):
            MagiCImage(Path("disk.img")).copy_in(
                Path("missing.prg"), "::/APP.PRG")


if __name__ == "__main__":
    unittest.main()
