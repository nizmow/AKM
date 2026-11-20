# Testing

## NeoST: console tools

`mise run test:console-tools-neost` builds `MAC2ST.TTP`, `DUMP.TTP`, `FC.TTP`,
`SPLIT.TTP` and the generic Pexec driver, then runs
`scripts/neost-console-tools-smoke.py`. Every case gives the archived and rebuilt
binary the same disposable `C:` fixture and compares child status, console
output and guest file hashes.

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

## Shared host-side helpers

`scripts/neost_image.py` wraps disposable image cloning, `MAGX.INF`
verification, guest directory creation and `mcopy` in/out operations.
`scripts/neost_screen.py` parses NeoST PPM screenshots and computes pixel diffs
and bounding boxes. `scripts/neost_host.py` combines those helpers with
`NeoSTServer` for one booted session; its API and limits are documented in
[NeoST_HOST.md](NeoST_HOST.md). Scenario runners keep their own machine profile,
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

### Run

Set `HEADLESS`, `TOS206_ROM`, `MAGIC_DISK`, and `GEMDRIVE_ROOT` to the local
artifacts. Use a new output directory for each run:

```sh
mise run apps -- shutdown
OUT=$(mktemp -d build/atari/neost-magic-shutdown.XXXXXX)
{
  "$HEADLESS" --version
  sha256sum "$MAGIC_DISK" "$TOS206_ROM"
  find "$GEMDRIVE_ROOT" -type f -print0 | sort -z | xargs -0 sha256sum
} >"$OUT/inputs.txt"
cp --reflink=auto "$MAGIC_DISK" "$OUT/magic.img"
cp -a "$GEMDRIVE_ROOT" "$OUT/GEMDRIVE"
mcopy -i "$OUT/magic.img@@1024" \
  build/atari/apps/shutdown/SHUTDOWN.PRG \
  ::/GEMSYS/MAGIC/START/SHUTDOWN.PRG

"$HEADLESS" --machine megaste --mem 4m --mono --no-disk \
  --acsi "$OUT/magic.img" --gemdos "$OUT/GEMDRIVE" \
  --auto 'C:\AUTO\MAGXBOOT.TOS' \
  --frames 18000 --scancode-at 6000 72 \
  --screenshot "$OUT/final.ppm" "$TOS206_ROM" \
  >"$OUT/headless.log" 2>&1
```

The tested ACSI image's system partition starts at sector 2, hence the `@@1024`
byte offset used by `mcopy`. Check the partition table and `#_APP` path in
`MAGX.INF` before using a different installation; do not assume these match.
The fixed Enter injection at frame 6000 and frame budget reproduce the current
NeoST boot sequence. If the ROM, image, or emulator changes, inspect the screen
and recalibrate rather than treating elapsed frames as proof of a successful
boot.

Pass only when the screenshot shows MagiC's **“Shutdown was successful”**
confirmation. Do not activate Restart or Cold Boot: SHUTDOWN is in the startup
folder, so rebooting the test image can relaunch it. Preserve the screenshot and
headless log as run evidence. This verifies the shutdown request through the
confirmation dialog, not the final reboot/cold-boot path.
