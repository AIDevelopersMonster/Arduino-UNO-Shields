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
| Last printed progress | 5 s remaining: UDP=458, TCP=10, errors=0 |
| Displayed free SRAM samples | 1,133 B |
| Terminal runner verdict | RESULT PASS / Baseline |

The last progress counters precede completion and are not final counters.
Final summary/log/hash files were not supplied; their contents are not
reconstructed. Status is based on the operator's actual terminal verdict,
not a synthetic reconstruction of the full network verdict.

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
