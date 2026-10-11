/*
 * TEST-11 — RAW SD CMD0 AFTER LAN
 * Purpose: distinguish SD-library lifecycle failure from lower-level
 * SPI/electrical failure after W5100 activity.
 *
 * No SD.h is used.
 * Commands:
 *   ?   status/help
 *   C0  raw SD CMD0 probe (expects R1=0x01)
 *   L1  start W5100 DHCP + UDP 5005
 *   L0  stop UDP and park LAN CS
 *   R   raw W5100 software reset
 *
 * Recommended sequence:
 *   C0
 *   L1
 *   L0
 *   C0
 *
 * If CMD0 passes before LAN and fails after LAN, the fault is below
 * the SD filesystem wrapper.
 */
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <KON_SPI_Shared.h>
#include <string.h>

const uint8_t SD_CS = 4;
const uint8_t ETH_CS = 10;
const uint16_t UDP_PORT = 5005;

byte mac[] = {0x02,0x4B,0x4F,0x4E,0x5B,0x11};

KON_SPI_Shared bus(SD_CS, ETH_CS);
EthernetUDP udp;

char cmd[8];
uint8_t used = 0;
bool lanOn = false;
static bool rawReady = false;
static uint16_t rawReadyPolls = 0;

static uint8_t rawSdCmd0() {
  // Known idle boundary: both CS high, 80 clocks at 250 kHz.
  pinMode(SD_CS, OUTPUT);
  pinMode(ETH_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  digitalWrite(ETH_CS, HIGH);

  SPI.begin();
  SPI.beginTransaction(SPISettings(250000UL, MSBFIRST, SPI_MODE0));

  for (uint8_t i = 0; i < 10; ++i) SPI.transfer(0xFF);

  digitalWrite(SD_CS, LOW);

  // Mirror Sd2Card::cardCommand() more closely: after selecting the card,
  // wait until DO/MISO returns 0xFF (not busy) before sending CMD0.
  rawReady = false;
  rawReadyPolls = 0;
  unsigned long t0 = millis();
  do {
    uint8_t b = SPI.transfer(0xFF);
    ++rawReadyPolls;
    if (b == 0xFF) {
      rawReady = true;
      break;
    }
  } while ((unsigned long)(millis() - t0) < 300UL);

  if (!rawReady) {
    digitalWrite(SD_CS, HIGH);
    SPI.transfer(0xFF);
    SPI.endTransaction();
    return 0xFF;
  }

  // CMD0, arg=0, valid CRC=0x95.
  SPI.transfer(0x40);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer(0x95);

  uint8_t r1 = 0xFF;
  for (uint16_t i = 0; i < 256; ++i) {
    r1 = SPI.transfer(0xFF);
    if ((r1 & 0x80) == 0) break;
  }

  digitalWrite(SD_CS, HIGH);
  SPI.transfer(0xFF); // 8 trailing clocks with card deselected.
  SPI.endTransaction();

  return r1;
}

static uint8_t sampleMisoPullup() {
  // Passive diagnostic only: both slaves deselected.
  digitalWrite(SD_CS, HIGH);
  digitalWrite(ETH_CS, HIGH);
  pinMode(MISO, INPUT_PULLUP);
  delayMicroseconds(10);
  uint8_t v = digitalRead(MISO);
  pinMode(MISO, INPUT);
  return v;
}

void help() {
  Serial.println(F("COMMANDS: ? / C0 / L1 / L0 / R"));
  Serial.println(F("C0: raw SD CMD0; PASS requires R1=0x01"));
  Serial.println(F("L1: W5100 DHCP + UDP 5005"));
  Serial.println(F("L0: stop UDP + park LAN CS"));
  Serial.println(F("R : raw W5100 MR software reset"));
}

void status() {
  Serial.print(F("STATE lan="));
  Serial.print(lanOn ? F("ON") : F("OFF"));
  Serial.print(F(" D4="));
  Serial.print(digitalRead(SD_CS) ? F("HIGH") : F("LOW"));
  Serial.print(F(" D10="));
  Serial.print(digitalRead(ETH_CS) ? F("HIGH") : F("LOW"));
  Serial.print(F(" MISO_PULLUP="));
  Serial.println(sampleMisoPullup() ? F("HIGH") : F("LOW"));
}

void doC0() {
  if (lanOn) {
    Serial.println(F("BLOCK: SEND L0 FIRST"));
    return;
  }
  uint8_t before = sampleMisoPullup();
  uint8_t r1 = rawSdCmd0();
  uint8_t after = sampleMisoPullup();

  Serial.print(F("C0 MISO before="));
  Serial.print(before ? F("HIGH") : F("LOW"));
  Serial.print(F(" ready="));
  Serial.print(rawReady ? F("YES") : F("NO"));
  Serial.print(F(" polls="));
  Serial.print(rawReadyPolls);
  Serial.print(F(" R1=0x"));
  if (r1 < 0x10) Serial.print('0');
  Serial.print(r1, HEX);
  Serial.print(F(" after="));
  Serial.println(after ? F("HIGH") : F("LOW"));

  if (r1 == 0x01) Serial.println(F("C0 RAW CMD0: PASS"));
  else Serial.println(F("C0 RAW CMD0: FAIL"));
}

void doL1() {
  if (lanOn) {
    Serial.println(F("LAN ALREADY ON"));
    return;
  }

  bus.prepareLAN();
  Serial.println(F("L1 prepareLAN: OK"));

  bool rst = bus.resetW5100(true);
  Serial.print(F("L1 W5100 soft reset: "));
  Serial.println(rst ? F("PASS") : F("FAIL"));

  bus.prepareLAN();
  Ethernet.init(ETH_CS);

  if (Ethernet.begin(mac, 10000, 2000) == 0) {
    Serial.println(F("L1 DHCP: FAIL"));
    bus.releaseLAN();
    return;
  }

  if (!udp.begin(UDP_PORT)) {
    Serial.println(F("L1 UDP begin: FAIL"));
    bus.releaseLAN();
    return;
  }

  lanOn = true;
  Serial.print(F("L1 LAN PASS IP="));
  Serial.println(Ethernet.localIP());
  Serial.print(F("UDP="));
  Serial.println(UDP_PORT);
}

void doL0() {
  if (lanOn) udp.stop();
  bus.releaseLAN();
  lanOn = false;
  Serial.println(F("L0 UDP stop + LAN release: DONE"));
  status();
}

void doR() {
  if (lanOn) {
    Serial.println(F("BLOCK: SEND L0 FIRST"));
    return;
  }
  Serial.print(F("R W5100 soft reset: "));
  Serial.println(bus.resetW5100(true) ? F("PASS") : F("FAIL"));
  status();
}

void execute() {
  cmd[used] = 0;
  used = 0;

  if (!strcmp(cmd, "?")) {
    status();
    help();
  } else if (!strcmp(cmd, "C0")) {
    doC0();
  } else if (!strcmp(cmd, "L1")) {
    doL1();
  } else if (!strcmp(cmd, "L0")) {
    doL0();
  } else if (!strcmp(cmd, "R")) {
    doR();
  } else {
    help();
  }
}

void setup() {
  Serial.begin(115200);
  bus.begin();
  Serial.println(F("KON TEST-11 / RAW SD CMD0 AFTER LAN"));
  status();
  help();
}

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      if (used) execute();
    } else if (used < sizeof(cmd) - 1 && c >= 32 && c <= 126) {
      cmd[used++] = c;
    }
  }
}
