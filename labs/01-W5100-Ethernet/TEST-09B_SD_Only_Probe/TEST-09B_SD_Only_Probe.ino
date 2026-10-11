/*
 * LAB-01B TEST-09B diagnostic: microSD-only SPI probe (no Ethernet library).
 * W5100 deselected on D10, microSD selected by SD.begin(D4).
 * Read-only operation: NO file created, deleted, formatted or modified.
 * Printed serial output only AFTER SD initialization.
 */
#include <SPI.h>
#include <SD.h>
const byte ETH_CS=10;
const byte SD_CS=4;
void setup() {
  Serial.begin(115200);
  pinMode(ETH_CS,OUTPUT);
  digitalWrite(ETH_CS,HIGH);
  pinMode(SD_CS,OUTPUT);
  digitalWrite(SD_CS,HIGH);
  delay(400);
  Serial.println(F("TEST-09B SD-ONLY PROBE / NO ETHERNET"));
  bool ok=SD.begin(SD_CS);
  Serial.print(F("SD.begin(4): "));
  Serial.println(ok ? F("PASS") : F("FAIL"));
  Serial.println(F("TEST COMPLETE"));
}
void loop() {}
