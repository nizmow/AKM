#!/usr/bin/env python3
"""Verify MagiC's English UK locale: language, identity, fallback and overrides.

Each case boots a rebuilt kernel on a disposable MagiC image with a probe in
`GEMSYS/MAGIC/START`.  The probe uses only public interfaces (`appl_getinfo`,
XBIOS `Keytbl`/`Bioskeys`, cookies) plus the BIOS console, and writes its
results to the guest `E:` host directory.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile

from neost_common import acquire_headless, create_output, resolve_rom, run
from neost_image import MagiCImage
from neost_server import NeoSTServer

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
LOCALE_BUILD = BUILD / "locale"
KEYMAPS = ROOT / "base/keyb-sys/keymaps.json"
GERMAN_KERNEL = LOCALE_BUILD / "de/MAGIC.RAM"
PROBE_GUEST = "::/GEMSYS/MAGIC/START/LOCALE.PRG"
KEYTABLS_GUEST = "::/GEMSYS/MAGIC/XTENSION/KEYTABLS.SYS"
TABLE_BYTES = 128
TABLES = 9
# Scancodes: A, shift and caps lock, then digits 3 and 4.  The expected
# console bytes are pinned per layout, independent of keymaps.json:
# shift+3 is the representative difference (UK 0x9C, German 0xDD).
SCAN_KEYS = (0x1E, 0x04, 0x05)
UK_KEYS = (0x61, 0x41, 0x41, 0x33, 0x9C, 0x34, 0x24)
GERMAN_KEYS = (0x61, 0x41, 0x41, 0x33, 0xDD, 0x34, 0x24)


def keymaps() -> dict:
    data = json.loads(KEYMAPS.read_text(encoding="utf-8"))
    if data.get("format") != 1 or "UK" not in data.get("layouts", {}):
        raise SystemExit(f"{KEYMAPS}: unexpected keymap data")
    return data


def layout_tables(data: dict, name: str) -> bytes:
    layout = data["layouts"][name]
    order = data["write_order"]
    return b"".join(bytes(layout["tables"][table]) for table in order)


def expected_keys(tables: bytes) -> tuple[int, ...]:
    t0 = tables[0:TABLE_BYTES]
    t1 = tables[TABLE_BYTES:2 * TABLE_BYTES]
    t2 = tables[2 * TABLE_BYTES:3 * TABLE_BYTES]
    return (t0[0x1E], t1[0x1E], t2[0x1E], t0[0x04], t1[0x04], t0[0x05], t1[0x05])


def build(*, skip: bool) -> Path:
    kernel = LOCALE_BUILD / "MAGIC.RAM"
    german = GERMAN_KERNEL
    probe = LOCALE_BUILD / "apps/locale/LOCALE.PRG"
    if skip:
        for path in (kernel, german, probe):
            if not path.is_file():
                raise SystemExit(f"missing build output: {path}")
        return probe
    container = str(ROOT / "scripts/atari-container")
    for language, target in (("en", kernel), ("de", german)):
        relative = target.relative_to(ROOT)
        result = run([container, "run", "make", "-j4",
                      f"BUILD_DIR={relative.parent}", f"KERNEL_LANG={language}",
                      str(relative)], cwd=ROOT, timeout=1200)
        if result.returncode:
            raise SystemExit(result.stdout or "kernel build failed")
    result = run([container, "run", "make", "-j4", "-f", "Makefile.apps",
                  f"BUILD_DIR={(LOCALE_BUILD / 'apps').relative_to(ROOT)}",
                  "locale-probe"],
                 cwd=ROOT, timeout=600)
    if result.returncode:
        raise SystemExit(result.stdout or "probe build failed")
    return probe


def run_case(headless: Path, rom: Path, source: Path, probe: Path,
             folder: Path, *, kernel: Path, override: bytes | None,
             offset: int) -> dict[str, bytes]:
    image = MagiCImage.clone(source, folder / "magic.img", partition_offset=offset)
    image.verify_magic()
    image.install([(kernel, "::/MAGIC.RAM"), (probe, PROBE_GUEST)])
    image.remove(KEYTABLS_GUEST)
    if override is not None:
        local = folder / "KEYTABLS.SYS"
        local.write_bytes(override)
        image.copy_in(local, KEYTABLS_GUEST)
    guest = folder / "GEMDRIVE"
    (guest / "E").mkdir(parents=True)
    options = ["--machine", "megaste", "--mem", "4m", "--mono", "--no-disk",
               "--acsi", str(image.path), "--gemdos", str(guest),
               "--auto", r"C:\AUTO\MAGXBOOT.TOS"]
    (folder / "server").mkdir()
    results = guest / "E"
    with NeoSTServer(headless, rom, folder / "server", options=options,
                     timeout=240) as server:
        # Wait for the START probe to publish its report before injecting keys;
        # input sent during the TOS/MagiC boot is discarded by the console.
        for _ in range(40):
            server.run_frames(250)
            if (results / "LOCALE.TXT").is_file():
                break
        else:
            raise RuntimeError(f"{folder.name}: probe never reported readiness")
        server.press_key(SCAN_KEYS[0], frames=5)
        server.chord((0x2A, SCAN_KEYS[0]), frames=5)
        server.press_key(0x3A, frames=5)
        server.press_key(SCAN_KEYS[0], frames=5)
        server.press_key(0x3A, frames=5)
        server.press_key(SCAN_KEYS[1], frames=5)
        server.chord((0x2A, SCAN_KEYS[1]), frames=5)
        server.press_key(SCAN_KEYS[2], frames=5)
        server.chord((0x2A, SCAN_KEYS[2]), frames=5)
        server.screenshot(folder / "final.ppm")
        for _ in range(16):
            server.run_frames(500)
            report_file = results / "KEYS.TXT"
            if report_file.is_file() and report_file.read_bytes().endswith(b"DONE\n"):
                break
    return {name: (results / name).read_bytes() for name in
            ("BOOTKEYS.BIN", "RESETKEYS.BIN", "LOCALE.TXT", "KEYS.TXT")
            if (results / name).is_file()}


def report(raw: bytes) -> dict[str, str]:
    fields = {}
    for line in raw.decode("ascii", "replace").splitlines():
        key, _, value = line.partition("=")
        fields[key] = value
    return fields


def check(label: str, folder: Path, files: dict[str, bytes], *,
          tables: bytes, keys: tuple[int, ...], idt: int, language: int) -> None:
    missing = [name for name in ("BOOTKEYS.BIN", "RESETKEYS.BIN", "LOCALE.TXT", "KEYS.TXT")
               if name not in files]
    if missing:
        raise SystemExit(f"{label}: probe produced no {', '.join(missing)}")
    boot = files["BOOTKEYS.BIN"]
    reset = files["RESETKEYS.BIN"]
    fields = report(files["LOCALE.TXT"])
    key_lines = files["KEYS.TXT"].decode("ascii").split()
    actual_keys = tuple(int(line, 16) for line in key_lines[:-1])
    if key_lines[-1:] != ["DONE"] or len(actual_keys) != 7:
        raise SystemExit(f"{label}: incomplete key report {key_lines!r}")
    expected_size = TABLES * TABLE_BYTES
    if len(boot) != expected_size:
        raise SystemExit(f"{label}: boot tables are {len(boot)} bytes")
    if boot != tables:
        (folder / "expected-tables.bin").write_bytes(tables)
        raise SystemExit(f"{label}: fallback/override tables differ from the contract")
    if reset != boot:
        raise SystemExit(f"{label}: Bioskeys did not restore the effective default")
    if actual_keys != keys:
        raise SystemExit(f"{label}: translated keys {actual_keys!r} != {keys!r}")
    expected = {"language": str(language), "idt_present": "1", "idt": str(idt),
                "akp_present": "0", "keytbl": "1", "keytbl_query": "1",
                "bioskeys": "1", "ready": "1"}
    for name, value in expected.items():
        if fields.get(name) != value:
            raise SystemExit(f"{label}: {name}={fields.get(name)!r}, expected {value!r}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--magic-image", type=Path,
                        default=Path(os.environ.get("MAGIC_IMAGE", "")))
    parser.add_argument("--tos", type=Path,
                        default=Path(os.environ.get("TOS206_ROM", "")))
    parser.add_argument("--neost", type=Path, default=Path.home() / ".local/bin/neost")
    parser.add_argument("--neost-headless", type=Path)
    parser.add_argument("--partition-offset", type=int, default=1024)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--skip-build", action="store_true")
    args = parser.parse_args()
    source, rom = (path.expanduser().resolve() for path in
                   (args.magic_image, args.tos))
    if not source.is_file() or not rom.is_file():
        parser.error("supply an existing --magic-image and --tos "
                     "(or MAGIC_IMAGE/TOS206_ROM)")
    probe = build(skip=args.skip_build)
    kernel = LOCALE_BUILD / "MAGIC.RAM"
    data = keymaps()
    uk = layout_tables(data, "UK")
    # Nine distinct slots, exactly the file length READ_KTB accepts; the short
    # case is the same payload truncated just below that length, so a loader
    # that applies a partial read cannot masquerade as the fallback.
    override = b"".join(bytes([0x41 + index]) * TABLE_BYTES
                        for index in range(TABLES))
    cases = (
        ("uk-fallback", kernel, None, uk, UK_KEYS, 0x112D, 0),
        ("uk-override", kernel, override, override, expected_keys(override), 0x112D, 0),
        ("uk-short", kernel, override[:-1], uk, UK_KEYS, 0x112D, 0),
        ("de-fallback", GERMAN_KERNEL, None, None, GERMAN_KEYS, 0x112E, 1),
    )
    output = create_output(args.output, parent=BUILD, prefix="neost-locale")
    print(f"Evidence: {output}", flush=True)
    with tempfile.TemporaryDirectory(prefix="neost-locale-") as temporary:
        headless, _ = acquire_headless(args.neost_headless, args.neost, Path(temporary))
        for label, kernel_path, override_bytes, tables, keys, idt, language in cases:
            folder = output / label
            folder.mkdir(parents=True)
            files = run_case(headless, rom, source, probe, folder,
                             kernel=kernel_path or kernel,
                             override=override_bytes, offset=args.partition_offset)
            if tables is None:
                # German keeps its historical embedded fallback; assert identity
                # and translation instead of comparing it with the KEYB_DEI data.
                boot = files.get("BOOTKEYS.BIN", b"")
                if len(boot) != TABLES * TABLE_BYTES:
                    raise SystemExit(f"{label}: boot tables are {len(boot)} bytes")
                if files.get("RESETKEYS.BIN") != boot:
                    raise SystemExit(f"{label}: Bioskeys did not restore the default")
                fields = report(files.get("LOCALE.TXT", b""))
                key_lines = files.get("KEYS.TXT", b"").decode("ascii").split()
                actual = tuple(int(line, 16) for line in key_lines[:-1])
                if key_lines[-1:] != ["DONE"] or len(actual) != 7:
                    raise SystemExit(f"{label}: incomplete key report {key_lines!r}")
                if actual != keys:
                    raise SystemExit(f"{label}: translated keys {actual!r} != {keys!r}")
                for name, value in (("language", "1"), ("idt_present", "1"),
                                    ("idt", str(idt)), ("akp_present", "0"),
                                    ("keytbl", "1"), ("keytbl_query", "1"),
                                    ("bioskeys", "1")):
                    if fields.get(name) != value:
                        raise SystemExit(f"{label}: {name}={fields.get(name)!r}")
            else:
                check(label, folder, files, tables=tables, keys=keys,
                      idt=idt, language=language)
            print(f"{label}: PASS", flush=True)
    (output / "comparison.txt").write_text("PASS\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
