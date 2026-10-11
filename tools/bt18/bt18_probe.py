#!/usr/bin/env python3
"""Non-destructive UART AT fingerprint for DX-BT18 / BT18-LE-like modules.

This tool sends only documented identification/read commands. It cannot
identify the silicon manufacturer conclusively, and it never flashes firmware.
"""
from __future__ import annotations

import argparse
import json
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("Missing pyserial. Install: py -m pip install -r tools/bt18/requirements.txt", file=sys.stderr)
    sys.exit(2)

DUAL_READ = (
    "AT+TEST", "AT+VERS?", "AT+ADSS?", "AT+NAME?",
    "AT+BAUD?", "AT+PIN?", "AT+UUID?", "AT+FLOW?",
)
LE_READ = (
    "AT", "AT+VER", "AT+ADDR", "AT+NAME", "AT+BAUD",
    "AT+SERV", "AT+NOTI", "AT+WRIT", "AT+WAIT",
)
ENDINGS = {"crlf": b"\r\n", "cr": b"\r", "lf": b"\n"}


def receive(port: serial.Serial, seconds: float) -> bytes:
    chunks: list[bytes] = []
    until = time.monotonic() + seconds
    while time.monotonic() < until:
        part = port.read(512)
        if part:
            chunks.append(part)
        else:
            time.sleep(0.02)
    return b"".join(chunks)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list-ports", action="store_true", help="list available UART devices")
    parser.add_argument("--port", help="serial port, e.g. COM4")
    parser.add_argument("--baud", type=int, default=9600, help="UART speed; default: 9600")
    parser.add_argument("--profile", choices=["dual", "le", "both"], default="dual")
    parser.add_argument("--ending", choices=tuple(ENDINGS), default="crlf")
    parser.add_argument("--timeout", type=float, default=0.65, help="wait after EACH command")
    parser.add_argument("--json", type=Path, help="optional local evidence output; may contain MAC")
    args = parser.parse_args()

    if args.list_ports:
        ports = sorted(list_ports.comports(), key=lambda port: port.device)
        if not ports:
            print("No serial ports detected.")
        for port in ports:
            print(f"{port.device:10} {port.description}")
        return 0

    if not args.port:
        parser.error("--port COMx is required (or use --list-ports)")
    if args.baud <= 0 or not 0.05 <= args.timeout <= 10:
        parser.error("invalid --baud or --timeout")

    if args.profile == "dual":
        commands = DUAL_READ
    elif args.profile == "le":
        commands = LE_READ
    else:
        commands = tuple(dict.fromkeys(DUAL_READ + LE_READ))

    record: dict = {
        "tool": "KON Bluetooth Lab BT18-01",
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "port": args.port,
        "baud": args.baud,
        "profile": args.profile,
        "ending": args.ending,
        "results": [],
    }

    print(f"Port={args.port}  Baud={args.baud}  Profile={args.profile}")
    print("Read-only AT queries. No configuration writes or firmware operations.")
    try:
        with serial.Serial(args.port, args.baud, timeout=0.05, write_timeout=1) as device:
            time.sleep(0.2)
            for command in commands:
                device.reset_input_buffer()
                device.write(command.encode("ascii") + ENDINGS[args.ending])
                device.flush()
                raw = receive(device, args.timeout)
                rendered = raw.decode("utf-8", errors="replace").replace("\r", "\\r").replace("\n", "\\n")
                print(f"{command:<14} {rendered if raw else '(no response)'}")
                record["results"].append({
                    "command": command,
                    "response_text": raw.decode("utf-8", errors="replace"),
                    "response_hex": raw.hex(),
                })
    except (serial.SerialException, OSError) as error:
        print(f"UART error: {error}", file=sys.stderr)
        return 2

    replies = sum(bool(r["response_hex"]) for r in record["results"])
    print(f"Replies: {replies}/{len(commands)}. Identity is NOT certified by AT compatibility alone.")

    if args.json:
        try:
            args.json.parent.mkdir(parents=True, exist_ok=True)
            args.json.write_text(json.dumps(record, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        except OSError as error:
            print(f"Cannot write JSON: {error}", file=sys.stderr)
            return 2
        print(f"Saved local diagnostic: {args.json} (may contain Bluetooth MAC)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
