# NeoSTHost session contract

`scripts/neost_host.py` is the optional session layer for host-side scenarios.
It composes the existing helpers; it does not replace them.

```text
application scenario
  -> NeoSTHost          lifecycle, artifacts, coordinates, state slots
    -> NeoSTServer      NeoST --server protocol and process
      -> neost-headless
  -> MagiCImage         image clone/verify/install/extract
  -> neost_screen       PPM parsing and pixel comparison
```

## Responsibilities

`NeoSTHost` owns:

- a fresh output directory and server artifact directory;
- one `MagiCImage` and one `GEMDRIVE` guest root;
- image cloning, `MAGX.INF` verification and bundle installation;
- server start/stop;
- the initial boot frame budget;
- keyboard/chord forwarding;
- tracked relative mouse movement and clicks;
- screenshot paths under the host output directory;
- state-slot save/load forwarding.

It deliberately does **not** own:

- application launch timing beyond the configured `auto` path;
- fixtures or application assertions;
- per-application input scripts;
- screenshot comparison policy;
- destructive-tool safety rules.

Scenarios can always use `host.server` for protocol operations that are not
wrapped. They can also bypass `NeoSTHost` entirely; it is a convenience layer,
not a mandatory abstraction.

## Construction

```python
from pathlib import Path
from neost_host import NeoSTHost

host = NeoSTHost(
    headless=Path("/path/to/neost-headless"),
    rom=Path("/path/to/tos206uk.img"),
    output=Path("build/atari/neost-case"),
    options=["--machine", "megaste", "--mem", "4m", "--mono", "--no-disk"],
    auto=r"C:\AUTO\MAGXBOOT.TOS",
)
```

The output directory may already exist, but `output/server` must be empty when
the host starts. Use `neost_common.create_output` for timestamped fresh roots.

## Image preparation

Prepare the image **before** starting the host. The server fixes its media
options for the whole session.

```python
host = NeoSTHost(...)
image = host.prepare_image(Path("/path/to/magic.img"), partition_offset=1024)
image.install(((APP, r"::/GEMSYS/GEMDESK/MGSEARCH.APP"),
               (RSC, r"::/GEMSYS/GEMDESK/MGSEARCH.RSC")))
image.mkdir(r"::/SEARCHFIX")
image.copy_in(local_fixture, r"::/SEARCHFIX/ALPHA.ZZZ")

with host:
    host.boot(15000)
```

Or pass an already prepared `MagiCImage` in the constructor. The host appends
`--acsi` and `--gemdos` to the server options at `start()`.

## Lifecycle

```python
host = NeoSTHost(...)
host.prepare_image(Path("/path/to/magic.img"))

with host:
    host.boot(15000)
    host.screenshot("initial.ppm")
    host.calibrate_mouse((640, 400))
    host.click(300, 128)
    host.chord([0x2A, 0x09])  # *
    host.press_key(0x1C)      # Enter
    host.run_frames(3000)
    host.screenshot("result.ppm")
```

For an image passed in the constructor, `with NeoSTHost(...) as host:` is
sufficient.

`close()` is idempotent. Closing requires NeoST to acknowledge `quit` and exit
with status zero within two seconds; otherwise it raises `NeoSTTransportError`,
so a clean scenario cannot pass after an emulator crash during shutdown. Use the
context manager or call `close()` from an `else` branch, not a bare `finally`
that would replace an earlier scenario failure. Context-manager exit preserves
the original exception and keeps the output artifacts.

## Mouse coordinates

The NeoST server exposes only relative mouse events. `NeoSTHost` tracks a
coordinate after `calibrate_mouse()`:

1. move to the bottom-left edge with small relative steps;
2. set the origin to `(0, screen_height - 1)`;
3. apply later moves as deltas from the tracked point.

This assumes the guest cursor reaches the requested edge. For non-standard
screen dimensions, pass them to `calibrate_mouse()`. There is no server API to
query the absolute guest cursor position.

## State slots

```python
host.save_state(0)
host.load_state(0)
```

These are NeoST server slots. Save states contain machine RAM/device state, not
disk media. A case that writes the image must clone/reinstall media or extract
the resulting files explicitly.

## Non-goals

- No deep per-application regression suite.
- No application-specific frame timings or expected screens.
- No hidden fallback that launches a program by name.
- No replacement for the manual semantic regression pass.

A future snapshot matrix should sit above this layer and provide scenario data,
state invalidation and reports; it should not duplicate host plumbing.
