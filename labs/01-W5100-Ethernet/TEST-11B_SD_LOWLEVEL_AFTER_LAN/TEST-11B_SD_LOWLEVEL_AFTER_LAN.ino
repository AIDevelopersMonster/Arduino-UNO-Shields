/*
 * TEST-11B — Sd2Card low-level init before/after W5100 activity.
 *
 * Uses Arduino SD's proven Sd2Card implementation directly.
 * No filesystem mount, no root directory, no file I/O.
 *
 * Commands:
 *   ?   help
 *   C   low-level Sd2Card::init() probe on D4
 *   L1  start W5100 DHCP + UDP 5006
 *   L0  stop UDP + park LAN CS
 *   R   raw W5100 MR software reset
 *
 * Recommended sequence:
 *   C
 *   C
 *   L1
 *   L0
 *   C
 *
 * A post-LAN C failure prints the native Sd2Card errorCode/errorData.
 */

#include <SPI.h>
#include <SD.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <KON_SPI_Shared.h>
#include <string.h>

const uint8_t SD_CS = 4;
const uint8_t ETH_CS = 10;
const uint16_t UDP_PORT = 5006;

byte mac[] = {0x02,0x4B,0x4F,0x4E,0x5B,0x1B};

KON_SPI_Shared bus(SD_CS, ETH_CS);
EthernetUDP udp;
Sd2Card card;

char cmd[8];
uint8_t used = 0;
bool lanOn = false;

static const __FlashStringHelper* sdErrorName(uint8_t e) {
  switch (e) {
    case 0x00: return F("NONE");
    case 0x01: return F("CMD0");
    case 0x02: return F("CMD8");
    case 0x03: return F("CMD17");
    case 0x04: return F("CMD24");
    case 0x05: return F("CMD25");
    case 0x06: return F("CMD58");
    case 0x07: return F("ACMD23");
    case 0x08: return F("ACMD41");
    case 0x09: return F("BAD_CSD");
    case 0x0A: return F("ERASE");
    case 0x0B: return F("ERASE_SINGLE");
    case 0x0C: return F("ERASE_TIMEOUT");
    case 0x0D: return F("READ");
    case 0x0E: return F("READ_REG");
    case 0x0F: return F("READ_TIMEOUT");
    case 0x10: return F("STOP_TRAN");
    case 0x11: return F("WRITE");
    case 0x12: return F("WRITE_BLOCK_ZERO");
    case 0x13: return F("WRITE_MULTIPLE");
    case 0x14: return F("WRITE_PROGRAMMING");
    case 0x15: return F("WRITE_TIMEOUT");
    case 0x16: return F("SCK_RATE");
    default:   return F("UNKNOWN");
  }
}

void help() {
  Serial.println(F("COMMANDS: ? / C / L1 / L0 / R"));
  Serial.println(F("C : Sd2Card::init low-level probe, no filesystem"));
  Serial.println(F("L1: W5100 DHCP + UDP 5006"));
  Serial.println(F("L0: stop UDP + park LAN CS"));
  Serial.println(F("R : raw W5100 MR software reset"));
  Serial.println(F("SEQUENCE: C, C, L1, L0, C"));
}

void doCardProbe() {
  if (lanOn) {
    Serial.println(F("BLOCK: SEND L0 FIRST"));
    return;
  }

  bus.prepareSD();

  unsigned long t0 = millis();
  uint8_t ok = card.init(SPI_HALF_SPEED, SD_CS);
  unsigned long dt = millis() - t0;

  uint8_t ec = card.errorCode();
  uint8_t ed = card.errorData();

  bus.releaseSD(2);

  Serial.print(F("C init="));
  Serial.print(ok ? F("PASS") : F("FAIL"));
  Serial.print(F(" ms="));
  Serial.print(dt);
  Serial.print(F(" error=0x"));
  if (ec < 0x10) Serial.print('0');
  Serial.print(ec, HEX);
  Serial.print(' ');
  Serial.print(sdErrorName(ec));
  Serial.print(F(" data=0x"));
  if (ed < 0x10) Serial.print('0');
  Serial.print(ed, HEX);

  if (ok) {
    Serial.print(F(" type="));
    Serial.print(card.type());
  }
  Serial.println();
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
}

void doR() {
  if (lanOn) {
    Serial.println(F("BLOCK: SEND L0 FIRST"));
    return;
  }

  Serial.print(F("R W5100 soft reset: "));
  Serial.println(bus.resetW5100(true) ? F("PASS") : F("FAIL"));
}

void execute() {
  cmd[used] = 0;
  used = 0;

  if (!strcmp(cmd, "?")) help();
  else if (!strcmp(cmd, "C")) doCardProbe();
  else if (!strcmp(cmd, "L1")) doL1();
  else if (!strcmp(cmd, "L0")) doL0();
  else if (!strcmp(cmd, "R")) doR();
  else help();
}

void setup() {
  Serial.begin(115200);
  bus.begin();

  Serial.println(F("KON TEST-11B / SD LOW-LEVEL INIT AFTER LAN"));
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
