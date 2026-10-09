"""Host-only guided-runner checks: file UART adapter + real local UDP/TCP.

No UNO, Windows COM, physical cable or real DHCP is used. Each fixture responds
to actual console instructions, not predetermined operator timestamps.
"""
import argparse
import json
import os
import pathlib
import socket
import subprocess
import tempfile
import threading
import time

FAKE_SERIAL = """
class FakeSerial {
 [string]$Path; [int]$Offset=0; [bool]$IsOpen=$false; [bool]$DtrEnable; [bool]$RtsEnable; [int]$WriteTimeout
 FakeSerial([string]$path){$this.Path=$path}
 [void]Open(){$this.IsOpen=$true;[IO.File]::WriteAllText($this.Path+'.opened','open')}
 [void]Close(){$this.IsOpen=$false}
 [void]Dispose(){}
 [void]Write([string]$value){}
 [string]ReadExisting(){ $txt=[IO.File]::ReadAllText($this.Path); $r=$txt.Substring($this.Offset); $this.Offset=$txt.Length; return $r }
}
"""


def run_case(case, pwsh):
    root = pathlib.Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix="test07-guided-") as folder:
        work = pathlib.Path(folder)
        uart = work / "uart.txt"
        markers = work / "markers.txt"
        uart.write_text("")
        markers.write_text("")
        udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        udp.bind(("127.0.0.1", 0))
        udp.settimeout(0.1)
        tcp = socket.socket()
        tcp.bind(("127.0.0.1", 0))
        tcp.listen()
        tcp.settimeout(0.1)
        stop = threading.Event()
        startup = case == "StartupPass"
        state = {"down": startup, "ready": not startup, "restore": False}
        scenario = "StartupDhcp" if startup else "Cable"
        duration = 90 if case == "NoDisconnect" else (45 if startup else 60)
        source = (root / "Test-NetworkRobustness.ps1").read_text(encoding="utf-8")
        ctor = "[IO.Ports.SerialPort]::new($SerialPort,115200,[IO.Ports.Parity]::None,8,[IO.Ports.StopBits]::One)"
        assert source.count(ctor) == 1
        source = source.replace(ctor, "[FakeSerial]::new($SerialPort)")
        source = source.replace("Set-StrictMode -Version Latest", FAKE_SERIAL + "\nSet-StrictMode -Version Latest", 1)
        for name, port in (("Udp", udp.getsockname()[1]), ("Tcp", tcp.getsockname()[1])):
            old = "Test-" + name + "Probe $state.IP " + ("5001" if name == "Udp" else "5000")
            assert source.count(old) == (1 if name == 'Udp' else 2)
            source = source.replace(old, "Test-" + name + "Probe $state.IP " + str(port))
        (work / "Test-NetworkRobustness.ps1").write_text(source, encoding="utf-8")
        for name in ("NetworkRobustness.psm1", "TEST-07_Network_Robustness.ino"):
            (work / name).write_bytes((root / name).read_bytes())

        def serve_udp():
            while not stop.is_set():
                try:
                    data, peer = udp.recvfrom(2048)
                    if not state["down"]:
                        udp.sendto(data, peer)
                except socket.timeout:
                    pass
                except OSError:
                    break

        def serve_tcp():
            while not stop.is_set():
                try:
                    client, _ = tcp.accept()
                except socket.timeout:
                    continue
                except OSError:
                    break
                with client:
                    client.settimeout(2)
                    data = bytearray()
                    try:
                        while len(data) < 128:
                            byte = client.recv(1)
                            if not byte:
                                break
                            data += byte
                            if byte == b"\n":
                                if not state["down"]:
                                    client.sendall(b"ECHO " + data)
                                break
                    except (TimeoutError, ConnectionError, OSError):
                        pass

        def serve_uart():
            def emit(text):
                with uart.open("a", encoding="utf-8") as stream:
                    stream.write(text + "\n")
            while not stop.is_set() and not pathlib.Path(str(uart) + ".opened").exists():
                stop.wait(0.02)
            started = time.monotonic()
            emit("EVT ms=0 name=BOOT")
            emit("INFO test=07 version=0.1")
            emit("EVT ms=1 name=HARDWARE chip=W5100")
            if startup:
                emit("EVT ms=2 name=DHCP_BEGIN attempt=1")
            else:
                emit("EVT ms=2 name=SERVICES_READY ip=127.0.0.1")
            failed = rebound = False
            next_stat = 1
            while not stop.is_set():
                now = time.monotonic() - started
                if startup and now >= 6 and not failed:
                    emit(f"EVT ms={int(now*1000)} name=DHCP_FAIL elapsed_ms=6000")
                    failed = True
                if startup and state["restore"] and not rebound:
                    state["ready"] = True
                    emit(f"EVT ms={int(now*1000)} name=SERVICES_READY ip=127.0.0.1")
                    rebound = True
                if now >= next_stat:
                    ready = int(state["ready"])
                    ip = "127.0.0.1" if ready else "0.0.0.0"
                    emit(f"STAT ms={int(now*1000)} ready={ready} ip={ip} free=1000 min_free=900 "
                         "attempts=1 dhcp_ok=1 dhcp_fail=0 renew_ok=0 renew_fail=0 rebind_ok=0 "
                         "rebind_fail=0 rx=5 tx=5 drop=0 send_fail=0 tcp_ok=3 tcp_reject=0 restarts=1 max_dhcp_ms=6000")
                    next_stat = now + 2
                stop.wait(0.02)

        threads = [threading.Thread(target=f, daemon=True) for f in (serve_udp, serve_tcp, serve_uart)]
        for thread in threads:
            thread.start()
        environment = os.environ.copy()
        # Official PowerShell opt-out, set before the child starts. Fixture data
        # must remain local; no runtime telemetry is needed for these checks.
        environment['POWERSHELL_TELEMETRY_OPTOUT'] = '1'
        proc = subprocess.Popen([pwsh, "-NoProfile", "-File", str(work / "Test-NetworkRobustness.ps1"),
                                 "-SerialPort", str(uart), "-Scenario", scenario,
                                 "-DurationSeconds", str(duration), "-OutputDirectory", str(work / "runs"),
                                 "-MarkerFile", str(markers)], stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True, encoding="utf-8", env=environment)
        timer = threading.Timer(duration + 20, proc.kill)
        timer.start()
        console = []
        try:
            for text in proc.stdout:
                console.append(text)
                if "После отключения нажмите Enter" in text:
                    markers.write_text("READY\n")
                if "Service verified" in text and case == "HealthyTimeout":
                    state["down"] = True
                if "ШАГ 2: СЕЙЧАС ВЫНЬТЕ" in text and case != "NoDisconnect":
                    state["down"] = True
                if "Отказ подтверждён" in text and case == "EarlyRestore":
                    state["down"] = False
                if "ШАГ 3: СЕЙЧАС ПОДКЛЮЧИТЕ" in text:
                    # Known simulated operator delay is included in recovery timing.
                    time.sleep(0.25)
                    state["down"] = False
                    state["restore"] = True
            code = proc.wait(timeout=5)
            reports = list((work / "runs").glob("*/summary.json"))
            assert len(reports) == 1, "Missing mock report: " + "".join(console)
            report = json.loads(reports[0].read_text(encoding="utf-8-sig"))
            metrics = report["metrics"]
            positive = case in ("CablePass", "StartupPass")
            assert (code == 0) == positive, "Unexpected exit: " + "".join(console)
            assert report["status"] == ("PASS" if positive else "FAIL"), report
            if positive:
                assert metrics["FaultConfirmed"] and metrics["RestoreRequested"] and metrics["Recovered"]
                assert metrics["HealthyFail"] == 0 and metrics["OutageMs"] >= 15000
                assert metrics["RecoveryMs"] >= 200, "Operator delay missing from time origin"
                assert metrics["PostOK"] >= 24 and metrics["PostTcpOK"] >= 3
            if case == "HealthyTimeout":
                assert not metrics["RestoreRequested"] and "ШАГ 2: СЕЙЧАС ВЫНЬТЕ" not in "".join(console)
            if case == "EarlyRestore":
                assert metrics["PrematureRestore"] and not metrics["RestoreRequested"]
            if case == "NoDisconnect":
                assert not metrics["FaultConfirmed"] and not metrics["RestoreRequested"]
            print(f"MOCK ONLY / {case}: expected {report['status']} / recovery_ms={metrics['RecoveryMs']}", flush=True)
        finally:
            timer.cancel()
            stop.set()
            if proc.poll() is None:
                proc.kill()
                proc.wait()
            udp.close()
            tcp.close()
            for thread in threads:
                thread.join(timeout=3)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pwsh", default="pwsh")
    parser.add_argument("--case", choices=("CablePass", "StartupPass", "HealthyTimeout", "EarlyRestore", "NoDisconnect"), required=True)
    args = parser.parse_args()
    run_case(args.case, args.pwsh)
