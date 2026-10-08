import argparse
import ctypes
import msvcrt
import sys
import threading
import time

import serial


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

    enable_virtual_terminal_processing = 0x0004

    kernel32.SetConsoleMode(
        stdout_handle,
        mode.value | enable_virtual_terminal_processing,
    )


def reader_loop(ser, stop_event):
    while not stop_event.is_set():
        try:
            data = ser.read(256)
        except serial.SerialException as exc:
            print(f"\n[SERIAL ERROR] {exc}")
            stop_event.set()
            break

        if data:
            sys.stdout.write(
                data.decode("utf-8", errors="replace")
            )
            sys.stdout.flush()


def send_special(ser, prefix, code):
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
            ser.write(ANSI[name])
            return True

    if prefix == "\x00":
        function_keys = {
            "C": "F9",
            "D": "F10",
        }

        name = function_keys.get(code)

        if name:
            ser.write(ANSI[name])
            return True

    return False


def main():
    parser = argparse.ArgumentParser(
        description="KSC Raw TTY"
    )

    parser.add_argument(
        "-p",
        "--port",
        default="COM4"
    )

    parser.add_argument(
        "-b",
        "--baud",
        type=int,
        default=115200
    )

    args = parser.parse_args()

    enable_windows_vt()

    ser = serial.Serial()

    ser.port = args.port
    ser.baudrate = args.baud
    ser.timeout = 0.05
    ser.write_timeout = 1
    ser.dtr = False
    ser.rts = False

    ser.open()

    stop_event = threading.Event()

    reader = threading.Thread(
        target=reader_loop,
        args=(ser, stop_event),
        daemon=True
    )

    reader.start()

    print()
    print("KSC RAW TTY 0.3")
    print(f"PORT={args.port} BAUD={args.baud}")
    print()
    print("ANSI screen mode enabled")
    print("Printable keys -> CHAR / shell text")
    print("Arrows         -> navigation")
    print("Enter          -> ENTER / open / apply")
    print("Backspace/Esc  -> BACK")
    print("Home           -> root")
    print("End            -> last item")
    print("Delete         -> cancel numeric edit")
    print("F9             -> MENU / help")
    print("F10            -> POWER / exit to shell")
    print("Q in Commander -> exit to shell")
    print("Ctrl-C         -> close Raw TTY")
    print()

    try:
        while not stop_event.is_set():
            ch = msvcrt.getwch()

            if ch == "\x03":
                break

            if ch in ("\x00", "\xe0"):
                ext = msvcrt.getwch()

                send_special(
                    ser,
                    ch,
                    ext
                )

                continue

            if ch == "\r":
                ser.write(b"\r")
                continue

            if ch == "\x08":
                ser.write(b"\x08")
                continue

            if ch == "\x1b":
                ser.write(b"\x1b")
                continue

            try:
                encoded = ch.encode("ascii")
            except UnicodeEncodeError:
                continue

            ser.write(encoded)

    except KeyboardInterrupt:
        pass

    finally:
        stop_event.set()

        time.sleep(0.1)

        try:
            ser.close()
        finally:
            sys.stdout.write("\x1b[0m\x1b[?25h")
            sys.stdout.flush()

        print()
        print("KSC RAW TTY CLOSED")


if __name__ == "__main__":
    main()
