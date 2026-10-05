#include <HY_M302.h>

HY_M302 shield;

void setup() {
  Serial.begin(115200);
  shield.begin();

  Serial.println(F("HY-M302 IR NEC monitor"));
  Serial.println(F("Point an NEC-compatible remote at the onboard IR receiver."));
}

void loop() {
  HY_M302::IrNecFrame frame;

  if (!shield.readIrNec(frame, 5000UL)) return;

  if (frame.repeat) {
    Serial.println(F("IR REPEAT"));
    return;
  }

  Serial.print(F("IR RAW=0x"));
  Serial.print(frame.raw, HEX);
  Serial.print(F(" ADDR=0x"));
  Serial.print(frame.address, HEX);
  Serial.print(F(" CMD=0x"));
  Serial.println(frame.command, HEX);
}
