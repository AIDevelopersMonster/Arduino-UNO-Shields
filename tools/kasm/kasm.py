#!/usr/bin/env python3
"""KonSol KASM assembler.

Converts human-readable KAP1/KAP2 assembly into the ASCII-hex .KAP format used
by KonSol 0.4/0.5. No third-party dependencies.
"""

from __future__ import annotations

import argparse
import pathlib
import shlex
import sys

COLORS = {
    "BLACK": 0,
    "WHITE": 1,
    "CYAN": 2,
    "YELLOW": 3,
    "GREEN": 4,
    "RED": 5,
    "BLUE": 6,
    "GREY": 7,
    "GRAY": 7,
}

MAX_LABELS = 8


class AsmError(ValueError):
    pass


def u8(value: int, what: str) -> int:
    if not 0 <= value <= 0xFF:
        raise AsmError(f"{what} must be 0..255")
    return value


def u16(value: int, what: str) -> int:
    if not 0 <= value <= 0xFFFF:
        raise AsmError(f"{what} must be 0..65535")
    return value


def parse_int(token: str, what: str) -> int:
    try:
        return int(token, 0)
    except ValueError as exc:
        raise AsmError(f"invalid {what}: {token}") from exc


def parse_reg(token: str) -> int:
    token = token.upper()
    if len(token) != 2 or token[0] != "R" or token[1] not in "0123":
        raise AsmError("register must be R0..R3")
    return int(token[1])


def parse_label_name(token: str) -> str:
    name = token.upper()
    if not name:
        raise AsmError("empty label name")

    first = name[0]
    if not (first == "_" or "A" <= first <= "Z"):
        raise AsmError("label must start with A-Z or _")

    for ch in name[1:]:
        if not (ch == "_" or "A" <= ch <= "Z" or "0" <= ch <= "9"):
            raise AsmError("label may contain only A-Z, 0-9 and _")

    return name


def parse_color(token: str) -> int:
    token_upper = token.upper()
    if token_upper in COLORS:
        return COLORS[token_upper]
    value = parse_int(token, "color")
    if not 0 <= value <= 7:
        raise AsmError("color must be 0..7 or a named KAP color")
    return value


def ascii_bytes(text: str, what: str) -> bytes:
    try:
        raw = text.encode("ascii")
    except UnicodeEncodeError as exc:
        raise AsmError(f"{what} must contain ASCII characters only") from exc
    if len(raw) > 255:
        raise AsmError(f"{what} is limited to 255 bytes")
    return raw


def hx(data: bytes) -> str:
    return data.hex().upper()


def encode_text(x_px: int, y: int, scale: int, color: int, text: str) -> bytes:
    if x_px % 2:
        raise AsmError("TEXT X must be even because KAP stores X/2")
    if not 0 <= x_px <= 318:
        raise AsmError("TEXT X must be 0..318")
    if not 0 <= y <= 239:
        raise AsmError("TEXT Y must be 0..239")
    if not 1 <= scale <= 4:
        raise AsmError("TEXT scale must be 1..4")
    raw = ascii_bytes(text, "TEXT string")
    return bytes([0x11, x_px // 2, y, scale, color, len(raw)]) + raw


def encode_draw_reg(x_px: int, y: int, scale: int, color: int, reg: int) -> bytes:
    if x_px % 2:
        raise AsmError("DRAW_REG X must be even because KAP stores X/2")
    if not 0 <= x_px <= 318:
        raise AsmError("DRAW_REG X must be 0..318")
    if not 0 <= y <= 239:
        raise AsmError("DRAW_REG Y must be 0..239")
    if not 1 <= scale <= 4:
        raise AsmError("DRAW_REG scale must be 1..4")
    return bytes([0x48, x_px // 2, y, scale, color, reg])


def assemble(source: str) -> tuple[int, list[str]]:
    statements: list[tuple[int, list[str]]] = []

    for lineno, original in enumerate(source.splitlines(), 1):
        line = original.strip()
        if not line or line.startswith("#") or line.startswith(";"):
            continue

        try:
            parts = shlex.split(line, comments=True, posix=True)
        except ValueError as exc:
            raise AsmError(f"line {lineno}: {exc}") from exc

        if parts:
            statements.append((lineno, parts))

    label_ids: dict[str, int] = {}

    for lineno, parts in statements:
        if parts[0].upper() != "LABEL":
            continue

        args = parts[1:]
        if len(args) != 1:
            raise AsmError(f"line {lineno}: usage: LABEL <name>")

        try:
            name = parse_label_name(args[0])
        except AsmError as exc:
            raise AsmError(f"line {lineno}: {exc}") from exc

        if name in label_ids:
            raise AsmError(f"line {lineno}: duplicate label: {name}")

        if len(label_ids) >= MAX_LABELS:
            raise AsmError(
                f"line {lineno}: KAP2 supports at most {MAX_LABELS} indexed labels"
            )

        label_ids[name] = len(label_ids)

    version = None
    output: list[str] = []
    mark_name = None

    for lineno, parts in statements:
        op = parts[0].upper()
        args = parts[1:]

        try:
            if op in ("KAP1", "KAP2"):
                if version is not None:
                    raise AsmError("format header may appear only once")
                if output:
                    raise AsmError("format header must be first")
                version = 1 if op == "KAP1" else 2
                output.append(hx(b"KAP1" if version == 1 else b"KAP2"))
                continue

            if version is None:
                raise AsmError("first instruction must be KAP1 or KAP2")

            if op == "CLS":
                if len(args) != 1:
                    raise AsmError("usage: CLS <color>")
                output.append(hx(bytes([0x10, parse_color(args[0])])))

            elif op == "TEXT":
                if len(args) != 5:
                    raise AsmError('usage: TEXT <x> <y> <scale> <color> "text"')
                x = parse_int(args[0], "X")
                y = u8(parse_int(args[1], "Y"), "Y")
                scale = u8(parse_int(args[2], "scale"), "scale")
                color = parse_color(args[3])
                output.append(hx(encode_text(x, y, scale, color, args[4])))

            elif op == "WAIT":
                if len(args) != 1:
                    raise AsmError("usage: WAIT <milliseconds>")
                ms = u16(parse_int(args[0], "milliseconds"), "milliseconds")
                output.append(hx(bytes([0x20, ms & 0xFF, ms >> 8])))

            elif op == "WAIT_TOUCH":
                if args:
                    raise AsmError("WAIT_TOUCH takes no arguments")
                output.append("21")

            elif op == "SERIAL":
                if len(args) != 1:
                    raise AsmError('usage: SERIAL "text"')
                raw = ascii_bytes(args[0], "SERIAL string")
                output.append(hx(bytes([0x30, len(raw)]) + raw))

            elif op == "EXIT":
                if args:
                    raise AsmError("EXIT takes no arguments")
                output.append("FF")

            elif op == "MOVI":
                if version != 2:
                    raise AsmError("MOVI requires KAP2")
                if len(args) != 2:
                    raise AsmError("usage: MOVI Rn <value>")
                reg = parse_reg(args[0])
                value = u16(parse_int(args[1], "value"), "value")
                output.append(hx(bytes([0x40, reg, value & 0xFF, value >> 8])))

            elif op in ("INC", "DEC", "GET_TOUCH_X", "GET_TOUCH_Y"):
                if version != 2:
                    raise AsmError(f"{op} requires KAP2")
                if len(args) != 1:
                    raise AsmError(f"usage: {op} Rn")
                reg = parse_reg(args[0])
                opcode = {
                    "INC": 0x41,
                    "DEC": 0x42,
                    "GET_TOUCH_X": 0x46,
                    "GET_TOUCH_Y": 0x47,
                }[op]
                output.append(hx(bytes([opcode, reg])))

            elif op == "CMPI":
                if version != 2:
                    raise AsmError("CMPI requires KAP2")
                if len(args) != 2:
                    raise AsmError("usage: CMPI Rn <value>")
                reg = parse_reg(args[0])
                value = u16(parse_int(args[1], "value"), "value")
                output.append(hx(bytes([0x43, reg, value & 0xFF, value >> 8])))

            elif op == "MARK":
                if version != 2:
                    raise AsmError("MARK requires KAP2")
                if len(args) != 1:
                    raise AsmError("usage: MARK <name>")
                if mark_name is not None:
                    raise AsmError("legacy KAP2 supports only one MARK")

                name = parse_label_name(args[0])
                if name in label_ids:
                    raise AsmError("MARK name conflicts with indexed LABEL")

                mark_name = name
                output.append("44")

            elif op == "LABEL":
                if version != 2:
                    raise AsmError("LABEL requires KAP2")
                if len(args) != 1:
                    raise AsmError("usage: LABEL <name>")

                name = parse_label_name(args[0])
                output.append(hx(bytes([0x4A, label_ids[name]])))

            elif op == "JMP":
                if version != 2:
                    raise AsmError("JMP requires KAP2")
                if len(args) != 1:
                    raise AsmError("usage: JMP <label>")

                name = parse_label_name(args[0])
                if name not in label_ids:
                    raise AsmError(f"unknown indexed label: {name}")

                output.append(hx(bytes([0x4B, label_ids[name]])))

            elif op in ("JNZ", "JZ"):
                if version != 2:
                    raise AsmError(f"{op} requires KAP2")
                if len(args) != 1:
                    raise AsmError(f"usage: {op} <target>")

                name = parse_label_name(args[0])

                if name in label_ids:
                    opcode = 0x4D if op == "JNZ" else 0x4C
                    output.append(hx(bytes([opcode, label_ids[name]])))
                elif mark_name == name:
                    output.append("45" if op == "JNZ" else "49")
                else:
                    raise AsmError(f"unknown branch target: {name}")

            elif op == "DRAW_REG":
                if version != 2:
                    raise AsmError("DRAW_REG requires KAP2")
                if len(args) != 5:
                    raise AsmError(
                        "usage: DRAW_REG <x> <y> <scale> <color> Rn"
                    )
                x = parse_int(args[0], "X")
                y = u8(parse_int(args[1], "Y"), "Y")
                scale = u8(parse_int(args[2], "scale"), "scale")
                color = parse_color(args[3])
                reg = parse_reg(args[4])
                output.append(hx(encode_draw_reg(x, y, scale, color, reg)))

            else:
                raise AsmError(f"unknown instruction: {op}")

        except AsmError as exc:
            raise AsmError(f"line {lineno}: {exc}") from exc

    if version is None:
        raise AsmError("source does not contain KAP1/KAP2 header")
    if not output or output[-1] != "FF":
        raise AsmError("program must end with EXIT")

    return version, output

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Assemble KonSol KAP1/KAP2 source into ASCII-hex .KAP"
    )
    parser.add_argument("source", type=pathlib.Path, help="input .kasm file")
    parser.add_argument(
        "-o", "--output", type=pathlib.Path, required=True, help="output .KAP file"
    )
    args = parser.parse_args()

    try:
        source = args.source.read_text(encoding="utf-8")
        version, lines = assemble(source)
        args.output.write_text("\n".join(lines) + "\n", encoding="ascii")
    except (OSError, AsmError) as exc:
        print(f"kasm: error: {exc}", file=sys.stderr)
        return 1

    print(f"KASM PASS: KAP{version} -> {args.output}")
    print(f"Instructions/records: {len(lines) - 1}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
