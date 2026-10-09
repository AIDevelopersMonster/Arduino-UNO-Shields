#include <SPI.h>
const uint8_t ETH_CS = 10;
const uint8_t SD_CS = 4;
SPISettings cfg(1000000, MSBFIRST, SPI_MODE0);
uint8_t rd(uint16_t a) {
  SPI.beginTransaction(cfg);
  digitalWrite(ETH_CS, LOW);
  SPI.transfer(0x0F); SPI.transfer(a >> 8); SPI.transfer(a & 255);
  uint8_t v = SPI.transfer(0);
  digitalWrite(ETH_CS, HIGH);
  SPI.endTransaction();
  return v;
}
void wr(uint16_t a, uint8_t v) {
  SPI.beginTransaction(cfg);
  digitalWrite(ETH_CS, LOW);
  SPI.transfer(0xF0); SPI.transfer(a >> 8); SPI.transfer(a & 255);
  SPI.transfer(v);
  digitalWrite(ETH_CS, HIGH);
  SPI.endTransaction();
}
void hex8(uint8_t v) {
  if (v < 16) Serial.print('0');
  Serial.print(v, HEX);
}
void setup() {
  Serial.begin(115200);
  pinMode(ETH_CS, OUTPUT); digitalWrite(ETH_CS, HIGH);
  pinMode(SD_CS, OUTPUT); digitalWrite(SD_CS, HIGH);
  SPI.begin(); delay(300);
  Serial.println(F("KON LAB W5100 TEST-01 / SPI 1MHz / CS D10 / SD OFF"));
  uint8_t mr = rd(0x0000), hi = rd(0x0017), lo = rd(0x0018);
  Serial.print(F("MR=0x")); hex8(mr); Serial.println();
  Serial.print(F("RTR original=0x")); hex8(hi); hex8(lo); Serial.println();
  wr(0x0017, 0x12); wr(0x0018, 0x34);
  uint8_t a = rd(0x0017), b = rd(0x0018);
  wr(0x0017, hi); wr(0x0018, lo);
  bool restored = rd(0x0017) == hi && rd(0x0018) == lo;
  Serial.print(F("RTR test=0x")); hex8(a); hex8(b); Serial.println();
  Serial.print(F("Restore=")); Serial.println(restored ? F("OK") : F("FAIL"));
  Serial.println((a == 0x12 && b == 0x34 && restored) ?
    F("RESULT: PASS / W5100 SPI READ WRITE OK") :
    F("RESULT: FAIL / W5100 SPI NOT VERIFIED"));
}
void loop() {}
