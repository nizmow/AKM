"""Execute the real CPX bridge assembly against native-ABI test callbacks."""

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BRIDGE = ROOT / "MagiC/APPS/CPX/cpx_entry.S"
FIXTURE = ROOT / "scripts/testdata/CPX_ABI.S"
FAILURES = {
    1: "xform callback arguments or word widths",
    2: "xform D2 preservation",
    3: "xform A2 preservation",
    5: "obfix D2 preservation",
    6: "obfix A2 preservation",
    15: "bridge stack balance or guard",
    16: "remaining GCC callee-saved registers",
    7: "alert callback argument or short return",
    8: "alert D2 preservation",
    9: "alert A2 preservation",
    10: "call2 arguments or full-width return",
    11: "call2 D2 preservation",
    12: "call2 A2 preservation",
    13: "zero-argument call2 filler/return",
    14: "one-argument call2/return",
    17: "scheduler callback argument/return",
    18: "scheduler D2 preservation",
    19: "scheduler A2 preservation",
    21: "native xform callback received incorrect arguments",
    22: "native obfix callback received incorrect arguments",
    23: "native alert callback received incorrect argument",
    24: "native call2 callback received incorrect arguments",
    25: "native zero-argument callback received incorrect filler",
    26: "native one-argument callback received incorrect argument",
    27: "native scheduler callback did not receive D0 settings",
    28: "packed slider-arrow arguments or horizontal direction",
    29: "slider-arrow D2 preservation",
    30: "slider-arrow A2 preservation",
}


class CpxBridgeAbiTest(unittest.TestCase):
    def test_real_bridges_on_linux_m68k(self):
        qemu = shutil.which("qemu-m68k")
        self.assertIsNotNone(qemu, "required tool qemu-m68k is not installed")
        wrapper = ROOT / "scripts/atari-container"
        self.assertTrue(wrapper.is_file(), f"missing assembler wrapper: {wrapper}")

        build_root = ROOT / "build/atari"
        build_root.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="cpx-abi-", dir=build_root) as temporary:
            directory = Path(temporary)
            bridge_obj = directory / "cpx_entry.o"
            fixture_obj = directory / "CPX_ABI.o"
            executable = directory / "cpx-abi.elf"
            for source, output in ((BRIDGE, bridge_obj), (FIXTURE, fixture_obj)):
                command = [str(wrapper), "run", "vasmm68k_mot", "-Felf", "-m68000",
                           "-spaces", "-o", str(output.relative_to(ROOT)),
                           str(source.relative_to(ROOT))]
                result = subprocess.run(command, cwd=ROOT, text=True,
                                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                         timeout=60)
                self.assertEqual(result.returncode, 0,
                                 f"assembling {source.name} failed:\n{result.stdout}")

            command = [str(wrapper), "run", "m68k-atari-mintelf-ld",
                       "-m", "m68kelf", "-Ttext=0x10000", "-e", "_start",
                       "-o", str(executable.relative_to(ROOT)),
                       str(fixture_obj.relative_to(ROOT)), str(bridge_obj.relative_to(ROOT))]
            result = subprocess.run(command, cwd=ROOT, text=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    timeout=60)
            self.assertEqual(result.returncode, 0, f"linking CPX fixture failed:\n{result.stdout}")

            result = subprocess.run([qemu, str(executable)], cwd=ROOT, text=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    timeout=10)
            if result.returncode:
                self.fail(f"CPX bridge ABI case {result.returncode}: "
                          f"{FAILURES.get(result.returncode, 'unexpected fixture failure')}\n"
                          f"qemu output:\n{result.stdout}")


if __name__ == "__main__":
    unittest.main()
