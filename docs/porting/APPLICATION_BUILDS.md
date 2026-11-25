# Standalone application and component builds

A porting catalogue, not a claim that these programs already build with GCC.
The root `Makefile` builds the kernel. The applications used
in the successful Hatari kernel tests were the existing release binaries.
`Makefile.apps` independently builds the application/helper/library targets
marked as builds or verified in the status tables below. Other entries are
planning items unless explicitly marked; source presence alone is not a
completed port.

**Recommendation:** build programs independently, with shared library/resource
prerequisites, then assemble an installation from selected outputs. Do not make
one giant application link or require rebuilding the desktop for every utility.
See [PORTING_NOTES.md](PORTING_NOTES.md) for the workflow and known pitfalls.

## Scope and conventions

- Every top-level directory in `MagiC/APPS` is accounted for below, including
  old versions, platform-specific programs, libraries and binary-only packages.
  Companion tools, extensions and historical/host projects are separate sections.
- Output names come from `.PRJ` files, READMEs or archived binaries. A name seen
  only in a binary archive does **not** establish a reproducible source build.
- **Source found** means source exists, not that every dependency is present.
  Old project files contain absolute developer paths and Pure C/Turbo C objects.
- Languages/resources and PRG/ACC modes should be parameters of a shared recipe
  where possible. Keep different versions/platforms in distinct output folders:
  several projects intentionally produce the same filename.
- Priorities are suggested sequencing, not difficulty estimates: **A** bootstrap
  a small independent program; **B** expand the usable system; **C** larger or
  special-format integration; **H** historical/host-specific/optional;
  **Blocked** no matching source found or explicitly unfinished.
- Each executable in a comma-separated output list needs its own output, even
  when one source file or recipe serves the whole family.

## 1. Main applications and desktop helpers

| Priority | Application / independent output | Source root | Purpose and build considerations |
|---|---|---|---|
| A / verified | `MOD_APP.TTP` | `MagiC/APPS/MOD_APP` | Command-line registration of file types/icons in `APPLICAT.INF`. GCC/MiNTLib build: `mise run apps -- mod-app`. Passed 17 Hatari comparison cases, including quoted arguments, backups and cache invalidation. Test against disposable INF files. |
| A / verified | `SHOWFILE.TTP` | `MagiC/APPS/SHOWFILE` | Older text-mode viewer, predecessor of MGVIEW. GCC/MiNTLib build: `make -f Makefile.apps showfile`. NeoST Pexec comparison matched sample text, EOF prompt, missing-file/no-argument behavior and child statuses against the archived binary. Keep as a distinct utility, not a second build of MGVIEW. |
| A / compared | `LIMITMEM.TTP` | `MagiC/APPS/LIMITMEM` | Builds with `make -f Makefile.apps limitmem`. NeoST comparison used disposable PRG copies; console output, marker creation and error cases matched the archive. EmuTOS's legacy `Fshrink` operation returns -ERANGE and leaves the removal fixture unchanged in both binaries; successful removal under MagiC and non-English runtime localization remain unverified. |
| A / verified | `MGCLOCK.PRG` | `MagiC/APPS/MGCLOCK` | Builds with `mise run apps -- mgclock`. Restores the original desktop-drawing clock loop using MiNTLib GEMlib AES/VDI bindings. `mise run test:mgclock-neost` verifies timer updates under EmuTOS and on cloned MagiC 6.20/TOS 2.06 images: MGCLOCK updates the desktop clock, APPLICAT alone stays static as a control, and the combined run differs from APPLICAT only inside the clock rectangle. CPU load, long-term stability and other desktops remain unmeasured. |
| B / builds, MagiC-tested | `WBDAEMON.PRG` | `MagiC/APPS/WBDAEMON` | Builds with `mise run apps -- wbdaemon`. Uses a word-sized `SC_WBACK` subcommand rather than MiNTLib's generic long argument. Under NeoST 0.6.5/MagiC 6.20/TOS 2.06 on a cloned disk, the guest regression probe verified registration, `AP_TERM` exit and release of the daemon slot. Periodic disk-flush timing remains unmeasured. |
| B / builds, MagiC-tested | `SHUTDOWN.PRG` | `MagiC/APPS/SHUTDOWN` | Builds with `mise run apps -- shutdown` using MiNTLib GEMlib and public MiNT bindings. The final checked `Ssync` aborts with a localized write-error alert instead of reporting success or resetting. Under NeoST with MagiC 6.20/TOS 2.06 and the rebuilt binary installed as `GEMSYS\GEMDESK\SHUTDOWN.PRG`, a server `Ctrl-Alt-Del` chord reached the successful-shutdown alert; a throwaway forced-error build showed the write-error alert and stayed at the desktop. Restart/cold-boot choices were not exercised. Under EmuTOS 1.04 it reaches the shutdown-completion timeout; the archived binary stops at unsupported AES function 130, so that is not a parity result. |
| B / builds, MagiC-launched | `VFATCONF.PRG` | `MagiC/APPS/VFATCONF` | MagiC VFAT long-filename configuration; `make -f Makefile.apps vfatconf` builds the GCC/MiNTLib executable and packages the default German `vfatconf.rsc`. Its dialog launched from a disposable GEMDOS drive under NeoST with a cloned MagiC disk; drive-state and `MAGX.INF` changes remain untested. |
| B / builds, MagiC-tested | `MGXCLOCK.PRG` | `MagiC/APPS/MGXCLOCK` | Desktop clock plus writeback daemon; `mise run apps -- mgxclock`. Uses GEMlib and MagiC's word-sized writeback subcommand instead of the removed legacy `Dcntl` command. Advertises `AP_TERM` and exits on it, so the registered daemon does not block shutdown. Under NeoST 0.6.5/MagiC 6.20/TOS 2.06 on a cloned disk, screenshots verified clock updates and the probe verified registration, `AP_TERM` exit, slot release and persistence of a cache-written file. Actual periodic-flush timing remains unmeasured. |
| B / builds, MagiC-tested | `APPLICAT.APP` | `MagiC/APPS/APPLICAT` | File-type/icon association editor; `mise run apps -- applicat` packages the executable, default German `applicat.rsc` and a self-contained starter `applicat.inf`. Under NeoST/MagiC, `-c` produced archive-compatible cache data excluding runtime icon pointers; the UI added and saved an association with independent INF/DAT checks. Icon rows now use the AES word-aligned stride for both `G_ICON` and `G_CICON`; 32/48-wide fixtures are unchanged. Path/special-icon editing, icon drag/drop and other resource languages remain untested. |
| B / builds, MagiC-tested | `MGVIEW.APP` | `MagiC/APPS/MGVIEW` | `mise run apps -- mgview` builds the self-contained text viewer with public MiNTLib/GEMlib and a bounded C replacement for its Pure C register-ABI delimiter scanner. Six archived/rebuilt NeoST/MagiC scenarios (LF, CRLF, Mac CR, quoted spaced path, true ARGV, empty file) exited successfully with unchanged fixtures and 42 matching content-region screenshots across rendering/navigation/tab/scroll states. Spaced paths require VFAT enabled on the fixture drive. Other display modes, window resize/full, multiple-window cycling and IPC remain untested. Archived `VIEW.PRG` is a separate historical binary, not this output. |
| B / builds, MagiC-tested | `MGEDIT.APP` | `MagiC/APPS/MGEDIT` | `mise run apps -- mgedit` builds the editor, unchanged German `mgedit.rsc` and independently rebuilt `EDITOBJC.SLB`. Public SDK bindings replace archived SLB/GEM utility objects. Six archived/rebuilt NeoST/MagiC cases cover edit/save, selection/clipboard cut/paste, options persistence, New/Save As, quoted spaced paths and true ARGV; file effects match and 14 full UI states are pixel-identical. The separator literal retains the Atari F6-FA bytes; word navigation was not rerun. Other display modes, font/colour selection and IPC remain untested. |
| B / builds, MagiC-tested | `MGNOTICE.APP` version 2 | `MagiC/APPS/MGNOTICE.2` | Newer notes application. `mise run apps -- mgnotice` builds `MGNOTICE.APP`, the German `mgnotice.rsc` and a copy of the independently built `EDITOBJC.SLB`. The shared `MagiC/APPS/GEMUT_MT_SDK.H` and `GEMUT_MT.C` helper replace the archived GEM utility objects. Under NeoST/MagiC, archived and rebuilt binaries produced identical initial/new/typed/escaped/quit screens, byte-identical 75-byte `.NOT` files containing `hello`, and identical reopened-note screens with the note window visible. Delete, font/colour/options, multiple-window and alternate-resource paths remain untested. |
| B / builds, MagiC-tested | `MGSEARCH.APP` | `MagiC/APPS/MGSEARCH` | `mise run apps -- mgsearch` builds the application and German `MGSEARCH.RSC`. It compiles an app-local `GEMUTILS`/`WINDOWS` variant and a copy of `core/magxdesk/SRC/PATTERN.C`; the pattern helper is renamed `mg_toupper` to avoid MiNTLib's exported `toupper`. Modern GEMlib replaces the old `MAGXLIB.LIB` calls. Under NeoST/MagiC, archived and rebuilt dialog and `*.ZZZ` result screens matched pixel-for-pixel; the fixture included a nested directory. Other drive/path forms, filters and file-open actions remain untested. |
| C / builds, MagiC-tested | `MGCOPY.APP` | `core/mgcopy` | Desktop copy/move/alias/delete helper; `mise run apps -- mgcopy` builds the app and German resource. Uses the shared `GEMUT_MT.C` helper, public MiNTLib/GEMlib dialog calls and an app-local DOS compatibility header. `mise run test:mgcopy-neost` checks rebuilt Pexec statuses and seeded copy/move/alias/delete effects, including nested trees and a D:→C: cross-drive move. Destinations are staged and published with rollback; a disposable-floppy probe proved disk-full replacement keeps the old destination and source and returns a nonzero status, while the archived binary destroyed both and returned zero. INF geometry round-trips unchanged. Cancellation, dialog-confirm, long-path and read-only paths remain untested. |
| C / builds, MagiC-started | `MGFORMAT.PRG` | `MagiC/APPS/MGFORMAT` | Floppy format/copy helper; `mise run apps -- mgformat` builds the executable and German resource. Uses the shared `GEMUT_MT.C` helper, native MagiC `wdlg` AES 160-167 bindings, a separate format/diskcopy thread and low-level floppy calls. `mise run test:mgformat-neost` verifies the archived/rebuilt format dialog under MagiC. Port ABI fixes use 16-bit Flopfmt sector IDs, a floppy `Rwabs` record word of 1, native `err_to_str` at DOSVARS offset 50 through a D0/A0 register bridge, and `WORD[8]` AES messages; object code was checked, but NeoST reports a write fault for physical formatting, so real media and UI-driven logical Init remain manual-hardware tests. |
| B / builds, MagiC-tested | `VT52.PRG` | `MagiC/APPS/VT52` | Terminal emulator. C plus VASM assembly, public GEMlib/VDI calls and source-built `DRAGDROP`. `mise run apps -- vt52` builds the executable and German resource. `Puntaes` uses the long `'AnKr'` magic and word subfn; VTSYS callbacks are register bridges; BIOS output chains when no VT52 window owns the caller. `mise run test:vt52-neost` launches a delayed TOS child through the BIOS console hook and compares the final archived/rebuilt terminal screens pixel-for-pixel. A disposable guest child also verified registration, cooked output/input, LINES/COLUMNS and window inheritance; low-memory vector reads and non-client chaining remain source-verified. Interactive keyboard/mouse terminal use remains untested. |
| B / builds, MagiC-tested | `MAGXDESK.APP` version 5 | `core/magxdesk` | Source-built GCC/MiNTLib target: `make -f Makefile.apps magxdesk`; packages unchanged German resources and does not use archived Pure C objects/libraries. `mise run test:magxdesk-neost` runs archived and rebuilt apps on separate cloned MagiC images and confirms pixel-identical desktop content. Cache buffers use GEMDOS `Malloc`/`Mfree`; the shell size is zero-extended; Save is bounded and rejects oversized INF files. A `#_BUF 9000` boot and 12 live APPLICAT cache saves left GEMDOS free memory unchanged and the desktop responsive; >32 KiB Save and shell-buffer Save paths remain source-verified. Version 4 is separate and untouched. |
| H | `MAGXDESK.APP` version 4 | `MagiC/APPS/MAGICDESK/MAGXDESK.4` | Older desktop source tree with a separate project. Do not overwrite the version-5 output. |
| H | `MGNOTICE.APP` version 1 | `MagiC/APPS/MGNOTICE` | Older notes implementation. C; preserve as an explicit historical variant, not the default. |
| H | `PRINT.TTP` | `MagiC/APPS/PRINT` | Text printing to an Epson-compatible printer. C; verify via captured printer output rather than physical hardware. |
| H | `MATRIX.PRG` | `MagiC/APPS/MATRIX` | Matrix/linear-equation GEM application. C; floating-point library and old Turbo C startup/library dependencies; German UI. |
| H | `BALLER.PRG` | `MagiC/APPS/BALLER` | Ballerburg. C, floating point, sound and data/resources. README documents the Lattice-to-Pure-C type changes and single-application-mode requirement; not a trivial generic GEM sample. |
| H | `INSTMAGC.PRG`, `RAWINST.TTP` | `MagiC/APPS/INSTMAGC` | Atari MagiC installer and raw-install companion. C sources include `RAWINST.C`, though the main PRJ builds only the installer. Disk/boot-sector/serial-number behavior needs dedicated review and scratch images. |
| H | `MGEDIT7.APP` | `MagiC/APPS/MGEDIT7` | Mac-integrated editor variant. C, Mac menus/navigation/API dependencies; not an ordinary Atari replacement for MGEDIT. |
| H | `MMXDAEMN.PRG` | `MagiC/APPS/MMXDAEMN` | MagiCMac X event daemon. C; depends on host-specific event/command interfaces. Keep out of the default Atari installation. |
| Blocked | `CHGRES.PRG` standard variant | `MagiC/APPS/CHGRES` | Resolution-change helper, archived program/resources but no C/assembly sources found in this directory. Its README appears copied from APPLICAT; do not infer its purpose from that text. |
| Blocked | `CHGRES.PRG` ET4000 variant | `MagiC/APPS/CHGRES.ET4` | TT/ET4000 graphics variant. README explicitly says no source. Preserve supplied binaries separately; do not claim a GCC port. |
| Blocked / host | `CHGRES.PRG` MagicOnLinux variant | `MagiC/APPS/CHGRES.MOL` | C/assembly project exists, but README explicitly marks it draft/incomplete; root `CHGRES.RSC` is empty. Inspect localized resources and host interface before treating it as deliverable. |

### Assembly program families and control-panel modules

These are standalone builds too, even though some do not involve GCC at all.

| Priority | Outputs | Source root | Variants / special handling |
|---|---|---|---|
| B / builds, MagiC-tested | `MCMD.TOS` (DE/EN/FR) | `core/mcmd` | `make -f Makefile.apps mcmd` builds German; `MCMD_LANG=en`/`fr` selects isolated outputs and `mcmd-languages` builds all three. VASM emits native TOS executables from normalized sources without archived MAS/ALN tools or objects. All three localized builds launched under NeoST/MagiC; archived/rebuilt batch runs matched child status and 16 guest files per language, with independent quoting/expansion, redirection, file/directory and child-execution assertions. Interactive editing, pipes, long filenames and AUTOEXEC/BOOT startup remain untested. |
| H | `KCMD.TOS`, `CMD.TOS`, accessory mode | `MagiC/APPS/CMD` | Historical variants and documentation; the shared active assembly family now lives in `core/mcmd/SRC`. Its source switches can target KAOS/TOS or accessory mode. Do not infer that these are validated because MCMD works. Bundled `MAS.TTP`/`ALN.TTP` are old build tools, not outputs of this project. |
| B / builds, MagiC-tested | `MAGXCONF.PRG` / `MAGXCONF.ACC` (DE/EN/FR) | `MagiC/APPS/MAGXCONF` | `make -f Makefile.apps magxconf` builds German; `MAGXCONF_LANG=en`/`fr` selects a language and `magxconf-languages` builds all six outputs. Native VASM TOS executables use the original basepage-based PRG/ACC detection, shared assembly includes and embedded resources; no archived objects or C ABI conversion. The GUI scenario checks Cancel/OK/Save masks, the four-byte self-save and accessory reboot persistence, with the competing MAGXCONF CPX disabled only in clones. Coverage and resource-caption differences are recorded in porting notes. Distinct from `MAGXCONF.CPX`. |
| B / builds, MagiC-tested | `FLP_PAR0.PRG`, `FLP_PAR1.PRG` | `MagiC/APPS/FLP_PAR` | `mise run apps -- flp-par` builds separate executables under `build/atari/apps/flp-par/`. The sources are ported in place to MiNTLib's public `Sconfig`; `SCB_FLPAR` is `0x800`. Archived and rebuilt binaries on cloned MagiC 6.20/TOS 2.06 images each passed the same NeoST 0.6.5 Pexec probe: `00000000 → 00000800 → 00000000`, zero child statuses and no other configuration bits changed. Parallel floppy hardware operation remains untested. |
| B / builds, MagiC-tested in application and accessory mode | `COPS.APP`; `COPS.ACC` | `MagiC/APPS/COPS` | `mise run apps -- cops` builds the control-panel host with MiNTLib/GEMlib, embedded resources and normalized `CLEARCPU.S`, without `EMU_DBG.LIB`. `mise run test:cops-neost` disables the accessory and compares the application panel and `MAGXCONF.CPX`/`TSLICE.CPX` dialogs against the archive. `mise run test:cops-accessory-neost` boots Release, rebuilt and missing `AUTO/ACCS/COPS.ACC`; the menu item, panel open/close/reopen and TSLICE dialog match. Board editing, Save/alerts, event-CPX callbacks and other CPXs remain untested. |
| C / builds, MagiC-tested | `MAGXCONF.CPX` | `MagiC/APPS/CPX` | `mise run apps -- magxconf-cpx` builds the classic 512-byte header plus relocatable TOS module. The original source is ported in place; local CPX ABI headers replace the absent XCONTROL SDK, and VASM shims supply 16-bit callback arguments. Under NeoST/MagiC with COPS, a scratch saved default was applied at boot and the opened dialog was pixel-identical to the archived module. `mise run test:magxconf-cpx-neost`. Save-button alerts, other CPX managers and other languages remain untested. |
| A / builds, MagiC-tested | `TSLICE.CPX` | `MagiC/APPS/CPX` | `mise run apps -- tslice-cpx` builds the classic 512-byte header plus relocatable TOS module. The source is ported in place to UTF-8 with public GEMlib calls; classic slider/mouse callbacks use packed 16-bit argument bridges, and `AESVARS.ctrl_timeslice` uses a dedicated stack-to-D0 bridge. `mise run test:tslice-cpx-neost` compares the archived/rebuilt dialogs opened through COPS on cloned MagiC images. Slider dragging, Save and scheduler side effects remain untested. |
| Binary package | `COPS.ACC`, `WDIALOG.PRG`, `CLOCK.CPX` | `MagiC/APPS/COPS108.GER` | Distributed COPS 1.08 package, no C/assembly source found in this directory. Source COPS above is a separate tree; do not assume exact binary/source correspondence or turn bundled components into fake source targets. |
| Dependency | `EMU_DBG.LIB` (legacy name) | `MagiC/APPS/EMUDBG.LIB` | C/assembly debug-output library with MagicOnLinux support and stdout fallback. Rebuild a compatible library for its consumers, not an application. COPS now builds without it because its debug calls are compiled out; the library still has no consumer. |

## 2. System components outside the application tree

These are separate artifacts needed for a fully rebuilt installation; they do
not belong in an application link or automatically in `MAGIC.RAM`.

| Component / outputs | Source | Build boundary |
|---|---|---|
| `MAGXBOOT.TOS`, `MAGXBOOT.PRG`, `MAGXBO32.PRG` (B / built, NeoST-tested) | `core/magxboot` | One assembly loader with `OUTSIDE`/`RESIDENT`/`MODIFY` switches. `mise run apps -- magxboot` assembles the shipped TOS and the `OUTSIDE` variant (`-DOUTSIDE=1`, 32-KB page alignment and the virtual-memory title) with VASM `-Ftos -tos-flags=7`; `MAGXBOOT.PRG` is the same binary as the TOS. The rebuild matches both references in header, TEXT/DATA/BSS (942/0/800 and 1016/0/800), flags and relocations, and differs only in two `adda.w #12,sp` encodings that VASM rewrites to `lea 12(sp),sp`. `mise run test:magxboot-neost` boots archive, rebuilt and missing loader on separate clones; both present cases must write `C:\SCREEN.TXT` and render an identical MAGXDESK screen, and the missing-loader case must reach neither. Other machines, TT-RAM starts and reset residency remain untested. |
| `HARDCOPY.PRG` (B / built, NeoST-tested) | `MagiC/KERNEL/XTENSION/HARDCOPY.S` | Separate assembly hardcopy extension. `mise run apps -- hardcopy` assembles it with VASM `-Ftos -tos-flags=1` from the normalized source; the archived binary is byte-identical to the Release `EXTRAS` copy. The rebuild has a well-formed header with TEXT/DATA/BSS 2652/0/0, flags 1 and a relocation stream; against the archive's 2666-byte TEXT it is 14 bytes smaller, entirely within TEXT, because VASM selects shorter `lea`/`subq` forms for six `adda`/`suba` operands; it also encodes label operands PC-relatively instead of absolute and `asl.w #1` as `add.w`, without changing size. `mise run test:hardcopy-neost` installs each build through a probe and requires status 0 and a changed `scr_dump`; a missing extension must launch nothing and leave the hook intact, with identical desktop screens. Dump execution and printer bytes are not verified: NeoST cannot capture this installation's printer path, and forcing a dump raises a guest exception with the archived binary too. |
| `WINFRAME.SLB` + `WINFRAME.RSC` (B / library built, MagiC-tested) | `MagiC/KERNEL/WINFRAME` | `mise run apps -- winframe` builds the alternate AES window-frame manager and copies its resource. Public GEMlib AESPB/AES registration replaces the legacy `mt_aes` call; the SLB header, lifecycle entries and classic TOS relocations are packaged independently. `mise run test:winframe-neost` compares the booted dialog frame against the archive, with WINFRAME enabled and disabled. |
| `WINFRAME.SLB` demo variant (B / library built, MagiC-tested) | `MagiC/KERNEL/WINFRAME.DEM` | Separate example project producing the same filename, its own 16-object `WINFRAME.RSC` and 18-pixel title height. `mise run apps -- winframe-demo` writes it to `build/atari/apps/winframe-demo/` and reuses the production register-to-stack bridges. `mise run test:winframe-demo-neost` boots archived and rebuilt pairs with the demo resource and a disabled control. |
| `MFA2.SYS`, `MFA4.SYS`, `MFA16.SYS`, `MFA256.SYS`, `MFA32K.SYS` (B / built, structurally verified; ST modes runtime-tested) | `core/mfa` | Five screen drivers (header 3.13) from one recipe family. Legacy `.PRJ` files and include strings are retained as references; the modern build normalizes the includes and uses the shared VDI definitions. Archives are `MagiC/KERNEL/VDI/DRIVERS/ATARI/*.SYS`, byte-identical to the `MagiC/Releases/MAGIC.A/GEMSYS/` copies. `mise run apps -- screen-drivers` builds all twelve MFA/MFM outputs. `mise run test:screen-drivers-neost` checks every output structurally and proves archived/rebuilt boots plus missing-driver rejection for `MFA2.SYS`/`MFA4.SYS`/`MFA16.SYS` on ST high/medium/low. TT/Falcon modes remain unverified. |
| `MFM2.SYS`, `MFM16.SYS`, `MFM16IP.SYS`, `MFM16M.SYS`, `MFM256.SYS`, `MFM32K.SYS`, `MFM4IP.SYS` (B / built, structurally verified) | `MagiC/KERNEL/VDI/SRC/MAGIC.DRV/MFM` | Seven Macintosh screen drivers (header 3.13; MFM4IP 3.14). Archives are `MagiC/KERNEL/VDI/DRIVERS/MAC/*.SYS`; the Release tree has no MFM copies. `MFM4.SYS` is requested by `load_MAC_driver` but has neither project nor binary. `ORIG/` is historical, not a product. The shared build and `mise run test:screen-drivers-neost` check their headers, organizations and relocations; no MagiCMac host is available, so runtime is unverified. |
| `OFF002.OSD` (B / built, MagiC/NVDI-tested) | `core/osd` | The V4.10 source builds with VASM `-Ftos -nosym -tos-flags=7`; output is byte-identical to `MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF002.OSD`. `mise run test:osd-off002-neost` installs it in a disposable image, opens a 1-plane MFDB through `v_opnbm`, draws and verifies the bitmap. The shipped Release is V4.11 with different text/data and entry offsets, so it is a structural comparison, not a byte reference. |
| `OFF004IP.OSD` (B / built, MagiC/NVDI-tested) | `core/osd` | The V4.10 source builds with the same flags plus `-no-opt`, which preserves the archive's `movem` encoding; output is byte-identical to `MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF004IP.OSD`. `mise run test:osd-off004ip-neost` runs archive and rebuild in separate clones, opens a 2-plane interleaved MFDB through `v_opnbm`, draws with pens 2 and 3, and compares pixel readbacks and the 1024-byte bitmap with an independent expectation. Release V4.11 remains a structural difference. |
| `OFF016IP.OSD` (B / built, MagiC/NVDI-tested) | `core/osd` | The V4.10 source builds with the same flags plus `-no-opt`; output is byte-identical to `MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF016IP.OSD`. `mise run test:osd-off016ip-neost` runs archive and rebuild in separate clones, requests a 16-colour/4-plane interleaved MFDB through `v_opnbm`, draws five pens, and compares eleven pixel readbacks and the 2048-byte bitmap with an independent expectation; a clone with the driver deleted must reject the open. The shipped Release driver reports the same header version but has different text/data sizes and entry offsets. |
| `OFF256IP.OSD` (B / built, MagiC/NVDI-tested) | `core/osd` | The V4.10 source builds with the same flags plus `-no-opt`; output is byte-identical to `MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF256IP.OSD` and to the shipped Release copy. The source folds the zero-displacement colour-map load to `movea.l (a2),a0`, matching the archive's encoding. `mise run test:osd-off256ip-neost` runs archive and rebuild in separate clones, requests a 256-colour/8-plane interleaved MFDB through `v_opnbm`, draws ten pens, and compares twenty-one pixel readbacks and the 4096-byte bitmap with an independent expectation; a clone with the driver deleted must reject the open. |
| `OFF016PK.OSD`, `OFF256PK.OSD`, `OFF32KFL.OSD`, `OFF32KPK.OSD`, `OFF16MOV.OSD` (B / built, MagiC/NVDI-tested) | `core/osd` | `mise run apps -- osd-batch` builds these five plus `OFF65KPK` (next row). OFF016PK matches its V4.10 source-tree archive and OFF32KFL the V4.11 Release byte-for-byte; OFF256PK, OFF32KPK and OFF16MOV have no byte-identical reference (different source/archive versions or TEXT/DATA layout) and are checked structurally plus at runtime. `mise run test:osd-off016pk-neost`, `...off256pk...`, `...off32kfl...`, `...off32kpk...` and `...off16mov...` each verify the requested layout, readbacks where reliable and the full bitmap, with a missing-driver control; OFF16MOV asserts its 16 KB bitmap and the word-swapped TrueColor readbacks. |
| `OFF65KPK.OSD` (B / built, MagiC/NVDI-tested) | `core/osd` | 16-bit packed offscreen driver (65,536 colours, 5/6/5 bits, organisation 129). `mise run apps -- off65kpk` builds it; `osd-batch` includes it. The orphan V4.11 source names an absent local `include\*.inc` tree, so its normalized copy uses the shared `kernel/VDI/SRC/MAGICVDI.2` `.I` includes and the VDI.I aliases `REPLACE`/`EX_OR`/`REV_TRANS` plus `planes`. `mise run test:osd-off65kpk-neost` verifies the V4.11 header, eleven pixel readbacks, the 8192-byte bitmap and missing-driver rejection. The checked-in references are different versions (`MagiC/KERNEL/VDI/DRIVERS/OSD` V4.10, Release V4.13), so no byte-identical claim. |

## 3. Build rules for humans and agents

For each selected application, a task should deliver:

1. One independently invokable build target, with its **actual** sources,
   startup code, compatible libraries, resource files and variant switches.
2. Outputs beneath an isolated build directory, never over archived release
   files. No hard-coded developer drive paths or edits to generated outputs.
3. An explicit install bundle: executable plus required RSC/data/help/SLB files,
   preserving the names and relative paths used by the program.
4. A small runtime scenario under the known-working MagiC installation. For a
   destructive utility, use disposable input files/images and test copies only.
5. A status report distinguishing compiled, linked, packaged, launched and
   functionally tested. Mark unresolved dependencies instead of stubbing them.

**Do not copy kernel flags into every application.** Ordinary applications can
use the toolchain's stack ABI and GEMlib, but integer widths, public structures,
assembly callbacks and the chosen library ABI must agree. Pure C/Turbo C `.O`
and `.LIB` files are not automatically reusable in a GCC link. Inventory each
assembly/library boundary before choosing a replacement.

Share infrastructure only after two real consumers establish the requirement:
GEM utility/window helpers, multitasking AES/DOS bindings, startup/ARGV handling,
SLB bindings, resource packaging and localization are recurring dependencies.
Keep one owner for shared build/library changes; independent agents can own
separate applications after the shared prerequisites are defined.

A useful first batch is `MOD_APP`, `SHOWFILE`, `LIMITMEM` and `MGCLOCK`, followed
by `SHUTDOWN`/`WBDAEMON` and a desktop helper. This is a proposed sequence, not
an assertion that those ports are already straightforward or complete.

## 4. Command-line tools, diagnostics and examples

Every `MagiC/TOOLS` directory is accounted for here. Most small utilities have
source but no project file; their archived executable is evidence of an intended
output, not a complete modern recipe.

| Priority / kind | Outputs | Source root | Purpose / constraints |
| A / built, tested | `CMDLINE.PRG` | `MagiC/TOOLS/CMDLINE` | `make -f Makefile.apps cmdline` builds with GCC/MiNTLib. NeoST raw-tail/environment comparison passed. ARGV launch ran with status 0 and matching tail/path; MiNTLib consumes the reserved `ARGV=` marker before `main`, an expected startup difference rather than a failure. |
| A / builds, NeoST-tested | `DUMP.TTP` | `MagiC/TOOLS/DUMP` | C hex dump utility; `make -f Makefile.apps dump` builds it. `mise run test:console-tools-neost` checks default and C-source modes against the archive with unchanged fixtures. |
| A / builds, NeoST-tested | `FC.TTP` | `MagiC/TOOLS/FC` | C file comparison utility with text/hex options; `make -f Makefile.apps fc` builds it. `mise run test:console-tools-neost` covers equal, differing, WORD and LONG modes; the port fixes undefined byte-fetch order without changing the observed output. |
| A / builds, NeoST-tested | `MAC2ST.TTP` | `MagiC/TOOLS/MAC2ST` | C Mac-CR to Atari-CRLF filter; `make -f Makefile.apps mac2st` builds it. Output goes to the console, not a newly created destination file. `mise run test:console-tools-neost` compares archived/rebuilt status, console output and fixtures. |
| A / builds, NeoST-tested | `SPLIT.TTP` | `MagiC/TOOLS/SPLIT` | C file splitter; `make -f Makefile.apps split` builds it. `mise run test:console-tools-neost` uses disposable fixtures to compare named output files and console behavior. |
| B / built, MagiC-tested | `SHOWFREE.TOS` | `MagiC/TOOLS/SHOWFREE` | `mise run apps -- showfree` builds it with MiNTLib startup and a word-width `Sconfig` trap. `mise run test:diagnostic-tools-neost` boots archived and rebuilt copies in `MAGIC\START`; both show the monitor and identical screens. |
| A / builds, NeoST-tested | `DIS1.TTP`, `DIS2.TTP` | `MagiC/TOOLS/DIS` | `mise run apps -- dis` builds both passes with MiNTLib public GEMDOS declarations. Sources are ported in place to UTF-8 with explicit Atari banner bytes; GEMDOS headers and object symbols use fixed 16/32-bit records, and machine words use `int16_t`. `mise run test:dis-neost` compares archived/rebuilt status and `.lbl`/`.rlo`/`.s` files on the bundled `TST` corpus, a recognized DRI object (`PCMXSTRT.O`) and raw/info/68030/MAS/range/error cases. Child console text is not captured by the current NeoST log. `TST` remains a corpus and generated artifacts, not additional default products. |
| B / built, MagiC-tested | `MAGXKERN.TTP` | `MagiC/TOOLS/MAGXKERN` | `mise run apps -- magxkern` builds it with fixed 16-bit kernel records. `mise run test:diagnostic-tools-neost` requires byte-identical archived/rebuilt output. |
| B / built, MagiC-tested | `MEMEXAMN.TTP` | `MagiC/TOOLS/MEMEXAMN` | `mise run apps -- memexamn` builds it with local fixed-width `SYSHDR`/`DOSVARS`/`AESVARS`/`MCB`/`BASPAG` records. The diagnostic test checks the banner, normalized early table, owner tokens and `KROM` terminator; block sizes and addresses differ with the larger rebuild. |
| B / built, MagiC-tested | `CRASHDMP.TOS` | `MagiC/TOOLS/CRASHDMP` | `mise run apps -- crashdmp` builds it against MiNTLib GEMDOS bindings with 16-bit exception-stack words. The diagnostic test covers the no-record path: both binaries exit 0 without output. Exception decoding remains untested. |
| H | `EXE2BIN.TTP` | `MagiC/TOOLS/EXE2BIN` | C executable/header conversion tool used by old build scripts. Mutates inputs; validate on copies. |
| H | `KRYPT.TTP` | `MagiC/TOOLS/KRYPT` | C XOR/update-file tool; preserve format/checksum behavior. |
| H | `FAST_SFX.TTP` | `MagiC/TOOLS/FAST_SFX` | C in-place patcher for a particular self-extracting archive layout. Not a general archive tool. |
| H | `ADDMEM.PRG` | `MagiC/TOOLS/ADDMEM` | Resident assembly memory utility; KAOS/MagiC-version-sensitive. |
| H | `DOS_MON.PRG` | `MagiC/TOOLS/DOS_MON` | Resident assembly DOS trap tracer. Hook, register and exception-frame conventions need review. |
| B / built, NeoST-tested | `KEYB_CZ.PRG`, `KEYB_DEI.PRG`, `KEYB_FRA.PRG`, `KEYB_SWG.PRG`, `KEYB_UK.PRG`, `KEYB_US.PRG` | `MagiC/TOOLS/KEYB_SYS` | Six C keyboard-table generators. `mise run apps -- keyb-sys` builds all six under `build/atari/apps/keyb-sys/`; individual `keyb-cz`…`keyb-us` targets select one. The sources stay Atari-encoded and compile with explicit `-finput-charset/-fexec-charset=ISO-8859-1` so table bytes survive; `<tos.h>` maps to `<mint/osbind.h>`. Three `'0x'` multi-character constants were replaced by the verified `'x'`. `mise run test:keyb-sys-neost` runs archived and rebuilt generators in separate disposable EmuTOS guests and requires matching child status and console output (status 0 and no output in the recorded run), a byte-identical `KEYTABLS.SYS` per locale, the 9×128-byte table block plus each dead-key trailer, and six distinct layouts. `ALL.PRJ` omits `KEYB_DEI`; it is built explicitly. |
| B / library built, MagiC-tested | `LOAD_IMG.SLB` | `MagiC/TOOLS/LOAD_IMG.SLB` | `mise run apps -- load-img` builds the five-export IMG loader with version-1 packed call records and a legacy raster callback adapter. `mise run test:slb-consumer-neost` compares rendered XIMG colours against the source-tree archive. The shipped Release library draws the same fixture with different colours; Release rendering equivalence is not claimed. |
| C / examples | `SLB_DEMO.SLB`, `SLC_DEMO.TOS`, `PH_BIT3.TTP` | `MagiC/TOOLS/SLB_DEMO` | Three projects: server library, client and program-header flag utility. Also contains reusable `SLB_BIND.C`. Test the library/client together; patch only copies with PH_BIT3. |
| H / Mac | `MEN_DEMO.TOS` | `MagiC/TOOLS/MAC_MENU` | C native Mac menu demo; MagiCMac cookie/API and resources, not a plain Atari test. |
| H / Mac | `NAV_DEMO.TOS` | `MagiC/TOOLS/NAVIGATE` | C Mac Navigation Services demo; Mac API/XCMD dependency. |
| H / Mac X | `XCMD.TTP` | `MagiC/TOOLS/MMXTEST` | C XCMD example. Requires the host's XCMD manager and an external sample shared library. |
| H / Mac X | `DEV_SER.TOS` | `MagiC/TOOLS/DEV_SER` | C/assembly resident device-driver example. Although the README is generic, this source uses the MagiCMac X `MgMx` interface; do not treat it as a ready ST serial driver. |
| Test only | `T.PRG` | `MagiC/TOOLS/FPUTEST` | Assembly FPU-state test; needs appropriate FPU hardware/emulation. |
| Test only | `ADR.PRG`, `BOMB.PRG`, `CA_OFF.PRG`, `CA_OFFPD.PRG`, `CA_ON.PRG`, `CA_SHOW.PRG`, `IL0008.PRG`, `IL4AFC.PRG`, `ILF000.PRG`, `MEMDE.PRG`, `NOFAST.PRG`, `PRIV_RTE.PRG`, `PRIV_SR.PRG`, `TRAP7.PRG`, `TST.PRG`, `X.PRG` | `MagiC/TOOLS/MISC_TST` | Individual assembly probes, some CPU/TT/cache-specific. Several intentionally crash or corrupt memory. Keep out of normal installs and automatic smoke suites. |
| Unclassified sources | No established output names | `MagiC/TOOLS/UNKNOWN` | `SUBST.C` and `Z.C` have program entry points; `QS.C` is sorting code without a main program. Investigate before inventing targets or including this folder in a default build. |
| Build infrastructure | No standalone executable | `MagiC/TOOLS/BUILD_AS` | Old MCMD assembler/linker templates expecting external MAS/PASM/linkers. Useful provenance, not applications to port. |

## 5. Loadable extensions and their test applications

| Priority | Outputs | Source root | Prerequisites / gaps |
|---|---|---|---|
| B/C / library built, MagiC-tested | `EDITOBJC.SLB`; companion `EDIT.PRG` test | `MagiC/EXTENSIO/EDIT_SLB` | `mise run apps -- edit-slb` independently builds the real editor library with VASM register-to-stack AES bridges and classic TOS relocation packaging; the archived MGEDIT verified editing, clipboard and options behavior before the rebuilt client was tested. The bundled library is not a retained release binary. The separate `EDIT.PRG` demonstration application remains unported. |
| B/C / library built, MagiC-tested | `XP_RASTR.SLB` | `MagiC/EXTENSIO/XP_RASTR.SLB` | `mise run apps -- xp-rastr` builds the two-export bitmap-expansion library with version-1 packed call records and register-ABI helper bridges. `mise run test:slb-consumer-neost` retrieves the legacy callbacks and checks exact four-plane conversion against the source-tree archive. Direct-colour modes are untested. |
| H | `ROMDRVR.PRG` | `MagiC/EXTENSIO/ROMDRVR` | Assembly driver exposing ROM through drive R:. A KAOS copy also exists; compare versions before sharing a recipe. |
| Blocked source build / optional print support | `PDLG.SLB` (DE/FR/EN) | `MagiC/EXTENSIO/PDLG_SLB` | Localized dialog sources and resources survive, but startup, AES dispatch and shared dialog inputs are missing. The shipped German binary loads under MagiC 6.20; dialog behavior is unverified without a GDOS printer driver. Not needed for core Atari boot or desktop use. If included in a distribution, treat the localized release binary as retained, not rebuilt, and check redistribution terms. See [porting notes](PORTING_NOTES.md#pdlgslb). |
| Blocked dependencies | `SPINMXFS.XFS`, older `SPINMAGC.XFS` / `CD-MXFS.PRG` | `MagiC/EXTENSIO/CD-MXFS` | C/assembly CD-ROM filesystem. PRJ names `spinmxfs.prg`; archived driver names differ. Depends on external/vendor CDFS/DOSIX libraries and missing headers. `CDTOOL.SYS` / `CONFIG.SYS` here are configuration text, not executable drivers. |

## 6. Reusable library/startup projects

These deserve prerequisite targets only when a concrete consumer needs them;
they are not a batch to rebuild. Start from the consumer's compile/link errors,
prefer modern MiNTLib/GEMlib where it satisfies the contract, and add a runtime
scenario where behavior is observable. Do not silently reuse proprietary-
toolchain objects in a GCC application.

| Legacy output / family | Source root | Role and porting boundary |
|---|---|---|
| `MT_AES.LIB` | `MagiC/LIB/MT_AES` | Multitasking AES bindings and related editor support. Check initialization, parameter blocks and callback conventions against the chosen application ABI. No current consumer; defer until a link proves a missing binding. |
| `MT_AES_I.LIB` | `MagiC/LIB/MT_AES_I` | Internal flavor; project references another tree's object through a stale path. Not automatically required by ordinary applications. No current consumer. |
| `MT_DOS.LIB` | `MagiC/LIB/MT_DOS` | Additional MagiC DOS wrappers. No current consumer. |
| `MAGXLIB.LIB` | `MagiC/LIB/MAGIXLIB` | Older assembly AES/GEM extensions missing from the original compiler libraries. MGSEARCH's rebuilt target does not link it; modern GEMlib supplies `form_xdo`, `form_xdial` and `objc_sysvar`, and `wind_find` is available. |
| `GEMUTILS`, `GEMUT_MT`, `GEMUT_M2`, `WINDOWS`, `WDIALOG`, `LL` objects | `MagiC/LIB/MYLIBS` | Distinct reusable helper implementations, not one monolithic application or interchangeable variants. MGSEARCH is the first current consumer of `GEMUTILS`/`WINDOWS`; its older `Rgetstring(WORD)` and `WINDOW` layout differ from the `GEMUT_MT`/APPLICAT variants. Compile only the variant a consumer uses; do not unify signatures. |
| `PCMXSTRT.O` | `MagiC/LIB/PCMXSTRT` | Pure C-specific startup with ARGV/quoting behavior. The required behavior is already covered by `MagiC/APPS/TOS_ARGS.H`; do not link the archived startup object. |
| `MAC_API.O` | `MagiC/LIB/MAC_API` | Host-Mac call glue; needs host cookie/API initialization. Relevant to MGEDIT7, native-menu/navigation demos, not standard Atari applications. No current Atari consumer. |

Also account for the application-local `EMUDBG.LIB`, VT52's `DRAGDROP.LIB`,
the SLB example's bindings and imported utility code. Similar filenames do not
prove identical versions or ABI compatibility.

## 7. Historical KAOS applications and utilities

Optional separate build families, not prerequisites for the current MagiC Atari
port. Do not turn historical copies into duplicate default targets.

| Outputs / classification | Source root | Notes |
|---|---|---|
| `KAOSDESK.PRG` | `KAOS/APPS/KAOSDESK.SRC/KAOSDESK` | Source desktop tree, with startup, helpers and localized resources. Port independently of MAGXDESK. |
| `ICNEDIT.PRG`, `MKICONS2.PRG` | `KAOS/APPS/KAOSDESK.SRC/ICNEDIT` | Icon editor and icon-building helper; separate outputs. |
| `ICX2RSC.PRG`, `RSC2ICX.PRG` | `KAOS/APPS/KAOSDESK.SRC/ICNEDIT/CONVERT` | Resource/icon conversion utilities; separate outputs, not editor modules. |
| Historical source snapshots | `KAOS/APPS/KAOSDESK.SRC` | `ORIGINAL` subtrees contain older desktop/editor projects. Preserve as optional versions, not additional current applications. |
| Binary distribution | `KAOS/APPS/KAOSDESK.224` | Older desktop, editor, KCMD and bundled utilities. Not a source-build root. |
| Binary distribution with stale PRJ | `KAOS/APPS/KAOSDESK.235` | A project file exists but matching C sources are absent here. Use the actual source tree, without assuming its version matches this release. |
| `ADDMEM.PRG`, `BOOTFIX.PRG`, `BOOTPARM.TTP`, `KAOSBOOT.PRG` | `KAOS/UTILS` | Separate assembly memory, boot-parent, CMOS boot-parameter and KAOS loader utilities. Platform/version-sensitive. |
| `KAOSCONF.ACC`, `KAOSCONF.TOS` | `KAOS/UTILS` | Assembly accessory and C console configuration programs; distinct implementations, not extension-only renames. |
| `DESK2.PRG`, `HDDELAY.TOS` | `KAOS/UTILS` | C desktop launcher and boot-delay utility. |
| `KAOSPTCH.TOS` / `.TTP`, `KAOSRELO.TOS` | `KAOS/UTILS` | C ROM patch/relocation tools. Mutate/generate OS images; use scratch copies. Treat TOS/TTP forms as variants pending inspection. |
| `ROMDRVR.PRG`, `SHELMAN.PRG` | `KAOS/UTILS` | Assembly ROM driver and TOS shell compatibility helper. ROMDRVR also exists under MagiC extensions. |
| `SETENV.PRG` | `KAOS/UTILS/SETENV` | Assembly AUTO-folder environment initializer. |
| `KAOSDECO.TOS`, `KAOSENCO.TOS` | `KAOS/UTILS/CODE` | C KAOS image/patch encoding tools, independent of normal applications. |
| `KAOSLIB.O` dependency | `KAOS/UTILS` | Assembly compatibility library; not an application. Headers/includes in the same folder are not build targets. |

## 8. Host support, test suites and unmapped release binaries

| Family | Location | Separate-build policy |
|---|---|---|
| Atari/UTF-8 conversion | `NON-TOS/atari-utf8-conversion` | Host C tool plus shell scripts, not an Atari executable. Scripts use `recode`; the kernel normalizer also reads the canonical conversion table. |
| FSORT / UNIQ | `NON-TOS/FSORT` | README classifies these as MS-DOS source, despite an old script naming `fsort.prg`. Keep host modernization separate from Atari application ports. |
| OS/emulator test programs | `MagiC/TEST/TESTS` | Families `TEST_68K`, `BIOSTEST`, `DOSTEST`, `AESTEST`, `VDITEST`; separate test executables, not installed applications. Existing projects include TEST_68, TGOETSCH, TESTPOP and VDI RASTER/POLYGON/PIXEL/LINES/LINES2/TRNFM/RECFL tests. Inspect each source/PRJ for actual output and hardware requirements. |
| Existing modern kernel regression | `kernel/DOS/compat/VFAT_CHECK.C` | Already a separate QEMU-user test target; not an application-port backlog item. |
| Release-only components not mapped to matching source projects here | `MagiC/Releases/MAGIC.A` | Examples: `AES_LUPE.APP`, `CLOCK.APP`, `CLOCKCOL.CPX`, `XCONTROL.ACC`, and CPXs such as GENERAL/COLOR/CONFIG/MODEM/PRINTER/SET_FLG/SI. Locate matching sources/provenance before creating claimed source builds; shipped files alone are not enough. |

The release tree is an installation reference, **not** a list of fresh compiler
outputs. Language copies, historical binaries, test corpora, imported compiler
libraries and resource data must not inflate the application build list.
Other OS/platform kernels and internal kernel objects are outside this catalogue.
