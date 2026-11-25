"""Real-process regression tests for the NeoST headless server client."""

from __future__ import annotations

from pathlib import Path
import shutil
import tempfile
import time
import unittest


from neost_common import create_output, resolve_rom
from neost_server import NeoSTError, NeoSTServer, NeoSTTransportError


ROOT = Path(__file__).resolve().parent.parent


def installed_neost() -> tuple[Path, Path]:
    executable = shutil.which("neost-headless")
    if not executable:
        raise RuntimeError("NeoST headless executable 'neost-headless' is required")
    binary = Path(executable).resolve()
    app_root = binary.parent.parent.parent
    try:
        rom = resolve_rom(None, app_root)
    except RuntimeError as error:
        raise RuntimeError("bundled EmuTOS ROM is required for NeoST tests") from error
    return binary, rom


class NeoSTServerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.executable, cls.rom = installed_neost()

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="neost-server-test-")
        self.root = Path(self.temp.name)
        self.output = create_output(None, parent=self.root, prefix="artifacts")
        self.server = None

    def tearDown(self):
        if self.server is not None and not self.server.closed:
            self.server.close()
        self.temp.cleanup()

    def start(self, **kwargs):
        kwargs.setdefault("timeout", 10.0)
        self.server = NeoSTServer(self.executable, self.rom, self.output, **kwargs)
        self.assertTrue(self.server.identity)
        self.assertNotIn("\n", self.server.identity)
        return self.server

    def test_protocol_operations_and_screenshot(self):
        server = self.start(timeout=60.0)
        first = server.observe()
        self.assertEqual(first.frame, 0)
        self.assertEqual(len(first.screen), 16)
        self.assertEqual(server.play(".*1200").frame, 1200)
        self.assertEqual(server.joy(0), None)
        server.key_make(0x1E)
        server.key_break(0x1E)
        server.press_key(0x1E, frames=1)
        server.chord((0x1D, 0x38), frames=1)
        server.mouse(1, -1, 0)
        self.assertEqual(len(server.peek(0, 16)), 16)
        server.probe("test", 0, 4)
        server.slots()
        image = self.root / "screen capture.ppm"
        server.screenshot(image)
        data = image.read_bytes()
        self.assertTrue(data.startswith(b"P6\n"))
        header, pixels = data.split(b"\n255\n", 1)
        self.assertRegex(header.decode("ascii"), r"^P6\n\d+ \d+$")
        dimensions = header.splitlines()[1].split()
        width, height = map(int, dimensions)
        self.assertGreater(width, 0)
        self.assertGreater(height, 0)
        self.assertEqual(len(pixels), width * height * 3)
        self.assertTrue(any(pixels))

    def test_mouse_move_and_click(self):
        server = self.start()
        server.move(16, -8, step=8, frames=1)
        clicked = server.click(1, frames=1)
        self.assertEqual(clicked.frame, 4)
        with self.assertRaises(ValueError):
            server.move(8, 0, step=0)
        with self.assertRaises(ValueError):
            server.click(0)

    def test_save_load_and_import_export_roundtrip(self):
        server = self.start()
        server.run_frames(3)
        server.save(1)
        saved = server.observe()
        saved_ram = server.peek(0, 16)
        server.run_frames(2)
        restored = server.load(1)
        self.assertEqual(restored.frame, saved.frame)
        self.assertEqual(restored.screen, saved.screen)
        self.assertEqual(server.peek(0, 16), saved_ram)
        state = self.output / "state.bin"
        server.export_state(1, state)
        self.assertTrue(state.is_file())
        server.import_state(2, state)
        imported = server.load(2)
        self.assertEqual(imported.frame, 0)
        self.assertEqual(imported.screen, saved.screen)
        self.assertEqual(server.peek(0, 16), saved_ram)
    def test_protocol_error_does_not_desynchronize_connection(self):
        server = self.start()
        with self.assertRaises(NeoSTError):
            server.insert("A", self.output / "missing.st")
        self.assertFalse(server.closed)
        self.assertEqual(server.run_frames(1).frame, 1)

    def test_timeout_is_bounded_reaps_process_and_preserves_logs(self):
        server = self.start()
        proc = server._proc
        started = time.monotonic()
        with self.assertRaises(NeoSTTransportError):
            server.run_frames(10_000_000, timeout=0.01)
        elapsed = time.monotonic() - started
        self.assertLess(elapsed, 5.0)
        self.assertTrue(server.closed)
        self.assertIsNotNone(proc.poll())
        self.assertTrue((self.output / "protocol.log").is_file())
        self.assertTrue((self.output / "diagnostics.log").is_file())
        with self.assertRaises(NeoSTTransportError):
            server.observe()

    def test_invalid_argument_is_rejected_before_protocol_side_effect(self):
        server = self.start()
        log = self.output / "protocol.log"
        before = log.read_bytes()
        with self.assertRaises(ValueError):
            server.insert("A", Path("bad\nname.st"))
        self.assertEqual(log.read_bytes(), before)
        self.assertFalse(server.closed)


if __name__ == "__main__":
    unittest.main()


