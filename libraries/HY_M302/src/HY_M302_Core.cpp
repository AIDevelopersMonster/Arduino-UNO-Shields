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

  // Safe startup state without cross-module calls. This keeps begin() small
  // and lets the linker omit unused RGB/buzzer modules.
  digitalWrite(_pins.buzzer, LOW);
  digitalWrite(_pins.rgb1, LOW);
  digitalWrite(_pins.rgb2, LOW);
  digitalWrite(_pins.rgb3, LOW);
  digitalWrite(_pins.led1, LOW);
  digitalWrite(_pins.led2, LOW);
}

void HY_M302::service() {
  // Each async service is responsible for returning immediately when disabled.
  serviceIrNec();
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
