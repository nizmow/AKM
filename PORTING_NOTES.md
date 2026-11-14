# GCC/VASM porting notes

These notes cover the modern **German Atari/68000 kernel** and the first
standalone GCC application port. Start with `README.md`, `Makefile.atari`
(kernel), `Makefile.apps` (applications) and `TODO.md` (next tasks).
Prefer small, reviewable source changes. Keep kernel compatibility work in
`MagiC/KERNEL/BUILD/GCC/`; do not use those private headers or ABI flags as an
application SDK.

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

The original Atari-encoded source was converted to UTF-8 comments, preserving
non-ASCII C literal bytes with octal escapes. Do not transcode target strings
implicitly while making a source file editable.

**Startup compatibility is functional behavior.** The installed MiNTLib parser
split a double-quoted path in a raw TOS command tail when the tail began with an
ordinary option. Original PCMXSTRT handled it. See the
[upstream parser](https://github.com/freemint/mintlib/blob/master/mintlib/crtinit.c).
MOD_APP now privately tokenizes raw tails with single/double quotes and doubled
apostrophes, while retaining MiNTLib's parsed ARGV vector for extended arguments.
Keep this application-local until a second real consumer establishes a shared
startup requirement; do not copy it blindly or port the entire old CRT.

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
4. Identify the source encoding instead of guessing from a successful decode.
   Atari ST, CP437 and Latin-1 differ. Convert comments independently from target
   literal bytes; preserve the latter with explicit escapes where needed.
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
| Text/records change size unexpectedly | Separate source encoding from target bytes. The shared assembler adapter encodes UTF-8 literals using the canonical Atari mapping and preserves legacy byte input. For legacy C input, pair `-finput-charset=ISO-8859-1` with `-fexec-charset=ISO-8859-1` when preserving raw bytes. Do not apply those flags indiscriminately to UTF-8 C sources. |
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
