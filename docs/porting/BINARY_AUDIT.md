# Checked-in Atari binary audit

Baseline: `4ee69b5` (main after PR #11). Scope: tracked paths under `base/`,
`core/`, and `kernel/` with `.APP`, `.ACC`, `.PRG`, `.TOS`, `.TTP`, `.SLB`,
`.CPX`, `.SYS`, or `.OSD` suffixes (case-insensitive), plus files with the TOS
`60 1a` header outside those suffixes. This yields 96 suffix matches and two
header matches: `WBDAEMON.BAK` and the `PCMXSTRT.O` DIS input. The latter is a
DRI object fixture, not a runnable program. The suffix matches include two
empty APPLICAT placeholders. The other DIS object inputs, `T.O` and `X.O`, and
non-executable fixtures such as `APPLICAT.BAK` are outside this inventory and
are not removal candidates.

| Decision | Paths | Action |
|---|---:|---|
| Archive | 23 | No live consumer. Exact copies remain in `pre-magic-retirement`; the 22-path deletion and CRASHDMP conversion are on this branch. |
| Keep | 55 | Active input, or explicitly retained compatibility, corpus, or unfinished-port reference. |
| Replace → archive | 20 | Still read by a runner. Preserve the relevant assertions or provenance before removing it in a separate test change. |

## Consumer boundaries

- `Makefile`, `Makefile.apps`, `kernel/mk/`, and the `mise.toml` entry points
  build into the configured output directories; a matching basename is not a
  dependency on a checked-in copy. The in-scope binary copied by a current Make
  rule is `base/mgformat/BIN/MGFORMAT.PRG`, for `mgformat-hardware-kit`.
- `scripts/make-release.py` packages executable products from its build directory,
  not from the component `BIN/` directories. The eight retained CPXs and
  `LANG/DE/GEMSYS/MAGIC/XTENSION/KEYTABLS.SYS` under `release/atari/` are actual
  distribution inputs outside this inventory. `core/osd/tests/release/` is a
  test reference directory, not a packaging root.
- The two binary fixtures under `scripts/testdata/release/` also stay outside
  the removal set: `HARDCOPY.PRG` is the release-identity side of
  `neost-hardcopy-smoke.py`; `LOAD_IMG.SLB` is the `--baseline release` path of
  `neost-slb-consumer-smoke.py`.
- Consumer names below refer to the live regression runners, not historical
  `.PRJ`/batch files or build logs. A documented past comparison does not prove
  a live dependency. A missing automated consumer does not make a preserved
  compatibility reference redundant.

## Byte recovery and per-path decisions

Every row has a matching Git blob in the published
[`pre-magic-retirement`](https://github.com/nizmow/AKM/tree/pre-magic-retirement)
tag (`88eb5ee`); `original-import` was also searched. `Same path` means the
listed checkout path is identical in that tag. Otherwise use the exact archive
path shown. To recover bytes without changing the checkout:

```sh
git fetch origin tag pre-magic-retirement
git show "pre-magic-retirement:$archived_path" > "$destination"
```

| Checkout path | Decision | Current consumer or retention reason | Archive path in `pre-magic-retirement` |
|---|---|---|---|
| `base/applicat/BIN/APPLICAT.APP` | Archive | No current consumer found; `Makefile.apps` builds the German application from source. FR/US files are empty placeholders. | Same path |
| `base/applicat/BIN/FRA/APPLICAT.APP` | Archive | No current consumer found; `Makefile.apps` builds the German application from source. FR/US files are empty placeholders. | Same path |
| `base/applicat/BIN/US/APPLICAT.APP` | Archive | No current consumer found; `Makefile.apps` builds the German application from source. FR/US files are empty placeholders. | Same path |
| `base/cmdline/BIN/CMDLINE.PRG` | Archive | Modern target produces its own executable; no checked-in binary consumer found. Past manual port comparisons are not active tests. | Same path |
| `base/cops/SRC/COPS.APP` | Keep | `neost-cops-smoke.py`: panel/CPX visual baseline. | Same path |
| `base/cops/tests/COPS.ACC` | Keep | `neost-cops-accessory-smoke.py`: installed accessory and panel/CPX visual baseline. | `MagiC/Releases/MAGIC.A/AUTO/ACCS/COPS.ACC` |
| `base/cpx/BIN/MAGXCONF.CPX` | Keep | `neost-magxconf-cpx-smoke.py`: boot configuration and dialog baseline. | Same path |
| `base/cpx/BIN/TSLICE.CPX` | Keep | `neost-tslice-cpx-smoke.py` visual baseline; `neost-cops-accessory-smoke.py` installs this CPX as test input. | Same path |
| `base/crashdmp/BIN/CRASHDMP.TOS` | Archive | Converted to a direct rebuilt-binary check: `neost-diagnostic-tools-smoke.py` runs only the rebuild on the no-record path and requires exit 0 with empty output. | Same path |
| `base/dis/BIN/DIS1.TTP` | Keep | `neost-dis-smoke.py`: disassembly/workfile bytes still come from legacy executable comparisons; explicit exit statuses alone are insufficient. | Same path |
| `base/dis/BIN/DIS2.TTP` | Keep | `neost-dis-smoke.py`: disassembly/workfile bytes still come from legacy executable comparisons; explicit exit statuses alone are insufficient. | Same path |
| `base/dis/TST/T.PRG` | Keep | `neost-dis-smoke.py`: input corpus, not a comparison executable. | Same path |
| `base/dis/TST/U.PRG` | Keep | `neost-dis-smoke.py`: input corpus, not a comparison executable. | Same path |
| `base/dis/TST/X.PRG` | Keep | Manual DIS corpus: documented `dis2 -t x.prg` example, not a case in the current runner. | Same path |
| `base/dis/tests/PCMXSTRT.O` | Keep | `neost-dis-smoke.py`: input corpus, not a comparison executable. | Same path |
| `base/dump/BIN/DUMP.TTP` | Replace → archive | `neost-console-tools-smoke.py`: fixed inputs and statuses already exist. Derive output from the command contract (CRLF filtering, dump modes, compare offsets/values, split bytes); do not drop console or file-content checks. | Same path |
| `base/edit-slb/BIN/EDITOBJC.SLB` | Keep | `neost-mgnotice-smoke.py`: legacy editor dependency for the archived MGNOTICE run, not a standalone editor oracle. | Same path |
| `base/edit-slb/TST_APP/EDIT/EDIT.PRG` | Keep | Reference executable for the separate, still-unported EDIT demonstration. No active build or runner consumes it. | Same path |
| `base/fc/BIN/FC.TTP` | Replace → archive | `neost-console-tools-smoke.py`: fixed inputs and statuses already exist. Derive output from the command contract (CRLF filtering, dump modes, compare offsets/values, split bytes); do not drop console or file-content checks. | Same path |
| `base/flp-par/BIN/FLP_PAR0.PRG` | Archive | Modern target produces its own executable; no checked-in binary consumer found. Past manual port comparisons are not active tests. | Same path |
| `base/flp-par/BIN/FLP_PAR1.PRG` | Archive | Modern target produces its own executable; no checked-in binary consumer found. Past manual port comparisons are not active tests. | Same path |
| `base/hardcopy/HARDCOPY.PRG` | Keep | `neost-hardcopy-smoke.py`: hook checks are explicit, but screen equality and release identity still use this reference. | Same path |
| `base/keyb-sys/BIN/KEYB_CZ.PRG` | Keep | `neost-keyb-sys-smoke.py`: name/status/size checks are explicit; key and dead-key contents still use archive equality. | Same path |
| `base/keyb-sys/BIN/KEYB_DEI.PRG` | Keep | `neost-keyb-sys-smoke.py`: name/status/size checks are explicit; key and dead-key contents still use archive equality. | Same path |
| `base/keyb-sys/BIN/KEYB_FRA.PRG` | Keep | `neost-keyb-sys-smoke.py`: name/status/size checks are explicit; key and dead-key contents still use archive equality. | Same path |
| `base/keyb-sys/BIN/KEYB_SWG.PRG` | Keep | `neost-keyb-sys-smoke.py`: name/status/size checks are explicit; key and dead-key contents still use archive equality. | Same path |
| `base/keyb-sys/BIN/KEYB_UK.PRG` | Keep | `neost-keyb-sys-smoke.py`: name/status/size checks are explicit; key and dead-key contents still use archive equality. | Same path |
| `base/keyb-sys/BIN/KEYB_US.PRG` | Keep | `neost-keyb-sys-smoke.py`: name/status/size checks are explicit; key and dead-key contents still use archive equality. | Same path |
| `base/limitmem/BIN/LIMITMEM.TTP` | Replace → archive | `neost-limitmem-smoke.py`: fixed statuses and MAGX marker change/no-change cases. Finish standalone expectations for all touched files and meaningful console output. | Same path |
| `base/load-img/BIN/LOAD_IMG.SLB` | Keep | `neost-slb-consumer-smoke.py`: default `--baseline source` uses this library; default and release paths still compare observed IMG colours. | Same path |
| `base/mac2st/BIN/MAC2ST.TTP` | Replace → archive | `neost-console-tools-smoke.py`: fixed inputs and statuses already exist. Derive output from the command contract (CRLF filtering, dump modes, compare offsets/values, split bytes); do not drop console or file-content checks. | Same path |
| `base/magxconf/BIN/FRA/MAGXCONF.PRG` | Keep | `neost-magxconf-smoke.py`: localized comparison frames alongside explicit save/cancel/reboot checks. | Same path |
| `base/magxconf/BIN/GER/MAGXCONF.ACC` | Keep | `neost-magxconf-smoke.py`: localized comparison frames alongside explicit save/cancel/reboot checks. | Same path |
| `base/magxconf/BIN/US/MAGXCONF.ACC` | Keep | `neost-magxconf-smoke.py`: localized comparison frames alongside explicit save/cancel/reboot checks. | Same path |
| `base/magxkern/BIN/MAGXKERN.TTP` | Keep | `neost-diagnostic-tools-smoke.py`: monitor output uses exact or normalized archive comparison beyond the explicit status/field checks. | Same path |
| `base/memexamn/BIN/MEMEXAMN.TTP` | Keep | `neost-diagnostic-tools-smoke.py`: monitor output uses exact or normalized archive comparison beyond the explicit status/field checks. | Same path |
| `base/mfm/BIN/MFM16.SYS` | Keep | MagiCMac compatibility reference retained by the retirement decision. `neost-screen-driver-smoke.py` validates rebuilt MFM structure, not these files; no host runtime comparison exists yet. | `MagiC/KERNEL/VDI/DRIVERS/MAC/MFM16.SYS` |
| `base/mfm/BIN/MFM16IP.SYS` | Keep | MagiCMac compatibility reference retained by the retirement decision. `neost-screen-driver-smoke.py` validates rebuilt MFM structure, not these files; no host runtime comparison exists yet. | `MagiC/KERNEL/VDI/DRIVERS/MAC/MFM16IP.SYS` |
| `base/mfm/BIN/MFM16M.SYS` | Keep | MagiCMac compatibility reference retained by the retirement decision. `neost-screen-driver-smoke.py` validates rebuilt MFM structure, not these files; no host runtime comparison exists yet. | `MagiC/KERNEL/VDI/DRIVERS/MAC/MFM16M.SYS` |
| `base/mfm/BIN/MFM2.SYS` | Keep | MagiCMac compatibility reference retained by the retirement decision. `neost-screen-driver-smoke.py` validates rebuilt MFM structure, not these files; no host runtime comparison exists yet. | `MagiC/KERNEL/VDI/DRIVERS/MAC/MFM2.SYS` |
| `base/mfm/BIN/MFM256.SYS` | Keep | MagiCMac compatibility reference retained by the retirement decision. `neost-screen-driver-smoke.py` validates rebuilt MFM structure, not these files; no host runtime comparison exists yet. | `MagiC/KERNEL/VDI/DRIVERS/MAC/MFM256.SYS` |
| `base/mfm/BIN/MFM32K.SYS` | Keep | MagiCMac compatibility reference retained by the retirement decision. `neost-screen-driver-smoke.py` validates rebuilt MFM structure, not these files; no host runtime comparison exists yet. | `MagiC/KERNEL/VDI/DRIVERS/MAC/MFM32K.SYS` |
| `base/mfm/BIN/MFM4IP.SYS` | Keep | MagiCMac compatibility reference retained by the retirement decision. `neost-screen-driver-smoke.py` validates rebuilt MFM structure, not these files; no host runtime comparison exists yet. | `MagiC/KERNEL/VDI/DRIVERS/MAC/MFM4IP.SYS` |
| `base/mgclock/BIN/MGCLOCK.PRG` | Archive | `neost-mgclock-smoke.py` runs the rebuilt clock and APPLICAT, not this binary. | Same path |
| `base/mgedit/BIN/MGEDIT.APP` | Archive | Modern target produces its own executable; no checked-in binary consumer found. Past manual port comparisons are not active tests. | Same path |
| `base/mgformat/BIN/MGFORMAT.PRG` | Keep | Both `neost-mgformat-smoke.py` and the `Makefile.apps:mgformat-hardware-kit` archive payload consume this file. | Same path |
| `base/mgnotice/BIN/MGNOTICE.APP` | Keep | `neost-mgnotice-smoke.py`: pixel and complete note-byte comparisons; standalone note/window checks cover only part of the behavior. | Same path |
| `base/mgsearch/BIN/MGSEARCH.APP` | Keep | `neost-mgsearch-smoke.py`: archive supplies comparison dialog and result pixels. | Same path |
| `base/mgview/BIN/MGVIEW.APP` | Archive | Modern target produces its own executable; no checked-in binary consumer found. Past manual port comparisons are not active tests. | Same path |
| `base/mgview/BIN/VIEW.PRG` | Archive | Historical binary distinct from the modern MGVIEW target; no live caller found. | Same path |
| `base/mgxclock/BIN/MGXCLOCK.PRG` | Archive | Modern target produces its own executable; no checked-in binary consumer found. Past manual port comparisons are not active tests. | Same path |
| `base/mod-app/MOD_APP.TTP` | Archive | Legacy copy; no live caller found. The current runner uses the separate reference path. | Same path |
| `base/mod-app/tests/MOD_APP.TTP` | Replace → archive | `neost-mod-app-smoke.py`: 17 fixed statuses. Define expected APPLICAT.INF/BAK/DAT contents and absence cases before dropping the archive snapshot comparison. | `base/mod-app/MOD_APP.TTP` |
| `base/showfile/BIN/SHOWFILE.TTP` | Archive | Modern target produces its own executable; no checked-in binary consumer found. Past manual port comparisons are not active tests. | Same path |
| `base/showfree/SHOWFREE.TOS` | Keep | `neost-diagnostic-tools-smoke.py`: monitor assertions are independent, final screen equality is not. | Same path |
| `base/shutdown/BIN/SHUTDOWN.PRG` | Archive | Modern target produces its own executable; no checked-in binary consumer found. Past manual port comparisons are not active tests. | Same path |
| `base/split/BIN/SPLIT.TTP` | Replace → archive | `neost-console-tools-smoke.py`: fixed inputs and statuses already exist. Derive output from the command contract (CRLF filtering, dump modes, compare offsets/values, split bytes); do not drop console or file-content checks. | Same path |
| `base/vfatconf/BIN/VFATCONF.PRG` | Archive | Modern target produces its own executable; no checked-in binary consumer found. Past manual port comparisons are not active tests. | Same path |
| `base/vt52/BIN/VT52.PRG` | Keep | `neost-vt52-smoke.py`: archive supplies terminal display comparison. | Same path |
| `base/vt52/SRC/VT52/VT52.PRG` | Archive | Legacy copy; no live caller found. The current runner uses the separate reference path. | Same path |
| `base/wbdaemon/BIN/WBDAEMON.BAK` | Archive | Executable backup, not a build input, package member source, or test fixture. | Same path |
| `base/wbdaemon/BIN/WBDAEMON.PRG` | Archive | Modern target produces its own executable; no checked-in binary consumer found. Past manual port comparisons are not active tests. | Same path |
| `base/winframe-demo/WINFRAME.SLB` | Keep | `neost-winframe-demo-smoke.py`: archive supplies window-frame comparison, separate from production WINFRAME. | Same path |
| `base/winframe/WINFRAME.SLB` | Archive | Legacy copy; no live caller found. The current runner uses the separate reference path. | Same path |
| `base/winframe/tests/WINFRAME.SLB` | Keep | `neost-winframe-smoke.py`: production window-frame comparison. | `base/winframe/WINFRAME.SLB` |
| `base/xp-rastr/BIN/XP_RASTR.SLB` | Keep | `neost-slb-consumer-smoke.py`: XP library for both archive baselines; IMG and conversion comparisons are not wholly independent. | Same path |
| `core/magxboot/SRC/MAGXBO32.PRG` | Keep | `neost-magxboot-smoke.py`: source-build identity with the archived MAGXBOOT variant, plus archived/rebuilt desktop comparison for the release TOS loader. | Same path |
| `core/magxboot/SRC/MAGXBOOT.PRG` | Keep | `neost-magxboot-smoke.py`: source-build identity with the archived MAGXBOOT variant, plus archived/rebuilt desktop comparison for the release TOS loader. | Same path |
| `core/magxboot/tests/MAGXBOOT.TOS` | Keep | `neost-magxboot-smoke.py`: source-build identity with the archived MAGXBOOT variant, plus archived/rebuilt desktop comparison for the release TOS loader. | `MagiC/Releases/MAGIC.A/AUTO/MAGXBOOT.TOS` |
| `core/magxdesk/SRC/MAGXDESK.APP` | Keep | `neost-magxdesk-smoke.py`: archive supplies desktop raster comparison. | Same path |
| `core/mcmd/SRC/ALN.TTP` | Archive | Legacy assembler/linker tools. `Makefile.apps` builds MCMD with VASM; the MCMD regression uses rebuilt interpreters. | Same path |
| `core/mcmd/SRC/MAS.TTP` | Archive | Legacy assembler/linker tools. `Makefile.apps` builds MCMD with VASM; the MCMD regression uses rebuilt interpreters. | Same path |
| `core/mfa/tests/archive/MFA16.SYS` | Keep | `neost-screen-driver-smoke.py`: ST archive/rebuild screen comparison; probe and missing-driver checks alone do not cover raster parity. | `MagiC/KERNEL/VDI/DRIVERS/ATARI/MFA16.SYS` |
| `core/mfa/tests/archive/MFA2.SYS` | Keep | `neost-screen-driver-smoke.py`: ST archive/rebuild screen comparison; probe and missing-driver checks alone do not cover raster parity. | `MagiC/KERNEL/VDI/DRIVERS/ATARI/MFA2.SYS` |
| `core/mfa/tests/archive/MFA256.SYS` | Keep | TT/Falcon historical compatibility reference. Current screen test checks rebuilt structure but does not consume this archive or exercise the mode. | `MagiC/KERNEL/VDI/DRIVERS/ATARI/MFA256.SYS` |
| `core/mfa/tests/archive/MFA32K.SYS` | Keep | TT/Falcon historical compatibility reference. Current screen test checks rebuilt structure but does not consume this archive or exercise the mode. | `MagiC/KERNEL/VDI/DRIVERS/ATARI/MFA32K.SYS` |
| `core/mfa/tests/archive/MFA4.SYS` | Keep | `neost-screen-driver-smoke.py`: ST archive/rebuild screen comparison; probe and missing-driver checks alone do not cover raster parity. | `MagiC/KERNEL/VDI/DRIVERS/ATARI/MFA4.SYS` |
| `core/mgcopy/TST/MGCOPY.TOS` | Archive | Historical TOS test executable. No live runner reads this copy; `mgcopy` and `mgcopy-test-driver` build their own programs. | Same path |
| `core/osd/tests/archive/MIN-ATAR/OFF002.OSD` | Keep | Individual OSD smoke runner requires exact rebuilt-byte parity with the MIN-ATAR image; preserve that check, distinct from runtime behavior. | `MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF002.OSD` |
| `core/osd/tests/archive/MIN-ATAR/OFF004IP.OSD` | Keep | Individual OSD smoke runner requires exact rebuilt-byte parity with the MIN-ATAR image; preserve that check, distinct from runtime behavior. | `MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF004IP.OSD` |
| `core/osd/tests/archive/MIN-ATAR/OFF016IP.OSD` | Keep | Individual OSD smoke runner requires exact rebuilt-byte parity with the MIN-ATAR image; preserve that check, distinct from runtime behavior. | `MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF016IP.OSD` |
| `core/osd/tests/archive/MIN-ATAR/OFF256IP.OSD` | Keep | Individual OSD smoke runner requires exact rebuilt-byte parity with the MIN-ATAR image; preserve that check, distinct from runtime behavior. | `MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF256IP.OSD` |
| `core/osd/tests/archive/MIN-ATAR/OFF32KFL.OSD` | Replace → archive | `neost-osd-batch-smoke.py`: explicit pixel sequences, generated expected bitmap, and missing-driver control. Preserve exact parity where a separate exact fixture applies. | `MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF32KFL.OSD` |
| `core/osd/tests/archive/OFF016PK.OSD` | Keep | `neost-osd-batch-smoke.py`: `exact` whole-file parity fixture, separate from the bitmap contract. | `MagiC/KERNEL/VDI/DRIVERS/OSD/OFF016PK.OSD` |
| `core/osd/tests/archive/OFF16MOV.OSD` | Replace → archive | `neost-osd-batch-smoke.py`: explicit pixel sequences, generated expected bitmap, and missing-driver control. Preserve exact parity where a separate exact fixture applies. | `MagiC/KERNEL/VDI/DRIVERS/OSD/OFF16MOV.OSD` |
| `core/osd/tests/archive/OFF256PK.OSD` | Replace → archive | `neost-osd-batch-smoke.py`: explicit pixel sequences, generated expected bitmap, and missing-driver control. Preserve exact parity where a separate exact fixture applies. | `MagiC/KERNEL/VDI/DRIVERS/OSD/OFF256PK.OSD` |
| `core/osd/tests/archive/OFF32KPK.OSD` | Replace → archive | `neost-osd-batch-smoke.py`: explicit pixel sequences, generated expected bitmap, and missing-driver control. Preserve exact parity where a separate exact fixture applies. | `MagiC/KERNEL/VDI/DRIVERS/OSD/OFF32KPK.OSD` |
| `core/osd/tests/archive/OFF65KPK.OSD` | Replace → archive | `neost-osd-batch-smoke.py`: explicit pixel sequences, generated expected bitmap, and missing-driver control. Preserve exact parity where a separate exact fixture applies. | `MagiC/KERNEL/VDI/DRIVERS/OSD/OFF65KPK.OSD` |
| `core/osd/tests/release/OFF002.OSD` | Replace → archive | Individual OSD smoke runner reads release reference for structure/provenance reporting; rebuilt checks and exact MIN-ATAR fixture are independent. Record the release variant in the report before retiring the live file lookup. | `MagiC/Releases/MAGIC.A/GEMSYS/OFF002.OSD` |
| `core/osd/tests/release/OFF004IP.OSD` | Replace → archive | Individual OSD smoke runner reads release reference for structure/provenance reporting; rebuilt checks and exact MIN-ATAR fixture are independent. Record the release variant in the report before retiring the live file lookup. | `MagiC/Releases/MAGIC.A/GEMSYS/OFF004IP.OSD` |
| `core/osd/tests/release/OFF016IP.OSD` | Replace → archive | Individual OSD smoke runner reads release reference for structure/provenance reporting; rebuilt checks and exact MIN-ATAR fixture are independent. Record the release variant in the report before retiring the live file lookup. | `MagiC/Releases/MAGIC.A/GEMSYS/OFF016IP.OSD` |
| `core/osd/tests/release/OFF016PK.OSD` | Replace → archive | `neost-osd-batch-smoke.py`: optional release structure/provenance report only; not the guest driver or `exact` input. Replace live lookup with documented reference provenance while preserving rebuilt checks. | `MagiC/Releases/MAGIC.A/GEMSYS/OFF016PK.OSD` |
| `core/osd/tests/release/OFF16MOV.OSD` | Replace → archive | `neost-osd-batch-smoke.py`: optional release structure/provenance report only; not the guest driver or `exact` input. Replace live lookup with documented reference provenance while preserving rebuilt checks. | `MagiC/Releases/MAGIC.A/GEMSYS/OFF16MOV.OSD` |
| `core/osd/tests/release/OFF256IP.OSD` | Replace → archive | Individual OSD smoke runner reads release reference for structure/provenance reporting; rebuilt checks and exact MIN-ATAR fixture are independent. Record the release variant in the report before retiring the live file lookup. | `MagiC/Releases/MAGIC.A/GEMSYS/OFF256IP.OSD` |
| `core/osd/tests/release/OFF256PK.OSD` | Replace → archive | `neost-osd-batch-smoke.py`: optional release structure/provenance report only; not the guest driver or `exact` input. Replace live lookup with documented reference provenance while preserving rebuilt checks. | `MagiC/Releases/MAGIC.A/GEMSYS/OFF256PK.OSD` |
| `core/osd/tests/release/OFF32KFL.OSD` | Keep | `neost-osd-batch-smoke.py`: `exact` whole-file parity fixture, separate from the bitmap contract. | `MagiC/Releases/MAGIC.A/GEMSYS/OFF32KFL.OSD` |
| `core/osd/tests/release/OFF32KPK.OSD` | Replace → archive | `neost-osd-batch-smoke.py`: optional release structure/provenance report only; not the guest driver or `exact` input. Replace live lookup with documented reference provenance while preserving rebuilt checks. | `MagiC/Releases/MAGIC.A/GEMSYS/OFF32KPK.OSD` |
| `core/osd/tests/release/OFF65KPK.OSD` | Replace → archive | `neost-osd-batch-smoke.py`: optional release structure/provenance report only; not the guest driver or `exact` input. Replace live lookup with documented reference provenance while preserving rebuilt checks. | `MagiC/Releases/MAGIC.A/GEMSYS/OFF65KPK.OSD` |
