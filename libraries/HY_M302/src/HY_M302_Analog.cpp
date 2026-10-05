#include "HY_M302.h"

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
