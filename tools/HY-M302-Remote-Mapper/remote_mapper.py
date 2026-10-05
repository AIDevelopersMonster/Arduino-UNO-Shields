#!/usr/bin/env python3
"""HY-M302 Remote Mapper — CLI + Tk GUI.

Workflow:
1. obtain the dedicated mapper sketch if missing;
2. compile it with arduino-cli and the local HY_M302 library;
3. upload it to an Arduino UNO;
4. ask the user to press named remote buttons one by one;
5. capture full NEC frames from the mapper firmware;
6. save JSON + generated C++ header/source files.

The generated files contain symbolic names such as KEY_OK instead of raw NEC
commands, so the same profile can be used by TEST firmware and KonSol-HY.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import queue
import re
import subprocess
import sys
import threading
import time
import urllib.request
from dataclasses import dataclass, asdict
from typing import Callable

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None

BAUD = 115200
DEFAULT_FQBN = "arduino:avr:uno"
SKETCH_REL = pathlib.Path(
    "libraries/HY_M302/examples/06_Remote_Mapper/06_Remote_Mapper.ino"
)
SKETCH_DIR_REL = SKETCH_REL.parent
LIB_REL = pathlib.Path("libraries")
OUT_REL = pathlib.Path("build/HY-M302/REMOTE-MAPPER")
RAW_SKETCH_URL = (
    "https://raw.githubusercontent.com/AIDevelopersMonster/"
    "Arduino-UNO-Shields/main/"
    "libraries/HY_M302/examples/06_Remote_Mapper/06_Remote_Mapper.ino"
)

DEFAULT_KEYS = [
    ("KEY_0", "0"),
    ("KEY_1", "1"),
    ("KEY_2", "2"),
    ("KEY_3", "3"),
    ("KEY_4", "4"),
    ("KEY_5", "5"),
    ("KEY_6", "6"),
    ("KEY_7", "7"),
    ("KEY_8", "8"),
    ("KEY_9", "9"),
    ("KEY_OK", "OK"),
    ("KEY_HOME", "HOME"),
    ("KEY_RETURN", "RETURN"),
    ("KEY_MENU", "MENU"),
    ("KEY_UP", "UP"),
    ("KEY_DOWN", "DOWN"),
    ("KEY_LEFT", "LEFT"),
    ("KEY_RIGHT", "RIGHT"),
    ("KEY_POWER", "POWER"),
]

FRAME_RE = re.compile(
    r"^FRAME RAW=0x([0-9A-Fa-f]+) ADDR=0x([0-9A-Fa-f]+) CMD=0x([0-9A-Fa-f]+)$"
)


class MapperError(RuntimeError):
    pass


@dataclass
class KeyCode:
    name: str
    label: str
    raw: int
    address: int
    command: int


def require_pyserial() -> None:
    if serial is None:
        raise MapperError(
            "pyserial is required. Install: python -m pip install pyserial"
        )


def repo_root_from_script() -> pathlib.Path:
    return pathlib.Path(__file__).resolve().parents[2]


def available_ports() -> list[str]:
    require_pyserial()
    return [p.device for p in list_ports.comports()]


def run_checked(args: list[str], cwd: pathlib.Path, log: Callable[[str], None]) -> None:
    log("> " + " ".join(args))
    proc = subprocess.run(
        args,
        cwd=str(cwd),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if proc.stdout:
        for line in proc.stdout.rstrip().splitlines():
            log(line)
    if proc.returncode != 0:
        raise MapperError(f"command failed with exit code {proc.returncode}")


def ensure_mapper_sketch(root: pathlib.Path, log: Callable[[str], None]) -> pathlib.Path:
    sketch = root / SKETCH_REL
    if sketch.exists():
        log(f"Mapper sketch: {sketch}")
        return sketch

    sketch.parent.mkdir(parents=True, exist_ok=True)
    log("Mapper sketch is missing locally; downloading repository copy...")
    try:
        with urllib.request.urlopen(RAW_SKETCH_URL, timeout=20) as response:
            data = response.read()
        sketch.write_bytes(data)
    except Exception as exc:
        raise MapperError(f"cannot download mapper sketch: {exc}") from exc

    log(f"Downloaded mapper sketch: {sketch}")
    return sketch


def compile_mapper(
    root: pathlib.Path,
    fqbn: str,
    log: Callable[[str], None],
) -> pathlib.Path:
    ensure_mapper_sketch(root, log)
    out = root / OUT_REL
    out.mkdir(parents=True, exist_ok=True)

    run_checked(
        [
            "arduino-cli",
            "compile",
            "--fqbn",
            fqbn,
            "--libraries",
            str(root / LIB_REL),
            "--output-dir",
            str(out),
            str(root / SKETCH_DIR_REL),
        ],
        root,
        log,
    )
    return out


def upload_mapper(
    root: pathlib.Path,
    port: str,
    fqbn: str,
    log: Callable[[str], None],
) -> None:
    out = compile_mapper(root, fqbn, log)
    run_checked(
        [
            "arduino-cli",
            "upload",
            "-p",
            port,
            "--fqbn",
            fqbn,
            "--input-dir",
            str(out),
        ],
        root,
        log,
    )


class MapperLink:
    def __init__(self, port: str, baud: int = BAUD):
        require_pyserial()
        self.port = port
        self.baud = baud
        self.ser = None

    def __enter__(self) -> "MapperLink":
        try:
            self.ser = serial.Serial(
                self.port,
                self.baud,
                timeout=0.10,
                write_timeout=2.0,
            )
        except serial.SerialException as exc:
            raise MapperError(f"cannot open {self.port}: {exc}") from exc

        # Classic UNO resets when the serial port opens.
        time.sleep(2.2)
        self.ser.reset_input_buffer()
        self.ser.write(b"ID\n")
        self.ser.flush()

        deadline = time.monotonic() + 3.0
        seen_id = False
        while time.monotonic() < deadline:
            line = self.readline(0.4)
            if not line:
                continue
            if line.startswith("HY_M302_REMOTE_MAPPER"):
                seen_id = True
            if seen_id and line == "READY":
                return self

        raise MapperError("mapper firmware did not answer with READY")

    def __exit__(self, exc_type, exc, tb) -> None:
        if self.ser is not None:
            self.ser.close()
            self.ser = None

    def readline(self, timeout: float = 0.2) -> str:
        if self.ser is None:
            raise MapperError("serial port is not open")
        old = self.ser.timeout
        self.ser.timeout = timeout
        try:
            raw = self.ser.readline()
        finally:
            self.ser.timeout = old
        return raw.decode("ascii", errors="replace").strip()

    def flush_frames(self) -> None:
        if self.ser is None:
            return
        self.ser.reset_input_buffer()

    def wait_frame(self, timeout: float = 15.0) -> KeyCode:
        if self.ser is None:
            raise MapperError("serial port is not open")

        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            line = self.readline(0.25)
            if not line:
                continue
            m = FRAME_RE.match(line)
            if not m:
                # REPEAT and informational lines are intentionally ignored.
                continue
            raw, addr, cmd = (int(x, 16) for x in m.groups())
            return KeyCode("", "", raw, addr, cmd)

        raise MapperError("timeout waiting for remote button")


def ensure_unique(result: list[KeyCode], candidate: KeyCode) -> None:
    for old in result:
        if old.address == candidate.address and old.command == candidate.command:
            raise MapperError(
                f"duplicate code: {candidate.name} conflicts with {old.name} "
                f"(ADDR=0x{candidate.address:X} CMD=0x{candidate.command:X})"
            )


def save_profile(root: pathlib.Path, profile_name: str, keys: list[KeyCode]) -> pathlib.Path:
    safe = re.sub(r"[^A-Za-z0-9_-]+", "_", profile_name).strip("_") or "remote"
    out = root / "profiles" / "HY-M302-Remotes" / safe
    out.mkdir(parents=True, exist_ok=True)

    payload = {
        "format": "HY_M302_REMOTE_MAP",
        "version": 1,
        "profile": profile_name,
        "protocol": "NEC",
        "keys": [asdict(k) for k in keys],
    }
    (out / "remote_map.json").write_text(
        json.dumps(payload, indent=2) + "\n", encoding="utf-8"
    )

    header = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        "namespace HY_M302_RemoteMap {",
        "enum Key : uint8_t {",
        "  KEY_NONE = 0,",
    ]
    for index, item in enumerate(keys, 1):
        header.append(f"  {item.name} = {index},")
    header += [
        "};",
        "",
        "struct Entry {",
        "  Key key;",
        "  uint16_t address;",
        "  uint8_t command;",
        "  uint32_t raw;",
        "};",
        "",
        "extern const Entry kEntries[];",
        "extern const uint8_t kEntryCount;",
        "Key decode(uint16_t address, uint8_t command);",
        "",
        "}  // namespace HY_M302_RemoteMap",
        "",
    ]
    (out / "HY_M302_RemoteMap.h").write_text("\n".join(header), encoding="utf-8")

    source = [
        '#include "HY_M302_RemoteMap.h"',
        "",
        "namespace HY_M302_RemoteMap {",
        "",
        "const Entry kEntries[] = {",
    ]
    for item in keys:
        source.append(
            f"  {{{item.name}, 0x{item.address:04X}, "
            f"0x{item.command:02X}, 0x{item.raw:08X}UL}},"
        )
    source += [
        "};",
        "",
        "const uint8_t kEntryCount = sizeof(kEntries) / sizeof(kEntries[0]);",
        "",
        "Key decode(uint16_t address, uint8_t command) {",
        "  for (uint8_t i = 0; i < kEntryCount; ++i) {",
        "    if (kEntries[i].address == address &&",
        "        kEntries[i].command == command) {",
        "      return kEntries[i].key;",
        "    }",
        "  }",
        "  return KEY_NONE;",
        "}",
        "",
        "}  // namespace HY_M302_RemoteMap",
        "",
    ]
    (out / "HY_M302_RemoteMap.cpp").write_text(
        "\n".join(source), encoding="utf-8"
    )

    return out


def learn_cli(args: argparse.Namespace) -> int:
    root = pathlib.Path(args.repo).resolve() if args.repo else repo_root_from_script()
    log = print

    if args.flash:
        upload_mapper(root, args.port, args.fqbn, log)

    learned: list[KeyCode] = []
    with MapperLink(args.port) as link:
        print("REMOTE MAPPER READY")
        print("Press each requested button once. Repeat frames are ignored.")
        print("Ctrl-C cancels without overwriting any profile.")

        for name, label in DEFAULT_KEYS:
            while True:
                link.flush_frames()
                input(f"[{label}] Press remote button, then press Enter here to arm...")
                print(f"Waiting for {label}...")
                try:
                    frame = link.wait_frame(args.timeout)
                except MapperError as exc:
                    print(f"{exc}; retrying")
                    continue

                frame.name = name
                frame.label = label

                try:
                    ensure_unique(learned, frame)
                except MapperError as exc:
                    print(exc)
                    print("Retry this key.")
                    continue

                print(
                    f"  {label}: RAW=0x{frame.raw:08X} "
                    f"ADDR=0x{frame.address:X} CMD=0x{frame.command:X}"
                )
                answer = input("Accept? [Y]es / [R]etry / [S]kip: ").strip().lower()
                if answer in ("", "y", "yes"):
                    learned.append(frame)
                    break
                if answer in ("s", "skip"):
                    break

    if not learned:
        raise MapperError("no keys learned")

    out = save_profile(root, args.name, learned)
    print(f"Saved profile: {out}")
    print(f"  {out / 'remote_map.json'}")
    print(f"  {out / 'HY_M302_RemoteMap.h'}")
    print(f"  {out / 'HY_M302_RemoteMap.cpp'}")
    return 0


class MapperGUI:
    def __init__(self, root_path: pathlib.Path):
        import tkinter as tk
        from tkinter import ttk

        self.tk = tk
        self.ttk = ttk
        self.root_path = root_path
        self.window = tk.Tk()
        self.window.title("HY-M302 Remote Mapper")
        self.window.geometry("760x610")

        self.port_var = tk.StringVar()
        self.name_var = tk.StringVar(value="iDroid-OrangePi")
        self.status_var = tk.StringVar(value="Ready")
        self.current_var = tk.StringVar(value="Not started")
        self.learned: list[KeyCode] = []
        self.worker_events: queue.Queue = queue.Queue()
        self.link: MapperLink | None = None
        self.step_index = 0

        top = ttk.Frame(self.window, padding=10)
        top.pack(fill="x")

        ttk.Label(top, text="COM port").grid(row=0, column=0, sticky="w")
        self.port_combo = ttk.Combobox(top, textvariable=self.port_var, width=18)
        self.port_combo.grid(row=0, column=1, padx=6)
        ttk.Button(top, text="Refresh", command=self.refresh_ports).grid(row=0, column=2)

        ttk.Label(top, text="Profile").grid(row=1, column=0, sticky="w", pady=(8, 0))
        ttk.Entry(top, textvariable=self.name_var, width=28).grid(
            row=1, column=1, padx=6, pady=(8, 0), sticky="w"
        )

        actions = ttk.Frame(self.window, padding=(10, 0))
        actions.pack(fill="x")
        ttk.Button(actions, text="1. Flash mapper", command=self.flash).pack(
            side="left", padx=(0, 8)
        )
        ttk.Button(actions, text="2. Start learning", command=self.start_learning).pack(
            side="left", padx=(0, 8)
        )
        self.capture_btn = ttk.Button(
            actions, text="Capture current key", command=self.capture_current
        )
        self.capture_btn.pack(side="left", padx=(0, 8))
        ttk.Button(actions, text="Skip", command=self.skip_current).pack(side="left")

        box = ttk.LabelFrame(self.window, text="Current step", padding=10)
        box.pack(fill="x", padx=10, pady=10)
        ttk.Label(box, textvariable=self.current_var, font=("Segoe UI", 14, "bold")).pack(
            anchor="w"
        )
        ttk.Label(
            box,
            text="Click Capture current key first, then press the requested remote button.",
        ).pack(anchor="w", pady=(6, 0))

        self.tree = ttk.Treeview(
            self.window,
            columns=("label", "raw", "addr", "cmd"),
            show="headings",
            height=13,
        )
        for col, title, width in [
            ("label", "Key", 130),
            ("raw", "RAW", 170),
            ("addr", "ADDR", 100),
            ("cmd", "CMD", 100),
        ]:
            self.tree.heading(col, text=title)
            self.tree.column(col, width=width, anchor="center")
        self.tree.pack(fill="both", expand=True, padx=10)

        bottom = ttk.Frame(self.window, padding=10)
        bottom.pack(fill="x")
        ttk.Button(bottom, text="Save files", command=self.save).pack(side="right")
        ttk.Label(bottom, textvariable=self.status_var).pack(side="left")

        self.refresh_ports()
        self._update_step_label()
        self.window.after(100, self.poll_events)
        self.window.protocol("WM_DELETE_WINDOW", self.close)

    def log_event(self, kind: str, payload) -> None:
        self.worker_events.put((kind, payload))

    def refresh_ports(self) -> None:
        try:
            ports = available_ports()
        except Exception as exc:
            self.status_var.set(str(exc))
            return
        self.port_combo["values"] = ports
        if ports and not self.port_var.get():
            self.port_var.set(ports[0])

    def _require_port(self) -> str:
        port = self.port_var.get().strip()
        if not port:
            raise MapperError("Select a COM port")
        return port

    def flash(self) -> None:
        try:
            port = self._require_port()
        except Exception as exc:
            self.status_var.set(str(exc))
            return

        self.status_var.set("Compiling/uploading mapper...")

        def work():
            try:
                upload_mapper(
                    self.root_path,
                    port,
                    DEFAULT_FQBN,
                    lambda msg: self.log_event("log", msg),
                )
                self.log_event("flash_ok", None)
            except Exception as exc:
                self.log_event("error", str(exc))

        threading.Thread(target=work, daemon=True).start()

    def start_learning(self) -> None:
        try:
            port = self._require_port()
            if self.link is not None:
                self.link.__exit__(None, None, None)
            self.link = MapperLink(port)
            self.link.__enter__()
            self.learned.clear()
            self.step_index = 0
            for item in self.tree.get_children():
                self.tree.delete(item)
            self._update_step_label()
            self.status_var.set("Mapper connected")
        except Exception as exc:
            self.status_var.set(str(exc))

    def _update_step_label(self) -> None:
        if self.step_index >= len(DEFAULT_KEYS):
            self.current_var.set("Learning complete")
            return
        _name, label = DEFAULT_KEYS[self.step_index]
        self.current_var.set(f"Press: {label}")

    def capture_current(self) -> None:
        if self.link is None:
            self.status_var.set("Start learning first")
            return
        if self.step_index >= len(DEFAULT_KEYS):
            self.status_var.set("All requested keys are complete")
            return

        name, label = DEFAULT_KEYS[self.step_index]
        self.status_var.set(f"Waiting for {label}...")
        self.link.flush_frames()

        def work():
            try:
                frame = self.link.wait_frame(15.0)
                frame.name = name
                frame.label = label
                ensure_unique(self.learned, frame)
                self.log_event("frame", frame)
            except Exception as exc:
                self.log_event("error", str(exc))

        threading.Thread(target=work, daemon=True).start()

    def skip_current(self) -> None:
        if self.step_index < len(DEFAULT_KEYS):
            self.step_index += 1
            self._update_step_label()

    def save(self) -> None:
        if not self.learned:
            self.status_var.set("No learned keys to save")
            return
        try:
            out = save_profile(self.root_path, self.name_var.get(), self.learned)
            self.status_var.set(f"Saved: {out}")
        except Exception as exc:
            self.status_var.set(str(exc))

    def poll_events(self) -> None:
        try:
            while True:
                kind, payload = self.worker_events.get_nowait()
                if kind == "flash_ok":
                    self.status_var.set("Mapper firmware uploaded")
                elif kind == "log":
                    self.status_var.set(str(payload))
                elif kind == "error":
                    self.status_var.set(str(payload))
                elif kind == "frame":
                    frame: KeyCode = payload
                    self.learned.append(frame)
                    self.tree.insert(
                        "",
                        "end",
                        values=(
                            frame.label,
                            f"0x{frame.raw:08X}",
                            f"0x{frame.address:X}",
                            f"0x{frame.command:X}",
                        ),
                    )
                    self.status_var.set(
                        f"Captured {frame.label}: CMD=0x{frame.command:X}"
                    )
                    self.step_index += 1
                    self._update_step_label()
        except queue.Empty:
            pass
        self.window.after(100, self.poll_events)

    def close(self) -> None:
        if self.link is not None:
            self.link.__exit__(None, None, None)
            self.link = None
        self.window.destroy()

    def run(self) -> int:
        self.window.mainloop()
        return 0


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="HY-M302 remote learning tool")
    p.add_argument("--repo", help="repository root; defaults to script repository")
    sub = p.add_subparsers(dest="command")

    sub.add_parser("ports", help="list serial ports")

    flash = sub.add_parser("flash", help="compile and upload mapper firmware")
    flash.add_argument("--port", required=True)
    flash.add_argument("--fqbn", default=DEFAULT_FQBN)

    learn = sub.add_parser("learn", help="interactive CLI remote learning")
    learn.add_argument("--port", required=True)
    learn.add_argument("--name", default="iDroid-OrangePi")
    learn.add_argument("--fqbn", default=DEFAULT_FQBN)
    learn.add_argument("--timeout", type=float, default=15.0)
    learn.add_argument(
        "--flash",
        action="store_true",
        help="compile/upload mapper before learning",
    )

    sub.add_parser("gui", help="open Tk GUI")
    return p


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    try:
        if args.command in (None, "gui"):
            root = pathlib.Path(args.repo).resolve() if args.repo else repo_root_from_script()
            return MapperGUI(root).run()

        if args.command == "ports":
            for port in available_ports():
                print(port)
            return 0

        root = pathlib.Path(args.repo).resolve() if args.repo else repo_root_from_script()

        if args.command == "flash":
            upload_mapper(root, args.port, args.fqbn, print)
            return 0

        if args.command == "learn":
            return learn_cli(args)

        parser.print_help()
        return 2

    except KeyboardInterrupt:
        print("\nCancelled.", file=sys.stderr)
        return 130
    except MapperError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
