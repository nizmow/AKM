"""A small session layer over image preparation and the NeoST server client.

`NeoSTHost` owns lifecycle and artifact plumbing only.  Application launch
paths, frame budgets, input sequences and success assertions stay in scenarios.
"""

from __future__ import annotations

from collections.abc import Sequence
from pathlib import Path

from neost_image import MagiCImage
from neost_server import NeoSTError, NeoSTServer


class NeoSTHost:
    """A booted NeoST session with a disposable image and guest tree."""

    def __init__(self, headless: Path, rom: Path, output: Path, *,
                 options: Sequence[str] = (), image: MagiCImage | None = None,
                 guest_root: Path | None = None, auto: str | None = None,
                 timeout: float = 180.0):
        self.headless = Path(headless)
        self.rom = Path(rom)
        self.output = Path(output)
        self.output.mkdir(parents=True, exist_ok=True)
        self.image = image
        self.guest_root = (Path(guest_root) if guest_root is not None
                           else self.output / "GEMDRIVE")
        self.guest_root.mkdir(parents=True, exist_ok=True)
        self.base_options = list(options)
        self.auto = auto
        self.timeout = timeout
        self._server: NeoSTServer | None = None
        self._mouse: tuple[int, int] | None = None

    @property
    def server(self) -> NeoSTServer:
        if self._server is None:
            raise RuntimeError("NeoSTHost has not been started")
        return self._server

    def prepare_image(self, source: Path, *,
                      partition_offset: int = 1024) -> MagiCImage:
        """Clone and verify a MagiC image under this host's output directory.

        Call this before :meth:`start`; a running server has already fixed its
        media options for the session.
        """
        if self._server is not None:
            raise RuntimeError("prepare the image before starting NeoSTHost")
        image = MagiCImage.clone(source, self.output / "magic.img",
                                 partition_offset=partition_offset)
        image.verify_magic()
        self.image = image
        return image

    def start(self, *, auto: str | None = None) -> NeoSTServer:
        """Start the NeoST server; safe to call once per host."""
        if self._server is not None:
            return self._server
        server_dir = self.output / "server"
        server_dir.mkdir(exist_ok=True)
        options = list(self.base_options)
        if self.image is not None:
            options += ["--acsi", str(self.image.path)]
        options += ["--gemdos", str(self.guest_root)]
        selected_auto = auto if auto is not None else self.auto
        if selected_auto is not None:
            options += ["--auto", selected_auto]
        self._server = NeoSTServer(self.headless, self.rom, server_dir,
                                   options=options, timeout=self.timeout)
        return self._server

    def boot(self, frames: int):
        """Run the initial boot/startup window."""
        return self.server.run_frames(frames)

    def screenshot(self, destination: str | Path) -> Path:
        path = Path(destination)
        if not path.is_absolute():
            path = self.output / path
        path.parent.mkdir(parents=True, exist_ok=True)
        self.server.screenshot(path)
        return path

    def run_frames(self, frames: int):
        return self.server.run_frames(frames)

    def press_key(self, scancode: int, frames: int = 1):
        return self.server.press_key(scancode, frames=frames)

    def chord(self, scancodes: Sequence[int], frames: int = 1):
        return self.server.chord(scancodes, frames=frames)

    def calibrate_mouse(self, screen: tuple[int, int] = (640, 400)) -> None:
        """Move to the bottom-left screen edge and set a known origin."""
        width, height = screen
        self.server.move(-width, height, step=8, frames=1)
        self._mouse = (0, height - 1)

    def move_to(self, x: int, y: int) -> None:
        """Move to absolute guest screen coordinates from the tracked position."""
        if self._mouse is None:
            raise RuntimeError("call calibrate_mouse() before move_to()")
        dx = x - self._mouse[0]
        dy = y - self._mouse[1]
        self.server.move(dx, dy, step=8, frames=1)
        self._mouse = (x, y)

    def click(self, x: int | None = None, y: int | None = None,
              button: int = 1):
        """Click at the current position, or move to (x, y) first."""
        if x is not None or y is not None:
            if x is None or y is None:
                raise ValueError("pass both x and y, or neither")
            self.move_to(x, y)
        return self.server.click(button)

    def save_state(self, slot: int = 0) -> None:
        self.server.save(slot)

    def load_state(self, slot: int = 0):
        return self.server.load(slot)

    def close(self) -> None:
        server, self._server = self._server, None
        if server is not None:
            server.close()

    def __enter__(self) -> "NeoSTHost":
        self.start()
        return self

    def __exit__(self, exc_type, exc, traceback) -> None:
        try:
            self.close()
        except NeoSTError:
            if exc_type is None:
                raise
