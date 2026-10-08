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
            sys.stdout.write(data.decode("utf-8", errors="replace"))
            sys.stdout.flush()


def main():
    parser = argparse.ArgumentParser(description="KSC Raw TTY")
    parser.add_argument("-p", "--port", default="COM4")
    parser.add_argument("-b", "--baud", type=int, default=115200)
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
        daemon=True,
    )
    reader.start()

    print()
    print("KSC RAW TTY")
    print(f"PORT={args.port} BAUD={args.baud}")
    print()
    print("Arrow keys -> ANSI arrows")
    print("Home       -> ANSI Home")
    print("Backspace  -> BACK")
    print("Esc        -> BACK after KSC timeout")
    print("Enter      -> ENTER")
    print("M/P/0..9   -> ordinary KSC keys")
    print("Ctrl-C     -> exit")
    print()

    try:
        while not stop_event.is_set():
            ch = msvcrt.getwch()

            if ch == "\x03":
                break

            if ch in ("\x00", "\xe0"):
                ext = msvcrt.getwch()

                mapping = {
                    "H": "UP",
                    "P": "DOWN",
                    "K": "LEFT",
                    "M": "RIGHT",
                    "G": "HOME",
                }

                name = mapping.get(ext)

                if name:
                    ser.write(ANSI[name])

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
                ser.write(ch.encode("ascii"))
            except UnicodeEncodeError:
                pass

    except KeyboardInterrupt:
        pass
    finally:
        stop_event.set()
        time.sleep(0.1)
        ser.close()
        print("\nKSC RAW TTY CLOSED")


if __name__ == "__main__":
    main()
