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
- [x] Verify the actual window, timer/redraw and move/close events under NeoST;
  the EmuTOS desktop returns after the close box is clicked.
- [ ] Verify coexistence with other applications under the MagiC kernel; NeoST
  confirms the EmuTOS window lifecycle, not MagiC multi-process behavior.

## Build conventions to confirm as consumers appear

- [ ] Confirm another application's startup requirements. MOD_APP's tokenizer
  is intentionally private; extract shared quote/ARGV compatibility only if a
  second real consumer needs it and the combined contract is tested.
- [ ] Preserve quoted paths, escaped apostrophes, empty arguments, long ARGV
  arguments and literal file-type patterns where the application supports them.
  A link against a modern CRT does not automatically preserve old startup behavior.
- [ ] Establish GUI structure/message widths using compatible SDK types. Do not
  restore Pure C's `int` assumptions globally with kernel ABI flags.
- [ ] For the first consumer that needs them, map GEM utility/window helpers,
  multitasking AES/DOS bindings and SLB bindings to compatible source builds or
  established SDK equivalents. Record actual missing dependencies.
- [ ] For the first resource-bearing port, define its RSC/data inputs and runtime
  search paths. Preserve existing translations/encoding; translation repair is
  a later phase.
- [ ] Build selected targets in fresh output directories and record prerequisites
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
- [ ] Keep crash probes, installers, header patchers and formatters out of the
  default smoke set. Give them disposable fixtures and explicit test targets.
- [ ] Record compiled, linked, packaged, launched and functionally tested status
  separately in the catalogue as each port advances.

## Following batches

- [ ] Small helpers/background programs: SHUTDOWN, WBDAEMON, VFATCONF, MGXCLOCK.
- [ ] Desktop helpers: APPLICAT, MGVIEW, MGEDIT, MGNOTICE version 2, MGSEARCH.
- [ ] Larger/threaded programs: MGCOPY, MGFORMAT, VT52, MAGXDESK version 5.
- [ ] Separate assembly builds: MCMD, MAGXCONF, FLP_PAR and appropriate utilities.
- [ ] Loadable components: editor/image/window-frame SLBs, CPXs and drivers.
- [ ] Account explicitly for remaining diagnostic, historical, host-specific,
  missing-source and external-dependency entries from the catalogue.

Keep compiler options and the known-working kernel steady during these ports.
MOD_APP's first MiNTLib binary is substantially larger than the Pure C original;
size/runtime-library experiments remain separate from this phase's compatibility
work. Distribution packaging and localization follow the roadmap, not this list.
