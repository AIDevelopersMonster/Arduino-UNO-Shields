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
bytes. At this stage the summary and serial.log had not yet been supplied;
their subsequent inspection and the repair are recorded below.
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

The requested diagnostic step was **read existing logs**, with no new hardware
run, upload, power/card change or UART-monitor session:

```powershell
$test = ".\labs\01-W5100-Ethernet\TEST-07_Network_Robustness"
$controls = Join-Path (Resolve-Path $test).Path 'runs/controls/card-present-sd-idle'
$run = (Get-ChildItem -LiteralPath $controls -Directory | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
Get-Item "$run\serial.log" | Select-Object Length
Get-Content "$run\serial.log"
Get-Content "$run\summary.json" -Raw
```

## Supplied logs and firmware 0.2 repair

The operator subsequently supplied the 168-byte serial.log and complete failed
summary. UART contains two lines: reset noise directly followed by the BOOT EVT
on the first line, then INFO for firmware 0.1 on the second. No DHCP_BEGIN,
HARDWARE or STAT is present. The observed BOOT text was not accepted by the
anchored parser because the line did not start with EVT. This is a framing
failure, not evidence that the MCU never executed setup.

| Supplied failed summary | Value |
| --- | --- |
| Actual duration / requested | 30.0307185 / 60 s |
| Completed / stable healthy time | false / 0 s |
| Parsed BOOT / STAT / firmware INFO | 0 / 0 / recognized |
| W5100 observation | false; no HARDWARE event, not a chip-detection measurement |
| Successful UDP / TCP / size coverage | 0 / 0 / 0 |
| MinFree | 32767, initial sentinel; no RAM sample was received |
| Verdict | FAIL, six unchanged reasons listed above |

The supplied firmware, module and runner SHA-256 values match the pre-repair
repository sources with Windows CRLF endings. The unchanged Get-NetworkVerdict
reproduces the same six FAIL reasons. The old raw log and result are not rewritten.

The lack of DHCP_BEGIN places the last UART observation before the first DHCP
call and is consistent with the pre-init stopServices path. The ordering defect
is established by source inspection and reproduced by a strict lifecycle model;
the raw UART is not an instruction trace and does not prove the exact physical
instruction or exclude a later UART fault.

Firmware **0.2** repairs this path:

- Track successful driver initialization separately from DHCP/service readiness.
  Before it succeeds, stopServices/closeAllSockets do not issue socket commands.
- After Ethernet.begin returns, use cached hardwareStatus before IP/RTR/RCR
  register access. No detected chip emits DHCP_FAIL reason=NO_HARDWARE and keeps
  the existing 5 s retry interval. A detected chip with failed DHCP also retries.
- STAT and a manual DHCP request before initialization avoid hardware accesses;
  STAT uses IP 0.0.0.0. Normal cleanup, lease reacquisition and service restart
  remain active after initialization.
- Set both inactive CS latches HIGH before switching them to OUTPUT. Add a line
  delimiter immediately before BOOT. Do not strip noise in the host parser or
  hide duplicate BOOTs. The runner now requires INFO version=0.2 so an old upload
  cannot be certified against the repaired source.

No library patch, SPI-frequency change, static-IP fallback, extra packet retry
or relaxed RAM/exchange/BOOT gate is added. Synchronous Ethernet 2.0.2 driver
waits after initialization remain outside a strict MCU deadline; no watchdog
or physical SPI recovery is claimed.

### Executed software verification

Actual Arduino CLI 1.3.1, AVR 1.8.6, Ethernet 2.0.2, arduino:avr:uno:
**BUILD PASS, Flash 20,622 / 32,256 B; static SRAM 880 / 2,048 B**.
The existing budgets of Flash <=29,000 B and static SRAM <=1,200 B pass.
Four unused tag warnings originate in AVR core new.cpp; none from the sketch.
The operator's AVR 1.8.8 build must record its own sizes and HEX hash.

Firmware LF SHA-256:
`B063E74E971F162052BBFE99A3276EE3663FE2EFD0CCB24E1F95465BF6F2DB1B`.
Local non-bootloader HEX SHA-256:
`38DC2415BEDDE19CCE5FD7266BBBC53F6A4F66708FEC8C91B0F6290A8044A753`.

[Eight lifecycle models](tests/test_startup_order.py) compile the actual entire
sketch against a driver that rejects register/socket calls before initialization.
They cover direct/normal startup, no chip then recovery, no lease then retry,
early STAT/manual DHCP, maintenance failure and address-change service restart.
The pre-repair sketch reproduces uninitialized socketDisconnect in this model;
the repaired sketch passes all eight. Native freeRam is not an AVR measurement.

[Six UART cases](tests/Invoke-StartupUartRegression.ps1) execute the actual
Drain-Serial/Write-Event functions with memory UART. Noise-merged BOOT stays
unrecognized; a delimited BOOT counts once; missing/duplicate BOOT is preserved;
the old firmware banner is rejected; fragmented input is assembled correctly.
The existing host self-test passes its positive/negative verdict, guidance and
real loopback UDP/TCP checks. All five log-path regression cases still pass.
The full guided StartupPass fixture also passes with the 0.2 banner, memory/file
UART and real loopback transport. Eight PowerShell files and the published
command blocks parse successfully.
These are software checks, not physical UNO/W5100 results.

### Next single hardware control

**Firmware 0.2 hardware result: PENDING.** Retain the currently inserted card,
Ethernet cable and USB. The application does not initialize SD or access files.
Pull on the current feature/w5100-test09-ethernet-sd branch, rebuild/upload the
changed firmware without UseExistingBuild, then run one automatic Baseline:

```powershell
git pull --ff-only
$test = ".\labs\01-W5100-Ethernet\TEST-07_Network_Robustness"
& "$test\Build-Test07.ps1" -UploadPort COM4
if ($?) {
    & "$test\Test-NetworkRobustness.ps1" -SerialPort COM4 -Scenario Baseline -DurationSeconds 60 -OutputDirectory "$test\runs\controls\card-present-fw02"
}
```

No key presses or planned cable/router interruption are needed. Preserve the
single resulting verdict and logs, including any FAIL. A future PASS would
confirm only this bounded Ethernet control with an application-uninitialized
card. It would not certify SD/network integration or transfer the six historical
scenario PASS results to the new firmware; DhcpOutage remains DEFERRED.
