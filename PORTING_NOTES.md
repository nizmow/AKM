# GCC/VASM porting notes

These notes cover the modern **German Atari/68000 kernel** and the first
standalone GCC application port. Start with `README.md`, `Makefile.atari`
(kernel), `Makefile.apps` (applications) and `TODO.md` (next tasks).
Prefer small, reviewable source changes. Keep kernel compatibility work in
`MagiC/KERNEL/BUILD/GCC/`; do not use those private headers or ABI flags as an
application SDK.

## Application SDK headers

The Podman image installs MiNTLib and GEMlib (`Containerfile.atari`). Their
target headers are under `/usr/m68k-atari-mintelf/include` and exposed through
the GCC sysroot at `/usr/m68k-atari-mintelf/sys-root/usr/include`. In particular,
`mint/basepage.h` and `mint/osbind.h` come from MiNTLib; `gem.h` and `libgem.a`
come from GEMlib. Use these public headers for ordinary application ports.

Legacy headers are not part of that SDK. The repository has a Borland-era
`TOS.H` snapshot under `MagiC/APPS/APPLICAT/SRC/IMPORT/`, plus MagiC-specific
imports and libraries there. CMDLINE has no local `TOS.H`/`AES.H` or project
file identifying its original include path, so the exact source of the
`<tos.h>` and `<aes.h>` it once used is not established here. The similarly
named `tos.h` and `aes.h` under `MagiC/KERNEL/BUILD/GCC/AES_C_INCLUDE/` are
private kernel compatibility headers, not an application SDK.

For CMDLINE, the port replaces those unavailable include expectations with
MiNTLib's basepage/OS interfaces and GEMlib's AES interface. Do not add legacy
import directories or kernel-private include paths to application builds merely
to make old includes resolve.

## Current evidence and limits

- GCC 15.2 MiNT ELF compiles the C; VASM assembles the Motorola source.
  This is not a conversion of the assembly to GNU assembler syntax.
- The complete kernel links and packages as `build/atari/MAGIC.RAM`.
- The last verified build was 264,175 bytes with 2,998 TOS relocations.
  These are observations, not size targets or golden test values.
- The emitted relocation stream was decoded and the BIOS/AES headers checked
  after simulated ST-RAM and TT-RAM relocation.
- The VFAT C/assembly regression passes under QEMU user emulation.
- Both the original English release and this German GCC kernel reached MAGXDESK
  using isolated copies of the same installation: Hatari 2.5.0, MegaSTE,
  68000 at 16 MHz, 4 MiB RAM, monochrome, TOS 2.06, HDDRIVER 10.18.
  Screenshots/logs are under `build/atari/hatari-test/`: baseline `settled.png`
  and GCC `boot.png`, each in its respective subdirectory.
- **This verifies startup, not stability, performance or full functionality.**
  Legacy warnings remain; other ABI boundaries still need runtime coverage.
- Both runs produced bus-error/illegal-instruction diagnostics during startup
  and still booted. Compare against the baseline and actual machine state;
  a diagnostic line alone does not establish a fatal crash.
- MagiC can run from ROM. This target produces the `MAGXBOOT`-loadable format;
  a ROM build requires its own packaging/layout and capacity check. Do not
  equate the size of `MAGIC.RAM` with a finished ROM image.

## First application port: MOD_APP

Use `mise run apps -- mod-app`, or invoke `Makefile.apps` through the existing
container wrapper. Output: `build/atari/apps/mod-app/MOD_APP.TTP`.
Applications are separate from `Makefile.atari`: default stack calls, 32-bit
`int`/`long`, and the toolchain's MiNTLib, without the kernel's ABI flags.
The source asserts that integer ABI to prevent accidentally mixing libraries.

This port needed no replacement startup object or archived Pure C libraries.
It uses public MiNTLib headers, standard decimal formatting instead of `itoa`,
explicit 32-bit cache signatures, and writable arrays for paths whose drive
letter is changed. The low-level `Malloc` trap returns a LONG address, requiring
an explicit pointer conversion. `errno.h` values are positive; returning a
GEMDOS-style allocation error requires `-ENOMEM`, not `ENOMEM`.

The original Atari-encoded MOD_APP source was converted to UTF-8 comments.
Target string bytes that depended on legacy encoding were preserved explicitly.
Source-encoding preservation is not a goal by itself: edit the existing source
for a port when that is the simpler path.

### Source encoding versus runtime bytes

Keep ported C source in UTF-8. That does not make UTF-8 the correct encoding for
strings displayed by TOS/AES: their non-ASCII bytes normally need Atari ST
encoding. CP850 is not an Atari ST substitute; for example, sharp S is `0x9e`
on Atari ST but `0xe1` in CP850. ASCII strings and UTF-8 comments need no
execution-charset conversion.

Use `NON-TOS/atari-utf8-conversion/atari_text.py` for strict text conversion:

```sh
python3 NON-TOS/atari-utf8-conversion/atari_text.py decode OLD.C -o OLD.C.utf8
python3 NON-TOS/atari-utf8-conversion/atari_text.py encode TEXT.utf8 -o TEXT.ST
printf '%s' 'Straße' | python3 NON-TOS/atari-utf8-conversion/atari_text.py c-literal
```

The last command produces `"Stra\236e"` for pasting as a C literal, not an
entire C source conversion. Fixed three-digit octal escapes preserve target
bytes without consuming following digits. Keep a readable UTF-8 comment where
useful. The tool reuses the repository's Atari table, preserves line endings,
rejects invalid/unrepresentable input, and refuses in-place conversion. No
encoding detection or Unicode normalization is attempted; review the known
source encoding before conversion. See the converter's `README.TXT` for its
bullet-character convention and full CLI contract.

Keep binary `.RSC` files unchanged; their embedded strings are not standalone
text files. A resource screenshot does not verify strings compiled from C.
Verify literal bytes in the compiled output separately when changing encoding.

**Startup compatibility is functional behavior.** The installed MiNTLib parser
split a double-quoted path in a raw TOS command tail when the tail began with an
ordinary option. Original PCMXSTRT handled it. See the
[upstream parser](https://github.com/freemint/mintlib/blob/master/mintlib/crtinit.c).
MOD_APP and APPLICAT share `MagiC/APPS/TOS_ARGS.H`, preserving single/double
quotes and doubled apostrophes in raw tails while retaining MiNTLib's parsed
ARGV vector for extended arguments. Storage belongs to each caller and lives
through its `main`; no replacement CRT or heap allocation is needed. The
17-case MOD_APP comparison passed again after extracting the shared parser.

A disposable Atari launcher invoked both original and rebuilt MOD_APP through
Pexec under the GCC kernel: 17 cases each, matching exit statuses and 48
byte-identical INF/BAK/DAT snapshot pairs. Coverage included all registration
modes, duplicate/no-write errors, bad/missing files, English and legacy German
sections, backup/cache behavior, both quote styles, escaped apostrophes, empty
arguments and a long ARGV path. Pexec reports the low 16-bit exit status:
65503 in the test log represents the documented -33 file-not-found result.
Evidence is under `build/atari/mod-app-smoke/results`; the test image and desktop
screenshot are under `build/atari/hatari-test/mod-app`. The launcher was
throwaway verification, not yet a reusable automated application test framework.

### CMDLINE

`make -f Makefile.apps cmdline` builds `MagiC/TOOLS/CMDLINE/SRC/CMDLINE.C` as
`build/atari/apps/cmdline/CMDLINE.PRG`. The port uses MiNTLib's public
`mint/basepage.h`, `mint/osbind.h` and GEMlib `gem.h` interfaces, linking `-lgem`;
it reads command-tail/environment fields through `BASEPAGE` rather than fixed
basepage offsets. The legacy `isprint` helper was renamed to avoid colliding
with libc. Both binaries ran under NeoST with a temporary Pexec driver and
returned status 0. Raw DOS/GEM tails and the program path matched in ordinary and
ARGV-style launches. For ARGV, the archived environment dump retains `ARGV=`;
MiNTLib NULs its leading `A` before `main` to consume the reserved marker. The
other environment bytes matched. This is expected MiNTLib startup behavior, not
a functional failure. Runtime basepage addresses were excluded; captures are
under `build/atari/neost-cmdline/`.

### SHOWFILE

`make -f Makefile.apps showfile` builds
`MagiC/APPS/SHOWFILE/SRC/SHOWFILE.C` as
`build/atari/apps/showfile/SHOWFILE.TTP`, using MiNTLib's public
`mint/osbind.h` and the default application ABI. The legacy source was converted
with `NON-TOS/atari-utf8-conversion/atari-utf8.c -r`, then adapted in place,
preserving its control flow. The port retains the VT52 viewer, paging/navigation
keys, reverse argument order, Wordplus control-byte substitution and localized
pager prompt selection (`COUNTRY`, default USA/English).

The original source included `<tos.h>`, `<tosdefs.h>` and `<country.h>`.
SHOWFILE has no project manifest or local copies identifying who supplied them,
so their exact compiler/library provenance (including whether they were Pure C
headers) is unverified. The port currently defines its GEMDOS error values,
console/read-mode constants and country IDs locally. Consider replacing or
factoring these definitions only when public SDK declarations or another real
consumer establish a reusable contract; avoid inventing a shared compatibility
header now.
Missing files and allocation failures remain silent, and the process returns
zero as the original entry point did. The archived binary and GCC output
matched under NeoST via a temporary Pexec driver: a two-line CR/LF file rendered
identically through the EOF prompt, and missing-file/no-argument launches
returned matching zero child statuses. The driver and logs are disposable
verification artifacts under `build/atari/showfile-runtime/`.

### Small console tools: MAC2ST, DUMP, FC, SPLIT

`make -f Makefile.apps mac2st`, `dump`, `fc` and `split` build
`build/atari/apps/{mac2st/MAC2ST.TTP,dump/DUMP.TTP,fc/FC.TTP,split/SPLIT.TTP}`.
The sources were converted with `scripts/atari_source.py`; runtime literals use
explicit Atari octal bytes. All four use MiNTLib's public `mint/osbind.h`
GEMDOS calls and the default application ABI.

The legacy `<tos.h>`/`<tosdefs.h>`/`<structs.h>` expectations were replaced with
public MiNTLib headers. DUMP's old `DTA` became `_DTA`, with `d_fname`/
`d_length` mapped to `dta_name`/`dta_size`; its local `isinf`/`isprint` names
were renamed to avoid built-in/libc collisions. FC's WORD and LONG paths had
multiple `*p++` updates in one expression, which is undefined C; they now fetch
bytes in sequence while preserving little-/big-endian ordering. DUMP writes
diagnostics to descriptor 2 with a console fallback; FC and SPLIT retain the
legacy `HDL_CON` (-1) handle so their observed missing-file output stays
compatible.

`mise run test:console-tools-neost` builds the tools and driver and compares
archived/rebuilt status, console output and guest files under NeoST/EmuTOS.
Cases cover MAC2ST filtering, DUMP default/`-c`, FC equal/differing/`-w`/`-l`,
and SPLIT's named outputs. Evidence from the initial run is under
`build/atari/neost-console-tools-final/`. Interactive FC input, MagiC-kernel behavior
and unusual file/directory forms remain untested.

### LIMITMEM

`make -f Makefile.apps limitmem` builds
`MagiC/APPS/LIMITMEM/SRC/LIMITMEM.C` as
`build/atari/apps/limitmem/LIMITMEM.TTP`, using MiNTLib's public headers and
the default application ABI. The legacy source was converted with the
repository Atari-to-UTF-8 converter and adapted in place. The executable-header
record is defined locally with fixed-width fields; GEMDOS errors are returned
with their negative TOS values rather than positive `errno` values.

`python3 scripts/neost-limitmem-smoke.py --output <new-directory>` compares the
archived and GCC executables under NeoST/EmuTOS using disposable PRG fixtures.
The test checks byte-identical console output, Pexec statuses, marker creation,
unchanged already-limited files, malformed headers/relocations, missing files
and invalid arguments. Both binaries match; setting the 64 KiB marker changes
the fixture as expected. Original release inputs are not modified.

EmuTOS returns -64 (`ERANGE`) from the legacy `Fshrink` surrogate
(`Fwrite(handle, 0, (void *)-1)`). Setting still writes the marker before that
error; removal returns the same status and leaves the marker in place in both
binaries. Successful removal therefore remains unverified under MagiC.
The legacy `nat_str` also read MagiC's system header for runtime country
selection; this port currently uses its English fallback because no public SDK
replacement was established. Non-English runtime selection remains unverified.

### MGCLOCK

`mise run apps -- mgclock` builds `MagiC/APPS/MGCLOCK/SRC/MGCLOCK.C` as
`build/atari/apps/mgclock/MGCLOCK.PRG`. The port uses MiNTLib's public
`<gem.h>` AES/VDI bindings and `<mint/osbind.h>` date/time calls, linked with
`-lgem`; it needs no resource file. The restored source is the original
desktop-drawing clock, not a windowed application: it computes a top-right
position from `WF_WORKXYWH` and font metrics, alternates an 18-character
date/time layout with an 8-character time layout, and redraws after a 1000 ms
`evnt_timer` delay. It has no close box or termination protocol. The original
packed DOS date/time formatting and display cycle are retained.

`mise run test:mgclock-neost` runs the rebuilt binary under EmuTOS, then boots
three clones of the supplied MagiC 6.20/TOS 2.06 image: MGCLOCK only, APPLICAT
only and both. It asserts that MGCLOCK's desktop area changes between successive
frames, that APPLICAT alone leaves that area static, and that the combined run
differs from the APPLICAT control only inside the clock rectangle. The verified
MagiC runs produced six clock-update frame pairs per MGCLOCK scenario; the
combined overlay changed `(489,2)-(630,14)`, while APPLICAT differed by 67,991
pixels outside the clock area. Evidence is under
`build/atari/neost-mgclock-refactor/`. The runner drives every scenario through
`scripts/neost_server.py`, NeoST's canonical server client, and keeps
`protocol.log`/`diagnostics.log` with each scenario's screenshots.

Inputs are `MAGIC_IMAGE`, `TOS206_ROM` and host `mtools`; the runner clones the
image for every scenario and assumes its C: partition starts at `@@1024` unless
`--partition-offset` is supplied. The check does not measure CPU load,
long-term stability, other desktops/OS versions or MagiC process termination.


### SHUTDOWN

`mise run apps -- shutdown` builds
`MagiC/APPS/SHUTDOWN/SRC/SHUTDOWN.C` as
`build/atari/apps/shutdown/SHUTDOWN.PRG` with MiNTLib/GEMlib and the default
application ABI. The German and French alert strings are ASCII C literals with
explicit Atari ST byte escapes (`\236` sharp S, `\202` e-acute, `\201`
u-umlaut, `\224` o-umlaut, `\205` a-grave). Do not reintroduce
`-fexec-charset=CP850`: CP850 byte `0x9e` is the multiplication sign, not sharp
S, so that flag hides the intended source character even when the emitted byte
happens to match. The non-debug localized literals were checked in the built
PRG.

### WBDAEMON

`mise run apps -- wbdaemon` builds `MagiC/APPS/WBDAEMON/SRC/WBDAEMON.C` as
`build/atari/apps/wbdaemon/WBDAEMON.PRG`. The source uses MiNTLib's `<gem.h>`,
`<mint/osbind.h>` and `<mint/sysbind.h>` instead of legacy `<aes.h>`, `<tos.h>`
and `<magx.h>`. AES message words use `short`, matching the 16-bit AES message
ABI under the application's 32-bit `int` ABI. The MagiC-only writeback
mode values remain local constants; `Ssync` uses the public trap binding.

Registration uses `trap_1_www(0x33, SC_WBACK, SCWB_SET)`, supplying MagiC's
word-sized subcommand. MiNTLib's generic `Sconfig(4, 1)` instead pushes a long
whose first word is zero, silently querying the daemon slot rather than setting
it. The corrected call matches MGXCLOCK's binding.

`mise run apps -- wbdaemon wbdaemon-test-driver` builds the application and
`wbdaemon/NEOSTEST.PRG` under the selected build directory. On a disposable
MagiC disk, replace the normal WBDAEMON startup binary with this build and add
the probe as `ZZPROBE.PRG` in the same `#_APP` folder. Run no other writeback
daemon in that fixture. The probe writes `C:\WBTEST.TXT`, checking a nonzero
daemon registration, `AP_TERM` delivery, process disappearance and release of
the daemon slot. This detects the incorrect generic-long binding.

Under NeoST 0.6.5, MegaSTE/4 MiB/monochrome, TOS 2.06 and a cloned MagiC 6.20
disk, the probe passed all four checks. Evidence is under
`build/atari/neost-wbdaemon-registration-20261118-221705/`. Periodic disk-flush
timing and data durability remain unmeasured.

### FLP_PAR0 / FLP_PAR1

`mise run apps -- flp-par` builds `FLP_PAR0.PRG` and `FLP_PAR1.PRG` from the
original sources in `MagiC/APPS/FLP_PAR/SRC/`. The Atari-encoded files were
decoded to canonical UTF-8 and adapted in place: legacy `<tos.h>`/`<magx.h>`
became MiNTLib's public `<mint/sysbind.h>`, with local `SCB_FLPAR` value
`0x800L`. Separate `flp-par0`/`flp-par1` targets use `Sconfig(mode, bitset)`,
a 16-bit subfunction and 32-bit bitset passed through GEMDOS function `0x33`.

The port preserves the original query/update sequence and zero exit status; it
does not check `Sconfig` errors, matching the archived sources. On cloned
MagiC 6.20/TOS 2.06 images under NeoST 0.6.5 (with the competing MAGXCONF CPX
disabled), archived and rebuilt binaries were each run through the same `Pexec`
probe. Both began with `SCB_FLPAR` unset and recorded
`00000000 → 00000800 → 00000000`, child statuses `0, 0`, and no changes to other
configuration bits. Evidence: `build/atari/neost-flp-par-clear-runtime/` and
`build/atari/neost-flp-par-archive-baseline/`. Physical parallel-floppy transfer
remains untested.

### VFATCONF

`make -f Makefile.apps vfatconf` builds
`MagiC/APPS/VFATCONF/SRC/VFATCONF.C` as
`build/atari/apps/vfatconf/VFATCONF.PRG` and copies the default German resource
to the expected lowercase `vfatconf.rsc` name. The build uses MiNTLib/GEMlib;
`VFATCONF_GEM.C` replaces only the legacy GEMUTILS calls this app needs.

The port retains its MagiC-specific `AESVARS2` boot-drive lookup and `DRVSTAT`
query with fixed-width fields, and the public `Dcntl` VFAT commands. AES message
words are 16-bit despite the application's 32-bit `int` ABI. Its non-ASCII
source text is confined to comments, so no CP850 execution charset is needed.
The dialog uses standard `form_do`; NeoST 0.6.5
with TOS 2.06 and a cloned MagiC disk launched it from a disposable GEMDOS
drive and displayed the dialog (`build/atari/vfatconf-runtime/final.ppm`). No
button was selected: MagiC drive-state queries, VFAT `Dcntl` changes and
`MAGX.INF` edits remain unverified. Use a disposable MagiC configuration and
VFAT volume for those operations.

### MGXCLOCK

`mise run apps -- mgxclock` builds
`build/atari/apps/mgxclock/MGXCLOCK.PRG`, with no resource file. The UTF-8 source
uses public MiNTLib/GEMlib headers, 16-bit AES/VDI parameters, a physical VDI
handle from `graf_handle`, and a 19-byte buffer for the 18-character date/time
plus NUL. The loop counter uses unsigned 16-bit wraparound rather than signed
overflow. The original desktop drawing and 500 ms timer loop remain; this is
not a windowed clock and does not add a new termination protocol.

The kernel marks `Dcntl(KER_SETWBACK = 0x0300)` as removed. Registration now
uses `trap_1_www(0x33, 4, 1)`: MagiC `Sconfig(SC_WBACK)` expects a **word**
subcommand. MiNTLib's generic `Sconfig(4, 1)` pushes a long, whose first word
is zero, silently querying the daemon instead of registering it. Each timer
iteration calls `Ssync` through trap `0x150`. Registration failure exits rather
than leaving a clock running without its advertised daemon.

Under NeoST 0.6.5, MegaSTE/4 MiB/monochrome, TOS 2.06 and a cloned MagiC 6.20
disk, the startup folder launched MGXCLOCK alongside MAGXDESK. Screenshots
verified initial date/time rendering and subsequent second updates. A guest
probe found the application and a nonzero registered writeback daemon pointer.
Evidence is under `build/atari/neost-mgxclock-KjyrfT/`. Disk-flush timing and
data durability were not independently measured.

`mise run apps -- mgxclock-test-driver` builds `mgxclock/NEOSTEST.PRG`
under the selected build directory. For the registration regression scenario,
use a disposable MagiC disk: replace its normal WBDAEMON startup entry with
MGXCLOCK, then add the probe as `ZZPROBE.PRG` in the same `#_APP` folder. No
other writeback daemon may run in this fixture. The probe waits 1.5 seconds,
queries the AES application and writeback registration, and writes PASS/FAIL
plus observations to `C:\MGXTEST.TXT`. The PASS check detects the generic
long-argument binding error; a clock screenshot alone does not.

### APPLICAT

`mise run apps -- applicat` compiles the eleven application modules plus the
application's `IMPORT/GEMUT_MT.C` helper using public MiNTLib/GEMlib headers and
`-lgem`. Output is `build/atari/apps/applicat/APPLICAT.APP`. The target also
copies the default German resource as `applicat.rsc` and the minimal association
fixture from `BIN/MINIMAL` as `applicat.inf`. The source-tree INF is not a safe
starter: it references external icon resources and old developer paths.

Run with the bundle directory as the working directory. The editor requires
MagiC's window-dialog/listbox extensions; it is not a plain-TOS GUI port.
Keep the executable, RSC and writable INF together. `-c` rebuilds the INF/DAT
without opening the editor; interactive Save also rebuilds the cache and sends
it to the running desktop. Use disposable configurations/images for testing.
English/French resources remain under `SRC/EN` and `SRC/FRA`, unchanged and
not runtime-verified by this port. Source comments are UTF-8; resource bytes
are preserved and no approximate execution charset is selected.

ABI boundaries are explicit: AES/VDI output fields and message words use
16-bit types, while ordinary application integers stay 32-bit. Dialog and
listbox callbacks use GEMlib's by-value `HNDL_OBJ_args`, `SLCT_ITEM_args` and
`SET_ITEM_args` structures to preserve MagiC's packed callback stack layout.
The DAT application record retains its signed 16-bit configuration field;
compile-time size/offset assertions guard the disk records. INF window
coordinates are scanned as shorts, and fixed four-byte special-object keys
are parsed through a separate NUL-terminated temporary buffer.

Under NeoST 0.6.5, MegaSTE/4 MiB/monochrome, TOS 2.06 and a cloned MagiC disk:

- The archived and GCC `-c` runs returned status 0. Their 2,256-byte starter
  caches match after excluding the three runtime ICONBLK pointer fields per
  icon; the header, tables, icon geometry and bitmap data match.
- The rebuilt editor opened its association windows. Program selection,
  window movement/backdrop, the new-file-type dialog and Save were exercised.
  Adding `*.ZZZ` to `MY APP` retained `*.OLD` and `C:\MY APP.APP`.
  Independent DAT decoding confirmed one application, two patterns attached
  to it, configuration 1, memory limit 0 and the preserved path.
- Saving closed the editor and the desktop accepted the updated cache.

Evidence is under `build/atari/applicat-check/` (`original`, `gcc-fixed` and
`ui-direct`). Path/special-icon edits, drag/drop, color icons and non-German
resource variants remain untested. Legacy four-character-constant warnings
and the existing `calc_ic` possibly-uninitialized warning remain.


### MGVIEW

`mise run apps -- mgview` builds the self-contained
`build/atari/apps/mgview/MGVIEW.APP`; no RSC or companion data is needed.
The original PRJ identifies three C modules, a register-ABI assembly delimiter
scanner and GEM utilities. The GCC build uses those C modules, public GEMlib,
local workstation/top-window operations and a bounded C `memchr2` instead of
linking Pure C objects. Raw-tail quoting reuses `MagiC/APPS/TOS_ARGS.H` while
retaining true ARGV startup. Edited source is UTF-8; displayed file bytes remain
Atari bytes, not host Unicode text.

AES messages remain 16-byte short-word records, VDI pointer outputs and coordinate
arrays use shorts, and events use the SDK's `EVMULT_IN`/`EVMULT_OUT` interface.
File size comes from the opened descriptor via `fstat`; failure cleanup releases
the descriptor, buffers and window before reporting the error. Empty files do
not require a zero-sized line-pointer allocation, and CR lookahead is bounded.

Under NeoST 0.6.5, MegaSTE/4 MiB/monochrome/TOS 2.06 and cloned MagiC images,
six cases per binary exited with status 0 and unchanged inputs: LF, CRLF, Mac CR,
quoted spaced filename, true ARGV spaced filename and empty file. Seven
content-region captures per case matched the archive exactly (42 comparisons).
Down/page-down, Home/End, tab width, horizontal scrolling and Ctrl-U closure
were exercised; Home restored the initial view, and tab/scroll inputs visibly
changed the content. Fixtures include blank lines, Atari high bytes, long lines
and a final unterminated line. Evidence is under
`build/atari/mgview-check/output/old-*` and `new-*`.

The spaced-path cases enable `#[vfat]` / `drives=c` in the **cloned** `MAGX.INF`.
The archive reports file-not-found without that prerequisite; that is not
evidence of a quoting bug. Startup can also leave a desktop window foreground:
top the viewer before sending keys, and target its actual title geometry
(an empty-file window is much narrower).

| Executable | TEXT | DATA | BSS | Symbols | File bytes |
|---|---:|---:|---:|---:|---:|
| Archived MGVIEW | 8094 | 1776 | 5830 | 0 | 10052 |
| GCC, default optimization/debug flags | 119444 | 1798 | 7396 | 55512 | 179192 |

Other display modes/OSes, resize/full, multiple-window cycling and VA_START/
drag-drop IPC were not exercised. `VIEW.PRG` remains an archived historical
binary, not a second claimed build of MGVIEW.

### MGEDIT and EDITOBJC.SLB

`mise run apps -- mgedit` produces `build/atari/apps/mgedit/MGEDIT.APP`,
the unchanged default German `mgedit.rsc` and source-built `EDITOBJC.SLB`.
`mise run apps -- edit-slb` independently produces the library under
`build/atari/apps/edit-slb/`. Run from the bundle directory on MagiC. The client
supplies a relative library search path; MagiC also searches `SLBPATH` or its
default `GEMSYS\MAGIC\XTENSION` directory. No archived SLB bindings, GEM utilities
or compiler objects are linked. Alternative resources remain unchanged.

The library implements AES 210–217 through MagiC's registration interface;
its version-9 SLB header exports zero numbered procedures. These are distinct
interfaces: the kernel dispatches registered AES handlers with `AESPB *` in A0,
whereas SLB init/open/close/exit use stack arguments. `EDIT_ENTRY.S` bridges A0
to the default GCC stack ABI. Its USERDEF callback takes the public `PARMBLK *`.
Private `EDITINFO`/`xedit_*` names avoid colliding with GEMlib's opaque
`XEDITINFO` and public client bindings.

A normal MiNT ELF executable starts with an ELF loader wrapper, not the required
SLB magic at the first TEXT byte. The dedicated linker script places the SLB
header at address zero, followed by code, DATA and NOLOAD BSS. The host packager
`scripts/elf_to_slb.py` emits classic TOS flags 15, records the BSS size and
encodes movable 32-bit absolute references, including BSS targets, as TOS base
relocations. Absolute constants and PC-relative references are not rebased.
The linked libc includes a real `init_clock` constructor: initializer pointers
belong in DATA, and custom startup must call them after setting `_base`.
Finalizers run in reverse order on unload. This packager currently validates
the editor-specific version/name/zero-export header, not arbitrary SLB variants.

The client uses public GEMlib event and editor APIs, short-word AES/VDI outputs,
16-byte messages and the SDK's by-value options callback record. `LONG` matches
the SDK's native `long`, guarded at four bytes: `int32_t` is `int` here and is not
a compatible `long *` target. INF scan/print helpers operate on shorts, including
GRECT coordinates. The shared TOS-tail adapter preserves quoting and true ARGV.
Edited source is UTF-8; RSC bytes and edited file contents remain Atari bytes.

Verification used NeoST 0.6.5, MegaSTE/4 MiB/monochrome/TOS 2.06 and disposable
MagiC images. First, the rebuilt library passed edit/save, clipboard and options
scenarios through the archived client. The complete rebuilt bundle then matched
six archived cases: insertion/save; select/copy/paste/cut/restore; tab-width
editing and INF persistence; New/Save As; quoted spaced filename; true ARGV
spaced filename. Saved text and INF bytes matched independently asserted
expectations; normal Save created no backup. Every child exited with status 0.
Fourteen full editor UI captures matched the archive pixel-for-pixel. Spaced-path
fixtures enabled VFAT on C: in the cloned INF; original images stayed unchanged.
Logs, screenshots and guest files are under
`build/atari/output/mgedit-archive-*`, `mgedit-library-cutover-*` and
`mgedit-complete-*`.

`mise exec -- python -m unittest discover -s scripts -p 'test_elf_to_slb.py'`
covers classic headers, base relocation including a 254-byte delta extension,
BSS pointers, absolute/PC-relative exclusions and malformed relocation rejection.

| Component | TEXT | DATA | BSS | Symbols | File bytes |
|---|---:|---:|---:|---:|---:|
| Archived MGEDIT | 15114 | 1992 | 6636 | 0 | 17366 |
| GCC MGEDIT | 130388 | 2238 | 8240 | 81498 | 217017 |
| Archived EDITOBJC | 16234 | 1556 | 1816 | 0 | 17960 |
| GCC EDITOBJC | 128680 | 1616 | 7256 | 0 | 132546 |

Other display modes/OSes, font and colour selection, resize/scroll interaction,
IPC and alternate resources were not exercised. The core still produces
possible-uninitialized-value warnings in font/selection paths; the ELF carrier
also produces a RWX segment warning. The delivered SLB is the validated classic
TOS image, not that ELF carrier. The separate `EDIT.PRG` demo is not rebuilt.

### MGNOTICE

`mise run apps -- mgnotice` builds `MagiC/APPS/MGNOTICE.2/SRC` as
`build/atari/apps/mgnotice/MGNOTICE.APP`, packages the default German
`mgnotice.rsc`, and copies the independently built `EDITOBJC.SLB`. The source
was converted to UTF-8 and adapted to public MiNTLib/GEMlib interfaces. It
reuses the shared `GEMUT_MT.C` utility implementation; `MagiC/APPS/GEMUT_MT_SDK.H`
now holds the public-SDK definitions used by both APPLICAT and MGNOTICE, while
`APPLICAT_GEM.H` remains a thin compatibility include. APPLICAT compiles again
after that move; its earlier runtime evidence was not rerun.

ABI boundaries that mattered:

- `WINDOW`'s AES/VDI fields and the message union are 16-bit. `WMESAG` is
  asserted to be eight AES words (16 bytes).
- The legacy 23-argument `evnt_multi` call becomes `mt_evnt_multi`; window
  messages are `short[8]`.
- `wind_create`, `wind_open` and `wind_calc` use the SDK's GRECT helpers;
  `graf_mkstate` takes four output pointers and `graf_dragbox` takes the
  expanded rectangle coordinates.
- The MagiC dialog callback uses the SDK's by-value `HNDL_OBJ_args`, not five
  separate arguments.
- `TEDINFO.te_color` bits 4–7 hold the text colour; the legacy `te_ucolor`
  union is not present in GEMlib.
- `edit_evnt` writes through a native `long *`, not `LONG`/`int32_t`.
- The GEMDOS directory record is `_DTA` (`dta_name`, `dta_size`), and
  `Mshrink` takes `(pointer, size)`.
- `notice_path` is a writable array; the code changes its drive byte from
  `Dgetdrv()`. `sprintf` replaces the nonstandard `itoa`.

`python3 scripts/neost-mgnotice-smoke.py` builds the application, test helper
and SLB, then boots archived and rebuilt bundles in separate MagiC 6.20/TOS 2.06
clones. It creates a note with Ctrl-N, types `hello`, leaves edit mode with Esc,
quits with Ctrl-Q, compares every UI state, extracts the saved note, and reboots
the same image to compare the reopened note. The delayed `NEOSTART.PRG` helper
is needed because MagiC starts `#_APP` entries before MAGXDESK owns the menu bar;
MGNOTICE needs foreground windows for input.

Verified results: initial, new, typed, escaped and quit screens are pixel-identical;
the 75-byte `0.NOT` files are byte-identical and contain `hello`; the reopened
screens are identical and the note window occupies `(98,98)-(141,117)`. Evidence
is under `build/atari/neost-mgnotice-refactor/`. Delete, font/colour/options,
multiple-window operation, window resize and French/US resources remain
unverified.

### MGSEARCH

`mise run apps -- mgsearch` builds `build/atari/apps/mgsearch/MGSEARCH.APP`
and copies the default German `MGSEARCH.RSC`. It compiles the consumer-specific
`GEMUTILS`/`WINDOWS` sources from `MagiC/LIB/MYLIBS/SRC` and an app-local copy
of `MAGXDESK.5/PATTERN.C`; the pattern helper is `mg_toupper` because MiNTLib's
libc already exports `toupper`. The old `MAGXLIB.LIB` is not linked: modern
GEMlib supplies `form_xdo`, `form_xdial`, `objc_sysvar` and `wind_find`.

The public-SDK bridge is `MagiC/LIB/MYLIBS/SRC/GEMUTILS_SDK.H`. It defines the
16-bit `WORD`, the 52-byte MiNT `XATTR` record and the old GEM constants that
MiNTLib no longer exports by default. `Dxreaddir` comes from `mintbind.h`.
`GEMUTILS` remains the older variant: its `Rgetstring(WORD)` signature and
`WINDOW` message type differ from `GEMUT_MT`; the two are not interchangeable.
The legacy event call uses GEMlib's single 32-bit interval, and the short
`wind_get`/`wind_set` calls are expanded to their four-value SDK forms.

`python3 scripts/neost-mgsearch-smoke.py` builds the application and its delayed
`NEOSTART.PRG`, then boots archived and rebuilt bundles in separate MagiC
6.20/TOS 2.06 clones with the same German resource. It types `*.ZZZ` into the
dialog, submits it, and compares the initial, typed and result screens. The
fixture contains `C:\SEARCHFIX\ALPHA.ZZZ` and a nested
`C:\SEARCHFIX\SUB\BETA.ZZZ`; both result screens matched pixel-for-pixel.
Evidence is under `build/atari/neost-mgsearch-refactor/`. Drive/path forms,
size/date filters, file-open messages, font variants and long-term operation
remain unverified.

### MGCOPY

`mise run apps -- mgcopy` builds `MGCOPY.APP` and copies the German
`MGCOPY.RSC` into `build/atari/apps/mgcopy/`. The four C modules use the shared
`GEMUT_MT.C` implementation and public MiNTLib/GEMlib bindings; no archived
`MT_AES.LIB`, `MT_DOS.LIB`, `PCTOSLIB.LIB` or Pure C startup object is linked.

The port adds `MGCOPY_GEM.H` for the records and constants absent from the
current public headers: the 44-byte GEMDOS `DTA`, the 52-byte MiNT `XATTR`,
`DISKINFO`, `DOSTIME`, `mutimbuf` and the MagiX `XTED` extension. It keeps
short-word AES/VDI outputs, uses GEMlib's `EVNT_multi` interface, and adapts the
three MagiC dialog callbacks to the SDK's by-value `HNDL_OBJ_args` record.
`MT_appl_init` becomes GEMlib's `mt_appl_init`. The shared `copy_mode` is the
application global rather than an accidental file-static duplicate.

`python3 scripts/neost-mgcopy-smoke.py` builds the application and delayed
`NEOSTART.PRG` driver, then boots archived and rebuilt bundles in separate
MagiC 6.20/TOS 2.06 clones. The driver waits for the desktop, invokes MGCOPY
through `Pexec` with `-q`, and records child statuses. Cases copy, move, alias
and delete top-level files; copy and move nested trees; and move a file from
`D:` to `C:` to exercise the cross-drive `ENSAME` fallback. The rebuilt and
archived runs produced the same seven zero statuses and identical source/
destination hashes. Nested copy leaves the source tree intact; nested move
removes it; the cross-drive source is removed and its destination is created.
Evidence is under `build/atari/neost-mgcopy-cross/`. Confirm-dialog interaction,
cancellation, long paths, read-only/error paths and alternate resources remain
untested.

### MGFORMAT

`mise run apps -- mgformat` builds `build/atari/apps/mgformat/MGFORMAT.PRG`
and copies the German `MGFORMAT.RSC`. The four application modules use the
shared `GEMUT_MT.C` helper and public MiNTLib/GEMlib calls; `MGFORMAT_GEM.H`
holds only the app-local DOS records (`DTA`, `DOSVARS`, `XAESMSG`) and the
MagiC DOS error aliases.

Two ABI details matter. GEMlib's `wdlg_*` names are PDLG/XaAES entry points
(AES 200-207), which try to load `PDLG.SLB`; MGFORMAT's source uses MagiC's
native `wdlg` API (AES 160-167). `MGFORMAT_WDLG.C` supplies those bindings with
the public `AESPB` and GEMlib `aes()` trap, and the dialog callbacks take the
SDK's by-value `struct HNDL_OBJ_args`. The format thread also needs its own
`mt_appl_init/myglobal`; `mt_appl_write`, `mt_appl_read` and `mt_appl_exit`
use that thread global, while resource loading and alerts use the main global.

The port replaces `Rwabs` with public `Lrwabs`, `ultoa` with `sprintf`, the
legacy 13-argument `evnt_multi` with the public MT call, and the old GEMDOS
`DTA`/`BPB` and `SConfig(SC_DOSVARS)` layouts with checked app-local records.
`MGFORMAT.PRG` must be started after MAGXDESK owns the desktop; the test helper
waits in `C:\GEMSYS\MAGIC\START`.

`python3 scripts/neost-mgformat-smoke.py` builds the application and delayed
helper, boots archived and rebuilt bundles from separate MagiC 6.20/TOS 2.06
clones with `C:\AUTO\MAGXBOOT.TOS`, and compares the `-f A:` format dialog.
The dialog screens differ only in the desktop free-bytes status line. NeoST's
floppy implementation reports a write fault when the dialog starts a physical
format, so successful formatting, disk copy and volume-label effects require
manual hardware or a different FDC implementation and remain unverified.

## Application porting workflow

Use these gates in order; the checklist below supplies the boundary details.

1. **Map contracts before editing broadly.** Read the original project, startup
   and actual inputs. Identify source/library/resource ownership and every
   C/assembly, OS, callback and persisted-data boundary. Establish argument widths,
   stack/register conventions, return values, structure layouts and semantic
   requirements from the public SDK and actual callees—not similarly named
   functions in an earlier port. Choose shared helpers only where contracts match.
2. **Capture a behavioral baseline.** Run the archive on small disposable fixtures
   under a pinned emulator/OS configuration. Record visible behavior and relevant
   outputs before changing the implementation. Choose cases around the
   application's contracts: for example, LF/CRLF/CR inputs for a text viewer,
   navigation at file boundaries, quoting, or save/backup behavior. If no matching
   executable is available, state that limit and derive independent expectations
   from source/docs; do not invent a parity result.
3. **Prove the shared boundary before spreading changes.** Adapt the minimum real
   source needed for a representative compile/link check. Exercise an uncertain
   runtime ABI with a focused guest scenario. Do not use stubs, archived objects
   or unresolved-symbol exceptions to make the check pass. Resolve shared
   declarations first; only then delegate disjoint modules against that contract.
4. **Port coherent slices, then integrate a stable snapshot.** Preserve original
   behavior while adapting complete call paths. Keep one integration owner and
   avoid compiling files while other owners are changing them. A successful
   sample module does not justify assuming every pointer, callback or record has
   the same layout; migrate all callers and remove obsolete adaptations.
5. **Verify the complete application, not just startup.** Build its independent
   install bundle and exercise representative user operations through the actual
   surface: rendering/navigation, editing/saving, or other advertised behavior.
   Compare meaningful outputs with the baseline and assert intended effects
   independently. Record compiled, packaged, launched and functionally tested
   status separately, along with remaining verification limits.

### Shared libraries and loadable components

Treat a legacy `.LIB`/`.O` or SLB source as a consumer-driven prerequisite, not
a separate porting batch. Start from the consumer's actual compile/link failures
and prefer modern MiNTLib/GEMlib where it supplies the same call; a successful
library build without a caller proves no useful contract.

Do not merge similar `MYLIBS` variants. `GEMUTILS` exposes
`Rgetstring(WORD)`, while `GEMUT_MT` requires `Rgetstring(WORD, WORD *)`; the
application-local `WINDOWS.H` copies also differ in callback and message types.
Build only the variant a consumer uses. MGSEARCH is the first current
`GEMUTILS`/`WINDOWS` consumer; link-test GEMlib's equivalents (`form_xdo`,
`form_xdial`, `objc_sysvar`, `wind_find`) against its PRJ's `MAGXLIB.LIB`
dependency before rebuilding the assembly library. For a runtime SLB, also
verify binding, relocation packaging and the client/library pair; a link-time
`.LIB` needs a real caller and a runtime check.

### Checklist for the next application

1. Read its PRJ, startup source and actual dependencies before choosing flags.
   Prefer public MiNTLib/GEMlib interfaces for ordinary applications; do not
   copy a legacy IMPORT directory or link its objects merely to silence errors.
2. Identify legacy integer/layout assumptions separately from calling convention.
   Use fixed-width SDK types for OS records and validate relevant sizes/offsets.
   A blanket `-mshort` change can make calls incompatible with the supplied libc.
3. Check writable string literals and nonstandard functions explicitly. A
   writable array fixes a mutated path; a cast or writable-memory accident does
   not make modifying a C string literal valid.
4. Do not preserve a legacy source encoding merely for its own sake. Convert
   known Atari ST text with the strict Python CLI described above, review it,
   then port the UTF-8 text in the canonical source. Use explicit Atari byte
   escapes for non-ASCII runtime literals and verify the emitted bytes; do not
   select CP850 as a blanket approximation.
5. Exercise startup through the actual caller: raw TOS command tails **and**
   ARGV, including quoted paths, empty arguments and application-specific literal
   patterns. A clean build did not catch MOD_APP's quote incompatibility.
6. Compare with the original executable on disposable fixtures, but also assert
   intended behavior independently: parity alone can preserve an existing bug.
   Check exit codes and complete output files, backups and cache side effects,
   including their absence on rejected operations.
7. Interpret status values at the correct layer: a raw GEMDOS trap returns
   negative errors, libc `errno` is positive, and Pexec exposes a 16-bit child
   exit status. Do not convert one convention into another indiscriminately.
8. Record TEXT, DATA, BSS and symbol sizes separately before drawing conclusions
   about a larger executable. Runtime-library and debug-symbol differences must
   not become a speculative optimization detour during a compatibility port.
9. Preserve scanner contracts when tightening bounds. Fixed-width four-byte
   keys are not C strings: scan into a five-byte temporary and copy the key,
   rather than rejecting the fourth byte or overwriting the following field.
   Legacy Pure C sources may also contain nested comments around disabled code;
   remove obsolete disabled blocks instead of treating compiler fallout as
   missing live functions.
10. Check generated edits at the actual affected lines. A successful rewrite
    preview is not proof that preprocessor line boundaries survived. Use fresh
    anchored ranges rather than guessed offsets, especially after other edits.
11. For GEM UI checks, avoid moving through `(0,0)` while establishing an
    absolute mouse position: it can activate the system menu. Move from an edge
    below the menu bar, confirm the actual foreground window, and target the
    visible text/control hotspot rather than assuming a whole list row is active.
12. For binary comparisons, identify runtime addresses and reader fixups before
    demanding byte equality. Exclude only established address fields, never
    unexplained differences in record widths, offsets or payload data.

## Kernel workflow

### 1. Establish a reproducible build

Run from the repository root. Build the container if it is not already present:

```sh
scripts/atari-container build

atari() {
    scripts/atari-container run "$@"
}

atari make -j4 -f Makefile.atari magic
```

Keep `MAGIC.RAM`, `kernel.elf` and `kernel.map` from the **same build**.
The ELF retains symbols and relocations; the map explains module placement.
The kernel target does not rebuild the desktop, boot loader or external drivers.

### 2. Pick one small, owned slice

Identify the original source, its build fragment and its callers. Read the
original `.PRJ` files as well as link manifests: `BUILD/ATARI/M.INP` alone omitted
VDI `SETUP.C`, which actually defines the required `MM_init` routine and is
listed in `MAGICVDI.PRJ`.

Examples of narrow build targets:

```sh
atari make -f Makefile.atari build/atari/kernel/aes-c/fsel.o
atari make -f Makefile.atari kernel-objects
```

Compilation proves syntax/types for that slice, not successful integration.
Finish with the complete `magic` target. Changes to common headers, assembler
options or the normalizer can affect every subsystem.

For agent work, assign disjoint source/build-fragment ownership and retain one
integration owner for the root Makefile, shared normalizer, linker and packager.
Ask workers to report changed files, assumptions and remaining errors. Integrate
first, then run the relevant checks; avoid simultaneous edits or builds of the
same generated files.

### 3. Fix causes, not diagnostics

Use the table below. Inspect the actual assembly callee and compiler-generated
caller at ABI boundaries. Do not add casts, dummy services, unresolved-symbol
exceptions or warning suppressions merely to obtain an image.

Generated includes and normalized assembly live under `build/atari/`. Fix the
original source or generator, **not generated files**. Ensure changed build
options and shared inputs are dependencies of their consumers.

### 4. Verify the changed behavior, then integrate

For filesystem/register-ABI changes, the existing runtime regression is:

```sh
atari make -f Makefile.atari filesystem build/atari/vfat-check.o
m68k-linux-gnu-ld -Ttext=0x10000 --gc-sections -z noexecstack -e _start \
    build/atari/vfat-check.o build/atari/vfat-glue.o build/atari/std.o \
    -o build/atari/vfat-check.elf
qemu-m68k build/atari/vfat-check.elf
atari make -j4 -f Makefile.atari magic
```

The host needs M68k Linux binutils and QEMU user emulation. Exit zero is success.
This exercises actual VFAT C/assembly calls, long filenames, checksums, alias
fallback and character conversion. It is **not** an AES/VDI test or a kernel boot.
The low link address matters: MagiC interprets negative LONG results as errors;
QEMU's usual high Linux link addresses can make successful pointers look negative.

For a new boundary, use a small deterministic scenario exercising actual code.
For syntax translation, assemble a tiny example and check its values/instructions:
text that looks plausible can still assemble incorrectly. Keep permanent tests
for meaningful regression risks, not incidental formatting or binary sizes.

### 5. Boot against a controlled baseline

1. Boot an original release successfully in Hatari with a suitable TOS ROM.
2. Keep that disk image untouched; clone it for the port. A 4 MiB ST is a useful
   initial configuration, not a proven minimum-memory requirement.
3. Replace only `MAGIC.RAM` in the clone. Retain the original `MAGXBOOT`, desktop,
   `GEMSYS` and configuration initially.
4. Record Hatari version, machine/CPU, RAM, TOS version, video mode and disk setup.
5. Capture the first fault's PC, registers and surrounding instructions. Relate
   runtime addresses to the matching ELF/map using the actual load base; do not
   blindly treat zero-based ELF addresses as runtime addresses.
6. After startup works, exercise directory access, the file selector, dialogs,
   fonts and window operations before benchmarking.

Do not change emulator hardware, compiler options and application installation
at the same time. Report separately: compiled, linked, packaged, runtime-tested,
booted, and functionally tested.

## Common problems and solutions

| Symptom / trap | Cause and appropriate fix |
|---|---|
| Code compiles but calls fail | Preserve the kernel ABI: `-m68000 -mshort -mfastcall`. Match integer/pointer widths, argument registers, stack arguments, return registers and preserved registers with the actual callee. Do not assume every Pure C boundary has been audited merely because the flag is accepted. |
| Temptation to link MiNTLib/GEMlib into the kernel | Their supplied binaries use the default stack ABI. Kernel code uses register calls and kernel services; only compatible libraries/helpers belong in this link. The Makefile selects the matching `mshort/mfastcall` libgcc. |
| Compiler substitutes libc string operations | Retain `-ffreestanding -fno-builtin`; kernel routines have non-libc contracts, including length/return-type differences. Retain `-fno-strict-aliasing` for existing overlapping structure views. |
| Callback type mismatch | Keep explicit `cdecl` on the callback typedef **and definition** where the original interface uses stack calls. Forward-declare structure tags before callback typedefs; tags first introduced inside a parameter list can denote distinct types. Do not cast the mismatch away. |
| `sizeof` / `offsetof` used in `#if` | The preprocessor cannot evaluate C type layouts. Use `_Static_assert` for the expected size/offset. Keep checks against assembly and on-disk interfaces. |
| Four-character magic gets truncated | With `-mshort`, multi-character integer constants can truncate. Use explicit 32-bit constants, e.g. `0x4e564449L` for `NVDI`. |
| Old C accepted by Pure C is rejected or behaves differently | Fix nested comments, function-versus-function-pointer declarations, casts used as assignment targets, and unsequenced expressions. For example, replace `*s++ = toupper(*s)` with assignment followed by increment. Preserve intended behavior, not undefined evaluation order. |
| Includes work on DOS but fail on Linux | Respect filename case and remove DOS path assumptions. Reuse the generated include aliases or existing compatibility include directories; do not create a second alias scheme. |
| Text/records change size unexpectedly | Separate source encoding from target bytes only where emitted bytes matter. Editing or converting legacy C sources is fine; verify affected strings/records. The shared assembler adapter encodes UTF-8 literals using the canonical Atari mapping and preserves legacy byte input. If legacy C bytes must remain unchanged, `-finput-charset=ISO-8859-1` plus `-fexec-charset=ISO-8859-1` can preserve them; do not apply those flags indiscriminately to UTF-8 C sources. |
| Assembly succeeds but jump tables are wrong | VASM needs `-spaces` for expressions such as `handler - table`. Without it, an expression can silently become just `handler`. Use it consistently across assembly slices. |
| VASM reports trailing garbage in an immediate | MAS uses size hints such as `#symbol.l+size` and `#(bufl+4).w`. The shared adapter removes the immediate hint while retaining the whole expression. Do not ignore the warning: VASM previously dropped `+size` in a DSP bound comparison. Preserve non-immediate address-size suffixes. |
| MAS syntax is rejected | Extend `mas_to_vasm.py`, reusing its handling of nested comments, named macro parameters/local labels, dot directives, `OFFSET`, `MODULE` and `REPT`/`ENDM`. Test nested blocks and labeled repeat openers. Avoid a second per-subsystem translator. |
| Register bit instruction has an invalid `.b` suffix | Register bit operations use long-register semantics; use `.l` for that case. Do not widen byte operations on memory. |
| A label named `text` is mistaken for a directive | Choose an unambiguous label, such as `text_start`, and migrate every caller and table reference. |
| `R_68K_PC16` overflow | A 68000 word-relative branch reaches only -32768..32767 bytes. Use absolute `jsr`/`jmp` for distant inter-module calls/tail calls, preserving control flow. Audit other calls to the same external routines; another object can push a previously fitting call out of range. |
| Duplicate exported symbol | Determine which module owns the public entry. The BIOS console helper was made private while STD retained public `putch`. Do not use linker options that arbitrarily accept duplicate definitions. |
| Missing platform-specific symbol | Trace its implementation and purpose. The common DOS call to Mac-only `bios_ptr` was obsolete debugging; its implementation was only `rts`, so the call/import/export and dead routine were removed. Do not manufacture an Atari stub for a missing real service. |
| Non-32-bit absolute relocation rejected by packager | Fixed low-memory symbols may legitimately use short absolute addresses and must remain absolute. Movable code/data addresses need representable relocation. Inspect the symbol section and original expression before widening instructions or changing packaging rules. |
| Warnings remain after a successful link | Triage against actual control flow and callee contracts. Examples include conditional drag-state initialization and `_wind_set` calls with opcode-dependent argument widths. Do not suppress warnings or claim they are harmless without checking the relevant path. |

## Link and loader invariants

`kernel.ld` and `elf_to_magic.py` implement the contract read from
`MagiC/KERNEL/BIOS/ATARI/BOOT/MAGXBOOT.S`:

- The zero-based ELF starts with the BIOS entry/header, not C startup code.
- `ENDE` terminates the loaded image. Keep its position deliberate.
- `MAGXBOOT` copies TEXT+DATA, not an independently allocated/cleared BSS.
  The linker therefore materializes zero-initialized storage in DATA.
- Loaded length must satisfy the loader's alignment requirement.
- Fixed low-memory symbols, including variable boundaries, do not move.
  References to movable storage need relocation; retain ELF relocations with
  `--emit-relocs` until packaging.
- Output has the classic TOS header and delta relocation stream, plus the
  BIOS/AES signatures and pointers expected by the loader.

Do not weaken packager assertions to accommodate an unexplained linker result.
An ELF that links is not automatically a valid TOS/MagiC image. ROM packaging
must be investigated separately rather than inferred from these RAM-loader rules.

## Size and performance experiments

First get a reproducibly working boot. Most of the kernel is still handwritten
assembly, so changing the C compiler cannot improve every path. `-m68000`
selects that CPU, but `-O2` can trade code size for speed; a larger file proves
neither better nor worse performance.

Compare optimization settings in separate build directories: Make does not
reliably notice changed command-line flags on existing objects.

```sh
atari make -j4 -f Makefile.atari BUILD_DIR=build/atari/o2 \
    'CFLAGS=-O2 -g -Wall -Wextra' magic
atari make -j4 -f Makefile.atari BUILD_DIR=build/atari/os \
    'CFLAGS=-Os -g -Wall -Wextra' magic
```

These are suggested experiments, not previously measured results. Compare the
same language/features, TEXT/DATA/storage layout, boot behavior and workloads
under identical emulator settings. Directory enumeration, file-selector sorting
and dialog operations are useful C-heavy candidates. Record both size and timing;
for cycle-sensitive conclusions, verify the emulator's timing model or use real
hardware. A smaller ST build may also be faster, but measure it.

## VT52

`mise run apps -- vt52` builds `VT52.PRG` and its German `VT52.RSC` into
`build/atari/apps/vt52/`. The target compiles the C modules and the eight
assembly modules with VASM, and builds `DRAGDROP` from
`SRC/LIB/QUELLEN/DRAGDROP/DRAGDROP.C` instead of linking the archived library.

Porting changes:

- The local compatibility header supplies the legacy `WORD`/`LONG` types,
  `DTA` field aliases and `EVNTDATA`; it uses public MiNTLib/GEMlib headers.
- Resource trees and strings are obtained with `rsrc_gaddr`, not by walking the
  private AES global resource pointer.
- Window/rectangle/mouse operations use public GEMlib calls. In particular,
  `wind_update`, `wind_get(WF_FIRSTXYWH/WF_NEXTXYWH)`, `wind_calc_grect` and
  `graf_mkstate` avoid the raw AESPB path, which produced
  `AES call 24622 not implemented`.
- VDI clipping, rectangle fill, colors and text use public GEMlib wrappers; the
  wide text call is used because the byte-array conversion path is unsuitable
  here.
- Assembly exports/imports use VASM `xdef`/`xref`; external short branches are
  long `jsr`/`jmp` because the linked PRG exceeds the 16-bit branch range.

The assembly-to-C call sites follow GCC's stack argument ABI rather than Pure
C's register convention, including the BIOS output and XBIOS hooks. The
terminal window is reopened and raised after `shel_write`; the public
`shel_write` path leaves it below the desktop otherwise.

`mise run test:vt52-neost` runs archived and rebuilt VT52 with the same German
resource and a delayed TOS child. The child writes through the hooked BIOS
console path, and the final terminal screens match pixel-for-pixel. Interactive
keyboard/mouse terminal operations remain untested.

## MAGXDESK version 5

`make -f Makefile.apps magxdesk` builds the version-5 C sources with MiNTLib and
GEMlib, using the default application ABI. It omits the legacy `MAIN.C` and
`MINSTART.S` startup and links no archived Pure C objects or libraries. The
unchanged German resource files are copied into the output bundle. Version 4 is
a separate source tree and was not part of this target.

`MAGXDESK_GEM.H` adapts legacy names to public SDK types and keeps MagiC-specific
structures explicit. The `APPLICAT.DAT` cache uses packed 18-byte application
records; `config`, `memlimit` and icon offsets are asserted because the older
v5 import header incorrectly inserted a reserved word. That mismatch caused
later cache entries to be read at the wrong offsets.

After `rsrc_rcfix` relocates cached icon pointers into the cache allocation,
do not shrink that allocation with `realloc`: if it moves the buffer, the
embedded pointers become stale. Keeping the loaded cache allocation intact
restores the lower file icons and preserves a pixel-identical startup desktop.

`mise run test:magxdesk-neost` clones the supplied MagiC image separately for
the archived and rebuilt executables, installs the same resources and compares
the desktop content region. The final run matched pixel-for-pixel; evidence is
under `build/atari/neost-magxdesk-20261121-093550`. This covers only the
initial desktop state, not the complete interaction surface.

Additional ABI boundaries:

- The kernel date formatter takes the output buffer in A0 and the DOS date in
  D0, and destroys D2 as well as caller-saved registers. `date_to_str` uses an
  inline-assembly bridge with an explicit D2 clobber so GCC preserves it across
  the public stack-ABI call.
- Puntaes keyboard conversion consumes a word-sized keycode after its word
  subfunction. Its binding pushes exactly ten bytes including the XBIOS opcode.
- The desktop tile raster request uses word-sized subfunction/mode fields;
  size and all field offsets after the magic are asserted.
- The returned `xp_rasterC` callback also uses Pure C's stack ABI: long
  word-count/plane-length, word plane-count, then two pointers (18 argument
  bytes). `expand_raster` builds that stack explicitly and declares D2
  clobbered so GCC preserves it; an ordinary GCC function-pointer call is
  incompatible even when its plane-count parameter is declared `WORD`.
- Trap comparisons use `GEMDOS_ELOCKED` and `GEMDOS_ERANGE`, never libc's
  positive errno names. TEDINFO disabled-field colors change bits 8–11, bit 7
  and bits 0–3, leaving frame color and fill pattern intact.

A focused NeoST/MagiC guest probe verified leap-day formatting against `_IDT`,
buffer guards, D2 preservation, Alt-A/Alt-1 conversion, monochrome IMG tile
loading and TEDINFO field preservation. Evidence:
`build/atari/neost-magxdesk-abi-20261121-095927/ABIPROBE.TXT`.
Locked-drive/overlong-name error scenarios and rendered alias-field colors
were not exercised by that probe.

A separate NeoST/MagiC color probe used ST low resolution (`#_DEV 2 0` on the
cloned image). The real kernel callback converted a four-plane, two-word raster
to the expected interleaved bytes, preserved a D2 sentinel and destination
guards, and `tile_init` loaded the matching 32×1 color IMG successfully.
Evidence: `build/atari/neost-magxdesk-raster-20261121-102022/RASTER.TXT`.
This checks raster conversion and tile loading, not desktop background painting
or other color depths.

## MCMD

`make -f Makefile.apps mcmd` builds German
`build/atari/apps/mcmd/de/MCMD.TOS`. `MCMD_LANG=en` and `MCMD_LANG=fr`
select isolated English/French outputs; `mcmd-languages` builds all three.
The existing `CONFIG.INC` selects only the MagiC program variant
(`KAOS=1`, `BOOT=1`, `MAGIX=1`, `ACC=0`). Historical TOS/KAOS/accessory builds
are not implied by this target.

The target normalizes the source/includes with the existing `mas_to_vasm.py`
and invokes VASM's native `-Ftos` writer, including BSS and TOS relocations.
No legacy MAS/ALN executables, startup objects or libraries are used.
MAS `IFF` maps to VASM `IFEQ`. Do not add `-spaces`: MCMD uses operand-column
comments that would otherwise be parsed as multiplication. `-noesc` preserves
literal Atari backslashes, and the unchanged legacy assembly bytes retain the
localized runtime encoding. Generated includes are flattened into the isolated
build directory; the canonical assembly sources are not rewritten.

`mise run test:mcmd-neost` uses a delayed Pexec driver, a batch fixture and a
source-built child program on separate MagiC image clones for each language
and executable. It asserts quoting/expansion, input/output and append
redirection, copy/rename/delete/mkdir/rmdir effects, conditional/GOTO execution,
child environment/stdin/stdout, an errorlevel branch, and shell return status.
It also compares all captured result files byte-for-byte against the archive.
The original image is never modified.

Preserve these observed legacy behaviors rather than quietly modernizing them
as part of the assembler port: grouping quotes are removed before forming an
external child's raw TOS tail, and `IF EXIST C:\WORK\` did not take its branch
on the test image, although `IF EXIST C:\WORK` did. The fixture retains both
directory forms. Conditional redirection creates an empty output file even
when its branch is not taken.

Interactive line editing, pipes, long filenames, startup AUTOEXEC/BOOT files,
disk errors and other machine/video configurations are outside this scenario.

The DE/EN/FR comparisons passed under NeoST 0.6.5/MagiC 6.20/TOS 2.06:
both programs returned status 0, and all 16 captured guest files per language
matched (48 byte-identical pairs). Evidence:
`build/atari/neost-mcmd-20261121-103027/`.

## MAGXCONF program/accessory

`make -f Makefile.apps magxconf` produces both `MAGXCONF.PRG` and `MAGXCONF.ACC`
under `build/atari/apps/magxconf/de/`. Select `MAGXCONF_LANG=en` or `fr`;
`magxconf-languages` builds all three languages in separate directories. The
program and accessory files are identical copies: the original startup code
detects the mode from the basepage parent pointer. Do not invent a separate
compile-time accessory ABI or confuse these outputs with `MAGXCONF.CPX`.

The source includes the existing `MagiC/INC_AS/{OSBIND,AESBIND,MAGIX}.INC`.
The shared MAS normalizer and VASM's native TOS writer replace the old tool
chain. Canonical `MAGXCONF.S` is UTF-8; `--atari-strings` restores Atari bytes
inside assembly literals, and `-noesc` retains literal backslashes. COUNTRY
1/2/3 selects German/French/English, retaining the original French default when
assembling without a selection. The VASM warning about the `global` label
conflicting with a directive is a name warning, not an unresolved symbol.

MAS starts an independent counter at each `OFFSET`/`OFFSET 0`. VASM reuses its
offset section, so normalization must emit an explicit `ORG 0` as well. Without
that reset, the OBJECT, TEDINFO, AESVARS and application BSS layouts accumulate:
the version lookup misses AESVARS and the application BSS is oversized.
MAGXCONF asserts the AESVARS magic/date/version offsets (12/16/48), and its
runtime comparison checks the displayed version/date values. Rebuilding the
three MCMD executables with this normalization change produced byte-identical
outputs; the kernel was not rebuilt as part of this application check.

Save finds the running file through `shel_find` and changes exactly four bytes
at file offset 30: the default configuration immediately after the two-byte
entry branch and 28-byte TOS header. There is no companion configuration file.
Accessory startup applies a valid saved default with Sconfig's word-sized
`'EL'` selector; `'AK'` queries the current mask. Keep this entry layout intact
and install writable copies.

`test:magxconf-neost` runs archived and rebuilt PRG/ACC copies on distinct
MagiC image clones. A guest query probe checks that Cancel preserves the mask,
OK changes only bit 4, Save additionally changes only bit 7, and an accessory
reboot restores the saved mask. The runner also requires that no executable
bytes outside the saved longword change. It activates the accessory through
the real desktop menu; the pinned fixture has MAGXCONF before Control Panels.

Disable a competing MAGXCONF CPX inside each clone before testing accessory
persistence: COPS calls its boot initializer, which restores a separate saved
configuration mask. The runner renames `AUTO\ACCS\CPX\MAGXCONF.CPX` to `.CPZ`
when present, leaving COPS and its menu entry available. Without that isolation
the supplied image overwrites the accessory's saved mask during boot. The
original disk and CPX source/build remain unchanged.

The archives and source are different resource revisions: besides Mag!X/MagiC,
the English archive has padded On labels, Abort instead of Cancel, and older
option wording. Full screenshots retain those differences. Pixel comparison
covers switch state margins, action-button borders, and numeric version/date
fields; it does not compare the changed captions or claim whole-dialog parity.
Other option bits, color/low-resolution displays, KAOS operation, and failure
paths for read-only/missing executables remain outside the scenario.

All DE/EN/FR program and accessory lifecycle comparisons passed under NeoST
0.6.5/TOS 2.06 on the supplied MagiC image: 39 paired switch/button/version/date
states, unchanged masks after Cancel, isolated bit changes after OK/Save,
four-byte-only executable modifications, and saved-mask restoration after each
accessory reboot. Full captures retain the caption-revision differences.
Evidence: `build/atari/neost-magxconf-20261121-110743/`.

## MAGXCONF.CPX

`mise run apps -- magxconf-cpx` builds `build/atari/apps/cpx/MAGXCONF.CPX`
from the original `MagiC/APPS/CPX/MAGXCONF.C`. A CPX file is a 512-byte
`struct cpxhead` followed by a classic TOS program: 28-byte header, text, data,
BSS size and TOS relocation deltas. The unchanged `MAGXCONF.HDR` supplies the
header/icon/title; `MAGXCONF.RSC/.RSO/.RSH` remain the original resource
inputs. `scripts/elf_to_cpx.py` packages the linked relocatable ELF, and
`MagiC/APPS/CPX/cpx.ld` keeps text/data/BSS contiguous from zero.

The external XCONTROL CPX SDK headers were not in the repository. `cpxhead.h`
and `cpxdata.h` reproduce the classic `XCPB`, `CPXINFO`, `CPXHEAD`, resource
index and MagiC-cookie records used by the archived module; `magxconf.h` and
`country.h` are local include shims. The callback field offsets were checked
against the archived module's disassembly (`getcookie` 80, `Country_Code` 84,
`Xform_do` 52, `rsh_obfix` 20, `CPX_Save` 72, `Get_Buffer` 76) and compile-time
asserts.

`struct save_vars` must be the first data-segment longword because the manager's
`CPX_Save` writes at `512 + 28 + text_length`. The source places it in
`.save_vars`, and `cpx.ld` keeps that section first. The boot initializer clears
the CPX's `SCONFIG_MASK` bits and ORs the saved default, so a scratch copy with
a nonzero default makes the effect observable.

Classic CPX callbacks take 16-bit arguments, but GCC passes `short` arguments
as 32-bit stack slots. `cpx_entry.S` provides VASM shims that repack
`Xform_do(fn, tree, edit, msg)`, `rsh_obfix(fn, tree, ob)` and
`XGen_Alert(fn, id)` to the classic 4+2+4/4+2/2-byte layouts. Without them the
dialog runs invisibly: `Xform_do` reads a shifted message pointer and
`rsh_obfix` receives the wrong tree/index, leaving invalid geometry. Every
callback invocation goes through a shim that saves and restores D2/A2:
`Xform_do`, `rsh_obfix`, `XGen_Alert`, `GetFirstRect`, `GetNextRect`,
`CPX_Save`, `Get_Buffer` and `getcookie`. Those registers are callee-saved for
the C caller, while the classic callbacks may clobber them (COPS' own bridges
declare the same clobbers). The generic `cpx_call2` bridge passes a zero filler
for zero/one-argument callbacks.

`scripts/cpx_abi_lint.py` enforces the known CPX callback-to-bridge mapping in
the ported MAGXCONF source, including callbacks with only 32-bit arguments.
Do not call or extract those callback pointers outside the approved bridge
argument. Add each newly ported consumer to the lint's default source list;
add a bridge contract before enabling a previously unused callback.
`mise run test:cpx-abi` exercises the actual assembly bridges against synthetic
native callees that check argument layout and deliberately destroy D2/A2.
These checks cover the CPX boundary, not arbitrary kernel/SLB function pointers.

`test:magxconf-cpx-neost` builds both modules, installs a scratch saved default
of `SCB_NFAST` into archived and rebuilt CPX copies, boots separate clones of
the MAGXCONF fixture with COPS enabled, checks the boot-time configuration
(`00000010`), opens Control Panels and double-clicks MagiC Config., and requires
pixel-identical dialog screenshots. The fixture's desktop menu and CPX list
coordinates are pinned. The archived module and rebuilt dialog matched exactly.

Limits: boot initialization and dialog rendering were exercised through the
COPS accessory in the supplied image. XCONTROL.ACC other managers, Save-button
confirmation/CPX_Save writes, switch toggling with OK, other languages and
failure alerts remain untested. The CPX header, icon and resource binaries are
retained unchanged. Evidence:
`build/atari/neost-magxconf-cpx-20261121-141502/`.

## DIS1/DIS2

`mise run apps -- dis` builds `build/atari/apps/dis1/DIS1.TTP` and
`build/atari/apps/dis2/DIS2.TTP` from `MagiC/TOOLS/DIS/SRC`. The Atari ST
sources were decoded to canonical UTF-8; runtime banner bytes were replaced
with explicit octal escapes (`\277` for `™`, `\275` for `©`, `\236` for `ß`).
Comments may remain UTF-8; execution strings must not.

This port is mostly a width exercise, because the sources assumed 16-bit
`int`:

- `DIS_COMPAT.H` declares the 28-byte GEMDOS program header and 14-byte
  GEMDOS symbol record with fixed 16/32-bit fields. `struct symbol` and
  `sizeof(PH)` must never use MiNTLib's 32-bit `int`.
- `DIS1`'s file buffer is `int16_t *`, and `getcode()`/`DEC` keep machine words
  in `int16_t`. Reading two bytes into an `int` would land in the high half on
  big-endian m68k and break every opcode comparison.
- DRI objects carry one 16-bit relocation slot per image word. `DIS2`'s
  `rlo_obj` allocation and all three declarations are `uint16_t *`; the
  `DIS1` label pass reads each slot into a `uint16_t`. A 32-bit slot type would
  double the stride and run past the allocated image.
- `_DTA` supplies `dta_size`/`dta_name`; `DIS_COMPAT.H` maps the old
  `d_length`/`d_fname` names. `Fsfirst` fills the DTA set in `main`, so
  `load_rlo` can use that size after its own lookup.
- Symbol types are 16-bit. `printtype` truncates before masking the defined/
  undefined bit; otherwise `0x8000` sign-extends and the fallback type text
  disappears.
- The MOVEM predecrement mask is a reversed 16-bit mask. `ISREG` tests
  `((uint16_t)regs >> (15-i)) & 1`; a 32-bit left shift tested bit 31 and
  dropped the selected register. The `48e7 0001` fixture decodes `sp`.
- `cerrws` now writes to `HDL_CON`; Pure C's `_BasPag->p_stdfh[STDERR]` test
  has no MiNTLib equivalent and is not needed for these console tools.

The `TST` corpus has executable programs without symbol tables. `DIS2` calls
`Malloc(0)` when the `.rlo` symbol length is zero; both the archived and rebuilt
binaries exit with status 1. The runtime check therefore patches only the test
`.rlo` symbol length to 4, which exercises label/relocation/symbol loading
without changing the tables. This is retained legacy behavior, not a port fix.

`test:dis-neost` compares archived/rebuilt child status and generated files
under NeoST/EmuTOS. Child console text is not captured: the NeoST log contains
only the driver's `STATUS` line, and the post-exit screenshot is the desktop.
Banners, usage and diagnostics therefore remain outside the automated check.
Raw `-e`, code/address/ASCII comments, 68030, MAS output, range, signed-word
and MOVEM edge cases, error cases and a recognized DRI object are covered. The object fixture is
`KAOS/APPS/KAOSDESK.SRC/ICNEDIT/IMPORT/PCMXSTRT.O`; its archived DIS1
workfiles and symbol table feed both DIS2 builds. `TST/T.O` is a Pure C
(`0x4efa`) object and is not treated as a DRI object. Interactive usage, `-t`
stdout capture and MagiC-kernel-specific behavior are not. Evidence:
`build/atari/neost-dis-20261122-113653/`.

## TSLICE.CPX

`mise run apps -- tslice-cpx` builds `build/atari/apps/cpx/TSLICE.CPX` from
`MagiC/APPS/CPX/TSLICE.C`, using the same 512-byte header, `cpx.ld` layout and
`elf_to_cpx.py` packaging as `MAGXCONF.CPX`. The saved
timeslice/priority record remains first in DATA and is written by `CPX_Save`.

Two foreign ABIs beyond the existing XCPB bridges matter here:

- `AESVARS.ctrl_timeslice` is a kernel entry point at offset 100. It takes a
  packed ticks/priority value in D0 and returns the old value in D0; it is not
  a normal C function pointer. `cpx_ctrl_timeslice` moves the C argument from
  the stack to D0, calls it, and preserves D2/A2.
- `Sl_x`, `Sl_arrow`, `Sl_dragx`, `Sl_dragy` and `MFsave` receive packed
  classic argument structures, not GCC's 32-bit argument slots. Dedicated
  assembly bridges build the 18/26/20/6-byte records in place. The C source
  still uses the readable separate-argument calls from the original.

`TSLICE.RSH` is compiled C resource data. Its English Cancel string was
corrected from `Abort` to `Cancel` to match `TSLICE.RSC` and the archived CPX;
the alert strings remain unchanged.

`test:tslice-cpx-neost` clones a MagiC image, installs the archived or rebuilt
CPX in `AUTO/ACCS/CPX`, opens Control Panels and double-clicks the `MagiC
Timeslice` entry. It requires the dialog to open and compares archived/rebuilt
screens pixel-for-pixel. The boot initializer is exercised implicitly because
the dialog reads back the scheduler timeslice/priority it sets. Slider
dragging, Save confirmation and actual scheduler behavior under load remain
untested. Evidence: `build/atari/neost-tslice-cpx-20261122-120331/`.

## COPS

`mise run apps -- cops` builds `build/atari/apps/cops/COPS.APP` in application
mode. The legacy `MagiC/APPS/COPS/INCLUDE` headers are not placed on the include
path: they supplied Pure C `MT_AES.H`/`MGX_DOS.H` declarations and would shadow
the toolchain's `stdio.h`, `stdint.h` and GEMlib headers. The source now uses
public `gem.h`/`gemx.h` and `mint/*.h`; `GLOBAL.H` keeps only the records MiNTLib
does not declare.

Contracts worth preserving:

- `_app` comes from MiNTLib startup (non-zero for an application, zero for an
  accessory). It is used to select the application/accessory paths.
- The 512-byte CPX disk header, 90-byte `XCPB`, 40-byte `CPXINFO` and the packed
  `Xform_do` argument record are asserted in `CPX.H`. The existing GCC callback
  bridges in `CPX_BIND.C` are unchanged.
- MiNTLib supplies `Dxreaddir()` but not its `XATTR` record, DTA field spellings
  (`dta_name`) or the `Keytbl()` table name (`caps`); `GLOBAL.H` defines the
  records or the callers use the public names.
- `EVNTDATA` remains a 16-bit record; the `graf_mkstate_evntdata()` and
  `evnt_button_evntdata()` macros expand to the public GEMlib calls.
- `COPS_RS.RSC` is generated C data (`COPS_RS.H`), so the executable is
  self-contained. `EMU_DBG.LIB` is not linked: `EMU_PRINTF` compiles the two
  debug calls out.
- `CLEARCPU.S` keeps its Pure Assembler form (`EXPORT`, `SUPER`) so `COPS.PRJ`
  still assembles it. The modern build normalizes it with
  `MagiC/KERNEL/BUILD/GCC/mas_to_vasm.py`, which turns `EXPORT` into `XDEF`,
  drops `SUPER`, converts `0x` literals and fixes directives. Verified object
  bytes contain `cpushl`, `cpusha`, `cinva` and both `movec %cacr` forms.
- The build rejects a `BUILD_DIR` whose `cops` output directory aliases
  `MagiC/APPS/COPS`; otherwise normalization or linking could overwrite the
  tracked source/archive on a case-insensitive checkout.

`test:cops-neost` validates the `#_APP`/`#_ACC` paths, disables the accessory,
removes inherited CPX modules and `COPS.INF` settings, and installs only
`MAGXCONF.CPX` and `TSLICE.CPX` in `C:\CPX`. It parks the pointer at a neutral
position before every capture, requires each dialog to differ from the panel
and the two dialogs to differ from each other, and requires Cancel to restore
everything right of the icon strip (the clicked icon keeps its selection
highlight). The panel, restored panel and both dialogs are then compared
pixel-for-pixel between archive and rebuild. Evidence:
`build/atari/neost-cops-20261123-195738/`. Accessory mode, board editing,
Save/alerts, event-CPX callbacks and other CPXs remain untested.

### OFF002.OSD offscreen driver

The OSD sources use Pure C/MAS syntax and Windows-style `INCLUDE` paths. Normalize
them with `MagiC/KERNEL/BUILD/GCC/mas_to_vasm.py`, including the MAGICVDI.2
`INCLUDE` files, then link with VASM `-Ftos -m68000 -noesc -nosym
-tos-flags=7`. `-nosym` prevents VASM from adding a symbol table where the
archived TOS executable has `slen=0`; `-tos-flags=7` preserves the header flag
word. The resulting V4.10 `OFF002.OSD` is byte-identical to the archived
MIN-ATAR file, including its TOS header, driver entry offsets and relocation
stream. The shipped Release driver is V4.11 and has different code/data sizes
and entry offsets; do not treat this source as a byte-identical V4.11 rebuild.

`v_opnbm()` is a VDI 100 call with `contrl[5]=1`, 20 intin words and the MFDB
pointer in `contrl[7/8]`. A caller-owned 64x64 one-plane MFDB with
`fd_wdwidth=4`, `fd_stand=0` and `fd_nplanes=1` selects `OFF002.OSD` on a
colour MagiC screen. The test probe fills a rectangle and checks pixel 1 and the
bitmap hash; see `MagiC/KERNEL/VDI/TESTDATA/OFF002.C`.

### OFF004IP.OSD offscreen driver

`mise run apps -- off004ip` builds `build/atari/apps/osd/OFF004IP.OSD` from
`MagiC/KERNEL/VDI/SRC/MAGIC.DRV/OSD/OFF004IP.S` through the same MAS-to-VASM
normalization and include set as OFF002. It is byte-identical to the archived
V4.10 `MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF004IP.OSD`. VASM's default
optimization rewrites `movem.l (sp)+,a0/a2` as two `movea.l` instructions; the
archive keeps the MOVEM encoding, so this target passes `-no-opt`. The shipped
Release driver is V4.11 with different text/data sizes and entry offsets; it is
not a byte reference.

`mise run test:osd-off004ip-neost` builds the driver and a probe
(`MagiC/KERNEL/VDI/TESTDATA/OFF004IP.C`), then runs the archived and rebuilt
files as `C:\GEMSYS\OFF004IP.OSD` in separate cloned MagiC 6.20/TOS 2.06 images.
The probe opens a caller-owned 64x64 two-plane interleaved MFDB
(`fd_wdwidth=4`, `fd_stand=0`, `fd_nplanes=2`) through `v_opnbm()`, fills bars
with VDI pens 2 and 3, reads five pixels and dumps the 1024-byte bitmap. The
driver maps pens `{0,3,1,2}`; raw colours 1 and 2 therefore exercise one plane
each, and samples at x=14..18 cross the 16-pixel word boundary. Expected
readbacks are `1/2 1/2 2/3 2/3 0/0` (raw/VDI), and the bitmap is asserted
independently, not only against the archive. Evidence is under
`build/atari/neost-off004ip-*`.

Limits: this exercises NVDI's selection and drawing for one organisation
(4 colours, 2 planes, format 0, organisation 1) on the tested colour
MegaSTE screen. Animation, other screen modes and long-term stability remain
untested.

### OFF016IP.OSD offscreen driver

`mise run apps -- off016ip` builds `build/atari/apps/osd/OFF016IP.OSD` from
`MagiC/KERNEL/VDI/SRC/MAGIC.DRV/OSD/OFF016IP.S`. It is byte-identical to the
archived V4.10 `MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF016IP.OSD`; like
OFF004IP it needs `-no-opt` so VASM keeps the archive's `movem` restores. The
shipped Release driver also reports header version 4.10 but has different
text/data sizes and entry offsets, so it is a structural comparison only.

`mise run test:osd-off016ip-neost` builds the driver and
`MagiC/KERNEL/VDI/TESTDATA/OFF016IP.C`. The probe requests the 16-colour,
4-plane, non-packed interleaved format explicitly (`intin[15..19]` = colour
count high/low words, planes, format, organisation), because a default request
inherits the screen organisation. It opens a caller-owned 64x64 MFDB
(`fd_wdwidth=4`, `fd_stand=0`, `fd_nplanes=4`; 32 bytes per row, 2048 bytes),
draws five bars with pens 2, 3, 4, 9 and 1, samples x=15 and x=16 for each bar
plus an untouched pixel, and dumps the bitmap. `color_map` maps those pens to
raw colours 1, 2, 4, 8 and 15; expected readbacks are
`1/2 1/2 2/3 2/3 4/4 4/4 8/9 8/9 15/1 15/1 0/0` (raw/VDI). The runner also
runs a clone with `OFF016IP.OSD` deleted; no other installed driver matches
this request, so `v_opnbm()` must be rejected and write no bitmap. Evidence is
under `build/atari/neost-off016ip-*`.

Limits: one organisation (16 colours, 4 planes, format 0, organisation 1) on
the tested colour MegaSTE screen; animation, other screen modes and long-term
stability remain untested.

### OFF256IP.OSD offscreen driver

`mise run apps -- off256ip` builds `build/atari/apps/osd/OFF256IP.OSD` from
`MagiC/KERNEL/VDI/SRC/MAGIC.DRV/OSD/OFF256IP.S` with the same includes and
`-no-opt` as OFF016IP. It is byte-identical to both the archived V4.10
`MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF256IP.OSD` and the shipped Release
copy (same SHA-256, 22,558 bytes, text length 0x5798). The source uses
`movea.l (a2),a0` where the original spelling was
`movea.l _color_map_ptr(a2),a0`; `_color_map_ptr` is `EQU 0`, and the archived
assembler folded the zero displacement while VASM `-no-opt` does not. Without
the folded spelling the text is two bytes longer and every later entry shifts.

`mise run test:osd-off256ip-neost` builds the driver and
`MagiC/KERNEL/VDI/TESTDATA/OFF256IP.C`. The probe requests the 256-colour,
8-plane, non-packed interleaved format explicitly (`intin[15..19]` = 0, 256,
8, 0, 1), opens a caller-owned 64x64 MFDB (`fd_wdwidth=4`, `fd_stand=0`,
`fd_nplanes=8`; 64 bytes per row, 4096 bytes), draws ten bars with pens 2, 3,
4, 9, 16, 32, 64, 128, 1 and 5, samples x=15 and x=16 for each bar plus an
untouched pixel, and dumps the bitmap. Expected readbacks are
`1/2 1/2 2/3 2/3 4/4 4/4 8/9 8/9 16/16 16/16 32/32 32/32 64/64 64/64
128/128 128/128 255/1 255/1 6/5 6/5 0/0` (raw/VDI). The runner compares
archive and rebuild, asserts the bitmap against an independent interleaved
layout, and runs a clone with `OFF256IP.OSD` deleted; the probe must report the
open as rejected and leave no bitmap dump. Evidence is under
`build/atari/neost-off256ip-*`.

Limits: one organisation (256 colours, 8 planes, format 0, organisation 1) on
the tested colour MegaSTE screen; animation, other screen modes and long-term
stability remain untested.

### Packed and direct-colour OSD drivers

`mise run apps -- osd-batch` builds `OFF016PK.OSD`, `OFF256PK.OSD`,
`OFF32KFL.OSD`, `OFF32KPK.OSD` and `OFF16MOV.OSD` under
`build/atari/apps/osd/`. All five use the MAS normalizer and the OSD include
set. `mas_to_vasm.py` now translates `ALIGN n` to VASM's exponent form
(`align log2(n)`): VASM's `align n` means 2^n, while these sources supply byte
counts, and the untranslated form padded by 64 KB.

`-no-opt` preserves the archived MOVEM encodings. OFF256PK is the exception:
its branches need VASM's size extension, so it builds with default
optimization. Zero-displacement folds were needed where the archived assembler
folded them but `-no-opt` does not: `move.b 0(a1),d4` became `move.b (a1),d4`
in OFF016PK, and `movea.l _color_map_ptr(a2),a0` became `movea.l (a2),a0` in
OFF256PK, OFF32KFL, OFF32KPK and OFF16MOV. OFF32KFL also uses explicit `.s` on
two previously unsized branches.

Reference parity is per driver: OFF016PK is byte-identical to its V4.10
source-tree archive; OFF32KFL is byte-identical to the shipped V4.11 Release.
OFF256PK (source V4.11; archive V3.02, Release V4.12), OFF32KPK (V4.11; archive
0410, Release 0413) and OFF16MOV (V4.13; both references differ, notably the
source keeps the `scrninfo`/palette tables in TEXT while the references place
them in DATA) have no byte-identical reference, so their structure and runtime
behavior are verified instead.

`mise run test:osd-<driver>-neost` runs the shared
`scripts/neost-osd-batch-smoke.py --driver <driver>` scenario. Each probe
requests its exact tuple through VDI 100/1, draws four-pixel bars crossing the
x=15/16 boundary, samples both sides, dumps the bitmap and checks an untouched
pixel. Indexed drivers use NVDI's colour map; direct-colour probes set pens
through VDI opcode 14. Archive and rebuild run in separate clones; a clone with
the driver deleted must report the open as rejected and write no bitmap. The
16MOV scenario asserts the full 16 KB bitmap and readbacks equal to the stored
longword with VDI's 16-bit word swap.

Limits: one organisation per driver on the tested colour MegaSTE screen. Three
drivers have no byte-identical reference; animation, other screen modes and
long-term stability remain untested.

### MagiC diagnostic tools

`mise run apps -- showfree magxkern memexamn crashdmp` builds the four tools
with MiNTLib's 32-bit `int` ABI. `Sconfig` still takes a word subfunction, so
`SHOWFREE`, `MAGXKERN` and `MEMEXAMN` call `trap_1_wwl(0x33, fn, arg)` rather
than a generic long binding. `SHOWFREE` and `CRASHDMP` need no guest
structures; the latter uses MiNTLib GEMDOS `Cconws`/`Pterm` and the crash
variables at `$380`–`$3cc`.

`MAGXKERN` and `MEMEXAMN` read 16-bit kernel records. `MAGXKERN` keeps the
original layout but scopes `#define int int16_t` around its structure
definitions; `MEMEXAMN` uses local fixed-width `SYSHDR`/`DOSVARS`/`AESVARS`/
`MCB`/`BASPAG` records. Do not replace this with `-mshort`: MiNTLib calls and
variadic output still use 32-bit `int`. `MEMEXAMN` also needs a writable drive
path buffer and treats an empty `argv[1]` as the live-system request.

`mise run test:diagnostic-tools-neost` installs each archive/rebuild under the
same guest name and compares status and output. MAGXKERN matches byte-for-byte;
MEMEXAMN's addresses and block sizes differ with the larger rebuilt image, so
the test compares the normalized early table plus owner tokens and the `KROM`
terminator. SHOWFREE loops until a key; the test compares screens and the first
monitor output instead of exit status.

### MFA/MFM screen drivers

`mise run apps -- screen-drivers` builds the five Atari MFA and seven MagiCMac
MFM screen drivers under `build/atari/apps/screen/`. The sources use the same
MAS normalizer and VDI include directory as the OSD drivers; this recipe stages
`HARDWARE.I` and `PIXMAP.I`, while the OSD recipe stages `SEEDFILL.I`. VASM runs with
`-DDUMMY=0 -no-opt -opt-allbra`, which preserves the archived MOVEM choices
while still shortening branches. The older MFM sources name the NVDI field at
offset 196 `_nvdi_PixMap_ptr`; the current `DRIVER.I` calls it
`_nvdi_vdi_setup_ptr`, so the build supplies the legacy alias as a define.

The rebuilt files match their archives' header versions and organizations but
are not byte-identical: file sizes differ by 2–10 bytes from VASM branch and
encoding choices. Parity is therefore structural, not byte-for-byte. `MFM4.SYS` is
requested by `load_MAC_driver` when a 2-plane non-ATARI2 display is selected,
but no such project or binary exists in the tree; `ORIG/` is historical.

`scripts/neost-screen-driver-smoke.py` builds the family plus a small
`SCREEN.PRG` boot probe and checks every TOS header, organization and
relocation. The probe writes `C:\SCREEN.TXT` only after MagiC starts. It boots
archived, rebuilt and missing `MFA2.SYS`/`MFA4.SYS`/`MFA16.SYS` at ST
high/medium/low: the positive runs produce the marker and identical screens,
while the missing file produces no marker and a materially different screen.
Recorded controls show `Bildschirm-Treiber nicht gefunden.` and the MCMD shell;
the automated check does not assert those strings or the later halt. TT/Falcon
modes require a profile NeoST does not expose, and MFM requires a MagiCMac host
with `VDI_SETUP_DATA`; the other nine drivers are structurally verified only.

### WINFRAME.SLB

`mise run apps -- winframe` builds `build/atari/apps/winframe/WINFRAME.SLB`
and copies `WINFRAME.RSC` beside it. The alternate window-frame manager
registers through MagiC's AES subcommand 6; it is not part of `MAGIC.RAM` and
the `#_SLB 0 WINFRAME.SLB` line in `MAGX.INF` is optional. The SLB header is
version 1, exports no numbered functions, and keeps the header/lifecycle
pointer relocations in the classic TOS image.

The source uses public GEMlib `AESPB`/`aes()` for registration instead of the
legacy Pure C `mt_aes.lib` and `_GemParBlk`. `_GemParBlk.global[10]` became
`aes_global[10]`; VDI work arrays, handles and coordinate arrays are `short`,
while the application-visible `WORD`/`LONG` types remain 16/32-bit. `TEDINFO`
uses the public GEMlib fields at the same reserved-word offsets. The resource
and handler layout still assume MagiC 6's `WININFO` contract and the 20-object
frame tree; do not reuse this recipe for a different kernel without checking
`wsizeof` and the callback offsets. The manager deliberately returns an error
when the AES reports fewer than 16 colours, so a monochrome desktop will not
show its frames; the runtime scenario boots a colour MegaSTE profile.

The kernel dispatches the eight `wbm_*` manager callbacks with a register ABI,
not the GCC stack ABI: `WINDOW *` is in A0 and scalar arguments are in D0/D1.
`WINFRAME_ENTRY.S` supplies register-to-stack bridges for `create`, `skind`,
`ssize`, `sslid`, `sstr`, `sattr`, `calc` and `obfind`; the handler table points
at those bridges. The lifecycle entries and the USERDEF frame callback remain
stack-based. Without the bridges the kernel reads stack garbage as the window
pointer, corrupting the tree.

`test:winframe-neost` boots three clones of the same MagiC image: archived
WINFRAME, rebuilt WINFRAME, and a clone with the `#_SLB` line disabled. It
requires the enabled/disabled booted dialog to differ, and compares the
archived/rebuilt screens pixel-for-pixel. The fixture's booted MAGXCONF dialog
provides the visible frame; the test does not drag, resize, shade or exercise
scrollbar paths. Evidence: `build/atari/neost-winframe-20261122-141115/`.

### XP_RASTR.SLB and LOAD_IMG.SLB

`mise run apps -- xp-rastr` builds `XP_RASTR.SLB`; `mise run apps -- load-img`
builds `LOAD_IMG.SLB`. Both use explicit SLB headers with exported function
tables: XP_RASTR has two functions and LOAD_IMG has five. The generic packager
validates the declared export count and each exported pointer relocation.

XP_RASTR's assembly helpers (`_vq_scrninfo`, `_vq_color`, `W_mul_L`,
`L_div_W`) take register arguments. `XPRASTR_BRIDGE.S` provides stack-to-register
wrappers and preserves D2/A2 around the VDI calls. The C source uses public
GEMlib `VDIPB` and MiNTLib allocation/VDI types. LOAD_IMG's C source uses
public GEMlib/MiNTLib types; its local `XATTR` mirrors the MiNT layout because
the SDK does not expose that name.

Keep the public version-1 calling convention, not the compiler's default
argument layout. MagiC's `Slbx_fn` replaces the handle with the caller's
process pointer and jumps to the numbered export without rearranging its
stack. Each C export in `LOAD_IMG.C` and `XP_RASTR.C` receives a single
`packed, aligned(2)` record **by value**: process pointer, 32-bit function
number, 16-bit argument-word count, and the export-specific payload.
`sizeof` and `offsetof` assertions pin the record to the native contract.
For example, LOAD_IMG export 0 sees `nargs` at `12(sp)` and its two pointer
arguments at `14(sp)` and `18(sp)` (including the return address). Separate
GCC `short` parameters would occupy four-byte stack slots. Keep normal
MiNTLib/GEMlib flags and leave the lifecycle entries on their own ABI.

Use assembly when layout isn't the whole contract. `LOAD_IMG_RASTER.S`
calls the legacy `xp_rasterC` callback with an 18-byte stack record and
preserves D2, which that callback may clobber. A packed C parameter alone
would not protect the register.

Do not use the shipped Release `LOAD_IMG.SLB` as a source-level oracle.
The archived binary under `TOOLS/LOAD_IMG.SLB/BIN` decodes in export 0, with
a per-plane byte stride. The Release binary initializes a private image
handle in export 0, reports an all-plane stride there, and decodes in
export 2. Compare the full load → convert → draw path; do not compare raw
descriptor buffers or accept both strides in a single buffer-size formula.
The Release and rebuilt versions both rendered the test XIMG but produced
different realized colours. The port does not claim Release-equivalent
colour rendering.
