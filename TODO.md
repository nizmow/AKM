# Phase 1 TODO

Concrete next work after the first standalone application port. Direction and
phase boundaries remain in [ROADMAP.md](ROADMAP.md); the full component list is
[APPLICATION_BUILDS.md](APPLICATION_BUILDS.md). Do not duplicate that inventory
here or interpret an unchecked item as a missing stub to manufacture.

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

## Following batches

The current core source-backed remainder is 27 build families: 14
application/tool/extension families, 7 shared library/startup families and 6
system-component families. Historical, test-only, blocked, binary-only and KAOS
entries are outside that count.

Shared families are consumer-driven, not a library-first batch. Each enters with
its first concrete consumer and a runtime check; the first-consumer boundaries
are recorded in [APPLICATION_BUILDS.md](APPLICATION_BUILDS.md) §6. Do not build
the remaining shared families ahead of their consumers.

- [x] **VFATCONF** — add `vfatconf` producing `VFATCONF.PRG` plus its required
  resource; launch reaches the dialog under MagiC, but configuration changes
  remain untested.
- [x] **MGXCLOCK** — add `mgxclock` and a registration test driver; verify clock
  updates and writeback daemon registration on a cloned MagiC disk. Actual
  disk-flush timing remains unmeasured.
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
  `MAGXDESK.5/PATTERN.C` copy, relying on modern GEMlib instead of `MAGIXLIB`.
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
- [x] **OSD phase 2** — `OFF016PK`, `OFF256PK`, `OFF32KFL`, `OFF32KPK` and
  `OFF16MOV` build from the legacy sources. OFF016PK matches the V4.10 archive
  and OFF32KFL the V4.11 Release byte-for-byte; the other three have no
  byte-identical reference and are verified structurally and at runtime.
  `mise run test:osd-<driver>-neost` checks each requested layout, readbacks
  and full bitmap, plus a missing-driver control. OFF16MOV asserts its 16 KB
  bitmap and the word-swapped TrueColor readbacks.
- [x] **COPS.APP** — add `cops` producing the application-mode control-panel
  host from the original C sources with MiNTLib/GEMlib; compile the embedded
  resource and normalize `CLEARCPU.S` through the MAS converter.
  `test:cops-neost` disables the accessory, opens `MAGXCONF.CPX` and
  `TSLICE.CPX` through the panel, and matches the panel and both dialogs
  pixel-for-pixel against the archive. Accessory mode, board editing,
  Save/alerts and event-CPX callbacks remain untested.
- [ ] **MFA/MFM screen drivers** — scope and reference binaries are in the
  [build catalogue](APPLICATION_BUILDS.md#2-system-components-outside-the-application-tree).
  - [x] Resolve the 12 canonical `.PRJ` inputs, includes and archived `.SYS`
    references. Exclude `MFM/ORIG`; `MFM4.SYS` is named by the loader but has no
    project or binary in the tree.
  - [x] Establish the archived `MFA2.SYS` baseline on a cloned MagiC image
    under NeoST with a mono monitor: the desktop appears; removing `MFA2.SYS`
    alone reaches `Bildschirm-Treiber nicht gefunden.` and the MCMD shell.
  - [ ] Find a MagiCMac host exposing the `VDI_SETUP_DATA` display ABI and
    establish the equivalent `MFM2.SYS` baseline and missing-driver control.
    NeoST's Atari profiles cannot exercise this path; absence of the host
    blocks MFM runtime proof, not MFM assembly.
  - [x] Build `mfa2` and `mfm2` as representative `Makefile.apps` targets.
    Check TOS relocations, the `NVDIDRV` header, entry offsets and organization
    against each archive. Compare bytes only for equivalent references.
  - [x] Repeat the archived MFA2 baseline with the rebuilt driver under the same
    guest filename: archive and rebuild render an identical 640x400 screen.
  - [x] Add the remaining canonical `mfa`/`mfm` family outputs. All twelve
    `.SYS` files build and pass header, organization and relocation checks.
  - [x] Exercise ST high/medium/low (`MFA2`/`MFA4`/`MFA16`): archived and
    rebuilt screens match; each missing file fails before the MagiC boot probe.
  - [ ] Cover `MFA256`/`MFA32K` with compatible TT/Falcon profiles and MFM with
    matching MagiCMac display formats where available. Record built, load-proven
    and functionally tested status separately in the catalogue.
- [x] Account explicitly in the catalogue for remaining diagnostic, historical,
  host-specific, missing-source and external-dependency entries.

Keep compiler options and the known-working kernel steady during these ports.
MOD_APP's first MiNTLib binary is substantially larger than the Pure C original;
size/runtime-library experiments remain separate from this phase's compatibility
work. Distribution packaging and localization follow the roadmap, not this list.
