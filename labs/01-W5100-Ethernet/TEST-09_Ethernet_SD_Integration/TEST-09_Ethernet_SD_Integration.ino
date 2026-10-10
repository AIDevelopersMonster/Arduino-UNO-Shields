// TEST-09 v0.2: UDP -> exclusive scratch file -> uncached SD read -> UDP.
// UNO/W5100, SD 1.3.0, Ethernet 2.0.2. No heap-allocating File wrapper.
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SD.h>
#include <string.h>
#include <stdlib.h>

const uint8_t ETH_CS = 10, SD_CS = 4;
const uint16_t UDP_PORT = 5001, CAPACITY = 128, RTR_ADDRESS = 0x0017;
const char FILE_NAME[] = "T09CHECK.BIN";
byte mac[] = {0x02, 0x4B, 0x4F, 0x4E, 0x51, 0x09};
EthernetUDP udp;
Sd2Card card;
SdVolume volume;
SdFile root, file;
uint8_t packet[CAPACITY];
char command[20], token[9];
uint8_t commandLength = 0;
bool overflow = false, used = false, active = false, ownsFile = false;
bool sdReady = false, ethernetReady = false, udpBound = false, busFault = false;
uint16_t received = 0, verified = 0, sent = 0;
uint32_t bytes = 0;
uint8_t errors = 0;
uint32_t started = 0, durationMs = 0, statTick = 0, maintainTick = 0, maxSdMs = 0;
uint16_t originalRtr = 0, finalCrc = 0;
int16_t minFree = 32767, initialFree = 0;
IPAddress boundIP;
extern int __heap_start;
extern void *__brkval;
int16_t freeRam() { int top; return (int)&top - (__brkval ? (int)__brkval : (int)&__heap_start); }
void sampleRam() { int16_t value = freeRam(); if (value < minFree) minFree = value; }
void prefix(const __FlashStringHelper *kind) { Serial.print(kind); Serial.print(F(" token=")); Serial.print(token); }
void initializeChipSelects() {
  digitalWrite(ETH_CS, HIGH); digitalWrite(SD_CS, HIGH);
  pinMode(ETH_CS, OUTPUT); pinMode(SD_CS, OUTPUT);
  SPI.begin();
}
bool releaseBus() {
  if (busFault || digitalRead(ETH_CS) != HIGH) { busFault = true; return false; }
  if (sdReady) card.readEnd();
  if (digitalRead(SD_CS) != HIGH) { busFault = true; return false; }
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(0xFF); // Complete SD handoff before the next Ethernet transaction.
  SPI.endTransaction();
  return true;
}
uint8_t readEthernet(uint16_t address) {
  if (!releaseBus()) return 0xFF;
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  digitalWrite(ETH_CS, LOW);
  SPI.transfer(0x0F); SPI.transfer(address >> 8); SPI.transfer(address & 0xFF);
  uint8_t value = SPI.transfer(0);
  digitalWrite(ETH_CS, HIGH); SPI.endTransaction(); return value;
}
uint16_t readRtr() { uint16_t high = readEthernet(RTR_ADDRESS); return (high << 8) | readEthernet(RTR_ADDRESS + 1); }
void hex16(uint16_t value) {
  if (value < 0x1000) Serial.print('0'); if (value < 0x100) Serial.print('0');
  if (value < 0x10) Serial.print('0'); Serial.print(value, HEX);
}
uint16_t crc16(uint16_t length) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < length; ++i) {
    crc ^= (uint16_t)packet[i] << 8;
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
  }
  return crc;
}
void fail(const __FlashStringHelper *stage) {
  ++errors; active = false;
  prefix(F("FAIL")); Serial.print(F(" stage=")); Serial.println(stage);
}
void finish(bool requested) {
  active = false;
  if (!busFault && file.isOpen() && !file.close()) ++errors;
  bool cleanup = false;
  if (requested && !errors && !busFault && ownsFile && root.isOpen()) {
    cleanup = SdFile::remove(&root, FILE_NAME);
    bool exists = file.open(&root, FILE_NAME, O_READ);
    if (exists) file.close();
    cleanup = cleanup && !exists;
    if (!cleanup) ++errors;
    ownsFile = !cleanup;
  }
  if (!busFault && root.isOpen() && !root.close()) ++errors;
  uint16_t first = 0, second = 0;
  if (ethernetReady && !busFault) {
    if (releaseBus()) {
      if (udpBound) { udp.stop(); udpBound = false; }
      first = readRtr(); second = readRtr();
    } else ++errors;
  }
  bool rtrOK = first == originalRtr && second == originalRtr && originalRtr != 0 && originalRtr != 0xFFFF;
  sampleRam();
  bool ramOK = minFree >= 512 && freeRam() >= initialFree - 64;
  bool passed = requested && !errors && !busFault && cleanup && rtrOK && ramOK && received >= 24 && received == verified && verified == sent;
  prefix(F("RESULT")); Serial.print(F(" status=")); Serial.print(passed ? F("PASS") : F("FAIL"));
  Serial.print(F(" rx=")); Serial.print(received); Serial.print(F(" verified=")); Serial.print(verified);
  Serial.print(F(" tx=")); Serial.print(sent); Serial.print(F(" bytes=")); Serial.print(bytes);
  Serial.print(F(" errors=")); Serial.print(errors); Serial.print(F(" cleanup=")); Serial.print(cleanup);
  Serial.print(F(" expected_rtr=")); hex16(originalRtr); Serial.print(F(" rtr1=")); hex16(first);
  Serial.print(F(" rtr2=")); hex16(second); Serial.print(F(" crc16=")); hex16(finalCrc);
  Serial.print(F(" initial_free=")); Serial.print(initialFree); Serial.print(F(" free=")); Serial.print(freeRam());
  Serial.print(F(" min_free=")); Serial.print(minFree); Serial.print(F(" max_sd_ms=")); Serial.print(maxSdMs);
  Serial.print(F(" bus_fault=")); Serial.print(busFault);
  Serial.print(F(" elapsed_ms=")); Serial.println(started ? millis() - started : 0);
}
bool prepare() {
  // Initialize SD before clocking the other slave on this cold/MCU-reset start.
  sdReady = card.init(SPI_HALF_SPEED, SD_CS);
  prefix(F("CARD")); Serial.print(F(" error_code=")); Serial.print(card.errorCode());
  Serial.print(F(" error_data=")); Serial.println(card.errorData());
  if (!sdReady) { fail(F("CARD_INIT")); return false; }
  uint32_t blocks = card.cardSize(); uint8_t type = card.type();
  bool mounted = volume.init(&card);
  prefix(F("SD")); Serial.print(F(" type=")); Serial.print(type); Serial.print(F(" blocks=")); Serial.print(blocks);
  Serial.print(F(" fat=")); Serial.println(volume.fatType());
  if (!blocks || type < 1 || type > 3 || !mounted || (volume.fatType() != 16 && volume.fatType() != 32)) { fail(F("CARD_VOLUME")); return false; }
  if (!root.openRoot(&volume)) { fail(F("ROOT_OPEN")); return false; }
  if (!file.open(&root, FILE_NAME, O_CREAT | O_EXCL | O_WRITE)) { fail(F("EXCLUSIVE_CREATE")); return false; }
  ownsFile = true;
  if (!file.close()) { fail(F("CREATE_CLOSE")); return false; }
  if (!releaseBus()) { fail(F("BUS_OWNERSHIP")); return false; }
  Ethernet.init(ETH_CS);
  int dhcp = Ethernet.begin(mac, 6000UL, 1000UL);
  sampleRam();
  if (dhcp != 1 || Ethernet.hardwareStatus() != EthernetW5100 || Ethernet.localIP() == IPAddress(0,0,0,0)) { fail(F("DHCP_OR_W5100")); return false; }
  ethernetReady = true;
  Ethernet.setRetransmissionTimeout(200); Ethernet.setRetransmissionCount(2);
  originalRtr = readRtr();
  if (!originalRtr || originalRtr == 0xFFFF || readRtr() != originalRtr) { fail(F("RTR_BEFORE")); return false; }
  if (!udp.begin(UDP_PORT)) { fail(F("UDP_BIND")); return false; }
  udpBound = true;
  boundIP = Ethernet.localIP(); initialFree = freeRam(); sampleRam();
  started = statTick = maintainTick = millis(); active = true;
  prefix(F("NET")); Serial.print(F(" ip=")); Serial.print(boundIP); Serial.print(F(" udp=5001 chip=W5100 rtr=")); hex16(originalRtr);
  Serial.print(F(" free=")); Serial.println(initialFree);
  return true;
}
bool sdRoundtrip(uint16_t length) {
  if (busFault || digitalRead(ETH_CS) != HIGH || digitalRead(SD_CS) != HIGH) return false;
  uint16_t expected = crc16(length);
  uint32_t tick = millis();
  // The name is owned by this run; never truncate an existing external file.
  bool written = file.open(&root, FILE_NAME, O_WRITE);
  written = written && file.truncate(0) && file.write(packet, length) == length && file.sync() && file.fileSize() == length;
  bool closed = file.isOpen() ? file.close() : false;
  if (!written || !closed || !SdVolume::cacheClear()) return false;
  // Cache discard makes the following data read reach the SD SPI device.
  bool opened = file.open(&root, FILE_NAME, O_READ);
  bool read = opened && file.fileSize() == length && file.read(packet, length) == (int16_t)length && file.read() == -1;
  closed = file.isOpen() ? file.close() : false;
  if (!read || !closed || crc16(length) != expected) return false;
  finalCrc = expected;
  sampleRam();
  uint32_t elapsed = millis() - tick; if (elapsed > maxSdMs) maxSdMs = elapsed;
  return digitalRead(ETH_CS) == HIGH && digitalRead(SD_CS) == HIGH;
}
void serviceUdp() {
  int length = udp.parsePacket(); if (length <= 0) return;
  if (length != 16 && length != 32 && length != 64 && length != 128) { fail(F("UDP_LENGTH")); return; }
  if (udp.read(packet, length) != length || memcmp(packet, token, 8) != 0) { fail(F("UDP_TOKEN_OR_READ")); return; }
  uint32_t seq = (uint32_t)packet[8] | ((uint32_t)packet[9] << 8) | ((uint32_t)packet[10] << 16) | ((uint32_t)packet[11] << 24);
  if (seq != received + 1 || received >= 10000) { fail(F("UDP_SEQUENCE_OR_LIMIT")); return; }
  IPAddress peer = udp.remoteIP(); uint16_t port = udp.remotePort(); ++received;
  if (!sdRoundtrip(length)) { fail(F("SD_WRITE_READ")); return; }
  ++verified; bytes += length;
  if (!releaseBus()) { fail(F("BUS_OWNERSHIP")); return; }
  bool ok = udp.beginPacket(peer, port) == 1;
  if (ok) { size_t count = udp.write(packet, length); int result = udp.endPacket(); ok = count == (size_t)length && result == 1; }
  if (!ok) { fail(F("UDP_SEND")); return; } ++sent;
  sampleRam();
}
void stats() {
  sampleRam(); prefix(F("STAT")); Serial.print(F(" elapsed_ms=")); Serial.print(millis() - started);
  Serial.print(F(" rx=")); Serial.print(received); Serial.print(F(" verified=")); Serial.print(verified);
  Serial.print(F(" tx=")); Serial.print(sent); Serial.print(F(" errors=")); Serial.print(errors);
  Serial.print(F(" free=")); Serial.print(freeRam()); Serial.print(F(" min_free=")); Serial.println(minFree);
}
void handleCommand() {
  if (!overflow && active && commandLength == 13 && strncmp(command, "STOP ", 5) == 0 && memcmp(command + 5, token, 8) == 0) {
    finish(true); return;
  }
  bool valid = !overflow && commandLength >= 12;
  for (uint8_t i = 4; i < 12 && valid; ++i) valid = (command[i] >= '0' && command[i] <= '9') || (command[i] >= 'A' && command[i] <= 'F');
  if (valid && !used && strncmp(command, "RUN ", 4) == 0 && command[12] == ' ') {
    const char *p = command + 13; bool digits = *p != 0;
    for (const char *q = p; *q; ++q) if (*q < '0' || *q > '9') digits = false;
    unsigned long seconds = digits ? strtoul(p, 0, 10) : 0;
    if (seconds < 30 || seconds > 600) { Serial.println(F("COMMAND_REJECTED")); return; }
    memcpy(token, command + 4, 8); token[8] = 0; used = true; durationMs = seconds * 1000UL;
    prefix(F("START")); Serial.print(F(" duration_s=")); Serial.println(seconds);
    if (!prepare()) finish(false);
  } else Serial.println(F("COMMAND_REJECTED"));
}
void setup() {
  initializeChipSelects(); Serial.begin(115200); delay(300); Serial.println();
  Serial.println(F("BOOT test=TEST09 fw=0.2 eth_cs=10 sd_cs=4 uart=115200")); Serial.println(F("READY"));
}
void loop() {
  while (Serial.available()) {
    char value = (char)Serial.read(); if (value == '\r') continue;
    if (value == '\n') { command[commandLength] = 0; handleCommand(); commandLength = 0; overflow = false; }
    else if (commandLength < sizeof(command) - 1 && !overflow) command[commandLength++] = value;
    else overflow = true;
  }
  if (!active) return;
  if (millis() - started > durationMs + 10000UL) { fail(F("STOP_DEADLINE")); finish(false); return; }
  if (millis() - maintainTick >= 1000UL) {
    if (!releaseBus()) { fail(F("BUS_OWNERSHIP")); finish(false); return; }
    int rc = Ethernet.maintain(); maintainTick = millis(); sampleRam();
    if (rc) { prefix(F("DHCP_MAINTAIN")); Serial.print(F(" rc=")); Serial.println(rc); }
    if (rc == 1 || rc == 3 || Ethernet.localIP() != boundIP) { fail(F("LEASE_OR_IP")); finish(false); return; }
  }
  serviceUdp();
  if (!active) { finish(false); return; }
  if (millis() - statTick >= 5000UL) { statTick = millis(); stats(); }
}
