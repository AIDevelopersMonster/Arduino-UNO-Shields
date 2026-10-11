// SD_ONLY v0.1: standalone diagnostic. No W5100 register reads or writes.
// Arduino SD 1.3.0 low-level API avoids the File wrapper's heap allocation.
#include <SPI.h>
#include <SD.h>
#include <string.h>

const uint8_t ETH_CS = 10;
const uint8_t SD_CS = 4;
const uint16_t INITIAL_BYTES = 2048;
const uint16_t TOTAL_BYTES = 2112;
const char TEST_FILE[] = "SDONLY.BIN";
Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file;
uint8_t buffer[64];
char command[24];
char token[9];
uint8_t commandLength = 0;
bool overflow = false;
bool hasRun = false;
bool busFault = false;
uint8_t checks = 0;
uint8_t failures = 0;
uint16_t lastCrc = 0;
uint16_t verifiedBytes = 0;
int16_t minFree = 32767;
int16_t initialFree = 0;
uint32_t runStarted = 0;

extern int __heap_start;
extern void *__brkval;
int16_t freeRam() {
  int local;
  return (int)&local - (__brkval ? (int)__brkval : (int)&__heap_start);
}
void sampleRam() {
  int16_t value = freeRam();
  if (value < minFree) minFree = value;
}
void prefix(const __FlashStringHelper *kind) {
  Serial.print(kind);
  Serial.print(F(" token="));
  Serial.print(token);
}
bool check(const __FlashStringHelper *name, bool passed) {
  sampleRam();
  if (digitalRead(ETH_CS) != HIGH || digitalRead(SD_CS) != HIGH) busFault = true;
  passed = passed && !busFault;
  ++checks;
  if (!passed) ++failures;
  prefix(F("CHECK"));
  Serial.print(F(" index=")); Serial.print(checks);
  Serial.print(F(" name=")); Serial.print(name);
  Serial.print(F(" status=")); Serial.print(passed ? F("PASS") : F("FAIL"));
  Serial.print(F(" free=")); Serial.println(freeRam());
  return passed;
}
void initializeChipSelects() {
  // Set inactive output latches BEFORE changing direction: no LOW select pulse.
  digitalWrite(ETH_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  pinMode(ETH_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  SPI.begin();
}
uint8_t expectedByte(uint16_t position) {
  return (uint8_t)((position * 73U) ^ (position >> 3) ^ 0xA5U);
}
uint16_t crcStep(uint16_t crc, uint8_t value) {
  crc ^= (uint16_t)value << 8;
  for (uint8_t i = 0; i < 8; ++i)
    crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
  return crc;
}
void hex16(uint16_t value) {
  if (value < 0x1000) Serial.print('0');
  if (value < 0x100) Serial.print('0');
  if (value < 0x10) Serial.print('0');
  Serial.print(value, HEX);
}
bool writeRange(uint16_t first, uint16_t last) {
  for (uint16_t position = first; position < last; position += sizeof(buffer)) {
    uint16_t count = last - position;
    if (count > sizeof(buffer)) count = sizeof(buffer);
    for (uint8_t i = 0; i < count; ++i) buffer[i] = expectedByte(position + i);
    if (file.write(buffer, count) != count) return false;
    sampleRam();
  }
  return file.sync() && file.fileSize() == last;
}
bool verifyFile(uint16_t length) {
  if (!file.open(&root, TEST_FILE, O_READ)) return false;
  bool passed = file.fileSize() == length;
  uint16_t crc = 0xFFFF;
  for (uint16_t position = 0; position < length && passed; position += sizeof(buffer)) {
    uint16_t count = length - position;
    if (count > sizeof(buffer)) count = sizeof(buffer);
    if (file.read(buffer, count) != (int16_t)count) { passed = false; break; }
    for (uint8_t i = 0; i < count; ++i) {
      if (buffer[i] != expectedByte(position + i)) passed = false;
      crc = crcStep(crc, buffer[i]);
    }
    sampleRam();
  }
  passed = passed && file.read() == -1;
  bool closed = file.close();
  lastCrc = crc;
  prefix(F("VERIFY")); Serial.print(F(" bytes=")); Serial.print(length);
  Serial.print(F(" crc16=")); hex16(crc); Serial.println();
  if (passed && closed) verifiedBytes = length;
  return passed && closed;
}
bool seekCheck() {
  const uint16_t positions[] = {0, 63, 64, 511, 512, 1023, 2047};
  if (!file.open(&root, TEST_FILE, O_READ)) return false;
  bool passed = true;
  for (uint8_t i = 0; i < sizeof(positions) / sizeof(positions[0]); ++i) {
    uint16_t position = positions[i];
    if (!file.seekSet(position) || file.curPosition() != position ||
        file.read() != expectedByte(position)) passed = false;
  }
  bool closed = file.close();
  return passed && closed;
}
void runChecks() {
  // Put SD into SPI mode before any clocks addressed to the other slave.
  bool sdReady = card.init(SPI_HALF_SPEED, SD_CS);
  prefix(F("CARD")); Serial.print(F(" error_code=")); Serial.print(card.errorCode());
  Serial.print(F(" error_data=")); Serial.println(card.errorData());
  if (!check(F("CARD_INIT"), sdReady)) return;

  uint32_t blocks = card.cardSize();
  uint8_t type = card.type();
  prefix(F("CARD_INFO")); Serial.print(F(" type=")); Serial.print(type);
  Serial.print(F(" blocks=")); Serial.println(blocks);
  if (!check(F("CARD_INFO"), blocks > 0 && type >= SD_CARD_TYPE_SD1 && type <= SD_CARD_TYPE_SDHC)) return;
  bool mounted = volume.init(&card);
  prefix(F("VOLUME")); Serial.print(F(" fat=")); Serial.println(volume.fatType());
  if (!check(F("FAT_VOLUME"), mounted && (volume.fatType() == 16 || volume.fatType() == 32))) return;
  if (!check(F("ROOT_OPEN"), root.openRoot(&volume))) return;

  // O_EXCL refuses an existing name. No truncation or automatic removal.
  if (!check(F("EXCLUSIVE_CREATE"), file.open(&root, TEST_FILE, O_CREAT | O_EXCL | O_WRITE))) return;
  uint32_t started = millis();
  bool written = writeRange(0, INITIAL_BYTES);
  bool closed = file.close();
  prefix(F("WRITE")); Serial.print(F(" bytes=")); Serial.print(INITIAL_BYTES);
  Serial.print(F(" elapsed_ms=")); Serial.println(millis() - started);
  if (!check(F("WRITE_2048"), written && closed)) return;
  if (!check(F("REOPEN_VERIFY_2048"), verifyFile(INITIAL_BYTES))) return;
  if (!check(F("SEEK_BOUNDARIES"), seekCheck())) return;

  bool append = file.open(&root, TEST_FILE, O_WRITE | O_APPEND);
  bool appended = append && file.fileSize() == INITIAL_BYTES &&
                  file.seekEnd() && writeRange(INITIAL_BYTES, TOTAL_BYTES);
  closed = file.isOpen() ? file.close() : false;
  if (!check(F("APPEND_64"), appended && closed)) return;
  if (!check(F("REOPEN_VERIFY_2112"), verifyFile(TOTAL_BYTES))) return;

  // Flush and discard the shared filesystem cache before reinitialization.
  bool rootClosed = root.close();
  bool cleared = SdVolume::cacheClear() != 0;
  bool remounted = rootClosed && cleared && card.init(SPI_HALF_SPEED, SD_CS) &&
                   volume.init(&card) && root.openRoot(&volume);
  if (!check(F("REMOUNT_VERIFY"), remounted && verifyFile(TOTAL_BYTES))) return;
  bool removed = SdFile::remove(&root, TEST_FILE);
  bool stillExists = file.open(&root, TEST_FILE, O_READ);
  if (stillExists) file.close();
  bool rootClosedAfterCleanup = root.close();
  if (!check(F("REMOVE_TEST_FILE"), removed && !stillExists && rootClosedAfterCleanup)) return;
  sampleRam();
  check(F("RAM"), minFree >= 512 && freeRam() >= initialFree - 64);
}
void runTest() {
  hasRun = true;
  runStarted = millis();
  initialFree = freeRam(); sampleRam();
  prefix(F("START")); Serial.print(F(" free=")); Serial.println(initialFree);
  runChecks();
  if (!busFault) {
    if (file.isOpen() && !file.close()) ++failures;
    if (root.isOpen() && !root.close()) ++failures;
  }
  sampleRam();
  prefix(F("RESULT"));
  Serial.print(F(" status=")); Serial.print(!busFault && failures == 0 && checks == 13 ? F("PASS") : F("FAIL"));
  Serial.print(F(" checks=")); Serial.print(checks);
  Serial.print(F(" failures=")); Serial.print(failures);
  Serial.print(F(" planned_bytes=")); Serial.print(TOTAL_BYTES);
  Serial.print(F(" bytes=")); Serial.print(verifiedBytes);
  Serial.print(F(" crc16=")); hex16(lastCrc);
  Serial.print(F(" free=")); Serial.print(freeRam());
  Serial.print(F(" min_free=")); Serial.print(minFree);
  Serial.print(F(" bus_fault=")); Serial.print(busFault);
  Serial.print(F(" elapsed_ms=")); Serial.println(millis() - runStarted);
}
void setup() {
  initializeChipSelects();
  Serial.begin(115200);
  delay(300);
  Serial.println(); // Separate any startup/bootloader bytes from the banner.
  Serial.println(F("BOOT test=SD_ONLY fw=0.1 eth_cs=10 sd_cs=4 uart=115200 eth_spi_hz=0 sd_init_spi_hz=250000 sd_data_spi_hz=4000000"));
  Serial.println(F("READY"));
}
void loop() {
  while (Serial.available()) {
    char value = (char)Serial.read();
    if (value == '\r') continue;
    if (value == '\n') {
      command[commandLength] = 0;
      bool valid = !overflow && commandLength == 12 && strncmp(command, "RUN ", 4) == 0;
      for (uint8_t i = 4; i < commandLength && valid; ++i)
        valid = (command[i] >= '0' && command[i] <= '9') || (command[i] >= 'A' && command[i] <= 'F');
      if (valid && !hasRun) {
        memcpy(token, command + 4, 8); token[8] = 0;
        runTest();
      } else Serial.println(F("COMMAND_REJECTED"));
      commandLength = 0; overflow = false;
    } else if (commandLength < sizeof(command) - 1 && !overflow) command[commandLength++] = value;
    else overflow = true;
  }
}
