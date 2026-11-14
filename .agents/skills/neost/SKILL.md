---
name: neost
description: Run isolated Atari application smoke tests and original/rebuilt comparisons with NeoST headless, console captures, and Pexec launchers.
---

# NeoST application checks

Use for Atari application runtime checks. NeoST/EmuTOS results do not establish
MagiC-kernel compatibility; retain the separate Hatari/MagiC checks.
Run commands from the repository root.

## Find the current workflow

Inspect `mise tasks`, `mise.toml` and the relevant runners under `scripts/`.
Prefer an existing application-specific task; read its `--help` and fixture
requirements rather than copying another application's assertions.

Use `neost-headless --help` for the installed emulator's options. If only an
AppImage is available, extract it with `--appimage-extract` in a scratch directory
and locate the bundled headless executable and ROM. Choose the ROM explicitly;
do not assume that finding the executable also locates a suitable ROM.

## Isolated runtime check

1. Build the selected target with `Makefile.apps`.
2. Give original/rebuilt binaries separate disposable host directories mapped to
   C:. Use identical guest filenames and fixtures. Never map a live installation
   or reuse a user's running emulator.
3. Choose the same machine, memory, ROM and frame budget for both runs.
4. Autostart the application, or a small driver for controlled input. For example:

```sh
"$HEADLESS" --machine st --mem 4m --no-disk --frames 1200 \
  --gemdos "$GUEST_C" --auto 'C:\NEOSTEST.PRG' \
  --console-log "$OUTPUT/console.log" \
  --screenshot "$OUTPUT/screen.ppm" "$ROM"
```

`HEADLESS`, `GUEST_C`, `OUTPUT` and `ROM` are explicitly chosen paths; create the
output and guest directories first. A clean emulator exit or elapsed frame count
is not proof the application ran: require guest completion/status markers and
assert meaningful output and filesystem effects independently of parity.

## Pexec drivers and gotchas

- A Pexec driver is a small Atari executable, autostarted by NeoST, which calls
  `Pexec(0, program, tail, environment)` and reports each child's result. It can
  prepare fixtures and invoke the real application without desktop interaction.
- Ordinary TOS tails are 128-byte buffers: byte 0 is the length, followed by text.
  Zero-initialize the buffer, allow at most 126 text bytes plus a terminator, and
  pass a double-NUL-terminated environment block when controlling environment
  contents. Keep the driver independent of the code under test.
- Use 8.3 guest names for compatibility with TOS filesystem and autostart paths.
  An empty console capture warrants inspecting the screenshot, not assuming pass.
- Read console captures as bytes or Latin-1, not assumed UTF-8. MiNTLib output
  can produce CR-CR-LF; normalize line endings when parsing result markers.
- Pexec child exit status is exposed in the low 16 bits. Normalize with `& 0xffff`
  where appropriate; do not confuse this with negative Pexec loader errors or
  positive libc errno values.
- Compare defined output fields, not uninitialized buffer padding or incidental
  runtime addresses. Startup libraries can rewrite command tails and environments
  before main; distinguish incoming OS data from the application's observed data.
- Quoted raw-tail tests are not ARGV tests. Exercise extended ARGV separately;
  never claim coverage merely because modern MiNTLib parses arguments.
- Keep logs/screenshots and state exactly what ran. Use disposable fixtures for
  patchers/installers; never include crash probes in normal smoke runs. Keep
  permanent regression scenarios in explicit application-specific tasks; remove
  throwaway driver sources after verification.
