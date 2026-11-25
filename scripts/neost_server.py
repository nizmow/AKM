"""Synchronous client for NeoST's line-oriented headless server."""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from collections.abc import Sequence
import queue
import re
import subprocess
import threading


class NeoSTError(RuntimeError):
    """A command was rejected by NeoST."""


class NeoSTTransportError(NeoSTError):
    """The server process or its protocol transport failed."""


@dataclass(frozen=True)
class Observation:
    frame: int
    screen: str
    ram: str | None = None
    probes: tuple[tuple[str, str], ...] = ()


class NeoSTServer:
    def __init__(self, executable: Path, rom: Path, output: Path, *,
                 options: Sequence[str] = (), timeout: float = 30.0,
                 slots: int = 64):
        if not isinstance(timeout, (int, float)) or timeout <= 0:
            raise ValueError("timeout must be positive")
        if not isinstance(slots, int) or isinstance(slots, bool) or not 1 <= slots <= 4096:
            raise ValueError("slots must be 1..4096")
        output = Path(output)
        if not output.is_dir() or any(output.iterdir()):
            raise ValueError("output must be an existing empty directory")
        args = [self._arg(executable), *(self._arg(x) for x in options),
                "--server", "--server-slots", str(slots), self._arg(rom)]
        self.output = output
        self.timeout = float(timeout)
        self._slots = slots
        self._protocol = (output / "protocol.log").open("w", encoding="utf-8")
        self._diagnostics = (output / "diagnostics.log").open("wb")
        self._lines: queue.Queue[bytes | None] = queue.Queue()
        self._closed = False
        self._proc: subprocess.Popen[bytes] | None = None
        self._stdout_thread: threading.Thread | None = None
        self._stderr_thread: threading.Thread | None = None
        try:
            self._proc = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                          stderr=subprocess.PIPE, bufsize=0)
            self._stdout_thread = threading.Thread(target=self._read_stdout, daemon=True)
            self._stderr_thread = threading.Thread(target=self._read_stderr, daemon=True)
            self._stdout_thread.start()
            self._stderr_thread.start()
            self.identity = self._request("hello")
        except BaseException as error:
            self._terminate()
            if isinstance(error, (NeoSTError, ValueError)):
                raise
            raise NeoSTTransportError("NeoST startup failed") from error

    @property
    def closed(self) -> bool:
        return self._closed

    @staticmethod
    def _arg(value: object) -> str:
        text = str(value)
        if any(c in text for c in "\r\n\0"):
            raise ValueError("arguments must not contain CR, LF, or NUL")
        return text

    def _read_stdout(self) -> None:
        assert self._proc and self._proc.stdout
        try:
            while True:
                line = self._proc.stdout.readline()
                if not line:
                    break
                self._lines.put(line)
        finally:
            self._lines.put(None)

    def _read_stderr(self) -> None:
        assert self._proc and self._proc.stderr
        while True:
            chunk = self._proc.stderr.read(65536)
            if not chunk:
                return
            try:
                self._diagnostics.write(chunk)
                self._diagnostics.flush()
            except OSError:
                # Keep draining stderr even if the diagnostics destination fails.
                pass

    def _request(self, command: str, timeout: float | None = None) -> str:
        if self._closed or self._proc is None or self._proc.stdin is None:
            raise NeoSTTransportError("NeoST server is closed")
        limit = self.timeout if timeout is None else timeout
        if not isinstance(limit, (int, float)) or limit <= 0:
            raise ValueError("timeout must be positive")
        try:
            self._protocol.write(command + "\n")
            self._protocol.flush()
            self._proc.stdin.write((command + "\n").encode("utf-8"))
            self._proc.stdin.flush()
            line = self._lines.get(timeout=limit)
        except (OSError, queue.Empty) as error:
            self._fail("server response timed out or transport failed", error)
        if line is None:
            self._fail("server closed stdout")
        try:
            response = line.decode("utf-8").removesuffix("\n").removesuffix("\r")
        except UnicodeDecodeError as error:
            self._fail("malformed server response", error)
        self._protocol.write(response + "\n")
        self._protocol.flush()
        if response == "ok":
            return ""
        if response.startswith("ok "):
            return response[3:]
        if response.startswith("err "):
            raise NeoSTError(response[4:])
        self._fail(f"malformed server response: {response!r}")

    def _fail(self, message: str, cause: BaseException | None = None) -> None:
        self._terminate()
        error = NeoSTTransportError(message)
        if cause is not None:
            raise error from cause
        raise error

    def _observation(self, payload: str) -> Observation:
        try:
            fields = payload.split()
            if len(fields) < 2:
                raise ValueError
            frame_key, frame_value = fields[0].split("=", 1)
            screen_key, screen_value = fields[1].split("=", 1)
            if frame_key != "frame" or screen_key != "screen":
                raise ValueError
            frame = int(frame_value)
            screen = screen_value
            index = 2
            ram = None
            if index < len(fields) and fields[index].startswith("ram="):
                ram_key, ram = fields[index].split("=", 1)
                if ram_key != "ram":
                    raise ValueError
                index += 1
            probes = []
            for field in fields[index:]:
                key, value = field.split("=", 1)
                if not key or len(key) > 64 or any(c.isspace() for c in key):
                    raise ValueError
                probes.append((key, value))
            if frame < 0 or not re.fullmatch(r"[0-9a-fA-F]{16}", screen):
                raise ValueError
            if ram is not None and not re.fullmatch(r"[0-9a-fA-F]{16}", ram):
                raise ValueError
        except (ValueError, KeyError) as error:
            self._fail("malformed observation", error)
        return Observation(frame, screen, ram, tuple(probes))

    def run_frames(self, count: int, timeout: float | None = None) -> Observation:
        if not isinstance(count, int) or isinstance(count, bool) or not 0 <= count <= 10_000_000:
            raise ValueError("count must be 0..10000000")
        return self._observation(self._request(f"run {count}", timeout))

    def observe(self) -> Observation:
        return self._observation(self._request("observe"))

    def play(self, script: str) -> Observation:
        return self._observation(self._request("play " + self._arg(script)))

    @staticmethod
    def _scan(code: int) -> None:
        if not isinstance(code, int) or isinstance(code, bool) or not 1 <= code <= 127:
            raise ValueError("scancode must be 1..127")

    def joy(self, port1: int, port0: int | None = None) -> None:
        if (not isinstance(port1, int) or isinstance(port1, bool) or not 0 <= port1 <= 255 or
                (port0 is not None and (not isinstance(port0, int) or isinstance(port0, bool) or not 0 <= port0 <= 255))):
            raise ValueError("joystick mask out of range")
        self._request(f"joy {port1:x}" + (f" {port0:x}" if port0 is not None else ""))

    def key_make(self, scancode: int) -> None:
        self._scan(scancode)
        self._request(f"key make {scancode:x}")

    def key_break(self, scancode: int) -> None:
        self._scan(scancode)
        self._request(f"key break {scancode:x}")

    def press_key(self, scancode: int, frames: int = 1) -> Observation:
        self._scan(scancode)
        if not isinstance(frames, int) or isinstance(frames, bool) or not 0 <= frames <= 10_000_000:
            raise ValueError("frames out of range")
        self.key_make(scancode)
        failure = None
        try:
            return self.run_frames(frames)
        except BaseException as error:
            failure = error
            raise
        finally:
            try:
                self.key_break(scancode)
            except BaseException:
                if failure is None:
                    raise

    def chord(self, scancodes: Sequence[int], frames: int = 1) -> Observation:
        codes = tuple(scancodes)
        for code in codes:
            self._scan(code)
        if not isinstance(frames, int) or isinstance(frames, bool) or not 0 <= frames <= 10_000_000:
            raise ValueError("frames out of range")
        made = []
        failure = None
        try:
            for code in codes:
                self.key_make(code)
                made.append(code)
            return self.run_frames(frames)
        except BaseException as error:
            failure = error
            raise
        finally:
            for code in reversed(made):
                try:
                    self.key_break(code)
                except BaseException:
                    if failure is None:
                        raise

    def mouse(self, dx: int, dy: int, button: int) -> None:
        """Send one raw relative IKBD mouse event.

        ``button`` is a state mask: bit 0 left, bit 1 right.  This command does
        not synthesize a click; use :meth:`click` for press/release.
        """
        if (any(not isinstance(x, int) or isinstance(x, bool) for x in (dx, dy, button)) or
                not -32768 <= dx <= 32767 or not -32768 <= dy <= 32767 or not 0 <= button <= 3):
            raise ValueError("invalid mouse arguments")
        self._request(f"mouse {dx} {dy} {button}")

    def move(self, dx: int, dy: int, *, step: int = 8, frames: int = 1) -> None:
        """Move by ``dx``/``dy`` in small relative steps.

        A single large relative event can exceed the IKBD packet range or TOS's
        per-event clamping.  Splitting keeps the motion in the same range as
        NeoST's ``--mouse-at`` ``L/R/U/D`` tokens.  ``frames`` advances the
        machine after each step; keep it at 1 unless deliberately batching.
        """
        if (any(not isinstance(x, int) or isinstance(x, bool) for x in (dx, dy, step, frames)) or
                not -32768 <= dx <= 32767 or not -32768 <= dy <= 32767 or
                not 1 <= step <= 127 or frames < 0):
            raise ValueError("invalid move arguments")
        while dx or dy:
            sx = max(-step, min(step, dx))
            sy = max(-step, min(step, dy))
            self.mouse(sx, sy, 0)
            dx -= sx
            dy -= sy
            if frames:
                self.run_frames(frames)

    def click(self, button: int = 1, *, frames: int = 1) -> Observation:
        """Click the current mouse position with a held-then-released button.

        ``frames`` must span at least one frame so the guest sees the press and
        release as separate events, matching ``--mouse-at``'s ``1`` token.
        """
        if (not isinstance(button, int) or isinstance(button, bool) or button not in (1, 2, 3) or
                not isinstance(frames, int) or isinstance(frames, bool) or frames < 1):
            raise ValueError("invalid click arguments")
        self.mouse(0, 0, button)
        self.run_frames(frames)
        self.mouse(0, 0, 0)
        return self.run_frames(frames)

    def peek(self, address: int, length: int) -> bytes:
        if (not isinstance(address, int) or isinstance(address, bool) or not 0 <= address < 1 << 24 or
                not isinstance(length, int) or isinstance(length, bool) or not 1 <= length <= 4096):
            raise ValueError("invalid peek range")
        data = self._request(f"peek {address:x} {length}")
        try:
            result = bytes.fromhex(data)
        except ValueError as error:
            self._fail("malformed peek response", error)
        if len(result) != length:
            self._fail("malformed peek response length")
        return result

    def probe(self, name: str, address: int, length: int) -> None:
        name = self._arg(name)
        if (not name or len(name) > 64 or any(c.isspace() or c == "=" for c in name) or
                not isinstance(address, int) or isinstance(address, bool) or not 0 <= address < 1 << 24 or
                not isinstance(length, int) or isinstance(length, bool) or length not in (1, 2, 4)):
            raise ValueError("invalid probe")
        self._request(f"probe {name}={address:x}:{length}")

    def _slot(self, slot: int) -> int:
        if not isinstance(slot, int) or isinstance(slot, bool) or not 0 <= slot < self._slots:
            raise ValueError("invalid slot")
        return slot

    def save(self, slot: int) -> None:
        self._request(f"save {self._slot(slot)}")

    def load(self, slot: int) -> Observation:
        return self._observation(self._request(f"load {self._slot(slot)}"))

    def export_state(self, slot: int, path: Path) -> None:
        self._request(f"export {self._slot(slot)} {self._arg(path)}")

    def import_state(self, slot: int, path: Path) -> None:
        self._request(f"import {self._slot(slot)} {self._arg(path)}")

    def screenshot(self, path: Path) -> None:
        self._request("shot " + self._arg(path))

    def slots(self) -> str:
        return self._request("slots")

    def insert(self, drive: str, path: Path) -> None:
        drive = self._arg(drive).upper()
        if drive not in ("A", "B"):
            raise ValueError("drive must be A or B")
        self._request(f"insert {drive} {self._arg(path)}")

    def eject(self, drive: str) -> None:
        drive = self._arg(drive).upper()
        if drive not in ("A", "B"):
            raise ValueError("drive must be A or B")
        self._request(f"eject {drive}")

    def _terminate(self) -> None:
        if self._closed:
            return
        self._closed = True
        proc = self._proc
        if proc is not None:
            if proc.poll() is None:
                try:
                    proc.kill()
                except OSError:
                    pass
                try:
                    proc.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    pass
            for pipe in (proc.stdin, proc.stdout, proc.stderr):
                if pipe is not None:
                    try:
                        pipe.close()
                    except OSError:
                        pass
        for thread in (self._stdout_thread, self._stderr_thread):
            if thread and thread is not threading.current_thread():
                thread.join(timeout=1)
        for stream in (self._protocol, self._diagnostics):
            try:
                stream.close()
            except OSError:
                pass

    def close(self) -> None:
        if self._closed:
            return
        proc = self._proc
        if proc is None:
            self._closed = True
            return
        try:
            self._request("quit", timeout=2)
            try:
                status = proc.wait(timeout=2)
            except subprocess.TimeoutExpired as error:
                raise NeoSTTransportError("NeoST did not exit after quit") from error
            if status:
                raise NeoSTTransportError(f"NeoST exited {status} after quit")
        finally:
            self._terminate()

    def __enter__(self) -> "NeoSTServer":
        return self

    def __exit__(self, exc_type, exc, traceback) -> None:
        try:
            self.close()
        except NeoSTError:
            if exc_type is None:
                raise
