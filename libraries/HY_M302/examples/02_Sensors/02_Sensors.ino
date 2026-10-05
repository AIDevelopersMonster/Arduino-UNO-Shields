#include <HY_M302.h>

HY_M302 shield;

void setup() {
  Serial.begin(115200);
  shield.begin();
}

void loop() {
  HY_M302::DhtReading dht = shield.readDht11();

  Serial.print(F("POT="));
  Serial.print(shield.readPotRaw());

  Serial.print(F(" LIGHT="));
  Serial.print(shield.readLightRaw());

  Serial.print(F(" LM35_C="));
  Serial.print(shield.readLm35C(), 1);

  Serial.print(F(" DHT="));
  if (dht.ok) {
    Serial.print(dht.temperatureC, 1);
    Serial.print(F("C "));
    Serial.print(dht.humidity, 1);
    Serial.println(F("%"));
  } else {
    Serial.println(F("ERR"));
  }

  delay(2000);
}
