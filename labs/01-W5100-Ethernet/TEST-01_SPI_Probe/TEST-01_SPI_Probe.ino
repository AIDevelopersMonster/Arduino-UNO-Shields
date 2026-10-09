/**
 * Arduino UNO & Shields | LAB-01B (blue W5100, no microSD)
 * TEST-01: Raw SPI Register Probe
 *
 * Purpose: Verify W5100 register-level access without Ethernet library or cable.
 * Target: Arduino UNO / ATmega328P (16 MHz, 32 KB flash, 2 KB SRAM).
 * Hardware: Blue WIZnet W5100 Ethernet Shield (HanRun RJ45 sample).
 * Setup: SPI Mode 0 at 1 MHz; W5100 D10; disabled SD D4; no microSD.
 * Serial monitor: 115200 baud. Build: arduino-cli --fqbn arduino:avr:uno.
 * Expected: PASS: RTR test pattern 0x1234 is read back and original RTR restored.
 * Limitations: No proof of Ethernet PHY, DHCP, link, HTTP or SD.
 * Source: https://github.com/AIDevelopersMonster/Arduino-UNO-Shields
 * Evidence: see sibling RESULT_2026-10-09.md when the test is certified.
 * Documentation-only revision: operational logic preserved.
 */

// The core SPI library is sufficient; no Ethernet stack is used in this test.
#include <SPI.h>
const uint8_t ETH_CS = 10;
const uint8_t SD_CS = 4;
SPISettings cfg(1000000, MSBFIRST, SPI_MODE0);
// Read one W5100 register using the legacy 4-byte SPI transaction (0x0F).
uint8_t rd(uint16_t a) {
  SPI.beginTransaction(cfg);
  digitalWrite(ETH_CS, LOW);
  SPI.transfer(0x0F); SPI.transfer(a >> 8); SPI.transfer(a & 255);
  uint8_t v = SPI.transfer(0);
  digitalWrite(ETH_CS, HIGH);
  SPI.endTransaction();
  return v;
}
// Write one W5100 register using opcode 0xF0; caller controls test values.
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
// Execute a reversible write/read/restore test on the retry-time register (RTR).
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
// One-shot diagnostic: no periodic work is necessary.
void loop() {}
