#!/usr/bin/env python3
"""KonSol Host Manager v0.1 for HOST1 / KonSol 0.7.

Windows-first Tkinter GUI for managing a KonSol appliance over USB-TTL Serial.
Requires pyserial. Tkinter is provided by the normal python.org Windows build.
"""

from __future__ import annotations

import binascii
import hashlib
import pathlib
import queue
import re
import subprocess
import sys
import threading
import time
import tkinter as tk
from tkinter import filedialog, messagebox, simpledialog, ttk

try:
    import serial
    from serial.tools import list_ports
except ImportError as exc:
    raise SystemExit(
        "pyserial is required. Install it with: python -m pip install pyserial"
    ) from exc

BAUD = 115200
HOST_TIMEOUT = 10.0
SAFE_PUTD_LINE = 59


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for value in data:
        crc ^= value << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


class Host1Error(RuntimeError):
    pass


class Host1Client:
    def __init__(self, logger):
        self.ser: serial.Serial | None = None
        self.lock = threading.Lock()
        self.rx = bytearray()
        self.logger = logger

    @property
    def connected(self) -> bool:
        return self.ser is not None and self.ser.is_open

    def connect(self, port: str) -> str:
        with self.lock:
            self._disconnect_unlocked()
            self.ser = serial.Serial(
                port=port,
                baudrate=BAUD,
                bytesize=8,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.05,
                write_timeout=2.0,
            )
            self.ser.dtr = True
            self.ser.rts = True
            self.rx.clear()

            time.sleep(2.5)
            self.ser.reset_input_buffer()
            self.rx.clear()

            lines = self._send_expect_unlocked("@PING", r"^@OK PONG HOST1$")
            return lines[-1]

    def disconnect(self) -> None:
        with self.lock:
            self._disconnect_unlocked()

    def _disconnect_unlocked(self) -> None:
        if self.ser is not None:
            try:
                if self.ser.is_open:
                    self.ser.close()
            finally:
                self.ser = None
                self.rx.clear()

    def _require(self) -> serial.Serial:
        if not self.connected or self.ser is None:
            raise Host1Error("KonSol is not connected")
        return self.ser

    def _emit(self, line: str) -> None:
        self.logger(line)

    def _write_line_unlocked(self, command: str) -> None:
        ser = self._require()
        encoded = command.encode("ascii")
        ser.write(encoded + b"\n")
        ser.flush()
        self._emit("> " + command)

    def _extract_buffered_line_unlocked(self) -> str | None:
        pos = self.rx.find(b"\n")
        if pos < 0:
            return None
        raw = bytes(self.rx[:pos])
        del self.rx[: pos + 1]
        return raw.rstrip(b"\r").decode("utf-8", "replace").strip()

    @staticmethod
    def _normalize_line(line: str) -> str:
        # After opening the USB serial port, DTR resets the UNO. The KonSol
        # human prompt "A:/> " has no trailing newline, so the first HOST1
        # response can legally arrive as:
        #
        #     A:/> @OK PONG HOST1
        #
        # Keep boot text visible in the log, but strip only this known prompt
        # prefix before HOST1 matching.
        marker = "A:/> "
        if line.startswith(marker + "@"):
            return line[len(marker):]
        return line

    def _next_line_unlocked(self, timeout: float = HOST_TIMEOUT) -> str:
        ser = self._require()
        deadline = time.monotonic() + timeout

        while time.monotonic() < deadline:
            line = self._extract_buffered_line_unlocked()
            if line is not None:
                if line:
                    self._emit(line)
                    return self._normalize_line(line)
                continue

            waiting = ser.in_waiting
            chunk = ser.read(waiting if waiting > 0 else 1)
            if chunk:
                self.rx.extend(chunk)

        raise TimeoutError("HOST1 response timeout")

    def _send_expect_unlocked(
        self, command: str, pattern: str, timeout: float = HOST_TIMEOUT
    ) -> list[str]:
        self._write_line_unlocked(command)
        match = re.compile(pattern)
        lines: list[str] = []
        deadline = time.monotonic() + timeout

        while time.monotonic() < deadline:
            remaining = max(0.1, deadline - time.monotonic())
            line = self._next_line_unlocked(remaining)
            lines.append(line)
            if line.startswith("@ERR"):
                raise Host1Error(line)
            if match.match(line):
                return lines

        raise TimeoutError(f"HOST1 timeout waiting for {pattern}")

    def request(self, command: str, pattern: str) -> list[str]:
        with self.lock:
            return self._send_expect_unlocked(command, pattern)

    def info(self) -> str:
        return self.request("@INFO", r"^@OK INFO ")[-1]

    def mem(self) -> str:
        return self.request("@MEM", r"^@OK MEM \d+$")[-1]

    def app_status(self) -> str:
        return self.request("@APP", r"^@OK APP ")[-1]

    def tasks(self) -> list[str]:
        with self.lock:
            self._write_line_unlocked("@PS")
            lines: list[str] = []
            while True:
                line = self._next_line_unlocked()
                if line.startswith("@ERR"):
                    raise Host1Error(line)
                lines.append(line)
                if re.match(r"^@END PS \d+$", line):
                    return lines

    def ls(self, path: str) -> list[dict[str, object]]:
        path = path.strip() or "/"
        with self.lock:
            self._write_line_unlocked(f"@LS {path}")
            items: list[dict[str, object]] = []
            while True:
                line = self._next_line_unlocked()
                if line.startswith("@ERR"):
                    raise Host1Error(line)
                if line.startswith("@D "):
                    items.append({"type": "DIR", "size": "", "name": line[3:]})
                elif line.startswith("@F "):
                    parts = line.split(" ", 2)
                    if len(parts) == 3:
                        items.append(
                            {"type": "FILE", "size": int(parts[1]), "name": parts[2]}
                        )
                elif re.match(r"^@END LS \d+$", line):
                    return items

    def put_file(self, local: pathlib.Path, remote: str, progress=None) -> dict[str, object]:
        data = local.read_bytes()
        crc = crc16_ccitt(data)
        sha = hashlib.sha256(data).hexdigest().upper()
        remote = remote.strip()
        if not remote.startswith("/"):
            remote = "/" + remote

        base_len = len(f"@PUTD {remote} ")
        chunk_size = min(20, (SAFE_PUTD_LINE - base_len) // 2)
        if chunk_size < 1:
            raise Host1Error("Remote path is too long for safe HOST1 PUTD transfer")

        with self.lock:
            self._send_expect_unlocked(f"@PUTB {remote}", r"^@OK PUTB$")

            total = len(data)
            for offset in range(0, total, chunk_size):
                chunk = data[offset : offset + chunk_size]
                hex_data = binascii.hexlify(chunk).decode("ascii").upper()
                self._send_expect_unlocked(
                    f"@PUTD {remote} {hex_data}", r"^@OK PUTD \d+$"
                )
                if progress:
                    progress(min(offset + len(chunk), total), total)

            self._send_expect_unlocked(
                f"@PUTE {remote} {len(data)} {crc:04X}",
                rf"^@OK PUTE {len(data)} {crc:04X}$",
            )

        return {
            "remote": remote,
            "size": len(data),
            "crc": f"{crc:04X}",
            "sha256": sha,
        }

    def get_file(self, remote: str, local: pathlib.Path, progress=None) -> dict[str, object]:
        with self.lock:
            self._write_line_unlocked(f"@GET {remote}")
            payload = bytearray()
            expected_size: int | None = None
            end_size: int | None = None
            end_crc: int | None = None

            while True:
                line = self._next_line_unlocked()
                if line.startswith("@ERR"):
                    raise Host1Error(line)
                match = re.match(r"^@BEGIN GET (\d+)$", line)
                if match:
                    expected_size = int(match.group(1))
                    continue
                match = re.match(r"^@DATA ([0-9A-Fa-f]+)$", line)
                if match:
                    payload.extend(binascii.unhexlify(match.group(1)))
                    if progress and expected_size is not None:
                        progress(len(payload), expected_size)
                    continue
                match = re.match(r"^@END GET (\d+) ([0-9A-Fa-f]{4})$", line)
                if match:
                    end_size = int(match.group(1))
                    end_crc = int(match.group(2), 16)
                    break

        if expected_size is None or end_size is None or end_crc is None:
            raise Host1Error("Incomplete GET response")
        if len(payload) != expected_size or end_size != expected_size:
            raise Host1Error(
                f"GET size mismatch: begin={expected_size} end={end_size} rx={len(payload)}"
            )

        crc = crc16_ccitt(bytes(payload))
        if crc != end_crc:
            raise Host1Error(f"GET CRC mismatch: local={crc:04X} remote={end_crc:04X}")

        local.write_bytes(payload)
        sha = hashlib.sha256(payload).hexdigest().upper()
        return {
            "local": str(local),
            "size": len(payload),
            "crc": f"{crc:04X}",
            "sha256": sha,
        }

    def run(self, remote: str) -> None:
        self.request(f"@RUN {remote}", r"^@OK RUN$")

    def stop(self) -> None:
        self.request("@STOP", r"^@OK STOP$")

    def delete(self, remote: str) -> None:
        self.request(f"@DEL {remote}", r"^@OK DEL$")

    def drain_unsolicited(self) -> list[str]:
        if not self.connected or not self.lock.acquire(blocking=False):
            return []
        try:
            ser = self._require()
            waiting = ser.in_waiting
            if waiting:
                self.rx.extend(ser.read(waiting))

            lines: list[str] = []
            while True:
                line = self._extract_buffered_line_unlocked()
                if line is None:
                    break
                if line:
                    lines.append(line)
            return lines
        finally:
            self.lock.release()


class HostManager(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("KonSol Host Manager — TEST-10")
        self.geometry("1040x720")
        self.minsize(900, 620)

        self.events: queue.Queue[tuple] = queue.Queue()
        self.client = Host1Client(self._log_from_worker)
        self.current_path = tk.StringVar(value="/")
        self.port_var = tk.StringVar()
        self.status_var = tk.StringVar(value="DISCONNECTED")
        self.info_var = tk.StringVar(value="KonSol: -    SD: -    APP: -    RAM: -")
        self.progress_var = tk.DoubleVar(value=0.0)

        self._build_ui()
        self.refresh_ports()
        self.after(80, self._process_events)
        self.after(150, self._poll_serial)
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    def _build_ui(self) -> None:
        top = ttk.Frame(self, padding=8)
        top.pack(fill="x")

        ttk.Label(top, text="Serial port:").pack(side="left")
        self.port_box = ttk.Combobox(
            top, textvariable=self.port_var, width=18, state="readonly"
        )
        self.port_box.pack(side="left", padx=(6, 4))
        ttk.Button(top, text="Refresh ports", command=self.refresh_ports).pack(
            side="left", padx=4
        )
        ttk.Button(top, text="Connect", command=self.connect_device).pack(
            side="left", padx=4
        )
        ttk.Button(top, text="Disconnect", command=self.disconnect_device).pack(
            side="left", padx=4
        )
        ttk.Label(top, textvariable=self.status_var).pack(side="right")

        status = ttk.Frame(self, padding=(8, 0, 8, 8))
        status.pack(fill="x")
        ttk.Label(status, textvariable=self.info_var).pack(side="left")
        ttk.Button(status, text="Refresh status", command=self.refresh_status).pack(
            side="right", padx=4
        )
        ttk.Button(status, text="Tasks", command=self.show_tasks).pack(
            side="right", padx=4
        )
        ttk.Button(status, text="APP", command=self.show_app_status).pack(
            side="right", padx=4
        )

        nav = ttk.Frame(self, padding=(8, 0, 8, 6))
        nav.pack(fill="x")
        ttk.Label(nav, text="SD path:").pack(side="left")
        ttk.Entry(nav, textvariable=self.current_path).pack(
            side="left", fill="x", expand=True, padx=6
        )
        ttk.Button(nav, text="Up", command=self.go_up).pack(side="left", padx=3)
        ttk.Button(nav, text="Refresh files", command=self.refresh_files).pack(
            side="left", padx=3
        )

        center = ttk.Panedwindow(self, orient="vertical")
        center.pack(fill="both", expand=True, padx=8, pady=(0, 8))

        files_frame = ttk.Frame(center)
        log_frame = ttk.Frame(center)
        center.add(files_frame, weight=3)
        center.add(log_frame, weight=2)

        columns = ("type", "size", "name")
        self.tree = ttk.Treeview(
            files_frame, columns=columns, show="headings", selectmode="browse"
        )
        self.tree.heading("type", text="Type")
        self.tree.heading("size", text="Size")
        self.tree.heading("name", text="Name")
        self.tree.column("type", width=80, anchor="center")
        self.tree.column("size", width=100, anchor="e")
        self.tree.column("name", width=600, anchor="w")
        self.tree.pack(side="left", fill="both", expand=True)
        self.tree.bind("<Double-1>", self._tree_double_click)

        scroll = ttk.Scrollbar(
            files_frame, orient="vertical", command=self.tree.yview
        )
        scroll.pack(side="right", fill="y")
        self.tree.configure(yscrollcommand=scroll.set)

        actions = ttk.Frame(self, padding=(8, 0, 8, 6))
        actions.pack(fill="x")
        ttk.Button(actions, text="Install KAP", command=self.install_kap).pack(
            side="left", padx=3
        )
        ttk.Button(actions, text="Download", command=self.download_selected).pack(
            side="left", padx=3
        )
        ttk.Button(actions, text="Run", command=self.run_selected).pack(
            side="left", padx=3
        )
        ttk.Button(actions, text="Stop", command=self.stop_app).pack(
            side="left", padx=3
        )
        ttk.Button(actions, text="Delete", command=self.delete_selected).pack(
            side="left", padx=3
        )
        ttk.Separator(actions, orient="vertical").pack(
            side="left", fill="y", padx=8
        )
        ttk.Button(
            actions, text="Build KASM -> KAP", command=self.build_kasm
        ).pack(side="left", padx=3)

        ttk.Progressbar(
            actions, variable=self.progress_var, maximum=100
        ).pack(side="right", fill="x", expand=True, padx=(16, 0))

        self.log = tk.Text(log_frame, height=12, wrap="none", state="disabled")
        self.log.pack(fill="both", expand=True)

    def _log_from_worker(self, line: str) -> None:
        self.events.put(("log", line))

    def _append_log(self, line: str) -> None:
        self.log.configure(state="normal")
        self.log.insert("end", line + "\n")
        self.log.see("end")
        self.log.configure(state="disabled")

    def _finish_progress(self) -> None:
        self.progress_var.set(100.0)
        self.after(900, lambda: self.progress_var.set(0.0))

    def _worker(self, fn, on_done=None) -> None:
        def run():
            try:
                result = fn()
                self.events.put(("done", on_done, result))
            except Exception as exc:
                self.events.put(("error", str(exc)))

        threading.Thread(target=run, daemon=True).start()

    def _process_events(self) -> None:
        try:
            while True:
                event = self.events.get_nowait()
                kind = event[0]
                if kind == "log":
                    self._append_log(event[1])
                elif kind == "progress":
                    done, total = event[1], event[2]
                    self.progress_var.set(
                        (done / total * 100.0) if total else 0.0
                    )
                elif kind == "done":
                    callback, result = event[1], event[2]
                    if callback:
                        callback(result)
                elif kind == "error":
                    self.progress_var.set(0.0)
                    if self.status_var.get().startswith("CONNECTING"):
                        self.status_var.set("DISCONNECTED")
                    self._append_log("ERROR: " + event[1])
                    messagebox.showerror("KonSol Host Manager", event[1])
        except queue.Empty:
            pass
        self.after(80, self._process_events)

    def _poll_serial(self) -> None:
        try:
            for line in self.client.drain_unsolicited():
                self._append_log(line)
        except Exception as exc:
            self._append_log("SERIAL: " + str(exc))
        self.after(150, self._poll_serial)

    def refresh_ports(self) -> None:
        ports = [p.device for p in list_ports.comports()]
        self.port_box["values"] = ports
        if self.port_var.get() not in ports:
            if "COM4" in ports:
                self.port_var.set("COM4")
            elif ports:
                self.port_var.set(ports[0])
            else:
                self.port_var.set("")

    def connect_device(self) -> None:
        port = self.port_var.get().strip()
        if not port:
            messagebox.showwarning(
                "KonSol Host Manager", "Select a serial port first"
            )
            return
        self.status_var.set("CONNECTING...")

        def done(_):
            self.status_var.set("CONNECTED " + port)
            self.refresh_status()
            self.refresh_files()

        self._worker(lambda: self.client.connect(port), done)

    def disconnect_device(self) -> None:
        try:
            self.client.disconnect()
        finally:
            self.status_var.set("DISCONNECTED")
            self.info_var.set("KonSol: -    SD: -    APP: -    RAM: -")

    @staticmethod
    def _parse_info(line: str) -> dict[str, str]:
        data: dict[str, str] = {}
        for token in line.split()[2:]:
            if "=" in token:
                key, value = token.split("=", 1)
                data[key] = value
        return data

    def refresh_status(self) -> None:
        def op():
            return self.client.info()

        def done(line):
            data = self._parse_info(line)
            self.info_var.set(
                "KonSol: {v}    HOST: {host}    SD: {sd}    APP: {app}    RAM: {ram} B    TASKS: {tasks}".format(
                    v=data.get("V", "?"),
                    host=data.get("HOST", "?"),
                    sd="READY" if data.get("SD") == "1" else "NO",
                    app="RUNNING" if data.get("APP") == "1" else "IDLE",
                    ram=data.get("RAM", "?"),
                    tasks=data.get("TASKS", "?"),
                )
            )

        self._worker(op, done)

    def show_tasks(self) -> None:
        self._worker(self.client.tasks)

    def show_app_status(self) -> None:
        self._worker(self.client.app_status)

    def refresh_files(self) -> None:
        path = self.current_path.get().strip() or "/"

        def done(items):
            self.tree.delete(*self.tree.get_children())
            for item in items:
                self.tree.insert(
                    "", "end", values=(item["type"], item["size"], item["name"])
                )

        self._worker(lambda: self.client.ls(path), done)

    def _remote_for_name(self, name: str) -> str:
        base = self.current_path.get().strip() or "/"
        if base == "/":
            return "/" + name
        return base.rstrip("/") + "/" + name

    def _selected(self) -> tuple[str, str] | None:
        selected = self.tree.selection()
        if not selected:
            messagebox.showinfo(
                "KonSol Host Manager", "Select a file or directory first"
            )
            return None
        values = self.tree.item(selected[0], "values")
        return str(values[0]), str(values[2])

    def _tree_double_click(self, _event) -> None:
        selected = self._selected()
        if selected and selected[0] == "DIR":
            self.current_path.set(self._remote_for_name(selected[1]))
            self.refresh_files()

    def go_up(self) -> None:
        path = pathlib.PurePosixPath(self.current_path.get().strip() or "/")
        parent = str(path.parent)
        if parent == ".":
            parent = "/"
        self.current_path.set(parent)
        self.refresh_files()

    def install_kap(self) -> None:
        local_name = filedialog.askopenfilename(
            title="Select KAP application",
            filetypes=[
                ("KonSol application", "*.KAP"),
                ("All files", "*.*"),
            ],
        )
        if not local_name:
            return
        local = pathlib.Path(local_name)
        default_remote = self._remote_for_name(local.name.upper())
        remote = simpledialog.askstring(
            "Install KAP",
            "Destination on KonSol microSD:",
            initialvalue=default_remote,
        )
        if not remote:
            return

        self.progress_var.set(0.0)

        def progress(done, total):
            self.events.put(("progress", done, total))

        def op():
            return self.client.put_file(local, remote, progress)

        def done(result):
            self._finish_progress()
            self._append_log(
                "INSTALL PASS: {remote}  size={size}  CRC={crc}  SHA256={sha256}".format(
                    **result
                )
            )
            self.refresh_files()

        self._worker(op, done)

    def download_selected(self) -> None:
        selected = self._selected()
        if not selected or selected[0] != "FILE":
            return
        remote = self._remote_for_name(selected[1])
        local_name = filedialog.asksaveasfilename(
            title="Save KonSol file",
            initialfile=selected[1],
            filetypes=[("All files", "*.*")],
        )
        if not local_name:
            return
        local = pathlib.Path(local_name)
        self.progress_var.set(0.0)

        def progress(done, total):
            self.events.put(("progress", done, total))

        def done(result):
            self._finish_progress()
            self._append_log(
                "DOWNLOAD PASS: {local}  size={size}  CRC={crc}  SHA256={sha256}".format(
                    **result
                )
            )

        self._worker(
            lambda: self.client.get_file(remote, local, progress),
            done,
        )

    def run_selected(self) -> None:
        selected = self._selected()
        if not selected or selected[0] != "FILE":
            return
        remote = self._remote_for_name(selected[1])
        if not remote.upper().endswith(".KAP"):
            messagebox.showwarning(
                "KonSol Host Manager",
                "Selected file is not a .KAP application",
            )
            return
        self._worker(
            lambda: self.client.run(remote),
            lambda _: self.refresh_status(),
        )

    def stop_app(self) -> None:
        self._worker(
            self.client.stop,
            lambda _: self.refresh_status(),
        )

    def delete_selected(self) -> None:
        selected = self._selected()
        if not selected or selected[0] != "FILE":
            return
        remote = self._remote_for_name(selected[1])
        if not messagebox.askyesno(
            "Delete", f"Delete {remote} from KonSol microSD?"
        ):
            return
        self._worker(
            lambda: self.client.delete(remote),
            lambda _: self.refresh_files(),
        )

    def build_kasm(self) -> None:
        source_name = filedialog.askopenfilename(
            title="Select KASM source",
            filetypes=[
                ("KonSol assembly", "*.kasm"),
                ("All files", "*.*"),
            ],
        )
        if not source_name:
            return
        source = pathlib.Path(source_name)
        output_name = filedialog.asksaveasfilename(
            title="Write KAP application",
            initialfile=source.with_suffix(".KAP").name,
            defaultextension=".KAP",
            filetypes=[("KonSol application", "*.KAP")],
        )
        if not output_name:
            return

        repo_root = pathlib.Path(__file__).resolve().parents[2]
        assembler = repo_root / "tools" / "kasm" / "kasm.py"
        output = pathlib.Path(output_name)

        def op():
            proc = subprocess.run(
                [
                    sys.executable,
                    str(assembler),
                    str(source),
                    "-o",
                    str(output),
                ],
                text=True,
                capture_output=True,
                check=False,
            )
            if proc.stdout:
                for line in proc.stdout.splitlines():
                    self._log_from_worker(line)
            if proc.stderr:
                for line in proc.stderr.splitlines():
                    self._log_from_worker(line)
            if proc.returncode != 0:
                raise Host1Error(
                    f"KASM failed with exit code {proc.returncode}"
                )
            return output

        def done(result):
            messagebox.showinfo("KASM", f"KAP created:\n{result}")

        self._worker(op, done)

    def _on_close(self) -> None:
        try:
            self.client.disconnect()
        finally:
            self.destroy()


def main() -> int:
    app = HostManager()
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
