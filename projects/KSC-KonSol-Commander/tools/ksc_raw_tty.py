import argparse
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
    print("KSC RAW TTY 0.2")
    print(f"PORT={args.port} BAUD={args.baud}")
    print()
    print("Printable keys -> CHAR")
    print("Arrows         -> KEY")
    print("Enter          -> ENTER")
    print("Backspace/Esc  -> BACK")
    print("Home           -> HOME")
    print("F9             -> MENU")
    print("F10            -> POWER")
    print("Ctrl-C         -> exit")
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

        ser.close()

        print()
        print("KSC RAW TTY CLOSED")


if __name__ == "__main__":
    main()
