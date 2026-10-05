#include <HY_M302.h>

HY_M302 shield;

void setup() {
  Serial.begin(115200);
  shield.begin();

  Serial.println(F("HY-M302 TEST-01"));
  Serial.println(F("D9 -> D10 -> D11 -> D12 -> D13"));
}

void loop() {
  shield.setRgbRaw(255, 0, 0);
  delay(2000);
  shield.setRgbRaw(0, 255, 0);
  delay(2000);
  shield.setRgbRaw(0, 0, 255);
  delay(2000);
  shield.rgbOff();

  shield.led2(true);
  delay(2000);
  shield.led2(false);

  shield.led1(true);
  delay(2000);
  shield.led1(false);

  Serial.print(F("SW1="));
  Serial.print(shield.button1Pressed());
  Serial.print(F(" SW2="));
  Serial.print(shield.button2Pressed());
  Serial.print(F(" POT="));
  Serial.print(shield.readPotRaw());
  Serial.print(F(" LIGHT="));
  Serial.print(shield.readLightRaw());
  Serial.print(F(" LM35="));
  Serial.println(shield.readLm35Raw());

  delay(1000);
}
