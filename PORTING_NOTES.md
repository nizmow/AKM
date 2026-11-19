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
`build/atari/neost-mgclock-server/`. The runner drives every scenario through
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
is under `build/atari/neost-mgnotice-verify2/`. Delete, font/colour/options,
multiple-window operation, window resize and French/US resources remain
unverified.

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
