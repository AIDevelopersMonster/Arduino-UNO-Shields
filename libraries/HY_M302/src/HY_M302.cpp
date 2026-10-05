#include "HY_M302.h"

HY_M302::HY_M302() : _pins() {}
HY_M302::HY_M302(const PinMap& pins) : _pins(pins) {}

void HY_M302::begin() {
  pinMode(_pins.sw1, INPUT_PULLUP);
  pinMode(_pins.sw2, INPUT_PULLUP);

  pinMode(_pins.buzzer, OUTPUT);
  pinMode(_pins.ir, INPUT);
  pinMode(_pins.gpio7, INPUT);
  pinMode(_pins.gpio8, INPUT);

  pinMode(_pins.rgb1, OUTPUT);
  pinMode(_pins.rgb2, OUTPUT);
  pinMode(_pins.rgb3, OUTPUT);
  pinMode(_pins.led1, OUTPUT);
  pinMode(_pins.led2, OUTPUT);

  digitalWrite(_pins.led1, LOW);
  digitalWrite(_pins.led2, LOW);
  rgbOff();
  buzzerOff();
}

bool HY_M302::button1Pressed() const {
  return digitalRead(_pins.sw1) == LOW;
}

bool HY_M302::button2Pressed() const {
  return digitalRead(_pins.sw2) == LOW;
}

void HY_M302::led1(bool on) {
  digitalWrite(_pins.led1, on ? HIGH : LOW);
}

void HY_M302::led2(bool on) {
  digitalWrite(_pins.led2, on ? HIGH : LOW);
}

void HY_M302::ledBlue(bool on) {
  led1(on);
}

void HY_M302::ledRed(bool on) {
  led2(on);
}

void HY_M302::setRGB(uint8_t red, uint8_t green, uint8_t blue) {
  analogWrite(_pins.rgb1, red);
  analogWrite(_pins.rgb2, green);
  analogWrite(_pins.rgb3, blue);
}

void HY_M302::setRgbRaw(uint8_t ch1, uint8_t ch2, uint8_t ch3) {
  analogWrite(_pins.rgb1, ch1);
  analogWrite(_pins.rgb2, ch2);
  analogWrite(_pins.rgb3, ch3);
}

void HY_M302::rgbOff() {
  setRGB(0, 0, 0);
}

void HY_M302::buzzerTone(unsigned int frequency, unsigned long durationMs) {
  if (durationMs == 0) tone(_pins.buzzer, frequency);
  else tone(_pins.buzzer, frequency, durationMs);
}

void HY_M302::buzzerOff() {
  noTone(_pins.buzzer);
  digitalWrite(_pins.buzzer, LOW);
}

int HY_M302::readPotRaw() const {
  return analogRead(_pins.pot);
}

int HY_M302::readLightRaw() const {
  return analogRead(_pins.light);
}

int HY_M302::readLm35Raw() const {
  return analogRead(_pins.lm35);
}

int HY_M302::readAnalog3Raw() const {
  return analogRead(_pins.analog3);
}

float HY_M302::readLm35C(float aref) const {
  const int raw = readLm35Raw();
  const float volts = (raw * aref) / 1023.0f;
  return volts * 100.0f;
}

uint32_t HY_M302::expectPulse(volatile uint8_t* inputReg,
                                uint8_t bitMask,
                                uint8_t level,
                                uint32_t maxLoops) {
  uint32_t count = 0;

  if (level == HIGH) {
    while ((*inputReg & bitMask) != 0) {
      if (++count >= maxLoops) return 0;
    }
  } else {
    while ((*inputReg & bitMask) == 0) {
      if (++count >= maxLoops) return 0;
    }
  }

  return count;
}

HY_M302::DhtReading HY_M302::readDht11() {
  DhtReading out = {0.0f, 0.0f, false};
  uint8_t data[5] = {0, 0, 0, 0, 0};

  // DHT11 host start signal.
  pinMode(_pins.dht, OUTPUT);
  digitalWrite(_pins.dht, LOW);
  delay(20);
  digitalWrite(_pins.dht, HIGH);
  delayMicroseconds(35);
  pinMode(_pins.dht, INPUT_PULLUP);

  const uint8_t bitMask = digitalPinToBitMask(_pins.dht);
  const uint8_t port = digitalPinToPort(_pins.dht);
  if (port == NOT_A_PIN) return out;

  volatile uint8_t* inputReg = portInputRegister(port);

  // Loop-count timeout only. Do not use micros() while interrupts are disabled:
  // on AVR, micros() depends on Timer0 overflow bookkeeping.
  const uint32_t maxLoops = microsecondsToClockCycles(120UL) / 4UL + 20UL;

  noInterrupts();

  // Sensor response: ~80 us LOW, then ~80 us HIGH.
  if (expectPulse(inputReg, bitMask, LOW, maxLoops) == 0 ||
      expectPulse(inputReg, bitMask, HIGH, maxLoops) == 0) {
    interrupts();
    return out;
  }

  // Each data bit: ~50 us LOW followed by either ~26-28 us HIGH (0)
  // or ~70 us HIGH (1). Compare HIGH against the preceding LOW duration;
  // this avoids relying on an absolute microsecond threshold.
  for (uint8_t i = 0; i < 40; ++i) {
    const uint32_t lowCycles =
        expectPulse(inputReg, bitMask, LOW, maxLoops);
    const uint32_t highCycles =
        expectPulse(inputReg, bitMask, HIGH, maxLoops);

    if (lowCycles == 0 || highCycles == 0) {
      interrupts();
      return out;
    }

    data[i / 8] <<= 1;
    if (highCycles > lowCycles) data[i / 8] |= 1;
  }

  interrupts();

  const uint8_t checksum =
      uint8_t(data[0] + data[1] + data[2] + data[3]);
  if (checksum != data[4]) return out;

  out.humidity = data[0] + data[1] * 0.1f;
  out.temperatureC = data[2] + data[3] * 0.1f;
  out.ok = true;
  return out;
}

bool HY_M302::inRange(unsigned long value, unsigned long minUs, unsigned long maxUs) {
  return value >= minUs && value <= maxUs;
}

bool HY_M302::readIrNec(IrNecFrame& frame, unsigned long startTimeoutUs) {
  frame.address = 0;
  frame.command = 0;
  frame.raw = 0;
  frame.repeat = false;
  frame.ok = false;

  const unsigned long leadLow = pulseIn(_pins.ir, LOW, startTimeoutUs);
  if (leadLow == 0) return false;
  if (!inRange(leadLow, 8000UL, 10000UL)) return false;

  const unsigned long leadHigh = pulseIn(_pins.ir, HIGH, 5000UL);
  if (inRange(leadHigh, 2000UL, 2800UL)) {
    const unsigned long repeatLow = pulseIn(_pins.ir, LOW, 1000UL);
    if (inRange(repeatLow, 300UL, 900UL)) {
      frame.repeat = true;
      frame.ok = true;
      return true;
    }
    return false;
  }

  if (!inRange(leadHigh, 4000UL, 5000UL)) return false;

  uint32_t raw = 0;
  for (uint8_t i = 0; i < 32; ++i) {
    const unsigned long bitLow = pulseIn(_pins.ir, LOW, 1000UL);
    if (!inRange(bitLow, 350UL, 800UL)) return false;

    const unsigned long bitHigh = pulseIn(_pins.ir, HIGH, 2200UL);
    if (inRange(bitHigh, 350UL, 900UL)) {
      // logical zero
    } else if (inRange(bitHigh, 1300UL, 2000UL)) {
      raw |= (uint32_t(1) << i);
    } else {
      return false;
    }
  }

  frame.raw = raw;

  const uint8_t b0 = uint8_t(raw & 0xFFUL);
  const uint8_t b1 = uint8_t((raw >> 8) & 0xFFUL);
  const uint8_t b2 = uint8_t((raw >> 16) & 0xFFUL);
  const uint8_t b3 = uint8_t((raw >> 24) & 0xFFUL);

  if (uint8_t(b2 ^ b3) != 0xFFU) return false;

  if (uint8_t(b0 ^ b1) == 0xFFU) {
    frame.address = b0;
  } else {
    frame.address = uint16_t(b0) | (uint16_t(b1) << 8);
  }

  frame.command = b2;
  frame.ok = true;
  return true;
}

void HY_M302::gpio7Mode(uint8_t mode) {
  pinMode(_pins.gpio7, mode);
}

void HY_M302::gpio8Mode(uint8_t mode) {
  pinMode(_pins.gpio8, mode);
}

int HY_M302::gpio7Read() const {
  return digitalRead(_pins.gpio7);
}

int HY_M302::gpio8Read() const {
  return digitalRead(_pins.gpio8);
}

void HY_M302::gpio7Write(uint8_t value) {
  digitalWrite(_pins.gpio7, value);
}

void HY_M302::gpio8Write(uint8_t value) {
  digitalWrite(_pins.gpio8, value);
}
