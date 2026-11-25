# Phase 1 TODO

Concrete next work after the first standalone application port. Direction and
phase boundaries remain in the [roadmap](../ROADMAP.md); the full component list is
[APPLICATION_BUILDS.md](APPLICATION_BUILDS.md). Do not duplicate that inventory
here or interpret an unchecked item as a missing stub to manufacture.

## Active queue — core Atari installation

1. [x] **COPS accessory mode.** The shipped and rebuilt executable both run as
   `AUTO/ACCS/COPS.ACC`; `test:cops-accessory-neost` verifies the menu item,
   panel open/close/reopen and TSLICE dialog against a missing-accessory control.
   No separate startup recipe was needed.
2. [x] **German Atari/ST core ZIP.** `release:core` writes a deterministic
   package; `test:release-neost` installs only its payload on a cleaned clone,
   verifies every member hash, and boots MAGXDESK plus COPS. The manifest
   records rebuilt versus retained inputs. File operations and the SHUTDOWN
   confirmation were checked separately on that payload.
3. **Use manifest gaps for the next core port.** PDLG, CHGRES and other
   retained files remain binary-only; choose the next source target from
   installed components whose rebuild is needed, not from unbuilt catalogue
   count. Keep optional demos, Mac/TT/Falcon variants and legacy mutators out of
   the default profile.

## Deferred and optional work

- **PDLG.SLB:** optional print-dialog support, not a boot prerequisite. The
  shipped German binary loads; no working dialog was observed on the fixture
  without a GDOS printer driver. No source target until the missing inputs are
  resolved. See [porting notes](PORTING_NOTES.md#pdlgslb).
- **MFA/MFM:** all 12 canonical drivers build. ST high/medium/low are tested;
  `MFA256`/`MFA32K` await a compatible TT/Falcon profile, and MFM awaits a
  MagiCMac `VDI_SETUP_DATA` host. Track these as runtime coverage, not ports.
- **SLB_DEMO, EDIT.PRG, legacy utilities and old application variants:** optional,
  not release gates. SLB_DEMO is a candidate for a small standalone port
  (local projects and archived binaries; legacy libraries still need mapping).
  Keep installer and disk/header mutators opt-in with disposable fixtures.
- **Shared libraries and startup:** only build when a concrete consumer needs
  them; see [the catalogue](APPLICATION_BUILDS.md#6-reusable-librarystartup-projects).
  Keep historical, host-specific, external-dependency and missing-source work
  in the catalogue, not the active Atari queue.

## Established by MOD_APP

- [x] Add `Makefile.apps` with an independent `mod-app` target.
- [x] Add `mise run apps -- <target/options>` through the existing Podman wrapper.
- [x] Link an application using the supplied MiNTLib's default stack ABI and
  32-bit integers, without archived Pure C startup objects or libraries.
- [x] Use public SDK trap/error headers instead of a private legacy import tree.
- [x] Preserve writable path buffers, Atari literal bytes and 32-bit file tags.
- [x] Discover and address the TOS-tail quoting difference from PCMXSTRT, while
  retaining extended ARGV support.
- [x] Compare old/new binaries under the GCC kernel in an isolated Hatari image:
  17 cases each, matching return codes and 48 byte-identical INF/BAK/DAT pairs.
- [x] Document the commands, ABI choice and observed startup behavior in
  [PORTING_NOTES.md](PORTING_NOTES.md).

## Next small ports

Each item is a separate source/build owner. Use public SDK declarations, inspect
legacy assumptions, and add one independent target/output; no generic application
framework or shared-library rewrite is required before these are attempted.

- [x] **CMDLINE** — add `cmdline` producing `CMDLINE.PRG`.
- [x] Compare DOS-tail and environment reports against the original under NeoST
  using the same Pexec fixture; compare meaningful fields, not runtime addresses.
- [x] Test ARGV-style Pexec launch under NeoST. Tail, program path and status
  matched. MiNTLib consumes the reserved `ARGV=` marker before `main`; this
  expected startup difference is not treated as a failure.
- [x] **SHOWFILE** — add `showfile` producing `SHOWFILE.TTP`.
- [x] Compare visible text and exit behavior for SHOWFILE against the archived
  binary under NeoST: sample text, EOF prompt, missing-file and no-argument cases
  matched, including child exit statuses.
- [x] **LIMITMEM** — add `limitmem` producing `LIMITMEM.TTP`; compare header
  changes, child statuses and errors against the archive on disposable PRG copies.
  EmuTOS's legacy `Fshrink` behavior prevents a successful removal test; details
  in [porting notes](PORTING_NOTES.md).
- [x] **MGCLOCK** — add `mgclock` producing `MGCLOCK.PRG` with MiNTLib GEMlib
  AES/VDI bindings.
- [x] Verify the restored desktop-drawing clock and timer behavior under NeoST;
  the window-event smoke was replaced by desktop-pixel checks.
- [x] Verify coexistence with other applications under the MagiC kernel;
  MGCLOCK and APPLICAT were booted together on a cloned image.

- [x] **WBDAEMON** — add independent `wbdaemon` target, launch it from MagiC's
  `#_APP` startup folder, and verify `AP_TERM` exit under NeoST. Periodic
  writeback remains unmeasured; see [the build catalogue](APPLICATION_BUILDS.md).
- [x] Correct WBDAEMON's `SC_WBACK` subcommand width; the MagiC regression probe
  verifies registration, `AP_TERM` exit and release of the daemon slot.

## Build conventions to confirm as consumers appear

- [x] Share the established raw-TOS-tail quote/ARGV adapter among MOD_APP,
  APPLICAT, MGVIEW and MGEDIT in `MagiC/APPS/TOS_ARGS.H`.
- [ ] Preserve quoted paths, escaped apostrophes, empty arguments, long ARGV
  arguments and literal file-type patterns where the application supports them.
  A link against a modern CRT does not automatically preserve old startup behavior.
- [x] Establish GUI structure/message widths with public SDK types and by-value
  callback argument records in APPLICAT, without changing the global integer ABI.
- [x] Map MGEDIT's required GEM utilities and SLB bindings to public SDK calls
  and source helpers; build `EDITOBJC.SLB` independently rather than linking
  archived SLB/GEM utility objects.
- [x] Define resource/data inputs and working-directory requirements for
  APPLICAT's executable/RSC/starter-INF bundle. Keep existing translations and
  binary resource encodings unchanged; translation repair remains a later phase.
- [x] Build selected targets in fresh output directories and record prerequisites
  that were previously supplied only by a developer's machine.

## Repeatable runtime verification

- [x] Add an explicitly invoked MOD_APP smoke runner. `scripts/neost-mod-app-smoke.py`
  runs the archived and GCC binaries through NeoST's headless TOS autostart.
- [x] Fix the NeoST profile at ST/4 MiB with bundled EmuTOS 1.04 by default;
  each binary gets a separate disposable GEMDOS C: tree. Capture logs, results
  and screenshots without touching the Hatari boot images or running GUI.
- [x] Keep fixture preparation, Pexec child-status capture and INF/BAK/DAT
  comparison in the runner, separate from MOD_APP itself.
- [x] Cover application, file-type and special-object registration, duplicate
  and missing-app errors, quoted/empty/long paths, and output-file parity.
- [x] Keep crash probes, installers, header patchers and formatters out of the
  default smoke set.
- [ ] Give crash probes, installers and formatters disposable fixtures and
  explicit test targets when they are ported.
- [ ] Split the catalogue's combined status labels into compiled, linked,
  packaged, launched and functionally tested for the next batch.

## Completed port batches

- [x] **VFATCONF** — add `vfatconf` producing `VFATCONF.PRG` plus its required
  resource; launch reaches the dialog under MagiC, but configuration changes
  remain untested.
- [x] **MGXCLOCK** — add `mgxclock` and a lifecycle test driver; verify clock
  updates, writeback daemon registration, `AP_TERM` exit, slot release and
  persistence of a cache-written file on a cloned MagiC disk. Actual
  periodic-flush timing remains unmeasured.
- [x] **APPLICAT** — independent bundle, cache compilation comparison and UI
  association-save scenario under MagiC; remaining paths are listed in porting notes.
- [x] **MGVIEW** — independent executable; six archived/rebuilt MagiC scenarios
  verify line endings, quoted/ARGV startup, empty input and keyboard navigation.
- [x] **MGEDIT / EDITOBJC.SLB** — independent library and complete editor bundle;
  six archived/rebuilt cases verify editing, clipboard, preferences, Save As
  and quoted/ARGV startup under MagiC.
- [x] **MGNOTICE version 2** — add `mgnotice` producing the application, German
  resource and rebuilt `EDITOBJC.SLB`; archived/rebuilt create, edit, save and
  reopen states match on a cloned MagiC image.
- [x] **MGSEARCH** — add `mgsearch` producing `MGSEARCH.APP` and its German
  resource; compile the older `GEMUTILS`/`WINDOWS` contract and an app-local
  `core/magxdesk/SRC/PATTERN.C` copy, relying on modern GEMlib instead of `MAGIXLIB`.
  Archived/rebuilt dialog and `*.ZZZ` result screens match on a cloned MagiC
  image, including a nested fixture directory.
- [x] **MAC2ST, DUMP, FC, SPLIT** — add independent console targets using
  MiNTLib public GEMDOS calls; `mise run test:console-tools-neost` compares
  archived/rebuilt status, console output and guest files through NeoST.
- [x] **MGCOPY** — add `mgcopy` producing the application and German resource;
  compare archived/rebuilt copy, move, alias and delete effects, nested-tree
  copy/move and a D:→C: cross-drive move on cloned MagiC images. Confirm dialog,
  cancellation, long-path and error paths remain untested.
- [x] **MGFORMAT** — add `mgformat` producing the application and German
  resource; native MagiC `wdlg` AES bindings and the format dialog are verified
  on cloned MagiC images. Physical format/copy effects remain hardware tests
  because NeoST's floppy formatter reports a write fault.
- [x] **VT52** — independent `vt52` target builds `VT52.PRG` and the German
  resource, with source-built `DRAGDROP`. The assembly/C hooks use GCC's stack
  ABI. `mise run test:vt52-neost` launches a delayed TOS child and compares the
  archived/rebuilt terminal screens pixel-for-pixel. Interactive input remains untested.
- [x] **MAGXDESK version 5** — source-built `magxdesk` target; `mise run test:magxdesk-neost` reaches the desktop on cloned MagiC images and compares the desktop content pixel-for-pixel. Corrected the 18-byte APPLICAT record layout and retained relocated icon-cache pointers by removing an unsafe post-relocation `realloc`. Version 4 is out of scope.
- [x] **MCMD** — independent VASM `mcmd`/`mcmd-languages` targets produce DE/EN/FR `MCMD.TOS` outputs. `test:mcmd-neost` verifies batch execution on cloned MagiC images: child statuses and 16 captured guest files per language match the archives. Retained legacy quoting/directory behavior and remaining paths are documented in porting notes.
- [x] **DIS1/DIS2** — independent `dis1`/`dis2` targets build the two-pass
  disassembler from the Atari ST sources converted to UTF-8. GEMDOS headers and
  symbol records use fixed-width fields, and machine words are `int16_t` under
  MiNTLib's 32-bit `int`. `test:dis-neost` compares archived/rebuilt status and
  `.lbl`/`.rlo`/`.s` output on the bundled corpus and a DRI object, including
  MOVEM, options and errors. Child console text is not captured.
- [x] **MAGXCONF program/accessory** — native VASM builds produce DE/EN/FR PRG/ACC pairs. `test:magxconf-neost` verifies Cancel/OK/Save, exact four-byte self-save, version/date display and accessory reboot persistence across all languages on cloned MagiC images. The competing configuration CPX is disabled only in the fixtures; remaining limits are in porting notes.
- [x] **FLP_PAR0/1** — ported the existing sources in place to MiNTLib's public
  `Sconfig` binding; archived and rebuilt binaries both passed the NeoST probe
  on cloned MagiC images: `00000000 → 00000800 → 00000000`, zero child statuses
  and no other bits changed. Physical parallel-floppy operation remains
  untested.
- [x] **MAGXCONF.CPX** — build the archived module format from the existing
  source; local CPX ABI headers and VASM shims reproduce the classic 16-bit
  callback arguments. `test:magxconf-cpx-neost` verifies the boot initializer
  and a pixel-identical dialog opened through COPS on cloned MagiC images.
- [x] **TSLICE.CPX** — independent `tslice-cpx` target; scheduler callback
  bridged stack→D0, slider/mouse callbacks use packed classic arguments, and
  the saved timeslice/priority record stays first in DATA.
  `test:tslice-cpx-neost` compares the COPS dialog with the archive.
- [x] **WINFRAME.SLB** — independent library/resource build; the colour MagiC
  booted frame matches the archive. Interactive paths and unload are untested.
- [x] **WINFRAME.SLB demo** — the separate example project builds beside the
  production library with its own 16-object resource; the archived/rebuilt
  booted frames match pixel-for-pixel.
- [x] **XP_RASTR.SLB / LOAD_IMG.SLB** — independent builds with version-1
  packed C export records. Four-plane conversion and XIMG rendering match the
  source-tree archives. The shipped Release XIMG colours differ; see porting notes.
- [x] **OFF002.OSD** — VASM build matches the archived V4.10 MIN-ATAR binary
  byte-for-byte; NVDI selects it for a 1-plane offscreen bitmap and draws into
  the caller-owned MFDB. Release V4.11 remains a different, unported version.
- [x] **OFF004IP.OSD** — VASM build matches the archived V4.10 MIN-ATAR binary
  byte-for-byte with `-no-opt`; archive and rebuild draw into a 2-plane
  interleaved MFDB with identical pixel readbacks and bitmap bytes, asserted
  independently. Release V4.11 remains a different, unported version.
- [x] **OFF016IP.OSD** — VASM build matches the archived V4.10 MIN-ATAR binary
  byte-for-byte with `-no-opt`; archive and rebuild render a 4-plane interleaved
  MFDB with identical pixel readbacks and bitmap bytes, and a driver-less clone
  rejects the open. The shipped Release is a different build and remains
  unported.
- [x] **OFF256IP.OSD** — VASM build matches the archived V4.10 MIN-ATAR binary
  byte-for-byte with `-no-opt` after folding the zero-displacement colour-map
  load; the Release copy is the same binary. Archive and rebuild render an
  8-plane interleaved MFDB with identical pixel readbacks and 4096-byte bitmap
  bytes, asserted independently, and a driver-less clone rejects the open.
- [x] **OSD phase 2** — `OFF016PK`, `OFF256PK`, `OFF32KFL`, `OFF32KPK`,
  `OFF65KPK` and `OFF16MOV` build from the legacy sources. OFF016PK matches the
  V4.10 archive and OFF32KFL the V4.11 Release byte-for-byte; the other four
  have no byte-identical reference and are verified structurally and at runtime.
  `mise run test:osd-<driver>-neost` checks each requested layout, readbacks
  and full bitmap, plus a missing-driver control. OFF16MOV asserts its 16 KB
  bitmap and the word-swapped TrueColor readbacks; OFF65KPK asserts its 5/6/5
  packed readbacks, 8192-byte bitmap and V4.11 header.
- [x] **COPS.APP** — add `cops` producing the application-mode control-panel
  host from the original C sources with MiNTLib/GEMlib; compile the embedded
  resource and normalize `CLEARCPU.S` through the MAS converter.
  `test:cops-neost` disables the accessory, opens `MAGXCONF.CPX` and
  `TSLICE.CPX` through the panel, and matches the panel and both dialogs
  pixel-for-pixel against the archive. Accessory mode, board editing,
  Save/alerts and event-CPX callbacks remain untested.
- [x] **KEYB_SYS** — add `keyb-sys` plus individual `keyb-cz`…`keyb-us`
  targets for all six keyboard-table generators. Keep the Atari-encoded
  sources and compile with explicit Latin-1 charsets; map `<tos.h>` to
  `<mint/osbind.h>`; replace three verified `'0x'` multi-character constants.
  `test:keyb-sys-neost` runs each archived/rebuilt pair in separate EmuTOS
  guests and requires byte-identical `KEYTABLS.SYS` files, expected lengths and
  six distinct layouts. Live keyboard-driver loading is untested.
- [x] **MAGXBOOT** — add `magxboot` producing the shipped TOS, the identical
  `MAGXBOOT.PRG` and the `OUTSIDE` `MAGXBO32.PRG` from one VASM recipe. The
  rebuild matches both references except two `adda.w #12,sp` encodings VASM
  rewrites to `lea 12(sp),sp`. `test:magxboot-neost` boots shipped, rebuilt and
  missing-loader clones; present loaders render an identical MAGXDESK screen and
  write the probe marker, and the missing case reaches neither. Other machines,
  TT-RAM starts and reset residency are untested.
- [x] **HARDCOPY** — add `hardcopy` (archived TOS flags) and
  `hardcopy-test-driver`; the rebuild matches the reference except six
  `adda`/`suba` sites VASM shortens to `lea`/`subq` (14 bytes), plus size-neutral
  PC-relative and `asl`/`add` encoding choices. `test:hardcopy-neost`
  installs archived, rebuilt and missing cases in separate clones: present
  builds replace `scr_dump`, the missing launch fails and leaves it intact, and
  screens match. Printer output and the forced dump are untested (emulator
  capture/printer limits).
- [x] **MFA/MFM screen drivers** — all twelve canonical outputs build and pass
  structural checks. NeoST verifies archived/rebuilt ST high/medium/low boots
  and missing-driver rejection. See the deferred runtime work above.
- [x] Account explicitly in the catalogue for remaining diagnostic, historical,
  host-specific, missing-source and external-dependency entries.

Keep compiler options and the known-working kernel steady during these ports.
MOD_APP's first MiNTLib binary is substantially larger than the Pure C original;
size/runtime-library experiments remain separate from this phase's compatibility
work. Distribution packaging and localization follow the roadmap, not this list.
