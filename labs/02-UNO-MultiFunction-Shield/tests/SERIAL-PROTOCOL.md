# LAB-02 serial protocol v0.6

Transport: USB serial, 115200 baud, ASCII, one command or event per line.

## Buzzer mode selection

The buzzer type is selected explicitly and the two control paths are mutually
exclusive:

- `CFG,BUZZER,ACTIVE`
- `CFG,BUZZER,PASSIVE`

Changing the type first forces the buzzer OFF.

In ACTIVE mode:

- `BUZ,ON`
- `BUZ,OFF`
- `BEEP,<milliseconds>`

`TONE` commands are rejected with `@ERR,BUZZER_MODE,ACTIVE`.

In PASSIVE mode:

- `TONE,<Hz>`
- `TONE,OFF`

`BUZ` and `BEEP` commands are rejected with
`@ERR,BUZZER_MODE,PASSIVE`.

The default after reset is ACTIVE because that is the bench-verified type on
the current physical shield.

## Other PC to Arduino commands

| Command | Meaning |
| --- | --- |
| `PING` | link check |
| `INFO` | report firmware, pins and configuration |
| `STATE?` | send an immediate state frame |
| `LED,1,ON` ... `LED,4,OFF` | control one LED |
| `LED,ALL,ON` / `OFF` | control all four LEDs |
| `DISP,1234` | put up to four numeric/minus characters on display |
| `TEST,ALL` | start automated stimulus sequence using selected buzzer type |
| `TEST,STOP` | stop automated sequence |
| `CFG,DIGITSEL,STD` | standard digit-select polarity |
| `CFG,DIGITSEL,INV` | inverted digit-select diagnostic mode |

BEEP duration is clamped to 10..5000 ms.
TONE frequency is clamped to 30..5000 Hz.

## Arduino to PC

Startup / info includes:

```text
@SYS,READY,MFSHIELD,LAB02,0.6
@SYS,MFSHIELD,LAB02,0.6,UNO,115200
@CFG,DIGITSEL,STD
@CFG,BUZZER,ACTIVE
```

The current sample should normally remain in ACTIVE mode. PASSIVE mode exists
for replacement parts or shield variants that actually use a passive sounder.
