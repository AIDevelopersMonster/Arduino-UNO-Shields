# LAB-02 serial protocol v0.4

Transport: USB serial, 115200 baud, ASCII, one command or event per line.

## PC to Arduino

| Command | Meaning |
| --- | --- |
| `PING` | link check |
| `INFO` | report firmware, pins and display configuration |
| `STATE?` | send an immediate state frame |
| `LED,1,ON` ... `LED,4,OFF` | control one LED |
| `LED,ALL,ON` / `OFF` | control all four LEDs |
| `DISP,1234` | put up to four numeric/minus characters on display |
| `BUZ,ON` / `BUZ,OFF` | primary active-buzzer control; D3 HIGH / LOW |
| `BEEP,200` | primary short active-buzzer pulse, duration in ms |
| `TONE,1000` | secondary PWM/modulation diagnostic in Hz |
| `TONE,OFF` | stop tone |
| `TEST,ALL` | start automated stimulus sequence |
| `TEST,STOP` | stop automated sequence |
| `CFG,DIGITSEL,STD` | standard digit-select polarity |
| `CFG,DIGITSEL,INV` | inverted digit-select diagnostic mode |

BEEP duration is clamped to 10..5000 ms.
TONE frequency is clamped to 30..5000 Hz.

## Arduino to PC

Startup / info:

```text
@SYS,READY,MFSHIELD,LAB02,0.4
@SYS,MFSHIELD,LAB02,0.4,UNO,115200
@PINS,BUZ=3,LATCH=4,CLK=7,DATA=8,LED=13/12/11/10,BTN=A1/A2/A3,POT=A0
@CFG,DIGITSEL,STD
```

State frame, nominally every 100 ms:

```text
@STATE,uptime_ms,pot,b1,b2,b3,c1,c2,c3,l1,l2,l3,l4,buzzer_mode,buzzer_hz,display,test_state
```

Where:

- `b1..b3`: debounced pressed state, 0/1;
- `c1..c3`: press counters;
- `l1..l4`: commanded LED states, 0/1;
- `buzzer_mode`: OFF, DC or TONE;
- `display`: current four-character display buffer;
- `test_state`: IDLE, DISPLAY, LED1..LED4, BUZZER, POT or DONE.

The GUI deliberately ignores continuous `@STATE` lines in its log unless
telemetry logging is enabled.
