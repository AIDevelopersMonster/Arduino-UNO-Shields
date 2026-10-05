/*
  HY-M302 DHT11 reference test

  Purpose:
    Verify the physical onboard DHT11 on D4 using the established
    Adafruit DHT sensor library, independently of the HY_M302 library.

  Required Arduino libraries:
    DHT sensor library by Adafruit
    Adafruit Unified Sensor (dependency)

  Hardware:
    Arduino UNO + HY-M302
    onboard DHT11 DATA = D4

  Serial:
    115200 baud
*/

#include <DHT.h>

static const uint8_t DHT_PIN = 4;
static const uint8_t DHT_TYPE = DHT11;

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();

  Serial.println(F("HY-M302 DHT11 ADAFRUIT REFERENCE TEST"));
  Serial.println(F("DHT11 DATA = D4"));
  Serial.println(F("Reading every 2 seconds..."));
}

void loop() {
  delay(2000);

  const float humidity = dht.readHumidity();
  const float temperatureC = dht.readTemperature();

  if (isnan(humidity) || isnan(temperatureC)) {
    Serial.println(F("DHT11 ERR"));
    return;
  }

  Serial.print(F("DHT11 OK T="));
  Serial.print(temperatureC, 1);
  Serial.print(F(" C RH="));
  Serial.print(humidity, 1);
  Serial.println(F(" %"));
}
