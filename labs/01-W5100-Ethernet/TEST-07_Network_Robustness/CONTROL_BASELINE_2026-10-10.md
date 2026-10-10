# Ethernet control after W5100_COMPARE — 10 October 2026

**Actual operator TEST-07 Baseline result: PASS.** This public report contains
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

## Next single control: same loaded firmware, card present, no SD init

**PENDING.** Keep the measured TEST-07 binary already loaded. Insert the same
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
24.04.3 LTS; Windows hardware confirmation awaits the next operator run.
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
