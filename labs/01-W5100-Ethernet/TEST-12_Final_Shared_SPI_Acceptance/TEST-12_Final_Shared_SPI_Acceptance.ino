/*
 * TEST-12 — FINAL SHARED SPI ACCEPTANCE
 *
 * Arduino UNO + W5100 Ethernet Shield + microSD
 * Final bounded acceptance test for shared SPI operation.
 *
 * Uses ONLY stock Arduino libraries:
 *   SPI, SD, Ethernet, EthernetUDP
 *
 * Automatic sequence after power-up:
 *   1. 1500 ms power-settle delay
 *   2. SD #1: init -> write -> close -> read/verify -> SD.end()
 *   3. LAN: DHCP -> UDP socket open -> close
 *   4. SD #2: init -> write -> close -> read/verify -> SD.end()
 *
 * No operator commands, no retries, no W5100 raw reset.
 * PASS requires every stage to pass in one uninterrupted run.
 *
 * D10 = W5100 CS
 * D4  = microSD CS
 */

#include <SPI.h>
#include <SD.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <string.h>

const uint8_t ETH_CS = 10;
const uint8_t SD_CS  = 4;
const uint16_t UDP_PORT = 5007;

const char TEST_FILE[] = "K12.TXT";
const char SD1_TEXT[]  = "KON TEST12 SD1 PASS";
const char SD2_TEXT[]  = "KON TEST12 SD2 PASS";

byte mac[] = {0x02, 0x4B, 0x4F, 0x4E, 0x5C, 0x12};

EthernetUDP udp;

void parkBus() {
  pinMode(ETH_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(ETH_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
}

void idleClocks(uint8_t bytes) {
  parkBus();
  SPI.begin();
  SPI.beginTransaction(SPISettings(250000UL, MSBFIRST, SPI_MODE0));
  while (bytes--) {
    SPI.transfer(0xFF);
  }
  SPI.endTransaction();
}

bool verifyFile(const char *expected) {
  File f = SD.open(TEST_FILE, FILE_READ);
  if (!f) return false;

  bool ok = true;
  const size_t n = strlen(expected);

  for (size_t i = 0; i < n; ++i) {
    if (f.read() != (int)(uint8_t)expected[i]) ok = false;
  }

  // println() emits CR LF on AVR Print.
  if (f.read() != '\r') ok = false;
  if (f.read() != '\n') ok = false;
  if (f.available() != 0) ok = false;

  f.close();
  return ok;
}

bool testSD(const char *label, const char *text) {
  parkBus();

  Serial.print(label);
  Serial.println(F(" INIT..."));

  if (!SD.begin(SD_CS)) {
    Serial.print(label);
    Serial.println(F(" INIT FAIL"));
    digitalWrite(SD_CS, HIGH);
    return false;
  }

  Serial.print(label);
  Serial.println(F(" INIT PASS"));

  if (SD.exists(TEST_FILE)) {
    if (!SD.remove(TEST_FILE)) {
      Serial.print(label);
      Serial.println(F(" REMOVE FAIL"));
      SD.end();
      digitalWrite(SD_CS, HIGH);
      return false;
    }
  }

  File f = SD.open(TEST_FILE, FILE_WRITE);
  if (!f) {
    Serial.print(label);
    Serial.println(F(" OPEN WRITE FAIL"));
    SD.end();
    digitalWrite(SD_CS, HIGH);
    return false;
  }

  size_t wrote = f.println(text);
  f.close();

  if (wrote != strlen(text) + 2) {
    Serial.print(label);
    Serial.println(F(" WRITE FAIL"));
    SD.end();
    digitalWrite(SD_CS, HIGH);
    return false;
  }

  Serial.print(label);
  Serial.println(F(" WRITE PASS"));

  bool verified = verifyFile(text);

  Serial.print(label);
  Serial.println(verified ? F(" READ VERIFY PASS") : F(" READ VERIFY FAIL"));

  SD.end();
  digitalWrite(SD_CS, HIGH);

  // Give the card trailing clocks while neither slave is selected.
  idleClocks(2);

  return verified;
}

bool testLAN() {
  parkBus();

  // SD must remain deselected for every Ethernet transaction.
  digitalWrite(SD_CS, HIGH);

  Serial.println(F("LAN INIT / DHCP..."));

  Ethernet.init(ETH_CS);

  if (Ethernet.begin(mac, 10000, 2000) == 0) {
    Serial.println(F("LAN DHCP FAIL"));
    digitalWrite(ETH_CS, HIGH);
    return false;
  }

  Serial.print(F("LAN DHCP PASS IP="));
  Serial.println(Ethernet.localIP());

  if (udp.begin(UDP_PORT) == 0) {
    Serial.println(F("LAN UDP BEGIN FAIL"));
    digitalWrite(ETH_CS, HIGH);
    return false;
  }

  Serial.print(F("LAN UDP PASS PORT="));
  Serial.println(UDP_PORT);

  udp.stop();
  digitalWrite(ETH_CS, HIGH);

  Serial.println(F("LAN STOP / D10 HIGH"));

  // Known idle boundary before returning to SD.
  idleClocks(4);

  return true;
}

void setup() {
  Serial.begin(115200);

  parkBus();
  SPI.begin();

  Serial.println();
  Serial.println(F("KON TEST-12 / FINAL SHARED SPI ACCEPTANCE"));
  Serial.println(F("UNO + W5100 + microSD / STOCK LIBRARIES ONLY"));
  Serial.println(F("POWER SETTLE 1500 ms..."));

  delay(1500);

  bool sd1 = testSD("SD1", SD1_TEXT);
  if (!sd1) {
    Serial.println(F("FINAL: FAIL / SD BASELINE"));
    return;
  }

  bool lan = testLAN();
  if (!lan) {
    Serial.println(F("FINAL: FAIL / LAN AFTER SD"));
    return;
  }

  bool sd2 = testSD("SD2", SD2_TEXT);
  if (!sd2) {
    Serial.println(F("FINAL: FAIL / SD AFTER LAN"));
    return;
  }

  Serial.println(F("FINAL: FULL PASS / SD -> LAN -> SD"));
}

void loop() {
}
