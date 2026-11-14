# Standalone application and component builds

A porting catalogue, not a claim that these programs already build with GCC.
The modern `Makefile.atari` currently builds the kernel. The applications used
in the successful Hatari kernel tests were the existing release binaries.
`Makefile.apps` now independently builds MOD_APP with GCC/MiNTLib and it has
passed the documented Hatari comparison. Other entries are planning items unless
explicitly marked verified; source presence alone is not a completed port.

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
| A | `SHOWFILE.TTP` | `MagiC/APPS/SHOWFILE` | Older text-mode viewer, predecessor of MGVIEW. C; localized. Keep as a distinct utility, not a second build of MGVIEW. |
| A | `LIMITMEM.TTP` | `MagiC/APPS/LIMITMEM` | Edits a program's memory limit. C; test only copies of executables. |
| A | `MGCLOCK.PRG` | `MagiC/APPS/MGCLOCK` | Minimal clock. C; small GEM application candidate. |
| B | `WBDAEMON.PRG` | `MagiC/APPS/WBDAEMON` | Disk writeback daemon. C; no localization needed. Test startup, periodic operation and exit under MagiC. |
| B | `SHUTDOWN.PRG` | `MagiC/APPS/SHUTDOWN` | Shutdown/restart helper. C; latest README says one tri-lingual application, not three separately localized binaries. Test last in an emulator session. |
| B | `VFATCONF.PRG` | `MagiC/APPS/VFATCONF` | Configure long filenames. C; legacy project depends on GEM utilities and MagiC bindings. |
| B | `MGXCLOCK.PRG` | `MagiC/APPS/MGXCLOCK` | Clock incorporating the writeback daemon. C; distinct from MGCLOCK, but share implementation/build support where appropriate. |
| B | `APPLICAT.APP` | `MagiC/APPS/APPLICAT` | Desktop file-type/icon association editor. C, multiple dialogs, resources and GEM helper dependencies. |
| B | `MGVIEW.APP` | `MagiC/APPS/MGVIEW` | Windowed text viewer, including classic Mac line endings. C/assembly. Project/release name is `MGVIEW.APP`; this directory also archives `VIEW.PRG`. |
| B | `MGEDIT.APP` | `MagiC/APPS/MGEDIT` | Text editor. C; depends on `EDITOBJC.SLB`, SLB bindings and GEM utilities. Port the library as a separate prerequisite. |
| B | `MGNOTICE.APP` version 2 | `MagiC/APPS/MGNOTICE.2` | Newer notes application. C; SLB bindings/resources. Recommended notes variant to port first. |
| B | `MGSEARCH.APP` | `MagiC/APPS/MGSEARCH` | Background file search. C; project references historical desktop pattern code, window helpers and GEM utilities through absolute paths. Resolve those inputs explicitly. |
| C | `MGCOPY.APP` | `MagiC/APPS/MGCOPY` | Desktop copy/move/delete helper. C; background/thread behavior and dialog libraries. Test nested trees, cancellation and error paths on scratch data. |
| C | `MGFORMAT.PRG` | `MagiC/APPS/MGFORMAT` | Floppy format/copy helper. C; threaded operations and low-level disk access. Use disposable floppy images only. |
| C | `VT52.PRG` | `MagiC/APPS/VT52` | Terminal emulator. C/assembly, AES/VDI/BIOS glue and `DRAGDROP.LIB`; the drag/drop library has its own project under `SRC/LIB/QUELLEN/DRAGDROP`. |
| C | `MAGXDESK.APP` version 5 | `MagiC/APPS/MAGICDESK/MAGXDESK.5` | Main desktop. C plus custom assembly startup and helper objects. Keep its helper programs independently buildable; package them together later. |
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
| B | `MCMD.TOS` | `MagiC/APPS/CMD` | MagiC command interpreter, assembly with DE/EN/FR includes. README identifies the recent localized MCMD as the built/tested variant. Package language selection explicitly. |
| H | `KCMD.TOS`, `CMD.TOS`, accessory mode | `MagiC/APPS/CMD` | Same assembly family can target KAOS/TOS or an accessory by source switches. These are distinct variants; do not infer that all are validated because MCMD works. Bundled `MAS.TTP`/`ALN.TTP` are old build tools, not outputs of this project. |
| B | `MAGXCONF.ACC` / `MAGXCONF.PRG` | `MagiC/APPS/MAGXCONF` | Assembly configuration accessory/program, including KAOS support and language variants. Distinct from the CPX below; archived output extension varies by language. |
| B | `FLP_PAR0.PRG`, `FLP_PAR1.PRG` | `MagiC/APPS/FLP_PAR` | Separate C sources to turn parallel floppy access off/on. One build family, two executables. |
| C | `COPS.APP` / accessory form | `MagiC/APPS/COPS` | Source control-panel host. C/assembly, CPX callbacks/resources and `EMUDBG.LIB`; PRJ selects `cops.app` with an accessory output commented out. Establish the desired mode rather than renaming an unverified build. |
| C | `MAGXCONF.CPX`, `TSLICE.CPX` | `MagiC/APPS/CPX` | Two distinct C modules with `.HDR`, `.RSC`, `.RSH` inputs. Need CPX entry/callback conventions and packaging, not just a normal PRG link. Source requires saved settings first in DATA. |
| Binary package | `COPS.ACC`, `WDIALOG.PRG`, `CLOCK.CPX` | `MagiC/APPS/COPS108.GER` | Distributed COPS 1.08 package, no C/assembly source found in this directory. Source COPS above is a separate tree; do not assume exact binary/source correspondence or turn bundled components into fake source targets. |
| Dependency | `EMU_DBG.LIB` (legacy name) | `MagiC/APPS/EMUDBG.LIB` | C/assembly debug-output library with MagicOnLinux support and stdout fallback. Rebuild a compatible library for its consumers, not an application. |

## 2. System components outside the application tree

These are separate artifacts needed for a fully rebuilt installation; they do
not belong in an application link or automatically in `MAGIC.RAM`.

| Component / outputs | Source | Build boundary |
|---|---|---|
| `MAGXBOOT` loader variants | `MagiC/KERNEL/BIOS/ATARI/BOOT` | Assembly loader. Source `MAGXBOOT.S`; archived `MAGXBOOT.PRG` and `MAGXBO32.PRG`, release installs `MAGXBOOT.TOS`. Determine switches and naming per variant. Keep original loader as baseline until its replacement is tested. |
| `HARDCOPY.PRG` | `MagiC/KERNEL/XTENSION/HARDCOPY.S` | Separate assembly hardcopy extension. |
| `WINFRAME.SLB` | `MagiC/KERNEL/WINFRAME` | Separate window-frame shared library with its own project; callback/layout ABI matters. |
| `WINFRAME.SLB` demo variant | `MagiC/KERNEL/WINFRAME.DEM` | Separate source/project producing the same filename. Use a distinct output directory and select at install time. |
| `MFA2.SYS`, `MFA4.SYS`, `MFA16.SYS`, `MFA256.SYS`, `MFA32K.SYS` | `MagiC/KERNEL/VDI/SRC/MAGIC.DRV/MFA` | Five independent screen-driver outputs, one family of recipes. Test suitable hardware/video configurations. |
| `MFM2.SYS`, `MFM16.SYS`, `MFM16IP.SYS`, `MFM16M.SYS`, `MFM256.SYS`, `MFM32K.SYS`, `MFM4IP.SYS` | `MagiC/KERNEL/VDI/SRC/MAGIC.DRV/MFM` | Seven driver projects; `ORIG/` contains another historical MFM4IP project, not a new product. |
| `OFF002.OSD`, `OFF004IP.OSD`, `OFF016IP.OSD`, `OFF016PK.OSD`, `OFF16MOV.OSD`, `OFF256IP.OSD`, `OFF256PK.OSD`, `OFF32KFL.OSD`, `OFF32KPK.OSD` | `MagiC/KERNEL/VDI/SRC/MAGIC.DRV/OSD` | Nine offscreen-driver outputs. Preserve the driver file format and dispatch interface. |

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
| A | `DUMP.TTP` | `MagiC/TOOLS/DUMP` | C hex dump utility. |
| A | `FC.TTP` | `MagiC/TOOLS/FC` | C file comparison utility with text/hex options. |
| A | `MAC2ST.TTP` | `MagiC/TOOLS/MAC2ST` | C Mac-CR to Atari-CRLF filter; output goes to the console, not a newly created destination file. |
| A | `SPLIT.TTP` | `MagiC/TOOLS/SPLIT` | C file splitter. Use disposable files to validate boundaries and output naming. |
| B | `SHOWFREE.TOS` | `MagiC/TOOLS/SHOWFREE` | C MagiC internal-memory monitor; custom startup and internal Sconfig interface. |
| B | `DIS1.TTP`, `DIS2.TTP` | `MagiC/TOOLS/DIS` | Two-pass C disassembler; separate projects/outputs, one user-facing workflow. `TST` contains a corpus and generated artifacts, not additional default products. |
| B | `MAGXKERN.TTP` | `MagiC/TOOLS/MAGXKERN` | C kernel/AES/application diagnostic; structure-layout dependencies. |
| B | `MEMEXAMN.TTP` | `MagiC/TOOLS/MEMEXAMN` | C memory allocator/dump inspector; version-sensitive internal structures. |
| B | `CRASHDMP.TOS` | `MagiC/TOOLS/CRASHDMP` | C exception-state decoder; fixed KAOS/MagiC crash-variable addresses. |
| H | `EXE2BIN.TTP` | `MagiC/TOOLS/EXE2BIN` | C executable/header conversion tool used by old build scripts. Mutates inputs; validate on copies. |
| H | `KRYPT.TTP` | `MagiC/TOOLS/KRYPT` | C XOR/update-file tool; preserve format/checksum behavior. |
| H | `FAST_SFX.TTP` | `MagiC/TOOLS/FAST_SFX` | C in-place patcher for a particular self-extracting archive layout. Not a general archive tool. |
| H | `ADDMEM.PRG` | `MagiC/TOOLS/ADDMEM` | Resident assembly memory utility; KAOS/MagiC-version-sensitive. |
| H | `DOS_MON.PRG` | `MagiC/TOOLS/DOS_MON` | Resident assembly DOS trap tracer. Hook, register and exception-frame conventions need review. |
| C / generators | `KEYB_CZ.PRG`, `KEYB_DEI.PRG`, `KEYB_FRA.PRG`, `KEYB_SWG.PRG`, `KEYB_UK.PRG`, `KEYB_US.PRG` | `MagiC/TOOLS/KEYB_SYS` | Six C keyboard-table generators plus startup. Each generates a different `KEYTABLS.SYS`; keep data outputs separate. `ALL.PRJ` omits the existing DEI project; do not rely solely on that aggregate. |
| C / dependency | `LOAD_IMG.SLB` | `MagiC/TOOLS/LOAD_IMG.SLB` | C/assembly image-decoding shared library. Separate SLB build; VDI callback interface and SLB binding prerequisite. |
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
| B/C | `EDITOBJC.SLB`; companion `EDIT.PRG` test | `MagiC/EXTENSIO/EDIT_SLB` | C/assembly editor library used by MGEDIT. Separate library and test-app recipes; AES/VDI, SLB bindings and resources. Existing projects contain stale/local-only startup/object references. |
| C | `XP_RASTR.SLB` | `MagiC/EXTENSIO/XP_RASTR.SLB` | C/assembly bitmap-expansion library with explicit SLB header code. |
| H | `ROMDRVR.PRG` | `MagiC/EXTENSIO/ROMDRVR` | Assembly driver exposing ROM through drive R:. A KAOS copy also exists; compare versions before sharing a recipe. |
| C / investigate | `PDLG.SLB`, `PDLG_FR.SLB`, `PDLG_UK.SLB` project variants | `MagiC/EXTENSIO/PDLG_SLB` | Print-dialog library, localized resources and platform bindings. Projects reference obsolete kernel paths and unresolved header/assembly inputs; map dependencies before promising a build. Release installs the selected language as `PDLG.SLB`. |
| Blocked dependencies | `SPINMXFS.XFS`, older `SPINMAGC.XFS` / `CD-MXFS.PRG` | `MagiC/EXTENSIO/CD-MXFS` | C/assembly CD-ROM filesystem. PRJ names `spinmxfs.prg`; archived driver names differ. Depends on external/vendor CDFS/DOSIX libraries and missing headers. `CDTOOL.SYS` / `CONFIG.SYS` here are configuration text, not executable drivers. |

## 6. Reusable library/startup projects

These deserve independent prerequisite targets, but not application launch tests.
Rebuild or replace them with an explicitly compatible implementation; do not
silently reuse proprietary-toolchain objects in a GCC application.

| Legacy output / family | Source root | Role and porting boundary |
|---|---|---|
| `MT_AES.LIB` | `MagiC/LIB/MT_AES` | Multitasking AES bindings and related editor support. Check initialization, parameter blocks and callback conventions against the chosen application ABI. |
| `MT_AES_I.LIB` | `MagiC/LIB/MT_AES_I` | Internal flavor; project references another tree's object through a stale path. Not automatically required by ordinary applications. |
| `MT_DOS.LIB` | `MagiC/LIB/MT_DOS` | Additional MagiC DOS wrappers. |
| `MAGXLIB.LIB` | `MagiC/LIB/MAGIXLIB` | Older assembly AES/GEM extensions missing from the original compiler libraries. Evaluate overlap with modern GEMlib before carrying everything forward. |
| `GEMUTILS`, `GEMUT_MT`, `GEMUT_M2`, `WINDOWS`, `WDIALOG`, `LL` objects | `MagiC/LIB/MYLIBS` | Distinct reusable helper implementations, not one monolithic application or interchangeable variants. Several application PRJs refer to these through absolute paths. |
| `PCMXSTRT.O` | `MagiC/LIB/PCMXSTRT` | Pure C-specific startup with ARGV/quoting behavior. Port required behavior to compatible application startup rather than linking it blindly. |
| `MAC_API.O` | `MagiC/LIB/MAC_API` | Host-Mac call glue; needs host cookie/API initialization. Relevant to MGEDIT7, native-menu/navigation demos, not standard Atari applications. |

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
| Existing modern kernel regression | `MagiC/KERNEL/BUILD/GCC/VFAT_CHECK.C` | Already a separate QEMU-user test target; not an application-port backlog item. |
| Release-only components not mapped to matching source projects here | `MagiC/Releases/MAGIC.A` | Examples: `AES_LUPE.APP`, `CLOCK.APP`, `CLOCKCOL.CPX`, `XCONTROL.ACC`, and CPXs such as GENERAL/COLOR/CONFIG/MODEM/PRINTER/SET_FLG/SI. Locate matching sources/provenance before creating claimed source builds; shipped files alone are not enough. |

The release tree is an installation reference, **not** a list of fresh compiler
outputs. Language copies, historical binaries, test corpora, imported compiler
libraries and resource data must not inflate the application build list.
Other OS/platform kernels and internal kernel objects are outside this catalogue.
