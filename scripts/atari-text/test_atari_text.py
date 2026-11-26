import subprocess
import sys
from pathlib import Path
import tempfile
import unittest

import atari_text


class AtariTextTests(unittest.TestCase):
    def test_all_bytes_round_trip_without_changing_line_endings(self):
        data = bytes(range(256)) + b"\r\n\n\r"
        self.assertEqual(atari_text.convert(atari_text.convert(data, "decode"), "encode"), data)

    def test_atari_specific_characters(self):
        text = "ßœŒĳĲσφ•"
        data = bytes.fromhex("9e b4 b5 c0 c1 e5 ed f9")
        self.assertEqual(atari_text.convert(text.encode("utf-8"), "encode"), data)
        self.assertEqual(atari_text.convert(data, "decode"), text.encode("utf-8"))

    def test_c_literal_escapes_and_adjacent_digits(self):
        self.assertEqual(atari_text.convert('ß7œF"\\\r\n\0??/'.encode(), "c-literal"),
                         b'"\\2367\\264F\\042\\134\\015\\012\\000\\077\\077/"\n')

    def test_cli_rejects_bad_input_without_touching_output(self):
        script = Path(atari_text.__file__).resolve()
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "input"
            output = Path(directory) / "output"
            output.write_bytes(b"keep")
            for data in (b"\xff", "€".encode("utf-8")):
                source.write_bytes(data)
                result = subprocess.run([sys.executable, str(script), "encode", str(source),
                                         "-o", str(output)], capture_output=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(output.read_bytes(), b"keep")
                self.assertEqual(source.read_bytes(), data)
            source.write_bytes(b"unchanged")
            alias = Path(directory) / "alias"
            alias.hardlink_to(source)
            for destination in (source, alias):
                result = subprocess.run([sys.executable, str(script), "decode", str(source),
                                         "-o", str(destination)], capture_output=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(source.read_bytes(), b"unchanged")


if __name__ == "__main__":
    unittest.main()
