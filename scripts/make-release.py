#!/usr/bin/env python3
"""Build and package a deterministic German Atari/ST installation ZIP."""
from __future__ import annotations

import argparse
import hashlib
import re
import sys
import subprocess
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
APPS = Path("apps")
RELEASE = ROOT / "release/atari"

REBUILT = {
    Path("MAGIC.RAM"): "MAGIC.RAM",
    APPS / "magxboot/MAGXBOOT.TOS": "AUTO/MAGXBOOT.TOS",
    APPS / "magxdesk/MAGXDESK.APP": "GEMSYS/GEMDESK/MAGXDESK.APP",
    APPS / "magxdesk/MAGXDESK.RSC": "GEMSYS/GEMDESK/MAGXDESK.RSC",
    APPS / "magxdesk/MAGXDESL.RSC": "GEMSYS/GEMDESK/MAGXDESL.RSC",
    APPS / "applicat/APPLICAT.APP": "GEMSYS/GEMDESK/APPLICAT.APP",
    APPS / "applicat/applicat.rsc": "GEMSYS/GEMDESK/APPLICAT.RSC",
    APPS / "mcmd/de/MCMD.TOS": "GEMSYS/GEMDESK/MCMD.TOS",
    APPS / "showfile/SHOWFILE.TTP": "GEMSYS/GEMDESK/SHOWFILE.TTP",
    APPS / "mgcopy/MGCOPY.APP": "GEMSYS/GEMDESK/MGCOPY.APP",
    APPS / "mgcopy/MGCOPY.RSC": "GEMSYS/GEMDESK/MGCOPY.RSC",
    APPS / "mgsearch/MGSEARCH.APP": "GEMSYS/GEMDESK/MGSEARCH.APP",
    APPS / "mgsearch/MGSEARCH.RSC": "GEMSYS/GEMDESK/MGSEARCH.RSC",
    APPS / "mgedit/MGEDIT.APP": "GEMSYS/GEMDESK/MGEDIT.APP",
    APPS / "mgedit/mgedit.rsc": "GEMSYS/GEMDESK/MGEDIT.RSC",
    APPS / "mgview/MGVIEW.APP": "GEMSYS/GEMDESK/MGVIEW.APP",
    APPS / "mgnotice/MGNOTICE.APP": "GEMSYS/GEMDESK/MGNOTICE.APP",
    APPS / "mgnotice/mgnotice.rsc": "GEMSYS/GEMDESK/MGNOTICE.RSC",
    APPS / "mgformat/MGFORMAT.PRG": "GEMSYS/GEMDESK/MGFORMAT.PRG",
    APPS / "mgformat/MGFORMAT.RSC": "GEMSYS/GEMDESK/MGFORMAT.RSC",
    APPS / "vt52/VT52.PRG": "GEMSYS/GEMDESK/VT52.PRG",
    APPS / "vt52/VT52.RSC": "GEMSYS/GEMDESK/VT52.RSC",
    APPS / "vfatconf/VFATCONF.PRG": "GEMSYS/GEMDESK/VFATCONF.PRG",
    APPS / "vfatconf/vfatconf.rsc": "GEMSYS/GEMDESK/VFATCONF.RSC",
    APPS / "shutdown/SHUTDOWN.PRG": "GEMSYS/GEMDESK/SHUTDOWN.PRG",
    APPS / "wbdaemon/WBDAEMON.PRG": "GEMSYS/MAGIC/START/WBDAEMON.PRG",
    APPS / "mod-app/MOD_APP.TTP": "GEMSYS/GEMDESK/MOD_APP.TTP",
    APPS / "cops/COPS.APP": "AUTO/ACCS/COPS.ACC",
    APPS / "cpx/MAGXCONF.CPX": "AUTO/ACCS/CPX/MAGXCONF.CPX",
    APPS / "cpx/TSLICE.CPX": "AUTO/ACCS/CPX/TSLICE.CPX",
    APPS / "winframe/WINFRAME.SLB": "GEMSYS/MAGIC/XTENSION/WINFRAME.SLB",
    APPS / "winframe/WINFRAME.RSC": "GEMSYS/MAGIC/XTENSION/WINFRAME.RSC",
    APPS / "load-img/LOAD_IMG.SLB": "GEMSYS/MAGIC/XTENSION/LOAD_IMG.SLB",
    APPS / "mgedit/EDITOBJC.SLB": "GEMSYS/MAGIC/XTENSION/EDITOBJC.SLB",
    APPS / "screen/MFA2.SYS": "GEMSYS/MFA2.SYS",
    APPS / "screen/MFA4.SYS": "GEMSYS/MFA4.SYS",
    APPS / "screen/MFA16.SYS": "GEMSYS/MFA16.SYS",
    APPS / "screen/MFA256.SYS": "GEMSYS/MFA256.SYS",
    APPS / "screen/MFA32K.SYS": "GEMSYS/MFA32K.SYS",
    APPS / "osd/OFF002.OSD": "GEMSYS/OFF002.OSD",
    APPS / "osd/OFF004IP.OSD": "GEMSYS/OFF004IP.OSD",
    APPS / "osd/OFF016IP.OSD": "GEMSYS/OFF016IP.OSD",
    APPS / "osd/OFF016PK.OSD": "GEMSYS/OFF016PK.OSD",
    APPS / "osd/OFF16MOV.OSD": "GEMSYS/OFF16MOV.OSD",
    APPS / "osd/OFF256IP.OSD": "GEMSYS/OFF256IP.OSD",
    APPS / "osd/OFF256PK.OSD": "GEMSYS/OFF256PK.OSD",
    APPS / "osd/OFF32KFL.OSD": "GEMSYS/OFF32KFL.OSD",
    APPS / "osd/OFF32KPK.OSD": "GEMSYS/OFF32KPK.OSD",
    APPS / "osd/OFF65KPK.OSD": "GEMSYS/OFF65KPK.OSD",
}

MINIMAL_FILES = {
    "MAGIC.RAM", "AUTO/MAGXBOOT.TOS", "GEMSYS/GEMDESK/MCMD.TOS",
    "GEMSYS/GEMDESK/MAGXDESK.APP", "GEMSYS/GEMDESK/MAGXDESK.RSC",
    "GEMSYS/GEMDESK/MAGXDESL.RSC", "GEMSYS/MFA2.SYS", "GEMSYS/OFF002.OSD",
}

RETAINED = [
    "MAGX.INF", "AUTOEXEC.BAT", "INSTALL.TXT",
    "GEMSYS/GEMDESK/APPLICAT.DAT", "GEMSYS/GEMDESK/APPLICAT.INF",
    "GEMSYS/GEMDESK/SHUTDOWN.INF", "GEMSYS/GEMDESK/MGXCLOCK.TXT",
    "GEMSYS/GEMDESK/HELP/**", "GEMSYS/GEMDESK/PAT/**", "GEMSYS/GEMDESK/RSC/**",
    "GEMSYS/GEMSCRAP/**", "GEMSYS/HOME/COPS.INF",
    "LANG/DE/GEMSYS/GEMDESK/*.RSC", "LANG/DE/GEMSYS/GEMDESK/HELP/*.HLP",
    "LANG/DE/GEMSYS/GEMDESK/MGNOTICE.TXT", "LANG/DE/GEMSYS/GEMDESK/MOD_APP.TXT",
    "LANG/DE/GEMSYS/GEMDESK/VFATCONF.RSC", "LANG/DE/GEMSYS/GEMDESK/VT52.RSC",
    "LANG/DE/GEMSYS/MAGIC/XTENSION/KEYTABLS.SYS",
    "AUTO/ACCS/CPX/GENERAL.CPX", "AUTO/ACCS/CPX/CLOCK.CPX",
    "AUTO/ACCS/CPX/COLOR.CPX", "AUTO/ACCS/CPX/CONFIG.CPX",
    "AUTO/ACCS/CPX/MODEM.CPX", "AUTO/ACCS/CPX/PRINTER.CPX",
    "AUTO/ACCS/CPX/SET_FLG.CPX", "AUTO/ACCS/CPX/SI.CPX",
    "AUTO/ACCS/NORMAL.PAL", "AUTO/ACCS/DUNKEL.PAL", "AUTO/ACCS/ATARIORG.PAL",
]


def expand(patterns: list[str]) -> list[Path]:
    found: list[Path] = []
    for pattern in patterns:
        matches = sorted(RELEASE.glob(pattern))
        if not matches:
            raise SystemExit(f"retained input missing: {pattern}")
        found.extend(path for path in matches if path.is_file())
    return found


def sanitized_inf(path: Path) -> bytes:
    data = path.read_bytes()
    if b"PDLG.SLB" in data:
        data = re.sub(rb"(?im)^[ \t]*#_SLB[ \t]+\d+[ \t]+PDLG\.SLB[ \t]*\r?\n", b"", data)
    return data


def build(build_dir: Path, rebuilt: dict[Path, str]) -> None:
    make = [str(ROOT / "scripts/atari-container"), "run", "make", "-j4"]
    subprocess.run([*make, f"BUILD_DIR={build_dir}",
                    str(build_dir / "MAGIC.RAM")], cwd=ROOT, check=True)
    subprocess.run([*make, "-f", "Makefile.apps", f"BUILD_DIR={build_dir / APPS}",
                    *(str(build_dir / path) for path in rebuilt if path.parts[0] == "apps")],
                   cwd=ROOT, check=True)


def collect(build_dir: Path, rebuilt: dict[Path, str], *,
            minimal: bool) -> dict[str, tuple[bytes, str]]:
    entries: dict[str, tuple[bytes, str]] = {}
    for relative, guest in rebuilt.items():
        local = ROOT / build_dir / relative
        if not local.is_file():
            raise SystemExit(f"rebuilt input missing: {local}")
        data = local.read_bytes()
        if not data:
            raise SystemExit(f"rebuilt input empty: {local}")
        entries[guest] = (data, f"rebuilt {local.relative_to(ROOT)}")
    if minimal:
        entries["MAGX.INF"] = ((ROOT / "core/MAGX.INF").read_bytes(), "repository core/MAGX.INF")
    else:
        for local in expand(RETAINED):
            guest = local.relative_to(RELEASE).as_posix()
            if guest in entries:
                continue
            data = sanitized_inf(local) if guest == "MAGX.INF" else local.read_bytes()
            entries[guest] = (data, f"retained MAGIC.A/{guest}")
    entries["LICENSE"] = ((ROOT / "LICENSE").read_bytes(), "repository GPLv3 text")
    return entries


def write_zip(entries: dict[str, tuple[bytes, str]], destination: Path) -> None:
    with zipfile.ZipFile(destination, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name in sorted(entries):
            data, _ = entries[name]
            info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--profile", choices=("core", "minimal"), default="core")
    parser.add_argument("--build-dir", type=Path, default=Path("build/atari"),
                        help="build directory inside the repository")
    parser.add_argument("--skip-build", action="store_true",
                        help="pack already-built inputs without invoking Make")
    args = parser.parse_args()
    try:
        build_dir = (ROOT / args.build_dir).resolve().relative_to(ROOT)
    except ValueError:
        parser.error("--build-dir must be inside the repository")
    minimal = args.profile == "minimal"
    rebuilt = {path: guest for path, guest in REBUILT.items()
               if not minimal or guest in MINIMAL_FILES}
    if not args.skip_build:
        try:
            build(build_dir, rebuilt)
        except subprocess.CalledProcessError as error:
            return error.returncode
    entries = collect(build_dir, rebuilt, minimal=minimal)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    write_zip(entries, args.output)
    if args.manifest:
        args.manifest.parent.mkdir(parents=True, exist_ok=True)
        lines = [f"{hashlib.sha256(data).hexdigest()}  {name}  {source}"
                 for name, (data, source) in sorted(entries.items())]
        args.manifest.write_text("\n".join(lines) + "\n")
    print(f"{args.output}: {len(entries)} files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
