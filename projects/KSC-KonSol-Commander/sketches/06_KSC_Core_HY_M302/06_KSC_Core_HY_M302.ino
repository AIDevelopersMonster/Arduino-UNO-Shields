#include <Arduino.h>
#include <KSC_Core.h>

#include "KSC_HYM302Target.h"

KscHyM302Target target;
KscCore ksc(Serial, target);

void setup() {
  Serial.begin(115200);
  delay(100);

  ksc.begin();
}

void loop() {
  ksc.service();
}
