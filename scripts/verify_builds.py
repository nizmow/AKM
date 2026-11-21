#!/usr/bin/env python3
"""Verify the expected outputs of the current Makefile.apps targets."""

import argparse
import json
import subprocess
import sys
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BUILD = ROOT / "build/atari/apps"
TARGETS = {
    "mod-app": ("mod-app", ["MOD_APP.TTP"]),
    "cmdline": ("cmdline", ["CMDLINE.PRG"]),
    "mac2st": ("mac2st", ["MAC2ST.TTP"]),
    "dump": ("dump", ["DUMP.TTP"]),
    "fc": ("fc", ["FC.TTP"]),
    "split": ("split", ["SPLIT.TTP"]),
    "showfile": ("showfile", ["SHOWFILE.TTP"]),
    "limitmem": ("limitmem", ["LIMITMEM.TTP"]),
    "mgclock": ("mgclock", ["MGCLOCK.PRG"]),
    "mgxclock": ("mgxclock", ["MGXCLOCK.PRG"]),
    "vfatconf": ("vfatconf", ["VFATCONF.PRG", "vfatconf.rsc"]),
    "shutdown": ("shutdown", ["SHUTDOWN.PRG"]),
    "wbdaemon": ("wbdaemon", ["WBDAEMON.PRG"]),
    "applicat": ("applicat", ["APPLICAT.APP", "applicat.rsc", "applicat.inf"]),
    "mgview": ("mgview", ["MGVIEW.APP"]),
    "mgnotice": ("mgnotice", ["MGNOTICE.APP", "mgnotice.rsc", "EDITOBJC.SLB"]),
    "mgsearch": ("mgsearch", ["MGSEARCH.APP", "MGSEARCH.RSC"]),
    "mgcopy": ("mgcopy", ["MGCOPY.APP", "MGCOPY.RSC"]),
    "mgformat": ("mgformat", ["MGFORMAT.PRG", "MGFORMAT.RSC"]),
    "vt52": ("vt52", ["VT52.PRG", "VT52.RSC"]),
    "mcmd-languages": ("mcmd", ["de/MCMD.TOS", "en/MCMD.TOS", "fr/MCMD.TOS"]),
    "magxconf-languages": ("magxconf", [
        f"{language}/MAGXCONF.{suffix}"
        for language in ("de", "en", "fr") for suffix in ("PRG", "ACC")
    ]),
    "edit-slb": ("edit-slb", ["EDITOBJC.SLB"]),
    "mgedit": ("mgedit", ["MGEDIT.APP", "mgedit.rsc", "EDITOBJC.SLB"]),
    "abi-check": ("abi-check", ["ABI_CHECK.o"]),
    "console-test-driver": ("console", ["RUNARG.PRG"]),
}


def check(build_dir):
    results = []
    for target, (directory, outputs) in TARGETS.items():
        for output in outputs:
            path = build_dir / directory / output
            results.append({
                "target": target,
                "file": str(path.relative_to(ROOT)),
                "present": path.is_file(),
                "size": path.stat().st_size if path.is_file() else 0,
            })
    return results


def build():
    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    build_dir = ROOT / "build/atari" / f"verify-{stamp}"
    command = [str(ROOT / "scripts/atari-container"), "run", "make", "-f", "Makefile.apps",
               f"BUILD_DIR={build_dir.relative_to(ROOT)}", *TARGETS]
    result = subprocess.run(command, cwd=ROOT, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    print(result.stdout, end="")
    if result.returncode:
        raise SystemExit(f"batch build failed with status {result.returncode}")
    return build_dir


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", action="store_true",
                        help="build every target into a fresh directory first")
    parser.add_argument("--build-dir", type=Path, help="inspect this output directory")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    build_dir = args.build_dir or (build() if args.build else DEFAULT_BUILD)
    if not build_dir.is_absolute():
        build_dir = ROOT / build_dir
    results = check(build_dir)
    missing = [item for item in results if not item["present"] or item["size"] == 0]
    if args.json:
        print(json.dumps({"build_dir": str(build_dir), "results": results}, indent=2))
    else:
        print(f"Build directory: {build_dir}")
        print(f"Expected files: {len(results)}; missing/empty: {len(missing)}")
        for item in results:
            state = "ok" if item["present"] and item["size"] else "MISSING"
            print(f"{state:7} {item['file']} ({item['size']} bytes)")
    return 1 if missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
