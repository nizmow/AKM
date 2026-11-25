"""Regression checks for NeoSTHost lifecycle and coordinate tracking."""

from pathlib import Path
from unittest import mock
import tempfile
import unittest

from neost_host import NeoSTHost
from neost_image import MagiCImage


class FakeServer:
    instances: list["FakeServer"] = []

    def __init__(self, executable, rom, output, *, options=(), timeout=30.0):
        self.executable = executable
        self.rom = rom
        self.output = output
        self.options = list(options)
        self.timeout = timeout
        self.closed = False
        self.moves: list[tuple[int, int]] = []
        self.clicks: list[int] = []
        self.saved: list[int] = []
        self.loaded: list[int] = []
        self.frames = 0
        self.shots: list[Path] = []
        FakeServer.instances.append(self)

    def run_frames(self, frames):
        self.frames += frames
        return frames

    def screenshot(self, path):
        self.shots.append(path)

    def move(self, dx, dy, *, step=8, frames=1):
        self.moves.append((dx, dy))

    def click(self, button=1, *, frames=1):
        self.clicks.append(button)
        return frames

    def save(self, slot):
        self.saved.append(slot)

    def load(self, slot):
        self.loaded.append(slot)
        return slot

    def press_key(self, scancode, frames=1):
        return scancode

    def chord(self, scancodes, frames=1):
        return tuple(scancodes)

    def close(self):
        self.closed = True


class HostTests(unittest.TestCase):
    def setUp(self):
        FakeServer.instances.clear()
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.patcher = mock.patch("neost_host.NeoSTServer", FakeServer)
        self.patcher.start()

    def tearDown(self):
        self.patcher.stop()
        self.temp.cleanup()

    def host(self, **kwargs):
        return NeoSTHost(self.root / "neost-headless", self.root / "tos.img",
                         self.root / "artifacts", **kwargs)

    def test_start_builds_media_and_auto_options(self):
        image = MagiCImage(self.root / "magic.img", 1024)
        host = self.host(options=["--machine", "megaste"], image=image,
                         auto=r"C:\AUTO\MAGXBOOT.TOS")
        server = host.start()
        self.assertIs(server, host.start())
        self.assertEqual(server.options[:2], ["--machine", "megaste"])
        self.assertIn("--acsi", server.options)
        self.assertIn(str(image.path), server.options)
        self.assertIn("--gemdos", server.options)
        self.assertIn("--auto", server.options)

    def test_coordinate_tracking_and_context_manager(self):
        with self.host(options=["--machine", "st"]) as host:
            host.calibrate_mouse((640, 400))
            host.move_to(100, 50)
            host.click()
            host.screenshot("frame.ppm")
            host.save_state(2)
            host.load_state(2)
            server = host.server
        self.assertEqual(server.moves, [(-640, 400), (100, -349)])
        self.assertEqual(server.clicks, [1])
        self.assertEqual(server.saved, [2])
        self.assertEqual(server.loaded, [2])
        self.assertTrue(server.closed)
        self.assertEqual(server.shots, [self.root / "artifacts/frame.ppm"])

    def test_image_must_be_prepared_before_start(self):
        host = self.host()
        host.start()
        with self.assertRaisesRegex(RuntimeError, "before starting"):
            host.prepare_image(self.root / "source.img")

    def test_move_to_requires_calibration(self):
        host = self.host()
        host.start()
        with self.assertRaisesRegex(RuntimeError, "calibrate_mouse"):
            host.move_to(1, 1)

    def test_click_requires_both_coordinates(self):
        host = self.host()
        host.start()
        with self.assertRaisesRegex(ValueError, "both x and y"):
            host.click(10)


if __name__ == "__main__":
    unittest.main()
