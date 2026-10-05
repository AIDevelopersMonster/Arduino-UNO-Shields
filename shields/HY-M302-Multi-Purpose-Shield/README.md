# HY-M302 Multi-Purpose 9-in-1 Shield

## Identification

The board family sold as **HY-M302** is a multi-purpose Arduino learning shield
with integrated sensors, indicators, controls and expansion headers.

Public HY-M302 listings and community documentation match the layout commonly
documented as the **Keyestudio KS0183 Multi-purpose Shield V1**. For this
repository, KS0183 documentation is therefore used as a **family-level reference**
only.

The exact electrical behavior of our physical HY-M302 sample must still be
bench-certified before family-level assumptions are promoted to verified facts.

Current repository status:

- family identification: **documented / high confidence**;
- pin-map below: **working reference, not yet bench-certified on our sample**;
- laboratory firmware: **not yet created**;
- physical PASS status: **not yet assigned**.

## Video

Overview / project-opening video: https://youtube.com/shorts/t4WG9GA9Qws

## On-board hardware

The HY-M302 / KS0183 family provides:

- DHT11 temperature/humidity sensor;
- LM35 analog temperature sensor;
- LDR / photocell light sensor;
- rotary potentiometer;
- IR receiver;
- active/self-oscillating buzzer on the tested sample;
- RGB LED;
- two additional indicator LEDs;
- two user push-buttons;
- RESET button;
- two free digital breakout channels;
- one free analog breakout channel;
- I2C breakout;
- TTL UART breakout;
- Arduino UNO shield form factor.

Keyestudio documents the corresponding KS0183 board as approximately
69 x 53 x 19 mm and 21 g.

## Working Arduino UNO pin map

| Function | UNO pin | Status |
| --- | ---: | --- |
| TTL UART RX/TX | D0 / D1 | family reference |
| SW1 | D2 | **bench verified** |
| SW2 | D3 | **bench verified** |
| DHT11 DATA | D4 | **bench verified** |
| Active/self-oscillating buzzer | D5 | **bench verified** |
| IR receiver / async NEC | D6 | **bench verified, 12/12 exact match** |
| Free digital breakout | D7 | family reference |
| Free digital breakout | D8 | family reference |
| RGB RED | D9 | **bench verified** |
| RGB GREEN | D10 | **bench verified** |
| RGB BLUE | D11 | **bench verified** |
| Red indicator LED | D12 | **bench verified** |
| Blue indicator LED | D13 | **bench verified** |
| Potentiometer | A0 | **bench verified, 0..1023** |
| LDR / light sensor | A1 | **bench verified** |
| LM35 | A2 | **bench FAIL on tested sample** |
| Free analog breakout | A3 | family reference |
| I2C SDA | A4 | family reference |
| I2C SCL | A5 | family reference |

TEST-01 physically verified D9=RED, D10=GREEN and D11=BLUE. The RGB LED uses
direct polarity on this sample: HIGH / PWM 255 turns a channel on, LOW / PWM 0
turns it off. D12 drives the red discrete LED and D13 drives the blue discrete
LED.

## Important UNO resource conflicts

This shield is useful precisely because it integrates many functions, but that
also means it consumes most of the ATmega328P I/O map.

### SPI conflict

UNO hardware SPI uses:

- D11 — MOSI
- D12 — MISO
- D13 — SCK

HY-M302 uses D11-D13 for onboard LEDs. Therefore SPI peripherals such as a
microSD module or W5100 Ethernet shield cannot be treated as automatically
compatible while the onboard LED circuitry remains connected.

D4 is also occupied by the DHT11 on this family, which conflicts with the common
microSD CS assignment used by many Arduino examples.

Any future SPI experiment must be treated as a separate compatibility test.

### Interrupt buttons

On Arduino UNO, D2 and D3 are the two classic external-interrupt pins. The
HY-M302 family intentionally connects its two buttons there, making the board
convenient for interrupt experiments.

### I2C

A4/A5 are exposed as SDA/SCL and may be used for I2C devices, subject to normal
5 V UNO electrical requirements and verification on the physical sample.

### UART

D0/D1 are exposed for TTL serial use. External UART hardware can therefore
interfere with USB-serial upload/monitor traffic and must be disconnected or
managed appropriately during programming.

## Sensor notes

### DHT11

Bench-verified on D4.

The physical sensor was first tested independently with the established Adafruit
DHT library:

```text
DHT11 OK T=30.3 C RH=39.0 %
DHT11 OK T=30.3 C RH=41.0 %
```

The project then retested the same physical sensor using the dependency-free
low-level driver in the HY_M302 library:

```text
DHT11 OK T=30.2 C RH=39.0 %
READ 1: T=30.2 C RH=40.0 %
READ 2: T=30.2 C RH=40.0 %
READ 3: T=30.2 C RH=40.0 %
```

The independent reference and in-house driver agree closely, so the D4 mapping,
the physical DHT11 and the project low-level decoder are all treated as
**PHYSICAL PASS** on this sample.

### LM35

The LM35 channel is connected to A2, but the sensor on our physical HY-M302
sample failed the temperature-response test.

Observed raw values during the bench session remained approximately:

```text
93 -> 91 -> 90 -> 89 -> 89
```

despite deliberate cooling of the sensor.

An operating LM35 should show a clear output-voltage decrease when cooled and a
progressive recovery while warming. That behavior was not observed.

Status for the tested sample: **PHYSICAL FAIL / sensor treated as defective**.

This does not establish that the HY-M302 design or all production batches have a
faulty LM35. The A2 pin assignment remains valid, and the library keeps generic
LM35 raw/conversion support for other boards or replacement sensors.

### LDR

Bench-verified on A1. On our physical HY-M302 sample the ADC code rises with
illumination and falls in darkness. Observed manual TEST-01 values were about
368 under ordinary room light and about 56-61 when the LDR was covered. These
values are sample/lighting dependent; the verified property is the direction of
response, not those exact thresholds.

### Potentiometer

Documented on A0 and suitable for deterministic analog-input testing across most
of the 0-1023 ADC range.

### IR receiver

Bench-verified on D6 with the Arduino-IRremote reference library and an Orange
Pi remote control. The receiver decoded standard NEC frames with address 0x04.

Observed unique commands in one 12-button pass:

```text
13 10 11 0F 0C 0D 0B 08 09 58 47 53
```

Repeat frames were also received normally. After correcting the project
decoder's leader timing and already-active-pulse handling, all 12 unique Orange
Pi remote frames matched Arduino-IRremote exactly in RAW value, address and
command.

The blocking decoder was then replaced by a non-blocking AVR implementation:
D6/PD6/PCINT22 captures edge timing in a short ISR, while a NEC state machine
decodes frames outside the ISR.

The new asynchronous decoder was physically tested across all 12 unique Orange
Pi remote commands. Every decoded full frame matched Arduino-IRremote exactly in
RAW value, address and command, with no wrong-frame result in the 12-button run.

Status: **ASYNC NEC PHYSICAL PASS — 12/12 functional correctness**.

A separate live/stress run with dropped-edge and dropped-frame counters is still
reserved as the final robustness certification step.

## Output notes

### Buzzer

Family-level documentation commonly describes the D5 device as passive, but our
physical HY-M302 sample behaves as an **active/self-oscillating buzzer**.

Bench result:

- D5 HIGH -> strongest sustained sound;
- D5 LOW -> silence;
- frequency-driven tone output is weaker.

For this tested sample the primary operating mode is therefore direct digital
ON/OFF, not frequency drive. This is recorded as **PHYSICAL PASS: ACTIVE BUZZER**.

### LEDs

The family uses five UNO outputs D9-D13:

- D9-D11 — RGB LED channels;
- D12-D13 — two discrete indicator LEDs.

TEST-01 bench result: D9=RED, D10=GREEN, D11=BLUE with direct polarity; D12 is
the red discrete LED and D13 is the blue discrete LED.

## Proposed laboratory sequence

A future dedicated lab can be opened only when the physical HY-M302 is on the
bench.

Recommended sequence:

1. **TEST-01 — Identification / pin inventory**
   - photograph both sides;
   - record PCB markings and visible component markings;
   - verify power and RESET behavior;
   - compile a physical pin-usage baseline.

2. **TEST-02 — Buttons + five LED channels**
   - verify D2/D3 button polarity;
   - verify D9-D13 output polarity;
   - identify the D9-D11 RGB color order.

3. **TEST-03 — Analog channels**
   - A0 potentiometer;
   - A1 LDR;
   - A2 LM35;
   - A3 free analog breakout.

4. **TEST-04 — DHT11**
   - temperature;
   - humidity;
   - repeated-read stability;
   - timeout/error handling.

5. **TEST-05 — Buzzer**
   - verify direct HIGH/LOW behavior;
   - compare optional tone drive;
   - certify active/passive behavior.

6. **TEST-06 — IR receiver**
   - carrier reception;
   - raw/decoded remote codes;
   - current Arduino-IRremote library compatibility.

7. **TEST-07 — Expansion interfaces**
   - D7/D8 free digital lines;
   - I2C on A4/A5;
   - TTL UART on D0/D1, if required.

After these pass separately, they can be combined into one unified HY-M302
diagnostic firmware.

## Repository policy

External documentation establishes only the likely **shield family and expected
wiring**.

For this project:

- online documentation = family reference;
- silkscreen/visual inspection = sample evidence;
- continuity measurements = used only when a specific ambiguity remains;
- successful firmware behavior on the physical shield = certification evidence.

A claim becomes **bench-verified** only after it is reproduced on our own sample.

## External references

- Keyestudio KS0183 Multi-purpose Shield V1 documentation:
  https://docs.keyestudio.com/projects/KS0183/en/latest/docs/KS0183.html
- Keyestudio KS0183 product page:
  https://www.keyestudio.com/products/keyestudio-multi-purpose-shield-v1-for-arduino-starter
- HY-M302 product listing / board-family reference:
  https://www.usinainfo.com.br/shieldsexpansores/shield-multifuncoes-hy-m302-para-arduino-com-dht11-lm35-receptor-ir-ldr-leds-buzzer-e-outros-4866.html
- Arduino Forum pin/resource discussion for HY-M302:
  https://forum.arduino.cc/t/conectar-shield-hy-m302-arduino-uno-e-modulo-cartao-micro-sd/1224288
- Fritzing community cross-reference HY-M302 / KS0183:
  https://forum.fritzing.org/t/looking-for-a-part-for-the-hy-m302-ks0183-multi-function-shield/22240


## Arduino library

Initial low-overhead library:

[HY_M302](../../libraries/HY_M302/)

The first version intentionally uses compact in-house routines for the DHT11 and
basic NEC IR decoding so Flash/SRAM cost stays visible on ATmega328P. Hardware
behavior remains subject to physical certification on our shield.


## Physical bench results

### TEST-01 / manual bench results

Verified on the physical HY-M302 sample:

- SW1 on D2: idle -> library reports 0, pressed -> 1;
- SW2 on D3: idle -> library reports 0, pressed -> 1;
- button electrical behavior is active LOW; the library exposes logical
  `pressed=true`;
- potentiometer on A0 reached the full ADC endpoints 0 and 1023 and stable
  intermediate values;
- LDR on A1 is bench-verified: ordinary room light produced about 368, while
  covering the sensor produced about 56-61; brighter -> higher ADC code;
- LM35 on A2 failed the physical cooling-response test on this sample and is
  treated as defective;
- A3 is currently unconnected, so raw values from it are expected to float and
  are not treated as measurements.

Previously in the same manual test:

- D9 = RGB RED;
- D10 = RGB GREEN;
- D11 = RGB BLUE;
- RGB polarity is direct: 0=off, 255=full;
- D12 = red discrete LED;
- D13 = blue discrete LED.
