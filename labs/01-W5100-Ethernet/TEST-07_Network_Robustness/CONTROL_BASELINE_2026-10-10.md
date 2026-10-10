# Ethernet control after W5100_COMPARE — 10 October 2026

**Previous standalone Baseline: PASS. Latest card-present Baseline: FAIL,
warmup deadline without stable service.** This public report contains
aggregate results and reproducible commands, without the supplied raw console,
local network addresses, absolute workstation paths or per-run identifiers.

| Observed measurement | Supplied result |
| --- | --- |
| Build / upload | BUILD PASS; measured binary uploaded; AVR 1.8.8 |
| Flash | 20,392 / 32,256 B |
| Static SRAM | 879 / 2,048 B |
| Scenario | Baseline, bounded 60 s, automatic |
| Network service | Verified; ready=1 |
| Final successful exchanges | 503 exact UDP echoes; 11 exact TCP exchanges |
| Error counters | HealthyFail=0, Corrupt=0, SendFail=0, Drop=0 |
| Actual host / healthy duration | 60.0298379 s / 54.799 s |
| UART / startup / coverage | 22 stats, one BOOT, firmware/W5100 confirmed, four UDP sizes |
| Displayed free SRAM samples | 1,133 B |
| Sampled minimum / drift | 1,127 B / 0 B |
| Terminal runner verdict | RESULT PASS / Baseline |

The operator subsequently supplied the completed summary, status=PASS with
no reasons. Its actual metrics replay PASS through unchanged Get-NetworkVerdict.
All three supplied source hashes match the pre-fix repository sources using
Windows CRLF endings. Full serial/probe/event logs and the HEX hash were not
supplied. The earlier 458 UDP / 10 TCP progress was five seconds before the
end; it is superseded by the actual summary counters above.

The preceding preparation explicitly requested microSD removal with all power
off. Those physical preparation responses were not repeated in the new console,
so independent observation of card absence is not claimed. This run confirms
current bounded Ethernet service for the reported firmware/sample. Firmware
and power state changed relative to the failed W5100_COMPARE, so it does not
identify the shared-bus failure cause or isolate SD presence alone. The prior
ETH_DRIVER_INIT FAIL remains recorded; full TEST-09 v0.2 hardware is PENDING.
This is a repeat Baseline, not a seventh scenario: TEST-07 remains 6 PASS /
1 DEFERRED, with DhcpOutage reserved for a separate isolated router.

## Card-present control: same loaded firmware, no SD init

**Now attempted: FAIL; observed details and next read-only step are below.**
The original reproducible preparation retained the measured TEST-07 binary.
Insert the same
card only with all UNO/Shield power disconnected, then run one Baseline 60 s.
Use a distinct output directory. No compiler/upload step, frequency change,
router DHCP change or retry-until-PASS sequence is needed. During measurement,
follow the automatic script; no key presses are needed. Final verdict is green
for PASS and red for FAIL.

```powershell
$test = ".\labs\01-W5100-Ethernet\TEST-07_Network_Robustness"
git pull --ff-only
$null = Read-Host "Отключи всё питание UNO/Shield. Вставь ту же microSD. Ethernet-кабель оставь подключённым. Затем нажми Enter"
$null = Read-Host "Подключи USB UNO. Закрой монитор Arduino. После подключения нажми Enter"
& "$test\Test-NetworkRobustness.ps1" -SerialPort COM4 -Scenario Baseline -DurationSeconds 60 -OutputDirectory "$test\runs\controls\card-present-sd-idle"
```

TEST-07 contains no SD library, SD initialization or file operations. D4 is
HIGH after pin setup; its unchanged startup uses OUTPUT before HIGH, so absence
of a short startup select pulse is not certified. A future PASS would establish
bounded Ethernet service with a physically present, application-uninitialized
card. It would not establish proper SPI handoff after SD initialization or
cancel the prior comparison failure. A FAIL would identify a narrower observed
failing condition, without deciding software versus electrical cause.

## Host logging failure and repair before measurement

The first card-present launch failed while opening events.jsonl, before the
serial constructor/open, clock or network probes. Physical preparation was
confirmed in the supplied terminal dialogue, but the measurement is **NOT RUN**.
This is a host harness error, not a failed hardware exchange or new SD verdict.

Cause: New-Item resolves a relative OutputDirectory against PowerShell's
location; .NET StreamWriter resolves it against the process working directory.
Those directories may differ. The runner now retains the created directory's
absolute FullName for all log and summary paths. Default/absolute output paths,
firmware, network logic and acceptance gates are unchanged. The runner's own
source hash changes; the prior supplied summary remains evidence of its
pre-fix source and is not rewritten.

The [host-only path regression](tests/Invoke-LogPathRegression.ps1) executes
the actual runner's initialization prefix and actual three StreamWriters with
intentionally different PowerShell/process directories. Before repair, the
relative nested case reproduces the missing events.jsonl parent exception.
After repair, all five cases pass: relative nested, relative parent with
spaces, current directory, absolute directory and default. They check files
are flushed in the requested folder and do not appear under the process
directory. No COM or network access occurs. Local PowerShell 7.5.2 on Ubuntu
24.04.3 LTS. The subsequent Windows launch successfully opened logs under the
requested output directory and entered the runner; the path issue did not recur.
Both changed PowerShell files parse. No new firmware build/upload is needed.

If already prepared with the card inserted and USB/Ethernet connected, retain
that preparation and retry the host measurement after pulling the repair:

```powershell
git pull --ff-only
$test = ".\labs\01-W5100-Ethernet\TEST-07_Network_Robustness"
& "$test\Test-NetworkRobustness.ps1" -SerialPort COM4 -Scenario Baseline -DurationSeconds 60 -OutputDirectory "$test\runs\controls\card-present-sd-idle"
```

The changed runner accepts the original relative parameter. An explicit
absolute output path is also supported. Do not upload a new firmware or
change clocks/router settings to address this host filesystem issue.

## Latest card-present attempt: FAIL at warmup deadline

After pulling the path repair, the operator ran the same loaded TEST-07 binary
with the card left inserted and SD still uninitialized by application code.
The runner created the intended nested log directory and entered WARMUP. Every
displayed progress line had UDP=0/TCP=0. It ended at the default warmup deadline
(30 s, rather than completing the requested 60 s) without a stable service.
The actual terminal verdict is **FAIL**, with these reported reasons:

- bounded run did not finish;
- runner error: Warmup deadline: no stable service;
- insufficient UART statistics;
- expected exactly one observed BOOT;
- W5100 not observed;
- insufficient successful exchanges or boundary coverage.

No UART STAT lines appear in the supplied console. Absence from the console
does not prove absence of raw UART bytes: the runner saves every complete
received line before parsing it and prints only STAT lines. A missing/invalid
BOOT count can also mean a malformed or duplicated BOOT, not necessarily zero
bytes. The summary and serial.log of this failed run have not yet been supplied.
This does not certify SD damage, electrical bus contention, exact chip-detection
failure or a physical reboot. Prior network and SD_ONLY PASS records stand.

Static inspection found an independent initialization-order defect in the
current TEST-07: the first acquireDhcp calls stopServices, then closeAllSockets,
before Ethernet.begin initializes the W5100 driver. setup's Ethernet.init only
sets the SS pin. The indexed EthernetClient.stop sends a socket command before
its timed close loop, and the driver's execCmdSn waits without a deadline for
the command register to clear. The application therefore enters low-level
socket operations before chip/SPI/fast-CS initialization; the 20 ms client
timeout does not bound that preceding register-command wait.

Inspected primary Ethernet 2.0.2 implementations:
[init/begin](https://github.com/arduino-libraries/Ethernet/blob/2.0.2/src/Ethernet.cpp),
[client stop](https://github.com/arduino-libraries/Ethernet/blob/2.0.2/src/EthernetClient.cpp),
[socketDisconnect](https://github.com/arduino-libraries/Ethernet/blob/2.0.2/src/socket.cpp),
[driver init/execCmdSn](https://github.com/arduino-libraries/Ethernet/blob/2.0.2/src/utility/w5100.cpp),
[AVR fast CS](https://github.com/arduino-libraries/Ethernet/blob/2.0.2/src/utility/w5100.h).
All five tagged source files match the locally inspected library. This identifies
an application defect but does not yet establish that the latest physical run
stopped there. Firmware and host criteria are unchanged in this follow-up.

Next step is **read existing logs**, with no new hardware run, upload, power/card
change or UART-monitor session. Use the most recent folder under this control:

```powershell
$test = ".\labs\01-W5100-Ethernet\TEST-07_Network_Robustness"
$controls = Join-Path (Resolve-Path $test).Path 'runs/controls/card-present-sd-idle'
$run = (Get-ChildItem -LiteralPath $controls -Directory | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
Get-Item "$run\serial.log" | Select-Object Length
Get-Content "$run\serial.log"
Get-Content "$run\summary.json" -Raw
```

The UART log will distinguish loss of BOOT framing from an execution stop before
DHCP_BEGIN or later within DHCP/driver initialization. The full failed summary
will provide exact BOOT/banner/stat counts and runner hashes. No physical
root-cause assignment or new hardware PASS is made from the terminal alone.
