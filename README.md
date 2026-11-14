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

Mise is a thin front end; Make still owns dependencies, compiler flags and
linking. Podman and mise must already be installed on the host. Review and trust
the project configuration, then use:

```sh
mise trust
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

Repeatable emulator checks use explicit, subsystem-scoped tasks. Run
`mise run test:mod-app-neost` (or `python3 scripts/neost-mod-app-smoke.py`).
Runner options and outputs: `python3 scripts/neost-mod-app-smoke.py --help`.
This tests MOD_APP under NeoST/EmuTOS; the MagiC kernel boot check remains under
Hatari.

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
