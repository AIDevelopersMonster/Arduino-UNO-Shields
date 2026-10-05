#include <HY_M302.h>

HY_M302 shield;

void setup() {
  shield.begin();
}

void loop() {
  shield.buzzerTone(523, 250);
  delay(500);
  shield.buzzerTone(659, 250);
  delay(500);
  shield.buzzerTone(784, 250);
  delay(1000);
}
