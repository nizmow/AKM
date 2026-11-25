# Testing

## NeoST: MagiC disk fixture

For the full set of MagiC smoke tests, use a known-bootable MagiC hard-disk
image with two `mtools`-readable FAT partitions that the guest sees as `C:` and
`D:`. Set `MAGIC_IMAGE` to the whole disk image and `TOS206_ROM` to your
TOS 2.06 ROM. Install `mtools` on the host. The image must already have
working hard-disk boot support; the reference fixture uses `HDDRIVER.SYS`.
The runners do not partition a blank disk or install a boot driver.

| Guest drive | Source | Default byte offset |
|---|---|---:|
| `C:` | MagiC boot volume; must contain `MAGX.INF` and `AUTO/MAGXBOOT.TOS` | `1024` |
| `D:` | Second disk partition, required for the MGCOPY cross-drive case; not required by the release smoke | `34200576` |
| `E:` | Host exchange directory where used; not a disk partition | — |

The runners clone the source image, inject files into the clone with
`mtools`, and attach the whole clone as an ACSI disk. Use
`--partition-offset` for a different `C:` offset and MGCOPY's
`--d-partition-offset` for `D:`. These options tell `mtools` where to read
and write; they do not change the partition table or guest drive letters.
To check the default offsets without writing or booting:

```sh
mdir -i "${MAGIC_IMAGE}@@1024" ::/MAGX.INF ::/AUTO/MAGXBOOT.TOS
mdir -i "${MAGIC_IMAGE}@@34200576" ::/
```

Scenarios that use `E:` create `GEMDRIVE/E` under the run output and pass the
parent `GEMDRIVE` directory to NeoST with `--gemdos`. Files written to guest
`E:\` can then be read directly on the host. No `E:` partition or manual
GEMDOS drive configuration is needed for those scenarios.

## NeoST: console tools

`mise run test:console-tools-neost` builds `MAC2ST.TTP`, `DUMP.TTP`, `FC.TTP`,
`SPLIT.TTP` and the generic Pexec driver, then runs
`scripts/neost-console-tools-smoke.py`. Every case gives the archived and rebuilt
binary the same disposable `C:` fixture and checks the case's declared child
status and generated guest outputs, plus console output. Fixture inputs are not
counted as outputs. Missing-file cases declare the tools' intentional
`Pterm(ENOENT)` status 65503; a negative Pexec loader status fails the case.

Cases cover:

- MAC2ST CR-to-CRLF console filtering.
- DUMP default and `-c` C-source output.
- FC equal, differing, WORD (`-w`) and LONG (`-l`) comparisons.
- SPLIT at an explicit hexadecimal boundary with two named output files.
- Missing-file status and console behavior for all four tools.

The runner uses NeoST's bundled EmuTOS by default and does not require `mtools`.
It uses a fresh artifact directory, isolated guest trees, and the generic
`scripts/neost-console-compare.py` case runner. It does not exercise interactive
FC input, directory/case-insensitive variants, MagiC-kernel behavior or
long-term stability.

## NeoST: MGCLOCK desktop clock and MagiC coexistence

`scripts/neost-mgclock-smoke.py` builds `MGCLOCK.PRG` and `APPLICAT.APP`, then
runs four bounded scenarios:

1. EmuTOS with `MGCLOCK.PRG` as the autostart application. The desktop clock
   area must change after initialization, proving the VDI timer loop runs.
2. MagiC with only `MGCLOCK.PRG` in `C:\GEMSYS\MAGIC\START\`.
3. MagiC with only the APPLICAT bundle in that startup folder. The same desktop
   area must stay pixel-identical across frames; this is the timer control.
4. MagiC with both applications. The clock must keep updating, while APPLICAT's
   windows remain pixel-identical to the control run.

The runner clones the supplied MagiC image for every scenario and installs files
with `mcopy`; the source image is never written. It rejects an image whose
tested partition offset does not contain `MAGX.INF`.

Required for the MagiC scenarios:

- A known-bootable MagiC ACSI image with `C:\AUTO\MAGXBOOT.TOS`.
- A TOS 2.06 ROM accepted by NeoST.
- `mtools` on the host.

Run with a fresh output directory:

```sh
MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img \
  python3 scripts/neost-mgclock-smoke.py --output build/atari/neost-mgclock-check
```

Use `--partition-offset` if the MagiC `C:` partition does not start at byte
1024 (`@@1024` in mtools syntax). `--skip-emutos` or `--skip-magic` isolates one
half of the check. Screenshots, `protocol.log` and `diagnostics.log` stay in
each scenario's `server/` directory.

The test compares pixels, not process exit status. It establishes the restored
desktop drawing, one-second timer updates and coexistence with APPLICAT under
the tested MagiC kernel; it does not measure CPU load, long-term stability or
behavior under a different desktop/OS version.

## NeoST: MGNOTICE archived/rebuilt lifecycle

`scripts/neost-mgnotice-smoke.py` builds `MGNOTICE.APP`, its German resource,
the rebuilt `EDITOBJC.SLB` and the `NEOSTART.PRG` test helper. It then runs the
archived and rebuilt bundles in separate clones of the same MagiC image:

1. Start MagiC and let `NEOSTART.PRG` bring MGNOTICE to the foreground after
   the desktop settles.
2. Create a note with Ctrl-N, type `hello`, leave edit mode with Esc, and quit
   with Ctrl-Q.
3. Compare every UI state and the saved `0.NOT` file.
4. Reboot the same image and compare the reopened note window.

MGNOTICE opens `EDITOBJC.SLB` with a NULL search path, so each variant's library
is installed in the default `GEMSYS\MAGIC\XTENSION` directory after removing any
`SLBPATH` override and `#_SLB` preload of that name; inherited `0.NOT` files are
cleared before the run. A third boot with the library removed from the clone
must produce neither the application UI nor a saved note. This control proves
the lifecycle used the library selected for that variant; the archived and
rebuilt `EDITOBJC.SLB` files are different builds.

The delayed launcher is required because MagiC starts `#_APP` entries before
MAGXDESK owns the menu bar; MGNOTICE needs foreground windows for input.

Required: a MagiC image with `C:\AUTO\MAGXBOOT.TOS`, TOS 2.06, and host
`mtools`. Run with fresh output:

```sh
MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img \
  python3 scripts/neost-mgnotice-smoke.py --output build/atari/neost-mgnotice-check
```

Use `--partition-offset` for a different MagiC partition offset. The source
image is cloned for each binary. Delete, font/colour/options, multiple windows,
resize and alternate resources are outside this scenario.

## NeoST: MGSEARCH archived/rebuilt dialog and search

`scripts/neost-mgsearch-smoke.py` builds `MGSEARCH.APP`, its German resource and
the delayed `NEOSTART.PRG` helper, then runs the archived and rebuilt bundles in
separate clones of the same MagiC image:

1. Start MagiC and let `NEOSTART.PRG` launch MGSEARCH from `GEMSYS\GEMDESK`.
2. Type `*.ZZZ` into the search dialog and submit it with Enter.
3. Compare the initial, typed and result screens.
4. Repeat with `C:\SEARCHFIX\ALPHA.ZZZ` and nested
   `C:\SEARCHFIX\SUB\BETA.ZZZ` installed in the image.

Required: a MagiC image with `C:\AUTO\MAGXBOOT.TOS`, TOS 2.06, and host
`mtools`. Run with fresh output:

```sh
MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img \
  python3 scripts/neost-mgsearch-smoke.py --output build/atari/neost-mgsearch-check
```

Use `--partition-offset` for a different MagiC partition offset. The source
image is cloned for each binary. Drive/path forms, size/date filters, file-open
messages, font variants and long-term operation are outside this scenario.

## NeoST: MGFORMAT startup dialog

`mise run test:mgformat-neost` builds `MGFORMAT.PRG`, its German resource and
`NEOSTART.PRG`, then boots archived and rebuilt bundles in separate clones of
the same MagiC image. `MAGX.INF` is pinned to load `GEMSYS\MAGIC\START` (any
`#_AUT` is removed) so the helper cannot be skipped by an inherited startup
path; the helper invokes `C:\MGFORMAT.PRG` with `-f A:` after the desktop is
ready. The scenario compares the dialog title, name label, name field and body,
excluding the desktop free-bytes status line and the blinking caret column, and
requires two settled captures to agree. A third boot with only
`C:\MGFORMAT.PRG` removed must differ substantially in those regions, proving
the archived/rebuilt dialog came from the installed program.

Required: a MagiC image with `C:\AUTO\MAGXBOOT.TOS`, TOS 2.06, and host
`mtools`. Run with fresh output:

```sh
MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img \
  python3 scripts/neost-mgformat-smoke.py --output build/atari/neost-mgformat-check
```

Physical formatting and disk-copy effects are outside this scenario: NeoST
reports a write fault when MGFORMAT starts a format operation. Those paths are
covered by the real-hardware matrix in [MANUAL_TESTING.md](MANUAL_TESTING.md);
cancellation, alternate resources and long-term threading remain unverified.

## NeoST: MAGXDESK v5 desktop

`mise run test:magxdesk-neost` builds the version-5 desktop and boots the
archived and rebuilt `MAGXDESK.APP` with the same German resources on separate
clones of the supplied MagiC image. `MAGX.INF` is pinned to
`#_SHL C:\GEMSYS\GEMDESK\MAGXDESK.APP` and any `#_AUT` entry is removed, so the
installed program is the tested shell rather than an inherited desktop path.

Each boot runs 15000 frames, parks the mouse outside the compared areas, and
takes two captures 600 frames apart. The desktop window title, window content
and icon area must be identical between the two captures and between archived
and rebuilt runs; the menu-bar clock and free-space status line are excluded. A
third boot with only `MAGXDESK.APP` removed must still boot and differ in the
compared regions (`>= 500` primary pixels, `>= 1000` total), proving the desktop
came from the installed program. Coordinates assume the pinned 640x400 mono
profile; interaction, dragging and alternate desktop layouts remain untested.

## NeoST: VT52 terminal session

`mise run test:vt52-neost` builds `VT52.PRG` with the source-built `DRAGDROP`,
its German resource and a delayed launcher. `MAGX.INF` is pinned to load
`GEMSYS\MAGIC\START` (any `#_AUT` is removed), and the archive or rebuild is
installed at `C:\VT52.PRG` beside `VT52.RSC` and `VT52DEMO.TOS`. The launcher
starts VT52 and the child writes a fixed line through the BIOS console path.

Two settled captures, 600 frames apart, must agree in the window title and both
output lines; the cursor and scrollbar columns and the mouse are excluded. A
third boot with only `C:\VT52.PRG` removed must differ from both positive runs
(`>= 200` primary pixels, `>= 1000` total), proving the terminal window came
from the installed program. Interactive keyboard/mouse use remains untested.

## NeoST: MGCOPY file operations

`mise run test:mgcopy-neost` builds `MGCOPY.APP`, its German `MGCOPY.RSC` and
`NEOSTART.PRG`, then runs the rebuilt bundle on a cloned MagiC image. The
delayed driver invokes MGCOPY through `Pexec` with `-q` and records the seven
child statuses in `C:\MGCOPY.RES`.

The clone starts without `C:\DEST` or a stale `MGCOPY.RES`. The scenario copies
`SRC.TXT`, moves `MOVE.TXT`, aliases `ALIAS.TXT` and deletes `DEL.TXT`
into/from `C:\DEST\`. It requires seven numbered `CASE nn 0` statuses and
checks the intended file effects directly:

- Copy: `DEST\SRC.TXT` exists and matches the source.
- Move: `DEST\MOVE.TXT` exists and the source is gone.
- Alias: `DEST\ALIAS.TXT` exists and the source remains.
- Delete: `DEL.TXT` is gone.
- Nested copy/move: `TREE` and `MTREE` contents and subdirectories match;
  the move removes the source tree, while the copy leaves it.
- Cross-drive move: `D:\XMOVE.TXT` is copied to `C:\DEST\XMOVE.TXT` and
  removed from D:.
- Every copied, moved or retained file is byte-identical to its seeded source;
  the alias destination is a MagiC `Fsymlink` record naming `C:\ALIAS.TXT`,
  not a byte copy.
- Absence is confirmed with the fail-closed `mdir` check, so an extraction error
  cannot masquerade as a deleted file.

The historical release archives are retained for reference, not used as a
pass/fail oracle for MGCOPY.

Required: a MagiC image with `C:\AUTO\MAGXBOOT.TOS`, TOS 2.06, and host
`mtools`. Run with fresh output:

```sh
MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img \
  mise run test:mgcopy-neost -- --output build/atari/neost-mgcopy-check
```

Use `--partition-offset` for a different MagiC C: partition offset and
`--d-partition-offset` for D: (default `34200576`, matching the test image).
Confirm-dialog interaction, cancellation, long paths and general error paths
are outside this scenario.

A separate disposable-floppy probe checks the failure contract in
[PORTING_NOTES.md](PORTING_NOTES.md#mgcopy): overwrite a 200 KiB destination
with a 600 KiB source that cannot fit, then require a nonzero `Pexec` status,
an unchanged old destination hash, a retained source, and no `~MGCPnn.*`
staging files. The archived binary returned zero, replaced the destination
with a partial 600 KiB file and deleted the moved source. Dialog-driven
cancel/rename/skip remain untested.

## Shared host-side helpers

`scripts/neost_image.py` wraps disposable image cloning, `MAGX.INF`
verification, guest directory creation, fail-closed presence/removal checks and
`mcopy` in/out operations. `exists()` reports a missing guest path only for
mtools' explicit not-found result and raises on media errors; `remove()` proves
the path is gone, so stale guest state cannot satisfy a scenario.
`scripts/neost_screen.py` parses NeoST PPM screenshots and computes pixel diffs
and bounding boxes. `scripts/neost_host.py` combines those helpers with
`NeoSTServer` for one booted session; its API and limits are documented in
[NeoST_HOST.md](NeoST_HOST.md). A clean session requires NeoST to acknowledge
`quit` and exit zero; context-manager shutdown preserves an earlier scenario
error instead of masking it. Scenario runners keep their own machine profile,
input timing, fixtures and assertions.

Run the helper regression checks with:

```sh
mise run test:neost-helpers
```

## NeoST: MagiC SHUTDOWN smoke

This is a manual, opt-in smoke test. NeoST's default EmuTOS ROM cannot verify
MagiC shutdown; this scenario boots the existing MagiC installation under TOS
2.06 and stops at its successful-shutdown confirmation dialog.

### Required artifacts

- NeoST `neost-headless` (or its AppImage) and a known version.
- A legally obtained TOS 2.06 ROM accepted by NeoST.
- A known-bootable MagiC ACSI disk image containing `C:\AUTO\MAGXBOOT.TOS`,
  `MAGIC.RAM`, and `MAGX.INF`.
- The matching GEMDOS host directory, if the Hatari setup mounts one (the tested
  setup provides E: from a `GEMDRIVE` tree).
- `mtools` (`mcopy`) and the GCC-built `SHUTDOWN.PRG` from
  `mise run apps -- shutdown`.

Do not commit ROMs, disk images, or other licensed installation files. Keep the
source image and GEMDOS directory untouched; the shutdown test must use fresh
copies. Record the hashes of both inputs and the NeoST version with each run.

### Install the binary under test

Set `MAGIC_DISK`, `TOS206_ROM`, and `GEMDRIVE_ROOT` to the local artifacts and
use a fresh output directory. MagiC's desktop launches
`C:\GEMSYS\GEMDESK\SHUTDOWN.PRG` when shutdown is requested. Replace that file,
not a copy in `GEMSYS\MAGIC\START`: a START copy only starts another initiator
and can leave the archived GEMDESK binary to draw the confirmation, so it does
not exercise the rebuilt final flush.

```sh
mise run apps -- shutdown
OUT=$(mktemp -d build/atari/neost-magic-shutdown.XXXXXX)
cp --reflink=auto "$MAGIC_DISK" "$OUT/magic.img"
cp -a "$GEMDRIVE_ROOT" "$OUT/GEMDRIVE"
{
  "$HEADLESS" --version
  sha256sum "$MAGIC_DISK" "$TOS206_ROM"
  find "$GEMDRIVE_ROOT" -type f -print0 | sort -z | xargs -0 sha256sum
} >"$OUT/inputs.txt"
mcopy -i "$OUT/magic.img@@1024" \
  build/atari/apps/shutdown/SHUTDOWN.PRG \
  ::/GEMSYS/GEMDESK/SHUTDOWN.PRG
```

The tested ACSI image's system partition starts at sector 2, hence the `@@1024`
byte offset. Check the partition table and the `#_APP`/`#_DSK` entries in
`MAGX.INF` before using a different installation; do not assume these match.

### Run

Start the image in NeoST, wait until MAGXDESK owns the desktop, then press
Ctrl–Alt–Del. For a bounded headless run, use the shared server client with
`headless`, `rom`, an existing empty `out` directory, the cloned image path,
and `guest_root` set as above:

```python
from neost_server import NeoSTServer

with NeoSTServer(headless, rom, out, options=[
        "--machine", "megaste", "--mem", "4m", "--mono", "--no-disk",
        "--acsi", str(magic_img), "--gemdos", str(guest_root),
        "--auto", r"C:\AUTO\MAGXBOOT.TOS"]) as server:
    server.run_frames(10000)
    server.chord([0x1D, 0x38, 0x53])   # Ctrl, Alt, Del make codes
    server.run_frames(6000)
    server.screenshot(out / "final.ppm")
```

The fixed frame budget reproduces the current NeoST boot sequence. If the ROM,
image, or emulator changes, inspect the screen and recalibrate rather than
treating elapsed frames as proof of a successful boot.

Pass only when the screenshot shows MagiC's **“Shutdown was successful”**
confirmation. Do not activate Restart or Cold Boot: rebooting the test image
re-launches the desktop helpers. Preserve the screenshot and headless log as
run evidence. This verifies the shutdown request through the confirmation
dialog, not the final reboot/cold-boot path.

To check the failure branch, build a variant whose final flush reports an
error (a throwaway build is enough), install it at the same GEMDESK path, and
repeat the run. It must show the localized write-error alert
(“Schreibfehler beim Sichern.” in German) and stay at the desktop; it must not
reach the success alert or either reboot choice. Record that evidence
separately from the normal run.

## NeoST: localized MCMD batch execution

`mise run test:mcmd-neost` builds the DE/EN/FR interpreters and guest drivers,
then compares each archive with its rebuilt counterpart on separate clones
of the supplied MagiC image:

```sh
MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img \
  mise run test:mcmd-neost
```

Host `mtools` is required. `--partition-offset` changes the default C: offset
of 1024 bytes; `--languages en` restricts the run. The machine profile is
MegaSTE/4 MiB/monochrome, with a delayed launcher installed in the image's
`GEMSYS\MAGIC\START` folder. Results, console logs and screenshots remain in a
fresh `build/atari/neost-mcmd-*` directory.

Each clone clears the outputs the batch creates (including `BAD.TXT`, `CHILD.RES`
and `WORK`) but keeps the seeded `INPUT.TXT`, so inherited files cannot satisfy
a result. `CHILD.RES` and `MCMD.RES` must be freshly written, with
`MCMD.RES` byte-equal to `STATUS 0\r\n`; absence probes use the fail-closed
`mdir` check. Checks cover batch argument/variable substitution, internal
quoting, output and append redirection, an external child's
stdin/stdout/environment and raw tail interpretation, copy/rename/delete and
directory effects, conditionals, GOTO, missing commands, and return to the
launcher. Guest contents are checked independently and compared byte-for-byte
between archived and rebuilt runs.
`PORTING_NOTES.md` records the retained external-quote and trailing-directory
behaviors. Interactive editing, pipes, long filenames and AUTOEXEC/BOOT startup
remain untested.

## NeoST: MAGXCONF configuration dialog and persistence

```sh
MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img \
  mise run test:magxconf-neost
```

The runner builds DE/EN/FR program/accessory pairs and the guest query probe,
then uses separate cloned MagiC images for each archive/rebuilt case. It drives
Cancel, OK and Save, checks runtime configuration masks, verifies the exact
four-byte executable modification, and reboots each accessory case to check
restoration from the saved default.

Required: NeoST, TOS 2.06, a bootable MagiC image and host `mtools`. The profile
is MegaSTE/4 MiB/monochrome with C: at offset 1024 by default. The fixture uses
`C:\AUTO\ACCS`, `C:\GEMSYS\MAGIC\START`, and a disposable host-backed E: for
guest query results. Its desktop menu has MAGXCONF before Control Panels;
different installations need their own activation coordinates.

Use `--languages de` or `--modes p`/`a` to narrow the scenario. Evidence goes to
a fresh `build/atari/neost-magxconf-*` directory. Screenshots preserve source/
archive caption differences; switch state margins, action-button borders and
numeric version/date fields are compared pixel-for-pixel. The competing MAGXCONF
CPX is disabled only in the clones so its boot initializer cannot overwrite the
accessory's saved configuration. See `PORTING_NOTES.md` for the self-save contract and
remaining verification limits.

## NeoST: MAGXCONF.CPX boot initializer and dialog

`mise run test:magxconf-cpx-neost` builds the rebuilt CPX and the MAGXCONF
configuration probe, then runs archived and rebuilt modules in separate clones
of the same MagiC image:

```sh
MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img \
  python3 scripts/neost-magxconf-cpx-smoke.py
```

The runner installs a scratch CPX copy with `SCB_NFAST` as its saved default, so
the boot initializer's effect is observable. After boot it requires the guest
query to report `00000010`, opens Control Panels, double-clicks MagiC Config.,
and compares the dialog screenshots. The fixture must have COPS in
`C:\AUTO\ACCS` and the CPX mirrored in `C:\AUTO\ACCS\CPX`; menu and list
coordinates are pinned to this installation. Other CPX managers, Save-button
confirmation and switch toggling remain outside the scenario.

## CPX foreign-ABI checks

```sh
mise run lint:cpx-abi
mise run test:cpx-abi
```

The lint defaults to `MagiC/APPS/CPX/MAGXCONF.C` and `TSLICE.C`. Explicit
source paths can be passed to `python3 scripts/cpx_abi_lint.py`. Known XCPB
callback members may only be used as the first argument to their approved
bridge; direct calls, pointer extraction and the wrong bridge are rejected.
Scalar XCPB fields are unaffected. The MAGXCONF and TSLICE CPX builds run this
gate even when their objects are up to date.

This is a token-based policy for known callback field names, not a C type
checker: it ignores comments/literals and does not resolve preprocessing,
pointer arithmetic or arbitrary aliases of the XCPB storage. It is not a
repository-wide ABI safety proof. Extend the source list and callback mapping
explicitly as consumers are ported.

A lint failure is a bridge-policy violation, not proof of a runtime failure.
Because matching uses field names rather than types, unrelated structures with
the same member names can produce false positives. Historical sources may also
be correct under their original compiler despite failing the GCC-port policy.
Treat findings as ABI boundaries to investigate or adapt, not confirmed crashes.

The regression task runs the lint tests and assembles the production
`cpx_entry.S` with the existing Atari toolchain container. A standalone m68k
ELF fixture executes under host `qemu-m68k`; no ROM, disk image, Linux cross
compiler or Atari application startup is needed. Synthetic native callbacks
check argument order and word widths, clobber D2/A2, and return sentinel
values. The caller checks register preservation and stack balance. Temporary
builds are isolated and removed after the run; missing prerequisites fail
rather than silently skipping coverage.

These tests verify bridge contracts, not behavior of a real CPX manager.
A regression failure demonstrates a failure under the fixture's tested ABI
contract. Passing establishes coverage of those cases in the assembled bridge,
not every possible call, real manager or whole-program ABI correctness.
The NeoST MAGXCONF.CPX scenario remains the application-level check.

## NeoST: DIS1/DIS2 disassembler

`mise run test:dis-neost` builds both passes and the generic console driver,
then runs the archived and rebuilt binaries against the same bundled `TST`
corpus under NeoST's EmuTOS:

```sh
python3 scripts/neost-dis-smoke.py
```

Each case gets its own disposable GEMDOS C: tree and declares its expected child
status and generated guest outputs; fixture inputs are excluded. The no-argument
and missing-file cases declare the tools' intentional `Pterm(1)` status, and a
negative Pexec loader status fails the case. Child console text is not checked:
NeoST's console log contains only the driver's `STATUS` line, and its screenshot
is taken after the driver returns to the desktop. Banners, usage and diagnostics
remain a documented gap.

- DIS1 on `T.PRG`, `U.PRG`, `T.O`, `-3 T.PRG`, a recognized DRI object
  (`KAOS/APPS/KAOSDESK.SRC/ICNEDIT/IMPORT/PCMXSTRT.O`) and no arguments.
  `T.O` is a Pure C (`0x4efa`) object, not a DRI object.
- DIS2 raw input (`-e`) with default, code/address/ASCII, 68030 and MAS/absolute
  output options, a hexadecimal range, signed 16-bit displacement and MOVEM
  reversed-mask fixtures, and a missing file.
- DIS2 with archived DIS1 workfiles for `T.PRG`, after setting the fixture
  `.rlo` symbol length to 4. The corpus executables have no symbol table, and
  both the archived and rebuilt DIS2 reject `Malloc(0)`.
- DIS2 with the archived DRI object workfiles and symbol table, exercising the
  object relocation-map path and source output.

No MagiC image, ROM download or `mtools` is needed because the tools use only
GEMDOS and console output. Interactive editing, `-t` stdout capture, memory
(`-m`) disassembly and MagiC-kernel-specific behavior remain untested. The
generated `.s` text is compared byte-for-byte against the archive, not
assembled or executed.

## NeoST: TSLICE.CPX dialog

`mise run test:tslice-cpx-neost` builds `TSLICE.CPX`, installs archived and
rebuilt copies into separate clones of the same MagiC image, and opens Control
Panels through the desktop menu:

```sh
MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img \
 python3 scripts/neost-tslice-cpx-smoke.py
```

It double-clicks the `MagiC Timeslice` CPX, requires the dialog to differ from
the Control Panels screen, and compares archived/rebuilt dialogs
pixel-for-pixel. The fixture must have COPS in `C:\AUTO\ACCS`; the pinned list
coordinate is installation-specific. The cloned image is cleaned of a stale
`ZCNFTEST.PRG` probe when present. Slider dragging, Save confirmation and
scheduler behavior under load remain outside the scenario.

## NeoST: COPS control-panel manager

`mise run test:cops-neost` builds `build/atari/apps/cops/COPS.APP` and runs
`base/cops/tests/neost-cops-smoke.py`. Each case clones the MagiC image and first
requires `MAGX.INF` to name `C:\GEMSYS\MAGIC\START` for `#_APP` and
`C:\AUTO\ACCS` for `#_ACC`. It disables `AUTO\ACCS\COPS.ACC` by renaming it to
`.ACX`, deletes inherited `C:\CPX\*.CPX`/`*.CPZ` and `COPS.INF` settings from
the clone, installs only `MAGXCONF.CPX` and `TSLICE.CPX`, and installs the
archived or rebuilt `COPS.APP` in `GEMSYS\MAGIC\START`:

1. Boot MagiC and bring the COPS panel forward.
2. Double-click `MagiC Config.` and capture the CPX dialog.
3. Cancel it and require the dialog area to be restored; only the
   double-clicked icon's selection highlight may remain.
4. Double-click `MagiC Timeslice` and capture that dialog.

Every capture parks the pointer at the same neutral desktop position first, so
a screen difference cannot come from cursor movement. The runner requires each
dialog to differ from the panel, the two dialogs to differ from each other, and
compares the panel, the restored panel and both dialogs pixel-for-pixel between
archive and rebuild.

Required: a MagiC image with `C:\AUTO\MAGXBOOT.TOS`, TOS 2.06, and host
`mtools`. Run with fresh output:

```sh
MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img \
  mise run test:cops-neost
```

Use `--partition-offset` for a different MagiC partition offset. Evidence stays
in `build/atari/neost-cops-*`. Accessory mode, CPX board editing, Save/alerts,
event-CPX callbacks and CPXs beyond the two fixtures remain untested.

## NeoST: COPS accessory mode

`mise run test:cops-accessory-neost` builds `build/atari/apps/cops/COPS.APP`
through `base/cops/tests/neost-cops-accessory-smoke.py`, clones the MagiC image,
removes inherited `GEMSYS\MAGIC\START\COPS.*` and
`AUTO\ACCS\COPS.ACC`, then installs the Release accessory, the rebuilt
executable under the same `COPS.ACC` guest name, or nothing for the control.
It fixes `TSLICE.CPX` and requires `MAGX.INF` to keep `#_ACC C:\AUTO\ACCS` and
`#_APP C:\GEMSYS\MAGIC\START`.

The runner opens the MAGiC menu and requires its accessory item to differ from
the no-accessory control, then opens the panel, double-clicks `MagiC
Timeslice`, cancels it, closes the panel and reopens it from the menu. Captures
park the pointer at a neutral position. Archive and rebuild must match
pixel-for-pixel except the double-clicked icon's selection highlight and cursor
pixels. Board editing, Save/alerts, event-CPX callbacks and other CPXs remain
untested. Evidence goes to `build/atari/neost-cops-accessory-*`.

## Release: German Atari/ST core ZIP

`mise run release:core` builds the exact file targets listed in the package
map, then writes `build/atari/release/core-de.zip` and a sorted SHA-256 manifest.
The package combines rebuilt kernel, boot loader, MAGXDESK, applications, MFA/OSD drivers,
COPS and CPX modules with retained Release data: `MAGX.INF`, desktop
associations, help, patterns and localized German resources. The retained
`MAGX.INF` has its `PDLG.SLB` line removed; PDLG is excluded as optional print
support. TT/Falcon/MFM, Mac-specific programs, historical demos and ROMs are
outside this profile.

`mise run test:release-neost` builds the selected outputs, including every packaged
OSD driver, packs twice and requires byte-identical ZIPs. The packager rejects a
missing or empty rebuilt input. It then clones a MagiC image, removes `AUTO`,
`BIN`, `EXTRAS`, `GEMSYS`, `LANG` and the root MagiC files, extracts only the
ZIP, and verifies every installed member's hash; a failed removal stops the run
before any member is copied. The clean payload must boot to
MAGXDESK, open its menu and open the COPS panel. Only `HDDRIVER.SYS` remains
from the fixture's boot infrastructure; it and retained Release CPX/desktop
files are not rebuilt. The profile is a mono MegaSTE/TOS 2.06 boot; file
operations and shutdown are exercised separately against the produced payload
(MGCOPY suite and `SHUTDOWN.PRG`), not by this runner.

`mise run release:minimal` builds `build/atari/release/minimal-de.zip` for the
same mono MegaSTE/TOS 2.06 fixture. It uses `core/MAGX.INF` and contains only
the kernel, boot loader, MAGXDESK and its resources, MCMD as the startup
fallback, MFA2, OFF002 and the license. The same install/hash checks run via
`mise run test:release-minimal-neost`; the guest must open the desktop menu
with accessory, autostart and extension directories absent, and its desktop
must differ from a second boot with `MAGXDESK.APP` removed.

Either smoke task accepts `-- --build-dir build/atari/<fresh-dir>` to build
without cached outputs. ZIPs, manifest and screenshots are kept in the run's
evidence directory.

## NeoST: image SLB ABI and rendering

`mise run test:slb-consumer-neost` builds `XP_RASTR.SLB`, `LOAD_IMG.SLB` and a
small GCC consumer (`SLBTEST.PRG`), installs the two libraries in a dedicated
`C:\SLBAUDIT` directory named explicitly by both `Slbopen` calls, and runs the
consumer from `GEMSYS\MAGIC\START\ZCNFTEST.PRG`. Inherited copies in
`GEMSYS\MAGIC\XTENSION` are removed, `SLBPATH` and `#_SLB` preloads for these
names are stripped, and the stale `SLBTEST.TXT` marker is cleared:

```sh
MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img \
 mise run test:slb-consumer-neost
```

The same probe runs against archived and rebuilt libraries on separate
MegaSTE/ST-low clones. Two further boots omit one library from `C:\SLBAUDIT`
(and from every inherited search path); the probe must report a negative
`Slbopen` status for that name instead of silently using another copy. It calls
the packed legacy XP export, checks an exact four-plane expansion, and calls
LOAD_IMG exports 0 → 2 → 3 → 1 to load, convert, draw and free `PAT6.IMG`. It
checks that drawing changes the screen and compares the realized VDI colours
of the 64×64 image, not palette indices,
bitmap storage or private descriptor fields. The success marker is written
after closing both libraries. Evidence goes to `build/atari/neost-slb-consumer-*`.

The default baseline is the source-tree `BIN` archives. The runner sets only
the minimal-allocation bit in a disposable copy of the source-tree LOAD_IMG
header; without it `Slbopen` returned `-39` on the fixture. To make the same
strict comparison against the unmodified shipped LOAD_IMG, run
`mise run test:slb-consumer-neost -- --baseline release` with the same image and
ROM settings. That baseline loads and draws, but its XIMG colours
differ from the source port; it is not a passing Release-equivalence test.
The test does not cover tiled drawing or direct-colour workstations.

The lower-level `mise run apps -- xp-rastr` / `load-img` checks remain
build/package-only: explicit headers, export counts (2 and 5), lifecycle
pointers and the classic TOS relocation stream.

## NeoST: OFF002.OSD 1-plane offscreen driver

`mise run test:osd-off002-neost` builds `OFF002.OSD` and a small VDI probe.
The build uses VASM's TOS output with `-nosym` and `-tos-flags=7`; it must be
byte-identical to `MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF002.OSD`. The
Release `GEMSYS/OFF002.OSD` is V4.11 and differs in text/data size and entry
offsets; the runner records that comparison but does not claim equivalence.

The MagiC scenario installs the rebuilt driver and probe in a disposable image,
boots `MAGXBOOT.TOS`, then asks VDI for `v_opnbm()` with a caller-owned 64x64,
one-plane MFDB. It fills a rectangle, reads pixel `1`, and checks the bitmap
hash. The runner copies the guest driver back and checks its SHA-256 against the
file it installed; it does not trace which driver NVDI loaded. This exercises
NVDI driver selection and drawing, not other OSD variants, animation, or
long-term stability. Evidence goes to `build/atari/neost-off002-*`.

Required: a MagiC image with `C:\AUTO\MAGXBOOT.TOS`, TOS 2.06, and host
`mtools`. Run with
`MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img mise run test:osd-off002-neost`.

## NeoST: OFF004IP.OSD 2-plane offscreen driver

`mise run test:osd-off004ip-neost` builds `OFF004IP.OSD` and a VDI probe. The
build must be byte-identical to
`MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF004IP.OSD`; VASM runs with `-no-opt`
so the archived `movem` encoding is preserved. Release V4.11 is recorded as a
structural difference, not a byte reference.

The scenario installs archive and rebuild as `C:\GEMSYS\OFF004IP.OSD` in
separate clones of the same MagiC image, boots `MAGXBOOT.TOS`, and opens a
caller-owned 64x64, two-plane interleaved MFDB through `v_opnbm()`. It fills
bars with VDI pens 2 and 3, compares five pixel readbacks, and compares the
1024-byte bitmap dump against both the other binary and an independent
expectation. The pens map to raw colours 1 and 2, so each plane is exercised,
and the samples cross the x=15/16 word boundary. The runner copies the guest
driver back and checks its SHA-256 against the file it installed; it does not
trace which driver NVDI loaded.

Required: a MagiC image with `C:\AUTO\MAGXBOOT.TOS`, TOS 2.06, and host
`mtools`. Run with
`MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img mise run test:osd-off004ip-neost`.
Evidence goes to `build/atari/neost-off004ip-*`. Animation, other OSD
organisations, other screen modes and long-term stability are outside this
scenario.

## NeoST: OFF016IP.OSD 4-plane offscreen driver

`mise run test:osd-off016ip-neost` builds `OFF016IP.OSD` and a VDI probe. The
build must be byte-identical to
`MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF016IP.OSD`; VASM runs with `-no-opt`
to preserve the archived `movem` encoding. The Release driver reports the same
header version but differs in text/data size and entry offsets, so it is
recorded structurally, not as a byte reference.

The scenario installs archive and rebuild as `C:\GEMSYS\OFF016IP.OSD` in
separate clones, requests a 16-colour, 4-plane, non-packed interleaved 64x64
MFDB through `v_opnbm()`, fills five pens, and compares eleven pixel readbacks
plus the 2048-byte bitmap against the other binary and an independent
expectation. A third clone deletes the driver; no other installed driver
matches the request, so the open must be rejected with no bitmap written. Each
positive case also copies the installed driver back and checks its SHA-256
against the file it installed; it does not trace which driver NVDI loaded.

Required: a MagiC image with `C:\AUTO\MAGXBOOT.TOS`, TOS 2.06, and host
`mtools`. Run with
`MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img mise run test:osd-off016ip-neost`.
Evidence goes to `build/atari/neost-off016ip-*`. Animation, other OSD
organisations, other screen modes and long-term stability are outside this
scenario.

## NeoST: OFF256IP.OSD 8-plane offscreen driver

`mise run test:osd-off256ip-neost` builds `OFF256IP.OSD` and a VDI probe. The
build must be byte-identical to
`MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF256IP.OSD`; the shipped Release file
is the same V4.10 binary. VASM runs with `-no-opt`, so the source uses the
folded `movea.l (a2),a0` spelling for the zero-displacement colour-map load
that the archived assembler shortened.

The scenario installs archive and rebuild as `C:\GEMSYS\OFF256IP.OSD` in
separate clones, requests a 256-colour, 8-plane, non-packed interleaved 64x64
MFDB through `v_opnbm()`, fills ten pens, and compares twenty-one pixel
readbacks plus the 4096-byte bitmap against the other binary and an independent
expectation. A third clone deletes the driver; the probe must report the open
as rejected and leave no retrievable bitmap dump. The runner does not enumerate
candidate drivers or trace NVDI's selection. Each positive case copies the
installed driver back and checks its SHA-256 against the file it installed.

Required: a MagiC image with `C:\AUTO\MAGXBOOT.TOS`, TOS 2.06, and host
`mtools`. Run with
`MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img mise run test:osd-off256ip-neost`.
Evidence goes to `build/atari/neost-off256ip-*`. Animation, other OSD
organisations, other screen modes and long-term stability are outside this
scenario.

## NeoST: packed and direct-colour OSD drivers

Six drivers share one scenario runner with per-driver profiles:

```sh
export MAGIC_IMAGE=/path/to/magic.img
export TOS206_ROM=/path/to/tos206uk.img
mise run test:osd-off016pk-neost   # 16 colours, 4 planes, packed nibbles
mise run test:osd-off256pk-neost   # 256 colours, 8 planes, packed bytes
mise run test:osd-off32kfl-neost   # 32768 colours, flat 15-bit words
mise run test:osd-off32kpk-neost   # 32768 colours, packed 15-bit words
mise run test:osd-off65kpk-neost   # 65536 colours, packed 16-bit words
mise run test:osd-off16mov-neost   # TrueColor, 32-bit pixels
```

`core/osd/tests/neost-osd-batch-smoke.py` builds the driver and probe, checks the
header organisation, and records archive/Release comparisons. OFF016PK must
match its source-tree archive and OFF32KFL the Release; the remaining four have
no byte-identical reference and are compared structurally.

Each case installs archive and rebuild as `C:\GEMSYS\<driver>.OSD` in separate
MagiC clones, requests the driver's exact VDI 100/1 tuple, draws bars crossing
x=15/16, and compares readbacks plus the full bitmap against an independent
layout. A clone with the driver deleted must report the open as rejected and
produce no bitmap. OFF16MOV asserts its 16 KB bitmap and the readbacks equal
to the stored longwords with VDI's 16-bit word swap.

Required: a MagiC image with `C:\AUTO\MAGXBOOT.TOS`, TOS 2.06, and host
`mtools`. Evidence goes to `build/atari/neost-<driver>-*`. Animation, other OSD
organisations, other screen modes and long-term stability are outside these
scenarios.

## NeoST: KEYB_SYS keyboard-table generators

`mise run test:keyb-sys-neost` builds all six generators and runs
`scripts/neost-keyb-sys-smoke.py`. Each pair (archived vs rebuilt
`KEYB_{CZ,DEI,FRA,SWG,UK,US}.PRG`) runs through the generic Pexec driver in its
own disposable GEMDOS tree and must produce:

- declared child status 0 and matching console output (empty in the recorded
  run);
- exactly one generated `KEYTABLS.SYS`, byte-identical across archive and
  rebuild (the 9×128-byte table block plus the
  locale's dead-key trailer: 116 bytes for CZ, 42 for the rest);
- six distinct layouts across the locales.

The runner uses NeoST's bundled EmuTOS; no MagiC image, TOS ROM or `mtools` is
required. Evidence goes to `build/atari/neost-keyb-sys-*`. This proves the
generated tables, not that MagiC's keyboard driver loads them at boot.

## NeoST: MAGXBOOT loader

`mise run test:magxboot-neost` runs the scenario in `core/magxboot/tests/`.
It builds the loader and shared screen probe, then boots three disposable
clones with an unchanged `MAGIC.RAM`:

- the shipped `MAGIC.A/AUTO/MAGXBOOT.TOS`;
- the rebuild from `build/atari/apps/magxboot/MAGXBOOT.TOS`;
- a clone with `C:\AUTO\MAGXBOOT.TOS` deleted.

Both present loaders must write `C:\SCREEN.TXT` (`MAGIC\r\n`) from the probe in
`GEMSYS/MAGIC/START` and render a pixel-identical 640x400 MAGXDESK screen. The
missing loader must reach neither; its screen must differ by more than 10% of
the pixels. The runner also checks each executable's TEXT/DATA/BSS, TOS flags
and absent symbols, and that the rebuilt `MAGXBOOT.PRG` equals the rebuilt TOS.

Required: a MagiC image with `C:\AUTO\MAGXBOOT.TOS`, TOS 2.06 and host
`mtools`. Evidence goes to a fresh `build/atari/neost-magxboot-*` directory.
Only the MegaSTE mono profile is exercised; `MAGXBO32` and the resident path are
not booted.

## NeoST: HARDCOPY extension

`mise run test:hardcopy-neost` builds `HARDCOPY.PRG` and `HCPROBE.PRG`, then
boots three disposable clones:

- the archived `MagiC/KERNEL/XTENSION/HARDCOPY.PRG`;
- the rebuild from `build/atari/apps/hardcopy/HARDCOPY.PRG`;
- a clone without the extension.

The probe runs from `GEMSYS/MAGIC/START`, reads `scr_dump` ($502), launches
`C:\GEMSYS\MAGIC\HCP\HARDCOPY.PRG`, and reads the hook again. Present builds
must return status 0 and change `scr_dump`; the missing case must fail the launch
and leave it unchanged. All three desktop screens must be pixel-identical.

Required: a MagiC image with `C:\AUTO\MAGXBOOT.TOS`, TOS 2.06 and host
`mtools`. Evidence goes to `build/atari/neost-hardcopy-*`. This covers
installation only: NeoST's `--printer` capture receives no bytes for this
installation, and arming a dump raises a guest exception with the archived
binary too, so the printer path is untested.

## NeoST: MagiC diagnostic tools

`mise run test:diagnostic-tools-neost` builds `SHOWFREE.TOS`, `MAGXKERN.TTP`,
`MEMEXAMN.TTP`, `CRASHDMP.TOS` and the shared `DIAG.PRG` runner. It clones the
MagiC image for archive and rebuild, installs both under the same guest name,
runs `DIAG.PRG` from `MAGIC\START`, and captures `DIAG.OUT`/`DIAG.STA`.

- `MAGXKERN`: status 0 and byte-identical output.
- `MEMEXAMN`: status 0; the banner, normalized early table, owner tokens and
  `KROM` terminator are present and match where comparable. Block sizes and
  addresses differ because the rebuilt executable is larger.
- `CRASHDMP`: status 0 with no output on the no-record path.
- `SHOWFREE`: both copies appear in `MAGIC\START`, print the monitor and
  `blocks used`, and render pixel-identical screens.

Required: the usual MagiC image with `MAGXBOOT.TOS`, an English-language TOS
2.06 ROM, `MAGIC\START`, and host `mtools`. Exception decoding, interactive
SHOWFREE exit, post-mortem `MEMEXAMN` dumps and crash triggering are not
covered.

## NeoST: MFA/MFM screen drivers

`mise run test:screen-drivers-neost` builds all five MFA and seven MFM screen
drivers and checks each rebuilt file's TOS branch, `NVDIDRV` header, version,
header length, type, organization, segment bounds and relocation table. It then
boots archived, rebuilt and missing drivers for ST high (`MFA2`), medium
(`MFA4`) and low (`MFA16`) in separate MagiC clones:

```sh
export MAGIC_IMAGE=/path/to/magic.img
export TOS206_ROM=/path/to/tos206uk.img
mise run test:screen-drivers-neost
```

For each mode the archive and rebuild are installed under
`C:\GEMSYS\<driver>.SYS`; the control removes that file. The script rewrites the
single active `#_DEV` setting to `1 0`, `3 0` or `2 0`. It installs `SCREEN.PRG`
in `C:\GEMSYS\MAGIC\START`; that probe writes `C:\SCREEN.TXT` only after MagiC
starts. Positive runs must produce the marker and pixel-identical screens; the
missing-driver run must produce no marker and a screen differing by more than
10% of its pixels. Recorded controls also show
`Bildschirm-Treiber nicht gefunden.` and the MCMD shell, but the automated check
asserts only the marker and pixel difference. Evidence goes to
`build/atari/neost-screen-*`.

Required: a MagiC image with `C:\AUTO\MAGXBOOT.TOS`, exactly one active
`#_DEV` setting in `MAGX.INF`, `C:\GEMSYS\MAGIC\START`, the three tested MFA
drivers present for `mdel`, TOS 2.06, and host `mtools`. Only ST
high/medium/low are runtime-tested. NeoST profiles do not cover TT/Falcon, and
MFM needs a MagiCMac host, so the other nine outputs are structural-only.

## NeoST: WINFRAME.SLB window frames

`mise run test:winframe-neost` builds the alternate window-frame library,
installs archived and rebuilt copies into separate MagiC clones, and also boots
a clone with `#_SLB 0 WINFRAME.SLB` disabled:

```sh
MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img \
 python3 scripts/neost-winframe-smoke.py
```

The scenario switches `MAGX.INF` to ST-low colour (`#_DEV 2 0`) because
WINFRAME refuses to initialize below 16 colours. It requires WINFRAME-enabled
and disabled booted screens to differ, and compares archived/rebuilt screens
pixel-for-pixel. The fixture's booted MAGXCONF dialog supplies the visible
window frame. Dragging, resizing, shading, scrollbar movement and unload
behavior are not exercised. Evidence is written to a fresh
`build/atari/neost-winframe-*` directory.

## NeoST: WINFRAME.DEM example library

`mise run test:winframe-demo-neost` builds the separate
`MagiC/KERNEL/WINFRAME.DEM` example project, installs the demo's own
16-object `WINFRAME.RSC` (not the Release production resource) beside the
archived and rebuilt libraries, and boots a clone with
`#_SLB 0 WINFRAME.SLB` disabled:

```sh
MAGIC_IMAGE=/path/to/magic.img TOS206_ROM=/path/to/tos206uk.img \
 python3 scripts/neost-winframe-demo-smoke.py
```

The fixture removes any `SLBPATH` override so `C:\GEMSYS\MAGIC\XTENSION` is
the only library search path. The demo declares `wsizeof = 542` and reuses the
production register-to-stack bridges; the booted MAGXDESK directory window must
differ from the disabled control and match the archive pixel-for-pixel. Only
that boot screen is covered; interactive resize, shade and scrollbar paths are
not. Evidence is written to a fresh `build/atari/neost-winframe-demo-*`
directory.
