"""Regression checks for artifact safety and subprocess failure diagnostics."""

from pathlib import Path
import sys
import tempfile
import unittest

from neost_common import create_output, resolve_rom, run_logged


class NeoSTCommonTests(unittest.TestCase):
    def test_existing_output_is_never_reused(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            output = create_output(root / "guest", parent=root, prefix="smoke")
            marker = output / "important"
            marker.write_bytes(b"keep")
            with self.assertRaises(FileExistsError):
                create_output(output, parent=root, prefix="smoke")
            alias = root / "alias"
            alias.symlink_to(output, target_is_directory=True)
            with self.assertRaises(FileExistsError):
                create_output(alias, parent=root, prefix="smoke")
            self.assertEqual(marker.read_bytes(), b"keep")

    def test_explicit_rom_takes_precedence_and_missing_rom_is_an_error(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            bundled = root / "usr/roms/etos192us.img"
            bundled.parent.mkdir(parents=True)
            bundled.write_bytes(b"bundled")
            explicit = root / "tos.img"
            explicit.write_bytes(b"explicit")
            self.assertEqual(resolve_rom(explicit, root), explicit.resolve())
            self.assertEqual(resolve_rom(None, root), bundled.resolve())
            with self.assertRaises(RuntimeError):
                resolve_rom(root / "missing", root)
            with self.assertRaises(RuntimeError):
                resolve_rom(None, None)

    def test_nonzero_exit_preserves_both_output_streams(self):
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "host.log"
            command = [sys.executable, "-c",
                       "import sys; print('out', flush=True); print('err', file=sys.stderr); sys.exit(7)"]
            with self.assertRaisesRegex(RuntimeError, "exited 7") as raised:
                run_logged(command, log)
            self.assertIn(str(log), str(raised.exception))
            self.assertEqual(log.read_text(), "out\nerr\n")

    def test_timeout_still_leaves_a_diagnostic_log(self):
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "host.log"
            command = [sys.executable, "-c", "import time; print('progress', flush=True); time.sleep(60)"]
            with self.assertRaisesRegex(RuntimeError, "timed out") as raised:
                run_logged(command, log, timeout=1)
            self.assertIn(str(log), str(raised.exception))
            self.assertIn("progress", log.read_text())


if __name__ == "__main__":
    unittest.main()
