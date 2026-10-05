#include "HY_M302.h"

void HY_M302::buzzerOn() {
  noTone(_pins.buzzer);
  pinMode(_pins.buzzer, OUTPUT);
  digitalWrite(_pins.buzzer, HIGH);
}

void HY_M302::buzzerTone(unsigned int frequency, unsigned long durationMs) {
  if (durationMs == 0) tone(_pins.buzzer, frequency);
  else tone(_pins.buzzer, frequency, durationMs);
}

void HY_M302::buzzerOff() {
  noTone(_pins.buzzer);
  digitalWrite(_pins.buzzer, LOW);
}
