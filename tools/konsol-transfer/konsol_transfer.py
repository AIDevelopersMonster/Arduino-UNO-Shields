#!/usr/bin/env python3
"""KonSol SD Writer: send a local KAP file through the KonSol serial shell.

The utility intentionally does not write the microSD card directly. It reproduces
the manual workflow used on the physical bench:

    WRITE /FILE.KAP <first chunk>
    APPEND /FILE.KAP <next chunk>
    APPEND /FILE.KAP <next chunk>
    ...

Each command is acknowledged by KonSol before the next command is sent.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys
import threading
import time
from dataclasses import dataclass
from typing import Callable, Iterable

PROMPT = b"A:/> "
DEFAULT_BAUD = 115200
MAX_KONSOL_COMMAND = 87  # firmware CMD_SIZE is 88 including the NUL terminator

try:
    import serial
    from serial.tools import list_ports
except ImportError:  # handled explicitly by main()/GUI
    serial = None
    list_ports = None


class TransferError(RuntimeError):
    pass


def require_pyserial() -> None:
    if serial is None:
        raise TransferError(
            "pyserial is required. Install it with: python -m pip install pyserial"
        )


def available_ports() -> list[str]:
    require_pyserial()
    return [p.device for p in list_ports.comports()]


def validate_83_path(path: str) -> str:
    path = path.strip().replace("\\", "/")
    if not path.startswith("/"):
        path = "/" + path

    if path == "/":
        raise TransferError("destination must be a file, not /")

    for part in [x for x in path.split("/") if x]:
        if "." in part:
            base, ext = part.rsplit(".", 1)
            if not (1 <= len(base) <= 8 and 1 <= len(ext) <= 3):
                raise TransferError(
                    f"'{part}' is not an 8.3 name; use at most 8+3 characters"
                )
        elif not (1 <= len(part) <= 8):
            raise TransferError(
                f"'{part}' is not an 8.3 directory name; use at most 8 characters"
            )

        if not re.fullmatch(r"[A-Za-z0-9_-]+(?:\.[A-Za-z0-9_-]+)?", part):
            raise TransferError(
                f"unsupported character in '{part}'; use letters, digits, _ or -"
            )

    return path.upper()


def default_destination(source: pathlib.Path) -> str:
    stem = re.sub(r"[^A-Za-z0-9_-]", "_", source.stem.upper())[:8] or "APP"
    suffix = re.sub(r"[^A-Za-z0-9_-]", "", source.suffix.lstrip(".").upper())[:3]
    if not suffix:
        suffix = "KAP"
    return f"/{stem}.{suffix}"


def read_kap(source: pathlib.Path) -> tuple[list[str], str]:
    try:
        text = source.read_text(encoding="ascii")
    except UnicodeDecodeError as exc:
        raise TransferError("KAP file must contain ASCII text only") from exc
    except OSError as exc:
        raise TransferError(str(exc)) from exc

    records: list[str] = []

    for lineno, raw in enumerate(text.splitlines(), 1):
        compact = "".join(raw.split())
        if not compact:
            continue
        if not re.fullmatch(r"[0-9A-Fa-f]+", compact):
            raise TransferError(f"line {lineno}: KAP data must be hexadecimal")
        if len(compact) % 2:
            raise TransferError(f"line {lineno}: odd number of hexadecimal digits")
        records.append(compact.upper())

    if not records:
        raise TransferError("KAP file is empty")

    normalized = "".join(records)
    if not (normalized.startswith("4B415031") or normalized.startswith("4B415032")):
        raise TransferError("file does not start with KAP1/KAP2 header")

    return records, normalized


def build_shell_commands(records: Iterable[str], destination: str) -> list[str]:
    dest = validate_83_path(destination)
    commands: list[str] = []
    first = True

    for record in records:
        rest = record
        while rest:
            verb = "WRITE" if first else "APPEND"
            prefix = f"{verb} {dest} "
            room = MAX_KONSOL_COMMAND - len(prefix)
            room -= room % 2  # never split an encoded byte between shell commands

            if room < 2:
                raise TransferError("destination path leaves no room for KAP data")

            chunk = rest[:room]
            rest = rest[len(chunk):]
            commands.append(prefix + chunk)
            first = False

    return commands


def extract_type_payload(response: str) -> str:
    lines = response.replace("\r", "").split("\n")
    marks = [i for i, line in enumerate(lines) if line.strip() == "-----"]
    if len(marks) < 2:
        raise TransferError("TYPE response did not contain file delimiters")
    return "\n".join(lines[marks[0] + 1:marks[1]])


@dataclass
class TransferResult:
    destination: str
    commands: int
    source_bytes: int
    verified: bool


class KonSolLink:
    def __init__(self, port: str, baud: int = DEFAULT_BAUD, timeout: float = 5.0):
        require_pyserial()
        self.port = port
        self.baud = baud
        self.timeout = timeout
        self.ser = None

    def __enter__(self) -> "KonSolLink":
        try:
            self.ser = serial.Serial(
                self.port,
                self.baud,
                timeout=0.10,
                write_timeout=2.0,
            )
        except serial.SerialException as exc:
            raise TransferError(f"cannot open {self.port}: {exc}") from exc

        # Opening a classic UNO serial port can reset the board. Give KonSol time
        # to boot, discard the boot banner, then request a fresh prompt.
        time.sleep(2.2)
        self.ser.reset_input_buffer()
        self.ser.write(b"\n")
        self.ser.flush()
        self._read_until_prompt(self.timeout)
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        if self.ser is not None:
            self.ser.close()
            self.ser = None

    def _read_until_prompt(self, timeout: float | None = None) -> str:
        if self.ser is None:
            raise TransferError("serial port is not open")

        deadline = time.monotonic() + (timeout or self.timeout)
        data = bytearray()

        while time.monotonic() < deadline:
            waiting = self.ser.in_waiting
            block = self.ser.read(waiting if waiting else 1)
            if block:
                data.extend(block)
                at = data.find(PROMPT)
                if at >= 0:
                    return bytes(data[:at]).decode("ascii", errors="replace")
            else:
                time.sleep(0.01)

        tail = bytes(data[-160:]).decode("ascii", errors="replace")
        raise TransferError(f"timeout waiting for KonSol prompt; received: {tail!r}")

    def command(self, command: str) -> str:
        if self.ser is None:
            raise TransferError("serial port is not open")
        if len(command) > MAX_KONSOL_COMMAND:
            raise TransferError(
                f"command is {len(command)} chars; KonSol limit is {MAX_KONSOL_COMMAND}"
            )

        self.ser.write(command.encode("ascii") + b"\n")
        self.ser.flush()
        return self._read_until_prompt()


def transfer_kap(
    port: str,
    source: pathlib.Path,
    destination: str,
    baud: int = DEFAULT_BAUD,
    verify: bool = True,
    log: Callable[[str], None] | None = None,
    progress: Callable[[int, int], None] | None = None,
) -> TransferResult:
    logger = log or (lambda _msg: None)
    records, normalized = read_kap(source)
    destination = validate_83_path(destination)
    commands = build_shell_commands(records, destination)

    logger(f"Source: {source}")
    logger(f"Destination: {destination}")
    logger(f"Shell commands: {len(commands)}")

    with KonSolLink(port, baud=baud) as link:
        for index, command in enumerate(commands, 1):
            logger(f"> {command}")
            response = link.command(command).strip()
            if response:
                for line in response.replace("\r", "").split("\n"):
                    if line:
                        logger(f"< {line}")

            if "ERR " in response or not any(
                line.startswith("OK ") for line in response.replace("\r", "").split("\n")
            ):
                raise TransferError(
                    f"KonSol rejected transfer command {index}/{len(commands)}: "
                    f"{response!r}"
                )

            if progress:
                progress(index, len(commands))

        verified = False
        if verify:
            logger(f"> TYPE {destination}")
            response = link.command(f"TYPE {destination}")
            payload = extract_type_payload(response)
            remote = "".join(payload.split()).upper()
            verified = remote == normalized
            logger("VERIFY: PASS" if verified else "VERIFY: FAIL")
            if not verified:
                raise TransferError("remote KAP content differs from local byte stream")

    return TransferResult(
        destination=destination,
        commands=len(commands),
        source_bytes=len(normalized) // 2,
        verified=verified,
    )


def run_gui() -> int:
    require_pyserial()

    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk

    root = tk.Tk()
    root.title("KonSol SD Writer")
    root.geometry("820x600")

    port_var = tk.StringVar(value="")
    file_var = tk.StringVar(value="")
    dest_var = tk.StringVar(value="/MULTI.KAP")
    verify_var = tk.BooleanVar(value=True)
    status_var = tk.StringVar(value="Ready")

    frame = ttk.Frame(root, padding=12)
    frame.pack(fill="both", expand=True)
    frame.columnconfigure(1, weight=1)
    frame.rowconfigure(6, weight=1)

    ttk.Label(frame, text="Serial port").grid(row=0, column=0, sticky="w", padx=(0, 8), pady=4)
    port_box = ttk.Combobox(frame, textvariable=port_var, width=18, state="readonly")
    port_box.grid(row=0, column=1, sticky="w", pady=4)

    def refresh_ports() -> None:
        ports = available_ports()
        port_box["values"] = ports
        if ports and port_var.get() not in ports:
            port_var.set(ports[0])
        if not ports:
            port_var.set("")

    ttk.Button(frame, text="Refresh", command=refresh_ports).grid(row=0, column=2, padx=4, pady=4)

    ttk.Label(frame, text="Local KAP").grid(row=1, column=0, sticky="w", padx=(0, 8), pady=4)
    ttk.Entry(frame, textvariable=file_var).grid(row=1, column=1, sticky="ew", pady=4)

    def browse() -> None:
        filename = filedialog.askopenfilename(
            title="Select KAP file",
            filetypes=[("KonSol application", "*.KAP *.kap"), ("All files", "*.*")],
        )
        if filename:
            p = pathlib.Path(filename)
            file_var.set(str(p))
            dest_var.set(default_destination(p))

    ttk.Button(frame, text="Browse...", command=browse).grid(row=1, column=2, padx=4, pady=4)

    ttk.Label(frame, text="SD destination").grid(row=2, column=0, sticky="w", padx=(0, 8), pady=4)
    ttk.Entry(frame, textvariable=dest_var, width=24).grid(row=2, column=1, sticky="w", pady=4)
    ttk.Checkbutton(frame, text="Verify with TYPE", variable=verify_var).grid(
        row=3, column=1, sticky="w", pady=4
    )

    progress_bar = ttk.Progressbar(frame, mode="determinate")
    progress_bar.grid(row=4, column=0, columnspan=3, sticky="ew", pady=(8, 4))

    button_row = ttk.Frame(frame)
    button_row.grid(row=5, column=0, columnspan=3, sticky="ew", pady=4)

    log_text = tk.Text(frame, wrap="none", height=22)
    log_text.grid(row=6, column=0, columnspan=3, sticky="nsew", pady=(8, 4))
    scroll = ttk.Scrollbar(frame, orient="vertical", command=log_text.yview)
    scroll.grid(row=6, column=3, sticky="ns", pady=(8, 4))
    log_text.configure(yscrollcommand=scroll.set)

    ttk.Label(frame, textvariable=status_var).grid(
        row=7, column=0, columnspan=3, sticky="w", pady=(4, 0)
    )

    def gui_log(message: str) -> None:
        def append() -> None:
            log_text.insert("end", message + "\n")
            log_text.see("end")
        root.after(0, append)

    def gui_progress(done: int, total: int) -> None:
        def update() -> None:
            progress_bar["maximum"] = total
            progress_bar["value"] = done
            status_var.set(f"Writing {done}/{total}")
        root.after(0, update)

    def worker() -> None:
        try:
            source = pathlib.Path(file_var.get())
            if not source.is_file():
                raise TransferError("select an existing local KAP file")
            if not port_var.get():
                raise TransferError("select a serial port")

            result = transfer_kap(
                port=port_var.get(),
                source=source,
                destination=dest_var.get(),
                verify=verify_var.get(),
                log=gui_log,
                progress=gui_progress,
            )

            def done() -> None:
                status_var.set(
                    f"PASS: {result.destination}, {result.commands} commands"
                )
                transfer_button.configure(state="normal")
                messagebox.showinfo(
                    "KonSol SD Writer",
                    f"PASS\n{result.destination}\n"
                    f"{result.commands} shell commands\n"
                    f"Verify: {'PASS' if result.verified else 'skipped'}",
                )
            root.after(0, done)

        except Exception as exc:
            error_message = str(exc)

            def failed(message: str = error_message) -> None:
                status_var.set("FAIL")
                transfer_button.configure(state="normal")
                messagebox.showerror("KonSol SD Writer", message)

            gui_log(f"ERROR: {error_message}")
            root.after(0, failed)

    def start_transfer() -> None:
        log_text.delete("1.0", "end")
        progress_bar["value"] = 0
        status_var.set("Connecting...")
        transfer_button.configure(state="disabled")
        threading.Thread(target=worker, daemon=True).start()

    transfer_button = ttk.Button(button_row, text="Write KAP to microSD", command=start_transfer)
    transfer_button.pack(side="left")
    ttk.Button(button_row, text="Clear log", command=lambda: log_text.delete("1.0", "end")).pack(
        side="left", padx=8
    )

    refresh_ports()
    root.mainloop()
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Write KAP files to microSD through the KonSol serial shell"
    )
    sub = parser.add_subparsers(dest="command")

    sub.add_parser("ports", help="list serial ports")
    sub.add_parser("gui", help="open the graphical writer")

    write = sub.add_parser("write", help="write a local KAP file through WRITE/APPEND")
    write.add_argument("--port", required=True, help="serial port, e.g. COM4")
    write.add_argument("--file", required=True, type=pathlib.Path, help="local .KAP file")
    write.add_argument("--dest", help="microSD 8.3 destination, e.g. /MULTI.KAP")
    write.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    write.add_argument(
        "--no-verify",
        action="store_true",
        help="skip TYPE read-back verification",
    )

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    try:
        if args.command in (None, "gui"):
            return run_gui()

        if args.command == "ports":
            for port in available_ports():
                print(port)
            return 0

        if args.command == "write":
            dest = args.dest or default_destination(args.file)
            result = transfer_kap(
                port=args.port,
                source=args.file,
                destination=dest,
                baud=args.baud,
                verify=not args.no_verify,
                log=print,
                progress=lambda done, total: print(f"[{done}/{total}]"),
            )
            print(
                f"TRANSFER PASS: {result.destination}; "
                f"{result.commands} commands; "
                f"{result.source_bytes} decoded bytes"
            )
            return 0

        parser.error("unknown command")

    except (TransferError, OSError) as exc:
        print(f"konsol-transfer: error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
