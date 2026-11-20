"""Regression checks for PPM parsing and screenshot comparison."""

from pathlib import Path
import tempfile
import unittest

from neost_screen import bbox_inside, diff_stats, read_ppm


def write_ppm(path: Path, pixels: bytes, width: int = 2, height: int = 2) -> None:
    path.write_bytes(b"P6\n%d %d\n255\n" % (width, height) + pixels)


class ScreenTests(unittest.TestCase):
    def test_read_ppm_returns_rgb_payload(self):
        with tempfile.TemporaryDirectory() as directory:
            image = Path(directory) / "frame.ppm"
            pixels = bytes(range(12))
            write_ppm(image, pixels)
            width, height, payload = read_ppm(image)
            self.assertEqual((width, height), (2, 2))
            self.assertEqual(payload, pixels)

    def test_diff_stats_reports_the_changed_pixel(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            first = root / "first.ppm"
            second = root / "second.ppm"
            write_ppm(first, b"\x00\x00\x00\xff\x00\x00"
                             b"\x00\xff\x00\xff\xff\xff")
            write_ppm(second, b"\x00\x00\x00\x00\x00\xff"
                              b"\x00\xff\x00\xff\xff\xff")
            count, bbox = diff_stats(first, second)
            self.assertEqual(count, 1)
            self.assertEqual(bbox, (1, 0, 1, 0))
            self.assertTrue(bbox_inside(bbox, (0, 0, 2, 2)))
            self.assertFalse(bbox_inside(bbox, (0, 0, 1, 2)))
            self.assertEqual(diff_stats(first, second, region=(0, 0, 1, 2))[0], 0)

    def test_truncated_payload_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            image = Path(directory) / "bad.ppm"
            write_ppm(image, b"\x00\x00\x00")
            with self.assertRaisesRegex(RuntimeError, "truncated"):
                read_ppm(image)


if __name__ == "__main__":
    unittest.main()
