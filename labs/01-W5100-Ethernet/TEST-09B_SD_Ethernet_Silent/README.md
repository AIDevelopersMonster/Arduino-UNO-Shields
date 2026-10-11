# TEST-09B — Shared SPI: W5100 Ethernet + microSD, Silent UART

**State:** Experimental / PENDING compilation and hardware verification. This replaces the *testing objective* of earlier unfinished TEST-09 work; no earlier files or branches are deleted.

## Goal

Prove useful **cooperative operation** of Ethernet and microSD on a single UNO and blue W5100 Shield: each UDP request causes a real SD append, close, reopen, seek, byte-for-byte readback, followed by a binary UDP echo. Both devices share SPI but are accessed **sequentially**, not simultaneously.

No serial data is sent or received during the run. After the terminal UDP command `K9END`, the Arduino sends the only Serial line and returns the same aggregate report by UDP; the test can therefore be completed *without opening a COM monitor*.

## Hardware and setup

- UNO ATmega328P; W5100 Ethernet CS **D10**; microSD CS **D4**; SPI via ICSP.
- Use the microSD specimen that allowed DHCP to work; power OFF before inserting/removing the card.
- Router and DHCP unchanged; PC and UNO connected to same trusted LAN.
- Firmware gets DHCP and listens to UDP **5003**; unique MAC `02:4B:4F:4E:59:0B`.
- **Data warning:** the firmware removes **K09B.DAT** on every boot, then creates it for test records. Back up this file, if it exists. It does not intentionally modify other files.
- On any power failure or early stop, the test file can be incomplete; do not use important SD data for this trial.

## Acceptance criteria

Host sends default **40** distinct 24-byte UDP binary datagrams. For every request, Arduino appends all 24 bytes to `K09B.DAT`, closes the file, reopens it, reads the new record back at a known offset, and compares exact bytes. Only then is the Echo transmitted; a final report reveals any storage-side error.

Host checks the returned address/port, payload length and all bytes, without retries. `K9END` requests counters and one final UART print:

`K9RESULT eth=1 sd=1 rx=40 echo=40 wr=40 rd=40 err=0 bytes=960 ...`

**FULL PASS only with** 40/40 correct UDP Echo replies, 40 writes, 40 verified reads, 0 SD errors and 960 bytes recorded. No output on COM is needed during the run. No SD throughput or long-term reliability is certified.

## PowerShell / Arduino CLI

If your checkout is still on `feature/w5100-test09-ethernet-sd` with local changes, do **not** switch branches or reset. Import only the new folder:

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git fetch origin
git restore --source=origin/main --worktree -- labs/01-W5100-Ethernet/TEST-09B_SD_Ethernet_Silent/
$sketch = '.\labs\01-W5100-Ethernet\TEST-09B_SD_Ethernet_Silent'
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
```

Insert the card with power OFF. Once powered up, **leave Serial Monitor closed**, to avoid DTR-triggered resets. The sketch is intentionally silent during setup and processing; DHCP failures can appear as a UDP timeout.

As there is no startup UART output, find the current Arduino IP in the router's existing DHCP leases/client list in **read-only** mode, matching MAC `02:4B:4F:4E:59:0B`. Alternatively use a previously observed address **only after confirming it remains assigned**. Do not alter DHCP configuration.

From another PowerShell 7 prompt (does not use COM):

```powershell
$ip = '192.168.1.85' # EXAMPLE ONLY: replace with current TEST-09B lease
& .\labs\01-W5100-Ethernet\TEST-09B_SD_Ethernet_Silent\Test-IntegratedSilent.ps1 -IP $ip -Count 40
```

**Send compile output and the host RESULT and DEVICE lines.** Hardware PASS is never presumed merely from GitHub publication.

## Key limitations

- The firmware initializes SD first, then Ethernet. If this SD affects W5100 electrical behavior, the test may fail at DHCP.
- The sketch does **not** measure UART latency or prove electromagnetic independence. No serial interaction occurs during the test; USB continues to supply power.
- A bad SD can block networking at the electrical level even with perfect CS sequencing. Success/failure is specific to the card and W5100 specimen.
