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

After MGSEARCH, the core source-backed remainder is 40 build families: 24
application/tool/extension families, 9 shared library/startup families and 7
system-component families. Historical, test-only, blocked, binary-only and KAOS
entries are outside that count.

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
- [ ] Desktop helpers: MGSEARCH.
- [ ] Larger/threaded programs: MGCOPY, MGFORMAT, VT52, MAGXDESK version 5.
- [ ] Separate assembly builds: MCMD, MAGXCONF and appropriate utilities;
  FLP_PAR is a C pair, not an assembly family.
- [ ] Loadable components: image/window-frame SLBs, CPXs and drivers.
- [x] Account explicitly in the catalogue for remaining diagnostic, historical,
  host-specific, missing-source and external-dependency entries.

Keep compiler options and the known-working kernel steady during these ports.
MOD_APP's first MiNTLib binary is substantially larger than the Pure C original;
size/runtime-library experiments remain separate from this phase's compatibility
work. Distribution packaging and localization follow the roadmap, not this list.
