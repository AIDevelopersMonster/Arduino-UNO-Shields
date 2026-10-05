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

void HY_M302::setRgbRaw(uint8_t ch1, uint8_t ch2, uint8_t ch3) {
  analogWrite(_pins.rgb1, ch1);
  analogWrite(_pins.rgb2, ch2);
  analogWrite(_pins.rgb3, ch3);
}

void HY_M302::rgbOff() {
  setRgbRaw(0, 0, 0);
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

bool HY_M302::readDhtBit(uint8_t pin, uint8_t& bit) {
  unsigned long start = micros();
  while (digitalRead(pin) == LOW) {
    if (micros() - start > 100) return false;
  }

  const unsigned long highStart = micros();
  while (digitalRead(pin) == HIGH) {
    if (micros() - highStart > 120) return false;
  }

  const unsigned long highLen = micros() - highStart;
  bit = highLen > 40 ? 1 : 0;
  return true;
}

HY_M302::DhtReading HY_M302::readDht11() {
  DhtReading out = {0.0f, 0.0f, false};
  uint8_t data[5] = {0,0,0,0,0};

  noInterrupts();
  pinMode(_pins.dht, OUTPUT);
  digitalWrite(_pins.dht, LOW);
  delay(18);
  digitalWrite(_pins.dht, HIGH);
  delayMicroseconds(30);
  pinMode(_pins.dht, INPUT_PULLUP);

  unsigned long start = micros();
  while (digitalRead(_pins.dht) == HIGH) {
    if (micros() - start > 100) { interrupts(); return out; }
  }
  start = micros();
  while (digitalRead(_pins.dht) == LOW) {
    if (micros() - start > 100) { interrupts(); return out; }
  }
  start = micros();
  while (digitalRead(_pins.dht) == HIGH) {
    if (micros() - start > 100) { interrupts(); return out; }
  }

  for (uint8_t i = 0; i < 40; ++i) {
    uint8_t b = 0;
    if (!readDhtBit(_pins.dht, b)) {
      interrupts();
      return out;
    }
    data[i / 8] <<= 1;
    data[i / 8] |= b;
  }
  interrupts();

  const uint8_t checksum = uint8_t(data[0] + data[1] + data[2] + data[3]);
  if (checksum != data[4]) return out;

  out.humidity = data[0] + data[1] * 0.1f;
  out.temperatureC = data[2] + data[3] * 0.1f;
  out.ok = true;
  return out;
}
