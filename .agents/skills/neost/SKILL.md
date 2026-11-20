---
name: neost
description: Operate NeoST headless or GUI runs; capture screens and state, drive input, and debug Atari programs.
---

# NeoST operations

Use the installed NeoST version's `--help` as the source of truth; options differ
between releases. Run commands from the repository root and record `--version`.

## Executables and configuration

- Prefer `neost-headless` for bounded, reproducible runs; use `neost` for an
  interactive GUI session.
- If only an AppImage is installed, extract it in a scratch directory with
  `neost --appimage-extract`, then locate `usr/bin/neost-headless` and any bundled
  ROM. Select the ROM explicitly; finding the executable does not guarantee a
  suitable ROM is selected.
- `--from-cfg FILE` replays an explicit `neost.cfg`; later CLI options override
  it. For repeatable runs, avoid implicit per-user settings and specify machine,
  memory, ROM, drives and frame budget on the command line.
- A headless ROM is the final positional argument. Record the ROM, image and
  configuration inputs used for each run.

## Bounded runs and storage

Set machine, memory, ROM and frame count explicitly. Example:

```sh
"$HEADLESS" --machine st --mem 4m --no-disk --frames 1200 \
  --screenshot "$OUTPUT/final.ppm" "$ROM" \
  >"$OUTPUT/headless.log" 2>&1
```

- `--gemdos DIR` redirects GEMDOS hard-disk access to a host directory. A
  single-letter subdirectory may appear as a separate drive; check the host log
  rather than assuming the mapping. Use fresh disposable data for writable runs.
- `--acsi IMG` mounts a block-device image. Clone it before booting; never mount
  a user's working image writable.
- `--auto PRG` requests an application autostart after TOS boot. For GUI
  interaction, use `--keys-at`, `--scancode-at` or `--mouse-at` rather than
  assuming the program received input.
- A clean emulator exit or elapsed frame count alone does not prove the guest
  ran. Check a guest completion/status marker, meaningful filesystem effects,
  console output, or the visible screen.

## Screenshots

- Headless final frame: `--screenshot FILE.ppm`.
- Frame series: `--shot-every N PREFIX`; output is named `PREFIX<frame>.ppm`.
  `--shot-from N` delays the series.
- GUI bounded run: `--run-frames N --shot FILE`. For repeated rendered-window
  captures use `--shot-window PREFIX START-FRAME INTERVAL`.
- Inspect screenshots when console output is empty or a UI path matters. Keep
  them with the host log and exact run parameters; a screenshot of the boot
  screen is not proof the application ran.

## Input

- `--keys-at N TEXT` types from frame N; `--key-hold N` sets the press duration.
- `--scancode-at N HEX[,HEX...]` injects raw ST scancodes.
- `--mouse-at N SCRIPT`: `L/R/U/D` move by 8 pixels, lowercase variants drag,
  `1`/`2` click left/right, and `.` waits one frame. `--mouse-step N` changes the
  movement distance.
- In server mode, `mouse DX DY BTN` is a raw relative IKBD event; `BTN` is a
  held state bitmask, not a click. Use `NeoSTServer.move(dx, dy)` for small
  relative steps and `NeoSTServer.click()` for a press/frame/release click.
  Large single deltas can exceed the IKBD packet range or TOS clamping.
- Use fixed frame input only with a pinned ROM, machine and boot sequence. If
  those change, inspect intermediate screenshots and re-establish the timing.

## State and memory capture

- `--save-state FILE` saves the complete emulator state at the end of a run;
  `--load-state FILE` restores it before execution and requires matching machine
  configuration.
- `--dump-at FRAME ADDRESS LENGTH FILE` writes a raw RAM range at a chosen frame.
  Address and length are hexadecimal.
- `--probe NAME=ADDRESS:LENGTH` samples 1-, 2- or 4-byte values without side
  effects; `--probe-every N` repeats samples at an interval.
- Distinguish a whole-machine save-state from a raw RAM dump. Preserve the exact
  ROM, disk copies and machine options needed to interpret either artifact.

## Debugging

Start with a bounded reproduction, final/interval screenshots and the host log.
Then narrow the trace to the failure window:

- `--trace FILE --trace-from FRAME` records instructions from a selected frame;
  `--regs` adds register state and `--irq` includes interrupts. Avoid tracing the
  whole boot unless necessary.
- `--break ADDRESS` stops before an instruction; `--until-pc ADDRESS` stops
  without a register dump. `--symbols FILE` and `--symbols-base ADDRESS` support
  symbol lookup; `--break-sym NAME` breaks on a loaded symbol.
- `--watch ADDRESS` breaks after reads or writes to a memory address.
- Use `--dump-at` or `--probe` to inspect a narrow memory location before
  collecting a large trace.
- For scripted regression scenarios, use `scripts/neost_server.py` as the
  canonical reusable client for NeoST 0.6.5's line-oriented server protocol.
  It owns process startup, protocol framing, bounded I/O, typed commands,
  observations and cleanup. Keep protocol-level behavior there; do not build
  another server wrapper or issue undocumented commands.

## NeoST regression scenarios

Use `scripts/neost_common.py` for emulator/ROM discovery and a fresh artifact
directory, then use `NeoSTServer` from `scripts/neost_server.py` as a context
manager. Supply the executable, ROM, output directory, and explicit machine
options; the client appends the ROM as NeoST's final argument. Capture artifacts
in that output directory and keep `protocol.log` separate from
`diagnostics.log`.

New task scenarios should compose the shared client with the task's own
preconditions, fixtures and checks. Keep machine profile, writable guest data,
input timing, expected screen/RAM values, and success assertions local to the
application scenario: those contracts vary by program and are not generic
client behavior. Use the focused `mise run test:neost-server` task for the
client's protocol regression tests.

For scenarios, issue explicit bounded runs and observe guest effects rather
than treating process exit as success. Pin the ROM and relevant machine options
when relying on frame timing. Do not reuse writable fixtures across runs.

## Application checks

- Inspect `mise tasks`, `mise.toml` and `scripts/` before adding a test runner.
  Prefer a focused existing test task and its fixture/assertion contract.
- Use `scripts/neost_image.py` (`MagiCImage.clone`, `verify_magic`, `mkdir`,
  `copy_in`, `copy_out`, `install`) for disposable image setup instead of
  reimplementing `cp`/`mdir`/`mmd`/`mcopy`.
- Use `scripts/neost_screen.py` (`read_ppm`, `diff_stats`, `bbox_inside`) for
  screenshot parsing and pixel comparisons.
- Use separate disposable guest trees and identical machine, ROM, frame and
  input settings when comparing binaries. Keep archived and rebuilt binaries
  under the same guest filename.
- For Pexec drivers, use 8.3 guest names. A standard TOS tail is 128 bytes:
  byte 0 is the text length, with up to 126 text bytes plus a terminator. Use a
  double-NUL-terminated environment when controlling it.
- Read console captures as bytes or Latin-1; MiNTLib output can use CR-CR-LF.
  Pexec child status is in the low 16 bits; distinguish that from loader errors
  and libc errno values.
- Compare defined output fields, not uninitialized padding or incidental runtime
  addresses. Quoted DOS tails are not ARGV tests.
- State which OS ROM/kernel was tested. NeoST results apply to that OS
  environment; they do not establish compatibility with a different kernel.
- Keep crash probes and state-mutating fixtures out of the default smoke set.
  Store evidence and remove throwaway drivers after verification.
