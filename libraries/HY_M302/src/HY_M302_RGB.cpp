#include "HY_M302.h"

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
