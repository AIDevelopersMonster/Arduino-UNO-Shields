import argparse
import ctypes
import msvcrt
from pathlib import Path
import sys
import threading
import time

import serial


SOF1 = 0x1B
SOF2 = 0x5D
MAX_PAYLOAD = 48
REMOTE_NAME_MAX = 15

TYPE_PING_REQ = 0x01
TYPE_MOUNT_REQ = 0x02
TYPE_LS_REQ = 0x03
TYPE_STAT_REQ = 0x04

TYPE_PING_RESP = 0x81
TYPE_MOUNT_RESP = 0x82
TYPE_LS_RESP = 0x83
TYPE_STAT_RESP = 0x84

TYPE_ERROR_RESP = 0x7F

ERR_BAD_LENGTH = 0x01
ERR_BAD_CRC = 0x02
ERR_TIMEOUT = 0x03
ERR_UNSUPPORTED = 0x04
ERR_NOT_FOUND = 0x10
ERR_NOT_DIR = 0x11
ERR_PATH = 0x12
ERR_HOST_IO = 0x13

ERR_NAMES = {
    ERR_BAD_LENGTH: "BAD_LENGTH",
    ERR_BAD_CRC: "BAD_CRC",
    ERR_TIMEOUT: "TIMEOUT",
    ERR_UNSUPPORTED: "UNSUPPORTED",
    ERR_NOT_FOUND: "NOT_FOUND",
    ERR_NOT_DIR: "NOT_DIR",
    ERR_PATH: "PATH",
    ERR_HOST_IO: "HOST_IO",
}

HOST_TYPE_NONE = 0
HOST_TYPE_DIR = 1
HOST_TYPE_FILE = 2

ANSI = {
    "UP": b"\x1b[A",
    "DOWN": b"\x1b[B",
    "RIGHT": b"\x1b[C",
    "LEFT": b"\x1b[D",
    "HOME": b"\x1b[H",
    "END": b"\x1b[F",
    "DELETE": b"\x1b[3~",
    "F9": b"\x1b[20~",
    "F10": b"\x1b[21~",
}


def crc8(data):
    crc = 0

    for value in data:
        crc ^= value

        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ 0x07) & 0xFF
            else:
                crc = (crc << 1) & 0xFF

    return crc


def build_frame(frame_type, seq, payload=b""):
    if len(payload) > MAX_PAYLOAD:
        raise ValueError("payload too large")

    body = bytes(
        [frame_type, seq, len(payload)]
    ) + payload

    return bytes([SOF1, SOF2]) + body + bytes(
        [crc8(body)]
    )


def enable_windows_vt():
    if sys.platform != "win32":
        return

    kernel32 = ctypes.windll.kernel32
    stdout_handle = kernel32.GetStdHandle(-11)
    mode = ctypes.c_uint32()

    if not kernel32.GetConsoleMode(
        stdout_handle,
        ctypes.byref(mode),
    ):
        return

    kernel32.SetConsoleMode(
        stdout_handle,
        mode.value | 0x0004,
    )


class WireDemux:
    IDLE = 0
    SOF1_SEEN = 1
    TYPE = 2
    SEQ = 3
    LEN = 4
    PAYLOAD = 5
    CRC = 6

    def __init__(self, on_tty, on_frame):
        self.on_tty = on_tty
        self.on_frame = on_frame
        self.state = self.IDLE
        self.frame_type = 0
        self.seq = 0
        self.length = 0
        self.payload = bytearray()
        self.body = bytearray()

    def reset(self):
        self.state = self.IDLE
        self.frame_type = 0
        self.seq = 0
        self.length = 0
        self.payload.clear()
        self.body.clear()

    def feed(self, data):
        for value in data:
            self.feed_byte(value)

    def feed_byte(self, value):
        if self.state == self.IDLE:
            if value == SOF1:
                self.state = self.SOF1_SEEN
            else:
                self.on_tty(bytes([value]))
            return

        if self.state == self.SOF1_SEEN:
            if value == SOF2:
                self.state = self.TYPE
                self.body.clear()
                return

            self.on_tty(bytes([SOF1]))

            if value == SOF1:
                self.state = self.SOF1_SEEN
            else:
                self.on_tty(bytes([value]))
                self.reset()
            return

        if self.state == self.TYPE:
            self.frame_type = value
            self.body.append(value)
            self.state = self.SEQ
            return

        if self.state == self.SEQ:
            self.seq = value
            self.body.append(value)
            self.state = self.LEN
            return

        if self.state == self.LEN:
            self.length = value
            self.body.append(value)
            self.payload.clear()

            if self.length > MAX_PAYLOAD:
                self.reset()
                return

            self.state = (
                self.CRC
                if self.length == 0
                else self.PAYLOAD
            )
            return

        if self.state == self.PAYLOAD:
            self.payload.append(value)
            self.body.append(value)

            if len(self.payload) >= self.length:
                self.state = self.CRC
            return

        if self.state == self.CRC:
            expected = crc8(self.body)

            if value == expected:
                self.on_frame(
                    self.frame_type,
                    self.seq,
                    bytes(self.payload),
                )

            self.reset()


class KscHost:
    def __init__(
        self,
        ser,
        export_root,
        show_frames=True,
    ):
        self.ser = ser
        self.export_root = Path(export_root).resolve()
        self.show_frames = show_frames
        self.stop_event = threading.Event()
        self.write_lock = threading.Lock()
        self.seq = 0
        self.ping_waiters = {}
        self.waiter_lock = threading.Lock()

        self.demux = WireDemux(
            self._on_tty,
            self._on_frame,
        )

    def _on_tty(self, data):
        sys.stdout.write(
            data.decode("utf-8", errors="replace")
        )
        sys.stdout.flush()

    def _frame_line(self, text):
        if not self.show_frames:
            return

        sys.stdout.write(
            f"\r\n[HOSTFS] {text}\r\n"
        )
        sys.stdout.flush()

    def _send_frame(self, frame_type, seq, payload=b""):
        self.send_bytes(
            build_frame(
                frame_type,
                seq,
                payload,
            )
        )

    def _send_error(self, seq, error_code):
        self._send_frame(
            TYPE_ERROR_RESP,
            seq,
            bytes([error_code]),
        )

    def _decode_ksc_path(self, payload):
        try:
            text = payload.decode("ascii")
        except UnicodeDecodeError:
            raise ValueError("non-ASCII path")

        if text == "/host":
            parts = []
        elif text.startswith("/host/"):
            tail = text[6:]

            if not tail:
                parts = []
            else:
                parts = tail.split("/")
        else:
            raise ValueError("path outside /host")

        for part in parts:
            if (
                not part
                or part in (".", "..")
                or ":" in part
                or "\\" in part
            ):
                raise ValueError("unsafe path")

        candidate = self.export_root.joinpath(
            *parts
        ).resolve()

        try:
            candidate.relative_to(
                self.export_root
            )
        except ValueError as exc:
            raise ValueError(
                "path escapes export root"
            ) from exc

        return candidate

    def _entry_type(self, path):
        if path.is_dir():
            return HOST_TYPE_DIR

        if path.is_file():
            return HOST_TYPE_FILE

        return HOST_TYPE_NONE

    def _visible_entries(self, directory):
        entries = []

        for path in directory.iterdir():
            host_type = self._entry_type(path)

            if host_type == HOST_TYPE_NONE:
                continue

            try:
                encoded = path.name.encode("ascii")
            except UnicodeEncodeError:
                continue

            if (
                not encoded
                or len(encoded) > REMOTE_NAME_MAX
            ):
                continue

            entries.append(
                (path.name, host_type)
            )

        entries.sort(
            key=lambda item: (
                0 if item[1] == HOST_TYPE_DIR else 1,
                item[0].casefold(),
            )
        )

        return entries[:255]

    def _handle_mount_req(self, seq, payload):
        if payload:
            self._send_error(
                seq,
                ERR_BAD_LENGTH,
            )
            return

        if not self.export_root.is_dir():
            self._send_error(
                seq,
                ERR_HOST_IO,
            )
            return

        self._frame_line(
            f"MOUNT /host -> {self.export_root}"
        )

        self._send_frame(
            TYPE_MOUNT_RESP,
            seq,
            b"\x01",
        )

    def _handle_stat_req(self, seq, payload):
        if not payload:
            self._send_error(
                seq,
                ERR_BAD_LENGTH,
            )
            return

        try:
            path = self._decode_ksc_path(
                payload
            )
        except ValueError:
            self._send_error(
                seq,
                ERR_PATH,
            )
            return

        if not path.exists():
            self._send_error(
                seq,
                ERR_NOT_FOUND,
            )
            return

        host_type = self._entry_type(path)

        if host_type == HOST_TYPE_NONE:
            self._send_error(
                seq,
                ERR_NOT_FOUND,
            )
            return

        self._send_frame(
            TYPE_STAT_RESP,
            seq,
            bytes([host_type]),
        )

    def _handle_ls_req(self, seq, payload):
        if len(payload) < 2:
            self._send_error(
                seq,
                ERR_BAD_LENGTH,
            )
            return

        index = payload[0]

        try:
            directory = self._decode_ksc_path(
                payload[1:]
            )
        except ValueError:
            self._send_error(
                seq,
                ERR_PATH,
            )
            return

        if not directory.exists():
            self._send_error(
                seq,
                ERR_NOT_FOUND,
            )
            return

        if not directory.is_dir():
            self._send_error(
                seq,
                ERR_NOT_DIR,
            )
            return

        try:
            entries = self._visible_entries(
                directory
            )
        except OSError:
            self._send_error(
                seq,
                ERR_HOST_IO,
            )
            return

        count = len(entries)

        if index == 0xFF:
            self._send_frame(
                TYPE_LS_RESP,
                seq,
                bytes([count]),
            )
            return

        if index >= count:
            self._send_error(
                seq,
                ERR_NOT_FOUND,
            )
            return

        name, host_type = entries[index]
        encoded = name.encode("ascii")

        response = bytes(
            [
                count,
                host_type,
                len(encoded),
            ]
        ) + encoded

        self._send_frame(
            TYPE_LS_RESP,
            seq,
            response,
        )

    def _on_frame(self, frame_type, seq, payload):
        if frame_type == TYPE_PING_REQ:
            if payload:
                self._send_error(
                    seq,
                    ERR_BAD_LENGTH,
                )
            else:
                self._send_frame(
                    TYPE_PING_RESP,
                    seq,
                )
            return

        if frame_type == TYPE_MOUNT_REQ:
            self._handle_mount_req(
                seq,
                payload,
            )
            return

        if frame_type == TYPE_STAT_REQ:
            self._handle_stat_req(
                seq,
                payload,
            )
            return

        if frame_type == TYPE_LS_REQ:
            self._handle_ls_req(
                seq,
                payload,
            )
            return

        if frame_type == TYPE_PING_RESP:
            self._frame_line(
                f"PING_RESP seq={seq} PASS"
            )

            with self.waiter_lock:
                event = self.ping_waiters.get(seq)

            if event:
                event.set()

            return

        if frame_type == TYPE_ERROR_RESP:
            code = payload[0] if payload else None
            name = ERR_NAMES.get(
                code,
                f"0x{code:02X}"
                if code is not None
                else "EMPTY",
            )

            self._frame_line(
                f"ERROR_RESP seq={seq} error={name}"
            )
            return

        self._frame_line(
            f"TYPE=0x{frame_type:02X} seq={seq} len={len(payload)}"
        )

    def reader_loop(self):
        while not self.stop_event.is_set():
            try:
                data = self.ser.read(256)
            except serial.SerialException as exc:
                self._frame_line(
                    f"SERIAL ERROR: {exc}"
                )
                self.stop_event.set()
                break

            if data:
                self.demux.feed(data)

    def next_seq(self):
        self.seq = (self.seq + 1) & 0xFF

        if self.seq == 0:
            self.seq = 1

        return self.seq

    def send_bytes(self, data):
        with self.write_lock:
            self.ser.write(data)
            self.ser.flush()

    def send_ping(self, timeout=1.0):
        seq = self.next_seq()
        event = threading.Event()

        with self.waiter_lock:
            self.ping_waiters[seq] = event

        self.send_bytes(
            build_frame(TYPE_PING_REQ, seq)
        )

        ok = event.wait(timeout)

        with self.waiter_lock:
            self.ping_waiters.pop(seq, None)

        if ok:
            self._frame_line(
                f"PING seq={seq} round-trip PASS"
            )
        else:
            self._frame_line(
                f"PING seq={seq} TIMEOUT"
            )

        return ok

    def send_bad_crc(self):
        seq = self.next_seq()
        frame = bytearray(
            build_frame(TYPE_PING_REQ, seq)
        )

        frame[-1] ^= 0xFF

        self._frame_line(
            f"sending bad-CRC frame seq={seq}"
        )

        self.send_bytes(frame)

    def send_partial_timeout(self):
        seq = self.next_seq()

        partial = bytes(
            [
                SOF1,
                SOF2,
                TYPE_PING_REQ,
                seq,
                0,
            ]
        )

        self._frame_line(
            f"sending partial frame seq={seq}; waiting for firmware timeout"
        )

        self.send_bytes(partial)

    def send_unsupported(self):
        seq = self.next_seq()

        self._frame_line(
            f"sending unsupported frame seq={seq}"
        )

        self.send_bytes(
            build_frame(0x55, seq)
        )


def send_special(host, prefix, code):
    if prefix == "\xe0":
        mapping = {
            "H": "UP",
            "P": "DOWN",
            "K": "LEFT",
            "M": "RIGHT",
            "G": "HOME",
            "O": "END",
            "S": "DELETE",
        }

        name = mapping.get(code)

        if name:
            host.send_bytes(ANSI[name])
            return True

    if prefix == "\x00":
        function_keys = {
            "C": "F9",
            "D": "F10",
        }

        name = function_keys.get(code)

        if name:
            host.send_bytes(ANSI[name])
            return True

    return False


def default_export_root():
    return (
        Path(__file__).resolve().parent.parent
        / "host-share"
    )


def main():
    parser = argparse.ArgumentParser(
        description="KSC Host 0.2 - TTY + remote /host service"
    )

    parser.add_argument(
        "-p",
        "--port",
        default="COM4",
    )

    parser.add_argument(
        "-b",
        "--baud",
        type=int,
        default=115200,
    )

    parser.add_argument(
        "--export",
        default=str(default_export_root()),
        help="PC directory exported as /host",
    )

    parser.add_argument(
        "--ping-on-start",
        action="store_true",
    )

    parser.add_argument(
        "--quiet-frames",
        action="store_true",
    )

    args = parser.parse_args()

    export_root = Path(
        args.export
    ).resolve()

    if not export_root.is_dir():
        raise SystemExit(
            f"Export directory does not exist: {export_root}"
        )

    enable_windows_vt()

    ser = serial.Serial()
    ser.port = args.port
    ser.baudrate = args.baud
    ser.timeout = 0.05
    ser.write_timeout = 1
    ser.dtr = False
    ser.rts = False
    ser.open()

    host = KscHost(
        ser,
        export_root=export_root,
        show_frames=not args.quiet_frames,
    )

    reader = threading.Thread(
        target=host.reader_loop,
        daemon=True,
    )

    reader.start()

    print()
    print("KSC HOST 0.2 - KSC-03B")
    print(f"PORT={args.port} BAUD={args.baud}")
    print(f"EXPORT={export_root}")
    print("MOUNT=/host")
    print()
    print("TTY:")
    print("  printable keys -> KSC terminal")
    print("  arrows/home/end/delete/F9/F10 -> KSC ANSI keys")
    print()
    print("HOSTFS:")
    print("  /host -> exported PC directory")
    print("  MOUNT / STAT / LS enabled")
    print("  file content streaming remains KSC-03C")
    print()
    print("Transport test hotkeys:")
    print("  Ctrl-P -> PING")
    print("  Ctrl-B -> bad CRC")
    print("  Ctrl-T -> partial frame / TIMEOUT")
    print("  Ctrl-U -> unsupported frame")
    print("  Ctrl-C -> close host")
    print()

    if args.ping_on_start:
        time.sleep(0.25)
        host.send_ping()

    try:
        while not host.stop_event.is_set():
            ch = msvcrt.getwch()

            if ch == "\x03":
                break

            if ch == "\x10":
                host.send_ping()
                continue

            if ch == "\x02":
                host.send_bad_crc()
                continue

            if ch == "\x14":
                host.send_partial_timeout()
                continue

            if ch == "\x15":
                host.send_unsupported()
                continue

            if ch in ("\x00", "\xe0"):
                ext = msvcrt.getwch()

                send_special(
                    host,
                    ch,
                    ext,
                )
                continue

            if ch == "\r":
                host.send_bytes(b"\r")
                continue

            if ch == "\x08":
                host.send_bytes(b"\x08")
                continue

            if ch == "\x1b":
                host.send_bytes(b"\x1b")
                continue

            try:
                encoded = ch.encode("ascii")
            except UnicodeEncodeError:
                continue

            host.send_bytes(encoded)

    except KeyboardInterrupt:
        pass

    finally:
        host.stop_event.set()
        time.sleep(0.1)

        try:
            ser.close()
        finally:
            sys.stdout.write(
                "\x1b[0m\x1b[?25h"
            )
            sys.stdout.flush()

        print()
        print("KSC HOST CLOSED")


if __name__ == "__main__":
    main()
