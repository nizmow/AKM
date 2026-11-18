# Testing

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
