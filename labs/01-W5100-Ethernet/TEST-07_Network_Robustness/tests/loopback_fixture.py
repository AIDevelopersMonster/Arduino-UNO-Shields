"""Host transport fixture only. Never a replacement for UNO/W5100 evidence."""
import json
import socket
import sys
import threading
from pathlib import Path

udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
udp.bind(('127.0.0.1', 0))
wrong_source = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
wrong_source.bind(('127.0.0.1', 0))
tcp = socket.socket()
tcp.bind(('127.0.0.1', 0))
tcp.listen()


def udp_loop():
    while True:
        data, addr = udp.recvfrom(2048)
        mode = data[0]
        if mode == 4:
            continue
        response = data
        if mode == 1:
            response = data[:-1] + bytes([data[-1] ^ 1])
        if mode == 2:
            response = data[:-1]
        (wrong_source if mode == 3 else udp).sendto(response, addr)


def tcp_loop():
    while True:
        c, _ = tcp.accept()
        with c:
            c.settimeout(1)
            request = bytearray()
            try:
                while len(request) < 128:
                    ch = c.recv(1)
                    if not ch:
                        break
                    request += ch
                    if ch == b'\n':
                        reply = b'ECHO ' + request
                        if request.startswith(b'BAD'):
                            reply = b'BAD ' + request
                        c.sendall(reply)
                        break
            except (TimeoutError, ConnectionResetError):
                pass


threading.Thread(target=udp_loop, daemon=True).start()
threading.Thread(target=tcp_loop, daemon=True).start()
Path(sys.argv[1]).write_text(json.dumps({'udp': udp.getsockname()[1], 'tcp': tcp.getsockname()[1]}))
threading.Event().wait()
