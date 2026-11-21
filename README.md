# Sources for Atari and Related

This repository is holding my sources for Atari and related computers, i.e. 68k code running inside a real machine or in an emulator:

* TOS
* KAOS (my improved TOS variant with more than 80 bugs removed)
* MagiX / MagiC for Atari computers
* MagiCMac for Mac computers
* MagicMac X for MacOS X on PPC
* AtariX for macOS on intel computers


# Included (among others)

Applications:

* MAGICDESK
* APPLICAT
* CHGRES
* MAGXCONF
* MATRIX
* MGEDIT
* MGFORMAT
* MGCOPY
* MGNOTICE
* MGVIEW
* SHUTDOWN
* VT52

Tools:

* MCMD (and CMD and KCMD, command line interpreter)
* DIS (68k Disassembler)

and various libraries, development demo code and test code

# Complete, including MVDI Sources

Upload complete as of 2018-11-20

# MagiC for Atari Releases

See MagiC/Releases/. Installation instructions included. Should mainly be in sync with MagicOnLinux.

# Atari cross-toolchain container

Build a Linux-hosted Atari C/C++ toolchain with Podman:

```sh
podman build -t localhost/magic-atari-toolchain:latest -f Containerfile.atari .
podman run --rm -it --userns=keep-id \
  -v "$PWD:/work" -w /work localhost/magic-atari-toolchain:latest
```

The image uses Thorsten Otto's [GCC 15.2 MiNT ELF release](https://tho-otto.de/crossmint.php)
(HTTPS archive pinned by SHA-256) with GNU binutils 2.42, MiNTLib, FDLIBM and GEMlib
from Vincent Rivière's [Debian packages](http://vincent.riviere.free.fr/soft/m68k-atari-mintelf/debian.php).
The image also builds a SHA-256-pinned VASM 2.0f Motorola assembler from a
[release mirror](https://github.com/StarWolf3000/vasm-mirror/tree/a13e7e728a3dd5a9ba468bf479119e0bc23e70fd).
Make, Python and ZIP tools are included.
Use `m68k-atari-mintelf-gcc -m68000 source.c -o program.tos` to compile and link
an Atari executable. The upstream package repository and initial signing-key
download use HTTP; subsequent package downloads are checked against signed APT metadata.

## Mise shortcuts (optional)

Mise selects pinned host tools as well as providing task shortcuts. Make still
owns dependencies, compiler flags and linking. Podman and mise must already be
installed on the host. Review and trust the project configuration, then use:

```sh
mise trust
mise install
mise exec -- python3 --version
mise exec -- neost-headless --version
mise tasks
mise run toolchain:build
mise run kernel -- -j4
mise run make -- filesystem
mise run atari -- m68k-atari-mintelf-gcc --version
mise run toolchain:shell
```

`make` forwards arbitrary targets, options and assignments to `Makefile.atari`;
`atari` forwards an arbitrary command and its arguments into the container.
For example, `mise run make -- BUILD_DIR=build/atari/experiment magic` keeps a
separate build. Invoke tasks from repository subdirectories too.

### Host tool versions

| Tool | Version management |
|---|---|
| Python | `mise.toml` pins 3.13.7 for host scripts and tests. |
| NeoST | `mise.toml` pins [0.6.5](https://github.com/habib256/neost/releases/tag/0.6.5) on Linux x86-64 through mise's GitHub backend, with an explicit SHA-256. |
| Atari GCC, VASM, MiNTLib/GEMlib, Make | Keep in `Containerfile.atari`; do not add a second host toolchain through mise. |
| Podman | Install through the host package manager; it needs host runtime integration. |
| Hatari, QEMU user emulation, M68k Linux binutils | Remain host prerequisites for the checks that use them; not currently mise-managed. |
| `glab` | Optional publishing/authentication helper; suitable for a future mise pin, not needed for builds. |

NeoST's install hook extracts the AppImage once, exposing `neost-headless` and
the bundled EmuTOS ROM without requiring FUSE. The smoke runners discover this
layout from `PATH`; a separate downloaded AppImage is no longer required when
using mise. Other platforms are not covered by this NeoST pin; provide an
emulator and ROM explicitly using the runner's options.


Use `mise exec -- <command>` without shell activation, or activate mise to get
the selected versions automatically. `mise run test:encoding` runs the text
converter regression checks. `mise run ports:scan` inventories the catalogue
and source roots; `mise run ports:deps` resolves legacy `.PRJ` module and header
references to repository candidates. `mise run ports:brief -- MGSEARCH` combines
both into one porting packet, and `mise run ports:compile` runs syntax-only
probes. `mise run test:abi` compiles the ABI layout checks;
`mise run verify:apps -- --build` does a fresh batch build. Use
`scripts/atari_source.py check|convert` for Atari text. Build the small console
tools with `mise run apps -- mac2st`, `-- dump`, `-- fc` or `-- split`. Run
`mise run test:console-tools-neost`; it builds the tools and console driver,
then compares archived/rebuilt status, output and file effects through
`scripts/neost-console-tools-smoke.py`.
Host Python pinning does not change the Python installed inside the toolchain
container. Proprietary TOS ROMs and writable
MagiC disk images are supplied separately, never installed by mise.


### NeoST regression scenarios

Use `scripts/neost_server.py` as the canonical reusable client for scripted
NeoST 0.6.5 scenarios. It handles the server protocol and process lifecycle;
scenario scripts should use `scripts/neost_common.py` for emulator discovery
and fresh artifact directories, then supply task-specific setup and checks.
Machine options, ROM choice, writable fixtures, input timing, and expected
application behavior stay with each scenario rather than becoming generic
client policy. The NeoST skill at `.agents/skills/neost/SKILL.md` documents this
workflow. Run `mise run test:neost-server` for protocol-client coverage and
`mise run test:neost-helpers` for image, screen and session helpers.

After authenticating with `glab`, configure its credential helper for Docker-compatible
registry clients such as Podman, then build and push:

```sh
glab auth configure-docker
mise run toolchain:publish
```

The publish task builds and pushes
`registry.gitlab.com/nizmow/atari-mac-magic-sources/atari-toolchain:latest`.
Override `ATARI_TOOLCHAIN_IMAGE` to publish under a different image name or tag.

The image is built explicitly, not rebuilt before every command. Build steps
may need network access; running containers have networking disabled. The shared
wrapper mounts the repository at `/work`, preserves the caller's user ID and
stdin, and allocates a terminal only for interactive use. Override the image for
both build and run with `ATARI_TOOLCHAIN_IMAGE=localhost/my-atari-toolchain:tag`.

Mise is not required: `scripts/atari-container run make -f Makefile.atari magic`
uses the same wrapper directly, and the original Podman commands still work.

## Standalone application builds

Applications use `Makefile.apps`, separately from the register-ABI kernel:

```sh
mise run apps -- mod-app
# Without mise:
scripts/atari-container run make -f Makefile.apps mod-app
```

This produces `build/atari/apps/mod-app/MOD_APP.TTP`, linked with the supplied
MiNTLib's default stack ABI and 32-bit integers. It does not use the archived
Pure C objects/libraries. `BUILD_DIR`, `CFLAGS`, `CPPFLAGS`, `LDFLAGS` and `LDLIBS`
can be supplied as Make assignments; do not add kernel `-mshort/-mfastcall` flags.

MOD_APP passed 17 cases per executable against the archived original under the
GCC-built kernel in Hatari. Return codes matched, as did 48 pairs of captured
INF/BAK/DAT files: application/file-type/special-object registration, duplicate
and error handling, backups/cache invalidation, quoted/empty arguments and long
ARGV paths. Tests used disposable D: files; the real installation was untouched.
See [PORTING_NOTES.md](PORTING_NOTES.md) and [TODO.md](TODO.md) for the startup
compatibility finding and the next application tasks.

`mise run apps -- applicat` builds the association editor bundle in
`build/atari/apps/applicat/`: `APPLICAT.APP`, `applicat.rsc` (German) and
`applicat.inf` (minimal internal-icon associations). Run from that writable
directory; saving, or running with `-c`, generates `applicat.dat`. No archived
Pure C libraries or prebuilt DAT file are used. Test on a disposable MagiC
installation: saving also sends the rebuilt cache to its running desktop.

`mise run apps -- mgview` produces `build/atari/apps/mgview/MGVIEW.APP`.
It needs no RSC or companion data: pass text filenames through a TOS command
tail or ARGV. LF, CRLF, classic Mac CR, spaced paths with VFAT enabled, empty
files and keyboard navigation were compared with the archive under NeoST/MagiC.
See the catalogue and porting notes for verification limits.

`mise run apps -- mgedit` builds `build/atari/apps/mgedit/`: `MGEDIT.APP`,
the unchanged German `mgedit.rsc` and a source-built `EDITOBJC.SLB`.
`mise run apps -- edit-slb` builds the library independently. Run from the
bundle's directory on MagiC; the loader also supports its normal `SLBPATH`/
`GEMSYS\MAGIC\XTENSION` search. No archived Pure C objects are linked.
Archived/rebuilt comparisons cover editing/saving, clipboard, preferences,
New/Save As and quoted/ARGV paths. Binary-format regression checks:
`mise exec -- python -m unittest discover -s scripts -p 'test_elf_to_slb.py'`.

`mise run apps -- mgnotice` builds `build/atari/apps/mgnotice/`: `MGNOTICE.APP`,
the German `mgnotice.rsc` and the rebuilt `EDITOBJC.SLB`. It reuses the shared
`GEMUT_MT.C` helper and neutral `MagiC/APPS/GEMUT_MT_SDK.H`. Archived/rebuilt
NeoST/MagiC cases create, edit, save and reopen a note with identical screens
and byte-identical `.NOT` output.

`mise run apps -- mgsearch` builds `build/atari/apps/mgsearch/`: `MGSEARCH.APP`
and `MGSEARCH.RSC`. The target compiles the consumer-specific `GEMUTILS`/
`WINDOWS` variant and an app-local copy of `MAGXDESK.5/PATTERN.C` against public
GEMlib; the older `MAGXLIB.LIB` dependency is not linked.

`mise run apps -- mgcopy` builds `build/atari/apps/mgcopy/`: `MGCOPY.APP` and
its German `MGCOPY.RSC`. The port uses the shared `GEMUT_MT.C` helper, public
MiNTLib/GEMlib dialog calls and an app-local DOS compatibility layer.

`mise run apps -- mgformat` builds `build/atari/apps/mgformat/`:
`MGFORMAT.PRG` and its German `MGFORMAT.RSC`. It uses native MagiC `wdlg`
AES bindings; physical floppy operations are not covered by the NeoST test.

`mise run apps -- vt52` builds `build/atari/apps/vt52/`: `VT52.PRG` and its
German `VT52.RSC`, with `DRAGDROP` compiled from source. The BIOS/XBIOS hooks
use GCC's stack ABI. `mise run test:vt52-neost` compares an archived/rebuilt
terminal session with a delayed TOS child; the final screens match
pixel-for-pixel.

`mise run apps -- magxdesk` builds `build/atari/apps/magxdesk/MAGXDESK.APP`
from the version-5 C sources using MiNTLib/GEMlib and the default application
ABI; it does not use archived Pure C objects. `mise run test:magxdesk-neost`
compares the rebuilt and archived desktops on separate cloned MagiC images;
the desktop content matches pixel-for-pixel. Version 4 remains separate.

`mise run apps -- mcmd` builds the German command interpreter as
`build/atari/apps/mcmd/de/MCMD.TOS`. Select `MCMD_LANG=en` or `MCMD_LANG=fr`,
or build all three with `mise run apps -- mcmd-languages`. VASM emits the TOS
executable directly from normalized assembly; no archived MAS/ALN tools or
objects are used. `mise run test:mcmd-neost` compares batch execution and guest
file effects against the localized archives on cloned MagiC images.

`mise run apps -- magxconf` builds German `MAGXCONF.PRG` and `MAGXCONF.ACC`
in `build/atari/apps/magxconf/de/`. Select `MAGXCONF_LANG=en` or `fr`, or use
`magxconf-languages` for all six outputs. The same assembly executable detects
PRG versus ACC startup; no GCC ABI conversion or archived objects are needed.
Save modifies the executable itself, so install writable copies and test on
cloned images. `mise run test:magxconf-neost` exercises Cancel/OK/Save and
accessory boot persistence. This is not the separate MAGXCONF CPX.

Repeatable emulator checks use explicit, subsystem-scoped tasks. Run
`mise run test:mod-app-neost` (or `python3 scripts/neost-mod-app-smoke.py`).
Runner options and outputs: `python3 scripts/neost-mod-app-smoke.py --help`.
This tests MOD_APP under NeoST/EmuTOS; the MagiC kernel boot check remains under
Hatari.

Run `mise run test:mgclock-neost` (or `python3 scripts/neost-mgclock-smoke.py`)
to verify the restored desktop clock and its coexistence with APPLICAT. The
MagiC half needs `MAGIC_IMAGE`, `TOS206_ROM` and host `mtools`; see
[TESTING.md](TESTING.md).

Run `mise run test:mgnotice-neost` (or `python3 scripts/neost-mgnotice-smoke.py`)
to compare the archived and rebuilt MGNOTICE note lifecycle. It uses the same
MagiC image, TOS 2.06 and `mtools` inputs.

Run `mise run test:mgsearch-neost` (or `python3 scripts/neost-mgsearch-smoke.py`)
to compare the archived and rebuilt MGSEARCH dialog and `*.ZZZ` search-result
screens on identical cloned images.

Run `mise run test:mgcopy-neost` (or `python3 scripts/neost-mgcopy-smoke.py`)
to compare MGCOPY copy, move, alias and delete effects on cloned MagiC images.

Run `mise run test:vt52-neost` (or `python3 scripts/neost-vt52-smoke.py`)
to compare archived and rebuilt VT52 terminal sessions on cloned MagiC images.
The scenario launches a delayed TOS child and compares the final terminal
screen pixel-for-pixel.

Run `mise run test:mgformat-neost` (or `python3 scripts/neost-mgformat-smoke.py`)
to compare the archived and rebuilt MGFORMAT format dialog on cloned MagiC
images. Physical floppy formatting and disk copy are covered by
[MANUAL_TESTING.md](MANUAL_TESTING.md).

The `scripts/neost-*-smoke.py` runners use `scripts/neost_common.py` for
executable/AppImage discovery, ROM selection, bounded subprocess execution,
host-log capture, fresh output directories and streaming file hashes.
Discovery prefers `--neost-headless`, then `PATH`, then AppImage extraction in
temporary storage. `--rom` overrides the bundled EmuTOS ROM. Existing artifact
directories are never reused; nonzero emulator exits and timeouts identify the
host log, with partial process output retained on timeout.

`scripts/neost_image.py` owns disposable image cloning, `MAGX.INF`
verification, guest directory creation and `mcopy` in/out operations.
`scripts/neost_screen.py` owns PPM parsing, pixel-diff bounding boxes and
region containment. `scripts/neost_host.py` is the optional session layer over
those helpers and `NeoSTServer`; see [NeoST_HOST.md](NeoST_HOST.md). Keep
machine/ROM choices, frame budgets, mouse/key input, fixtures and application
assertions in each runner; use the shared modules for the host mechanics.
Shared-host-helper checks:

```sh
mise run test:neost-helpers
```

These checks cover output-directory safety, ROM override behavior, failure logs,
timeout output retention and image/screenshot helpers; they do not replace the
application smoke runs.

## Direct build commands

Build the German Atari/68000 kernel from the repository root:

```sh
podman run --rm --network=none --userns=keep-id -v "$PWD:/work" -w /work \
  localhost/magic-atari-toolchain:latest make -j4 -f Makefile.atari magic
```

The output is `build/atari/MAGIC.RAM`. `kernel.elf` retains symbols and relocations;
`kernel.map` records the link layout. GCC builds the original C modules and VASM
assembles the original BIOS/GEMDOS/VDI/AES modules. The link follows
`MagiC/KERNEL/BUILD/ATARI/M.INP`, with the BIOS header first and `ENDE` last.
The packager emits the TOS header and delta relocation table expected by
`MAGXBOOT`; zero-initialized storage is included in DATA because that loader
copies only TEXT+DATA. It checks the BIOS/AES headers and movable pointers.

This is a kernel build, not a rebuilt installation/release ZIP: the desktop,
accessories, external drivers and boot loader are not rebuilt by this target.
For Hatari, keep a known-working original installation as the baseline, clone
its disk image, and replace only `MAGIC.RAM` in the clone. Use the original
`MAGXBOOT` and matching `GEMSYS` files. Both the original release and the GCC
kernel have reached MAGXDESK under Hatari 2.5.0: MegaSTE, 68000 at 16 MHz,
4 MiB RAM, monochrome, TOS 2.06 and HDDRIVER 10.18. This verifies startup,
not general functional correctness, stability or performance.

The VDI `SETUP.C` listed in `MAGICVDI.PRJ` is linked alongside the `M.INP`
modules. VASM's `-spaces` option is required for MAS expressions such as relative
keyboard dispatch entries. UTF-8 assembly strings are encoded as Atari ST bytes;
legacy byte strings are preserved. Legacy compiler warnings remain visible.

Build the filesystem kernel component from the repository root:

```sh
podman run --rm --userns=keep-id -v "$PWD:/work" -w /work \
  localhost/magic-atari-toolchain:latest make -f Makefile.atari filesystem
```

This produces `build/atari/filesystem.o`, a relocatable 68000 ELF component linking:

- GCC-built `DOS/VFAT.C` (long-filename record creation and directory reading);
- assembly `DOS/VFAT_S.S` (device-call glue and LFN checksum);
- assembly `DOS/XFS_VDOS.S` (DOS filesystem driver);
- assembly `ALLG/SRC/STD.S` (kernel memory, string and other runtime routines).

`make -f Makefile.atari vfat` still builds only `build/atari/vfat.o`.
The C flags retain 16-bit integers and register calls:
`-std=gnu11 -mshort -mfastcall -ffreestanding -fno-builtin -fno-strict-aliasing`.
Both C and assembly compilation check shared structure offsets.
C include aliases and MAS-to-VASM inputs are generated under `build/atari`.
The syntax adapter handles comments, include names, zero-based `OFFSET` blocks,
named macro arguments/local labels and expression spelling; instructions remain
Motorola assembly. Selected complete STD routines get separate ELF sections for
garbage collection. Its case-conversion tables use explicit Atari ST bytes so
UTF-8 source encoding cannot change their layout.
Kernel calls retain register arguments; explicitly `cdecl` callbacks retain stack arguments.
The image's MiNTLib/GEMlib binaries use the default stack ABI: do not link them into
the register-ABI kernel object.

The C/assembly regression harness runs without Atari ROMs. Compile it in the
container, then link and run on a host with M68k GNU binutils and QEMU user emulation:

```sh
podman run --rm --userns=keep-id -v "$PWD:/work" -w /work \
  localhost/magic-atari-toolchain:latest \
  make -f Makefile.atari filesystem build/atari/vfat-check.o
m68k-linux-gnu-ld -Ttext=0x10000 --gc-sections -z noexecstack -e _start \
  build/atari/vfat-check.o build/atari/vfat-glue.o build/atari/std.o \
  -o build/atari/vfat-check.elf
qemu-m68k build/atari/vfat-check.elf
```

The harness exits zero on success. It exercises actual C/assembly register calls
through a bounded in-memory directory device: creates multi-record long filenames
with an accented character, reads them back, checks the 8.3 alias, rejects short
output buffers and falls back to the alias after a corrupt LFN checksum.
It also checks Unicode termination/padding/fallback, the on-disk checksum and
Atari extended-character case conversion. The low link address is essential:
MagiC uses negative LONGs for errors and expects successful pointers below 2 GiB.
GCC currently warns that `nxt_longname` may be uninitialized in `_vf_readdir`;
the pointer is initialized when long-name state becomes valid, and the warning
has not been suppressed.

The standalone filesystem component still imports kernel services such as
`_fread`, `_fwrite`, `__fseek`, `Mxalloc`, `_lmul`, `act_pd`, BIOS state and AES
critical-section routines. Use the `magic` target above for the complete link;
the component is not independently bootable and contains no substitute services.

# Screenshots
<img alt="No" src="assets/magxdesk - 256 colours.png" width="1024">

# License

The files are licensed according to GPLv3, see LICENSE file.

# External Licenses

MVDI and sources have been provided by courtesy of Sven und Wilfried Behne and distributed under the main license.
