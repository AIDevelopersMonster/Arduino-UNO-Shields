/*
  LAB-04 / TEST-02
  Arduino UNO + MAR2406 2.4" TFT + microSD + FreeRTOS
  PC <-> microSD file manager over USB Serial

  Design:
    - HMI task owns the TFT.
    - FILE task owns Serial + SPI/microSD.
    - No LED heartbeat: D13 is SPI SCK for the SD card.
    - Touch is intentionally not used in this first file-transfer test.
      The goal is a reliable PC <-> SD transport within the UNO's 2 KB SRAM.

  Protocol: FRTOSFM/1
    PING
    INFO
    MOUNT
    LS [path]
    GET <path>
    PUT <size> <path>
    RM <path>
    MKDIR <path>
    RMDIR <path>

  Binary transfers use stop-and-wait blocks of 32 bytes so the UNO's small
  serial RX buffer cannot be overrun while the SD library is writing a sector.

  Verified hardware:
    Arduino UNO R3 / ATmega328P
    MAR2406 TFT: ILI9341, ROT1 / 320x240
    microSD: CS=D10, MOSI=D11, MISO=D12, SCK=D13
*/

#include <Arduino_FreeRTOS.h>

#include <SPI.h>
#include <SD.h>

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>

MCUFRIEND_kbv tft;

#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define YELLOW  0xFFE0
#define GREY    0x8410

const uint8_t SD_CS = 10;
const uint32_t SERIAL_BAUD = 115200UL;

const uint8_t BLOCK_SIZE = 32;
const uint16_t LINE_SIZE = 96;
const unsigned long RX_TIMEOUT_MS = 5000UL;
const unsigned long ACK_TIMEOUT_MS = 5000UL;

static uint8_t ioBuf[BLOCK_SIZE];
static char lineBuf[LINE_SIZE];
static uint8_t lineLen = 0;

enum OpCode : uint8_t {
  OP_BOOT = 0,
  OP_IDLE,
  OP_LIST,
  OP_GET,
  OP_PUT,
  OP_DELETE,
  OP_MKDIR,
  OP_RMDIR,
  OP_ERROR
};

struct UiStatus {
  uint8_t op;
  bool sdReady;
  uint32_t doneBytes;
  uint32_t totalBytes;
  uint16_t filesSeen;
  uint16_t errorCode;
};

static volatile UiStatus ui = {OP_BOOT, false, 0, 0, 0, 0};

static void TaskHMI(void *pvParameters);
static void TaskFileServer(void *pvParameters);

static void setStatus(uint8_t op, bool sdReady, uint32_t doneBytes,
                      uint32_t totalBytes, uint16_t filesSeen,
                      uint16_t errorCode) {
  taskENTER_CRITICAL();
  ui.op = op;
  ui.sdReady = sdReady;
  ui.doneBytes = doneBytes;
  ui.totalBytes = totalBytes;
  ui.filesSeen = filesSeen;
  ui.errorCode = errorCode;
  taskEXIT_CRITICAL();
}

static UiStatus getStatusCopy() {
  UiStatus s;
  taskENTER_CRITICAL();
  s.op = ui.op;
  s.sdReady = ui.sdReady;
  s.doneBytes = ui.doneBytes;
  s.totalBytes = ui.totalBytes;
  s.filesSeen = ui.filesSeen;
  s.errorCode = ui.errorCode;
  taskEXIT_CRITICAL();
  return s;
}

static uint16_t crc16Update(uint16_t crc, uint8_t data) {
  crc ^= (uint16_t)data << 8;
  for (uint8_t i = 0; i < 8; ++i) {
    if (crc & 0x8000) crc = (uint16_t)((crc << 1) ^ 0x1021);
    else crc <<= 1;
  }
  return crc;
}

static void printHex16(uint16_t value) {
  const char hex[] = "0123456789ABCDEF";
  Serial.write(hex[(value >> 12) & 0x0F]);
  Serial.write(hex[(value >> 8) & 0x0F]);
  Serial.write(hex[(value >> 4) & 0x0F]);
  Serial.write(hex[value & 0x0F]);
}

static const __FlashStringHelper *opName(uint8_t op) {
  switch (op) {
    case OP_BOOT:   return F("BOOT");
    case OP_IDLE:   return F("IDLE");
    case OP_LIST:   return F("LIST");
    case OP_GET:    return F("GET");
    case OP_PUT:    return F("PUT");
    case OP_DELETE: return F("DELETE");
    case OP_MKDIR:  return F("MKDIR");
    case OP_RMDIR:  return F("RMDIR");
    case OP_ERROR:  return F("ERROR");
    default:        return F("?");
  }
}

static void drawStaticScreen() {
  tft.fillScreen(BLACK);

  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(8, 7);
  tft.print(F("FREERTOS SD MANAGER"));

  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(264, 10);
  tft.print(F("T02"));

  tft.drawFastHLine(0, 30, 320, GREY);

  tft.setTextColor(WHITE);
  tft.setCursor(12, 49);
  tft.print(F("SD"));

  tft.setCursor(12, 76);
  tft.print(F("SERIAL"));

  tft.setCursor(12, 103);
  tft.print(F("OP"));

  tft.setCursor(12, 130);
  tft.print(F("BYTES"));

  tft.setCursor(12, 157);
  tft.print(F("FILES"));

  tft.setCursor(12, 184);
  tft.print(F("PROTOCOL"));

  tft.drawRect(8, 207, 304, 25, GREY);
  tft.setTextColor(GREY);
  tft.setCursor(19, 216);
  tft.print(F("PC <-> USB Serial <-> microSD"));
}

static void clearValueLine(int16_t y) {
  tft.fillRect(92, y - 3, 218, 17, BLACK);
}

static void drawStatus(const UiStatus &s) {
  clearValueLine(49);
  tft.setCursor(92, 49);
  tft.setTextColor(s.sdReady ? GREEN : RED);
  tft.print(s.sdReady ? F("READY / D10") : F("NOT MOUNTED"));

  clearValueLine(76);
  tft.setCursor(92, 76);
  tft.setTextColor(CYAN);
  tft.print(F("115200 8N1"));

  clearValueLine(103);
  tft.setCursor(92, 103);
  tft.setTextColor(s.op == OP_ERROR ? RED : YELLOW);
  tft.print(opName(s.op));
  if (s.errorCode) {
    tft.print(F("  ERR "));
    tft.print(s.errorCode);
  }

  clearValueLine(130);
  tft.setCursor(92, 130);
  tft.setTextColor(WHITE);
  tft.print(s.doneBytes);
  tft.print(F(" / "));
  tft.print(s.totalBytes);

  if (s.totalBytes) {
    uint8_t pct = (uint8_t)((s.doneBytes * 100UL) / s.totalBytes);
    tft.print(F("  "));
    tft.print(pct);
    tft.print('%');
  }

  clearValueLine(157);
  tft.setCursor(92, 157);
  tft.setTextColor(CYAN);
  tft.print(s.filesSeen);

  clearValueLine(184);
  tft.setCursor(92, 184);
  tft.setTextColor(GREEN);
  tft.print(F("FRTOSFM/1  BLOCK=32"));
}

static bool mountSD() {
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  bool ok = SD.begin(SD_CS);
  UiStatus s = getStatusCopy();
  setStatus(ok ? OP_IDLE : OP_ERROR, ok, 0, 0, 0, ok ? 0 : 1);
  return ok;
}

static char *skipSpaces(char *p) {
  while (*p == ' ' || *p == '\t') ++p;
  return p;
}

static void trimLine(char *s) {
  size_t n = strlen(s);
  while (n && (s[n - 1] == '\r' || s[n - 1] == '\n' || s[n - 1] == ' ' || s[n - 1] == '\t')) {
    s[--n] = 0;
  }
}

static bool readExact(uint8_t *dst, uint8_t count, unsigned long timeoutMs) {
  uint8_t got = 0;
  unsigned long lastData = millis();

  while (got < count) {
    while (Serial.available() && got < count) {
      int v = Serial.read();
      if (v >= 0) {
        dst[got++] = (uint8_t)v;
        lastData = millis();
      }
    }

    if (got >= count) return true;
    if ((unsigned long)(millis() - lastData) > timeoutMs) return false;

    vTaskDelay(1);
  }

  return true;
}

static bool waitForAck(unsigned long timeoutMs) {
  char ack[8];
  uint8_t n = 0;
  unsigned long started = millis();

  while ((unsigned long)(millis() - started) <= timeoutMs) {
    while (Serial.available()) {
      char c = (char)Serial.read();

      if (c == '\n') {
        ack[n] = 0;
        if (n && ack[n - 1] == '\r') ack[n - 1] = 0;
        return strcmp(ack, "ACK") == 0;
      }

      if (n < sizeof(ack) - 1) ack[n++] = c;
    }

    vTaskDelay(1);
  }

  return false;
}

static void commandInfo(bool sdReady) {
  Serial.print(F("OK FRTOSFM/1 SD="));
  Serial.print(sdReady ? F("READY") : F("NO"));
  Serial.print(F(" BLOCK="));
  Serial.print(BLOCK_SIZE);
  Serial.print(F(" BAUD="));
  Serial.print(SERIAL_BAUD);
  Serial.print('\n');
}

static void commandList(char *path, bool sdReady) {
  if (!sdReady) {
    Serial.print(F("ERR SD_NOT_READY\n"));
    setStatus(OP_ERROR, false, 0, 0, 0, 2);
    return;
  }

  path = skipSpaces(path);
  if (!*path) path = (char *)"/";

  File dir = SD.open(path);
  if (!dir) {
    Serial.print(F("ERR OPEN\n"));
    setStatus(OP_ERROR, true, 0, 0, 0, 3);
    return;
  }

  if (!dir.isDirectory()) {
    dir.close();
    Serial.print(F("ERR NOT_DIR\n"));
    setStatus(OP_ERROR, true, 0, 0, 0, 4);
    return;
  }

  setStatus(OP_LIST, true, 0, 0, 0, 0);
  Serial.print(F("BEGIN LS\n"));

  uint16_t count = 0;

  for (;;) {
    File entry = dir.openNextFile();
    if (!entry) break;

    Serial.write(entry.isDirectory() ? 'D' : 'F');
    Serial.write('\t');
    Serial.print((uint32_t)entry.size());
    Serial.write('\t');
    Serial.print(entry.name());
    Serial.write('\n');

    ++count;
    setStatus(OP_LIST, true, 0, 0, count, 0);

    entry.close();
    vTaskDelay(1);
  }

  dir.close();
  Serial.print(F("END LS\n"));
  setStatus(OP_IDLE, true, 0, 0, count, 0);
}

static void commandGet(char *path, bool sdReady) {
  if (!sdReady) {
    Serial.print(F("ERR SD_NOT_READY\n"));
    setStatus(OP_ERROR, false, 0, 0, 0, 2);
    return;
  }

  path = skipSpaces(path);
  if (!*path) {
    Serial.print(F("ERR PATH\n"));
    return;
  }

  File file = SD.open(path, FILE_READ);
  if (!file) {
    Serial.print(F("ERR OPEN\n"));
    setStatus(OP_ERROR, true, 0, 0, 0, 3);
    return;
  }

  if (file.isDirectory()) {
    file.close();
    Serial.print(F("ERR IS_DIR\n"));
    return;
  }

  uint32_t total = file.size();
  uint32_t done = 0;
  uint16_t crc = 0xFFFF;

  setStatus(OP_GET, true, 0, total, 0, 0);

  Serial.print(F("DATA "));
  Serial.print(total);
  Serial.write(' ');
  Serial.print(BLOCK_SIZE);
  Serial.write('\n');

  while (done < total) {
    uint8_t want = (uint8_t)min((uint32_t)BLOCK_SIZE, total - done);
    int got = file.read(ioBuf, want);

    if (got != want) {
      file.close();
      Serial.print(F("ERR READ\n"));
      setStatus(OP_ERROR, true, done, total, 0, 5);
      return;
    }

    for (uint8_t i = 0; i < want; ++i) crc = crc16Update(crc, ioBuf[i]);

    Serial.write(ioBuf, want);
    Serial.flush();

    if (!waitForAck(ACK_TIMEOUT_MS)) {
      file.close();
      Serial.print(F("ERR ACK_TIMEOUT\n"));
      setStatus(OP_ERROR, true, done, total, 0, 6);
      return;
    }

    done += want;
    setStatus(OP_GET, true, done, total, 0, 0);
  }

  file.close();

  Serial.print(F("END "));
  printHex16(crc);
  Serial.write('\n');

  setStatus(OP_IDLE, true, total, total, 0, 0);
}

static void commandPut(char *args, bool sdReady) {
  if (!sdReady) {
    Serial.print(F("ERR SD_NOT_READY\n"));
    setStatus(OP_ERROR, false, 0, 0, 0, 2);
    return;
  }

  args = skipSpaces(args);

  char *endNum = NULL;
  uint32_t total = strtoul(args, &endNum, 10);

  if (endNum == args) {
    Serial.print(F("ERR SIZE\n"));
    return;
  }

  char *path = skipSpaces(endNum);
  if (!*path) {
    Serial.print(F("ERR PATH\n"));
    return;
  }

  if (SD.exists(path)) {
    if (!SD.remove(path)) {
      Serial.print(F("ERR REMOVE_OLD\n"));
      setStatus(OP_ERROR, true, 0, total, 0, 7);
      return;
    }
  }

  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.print(F("ERR CREATE\n"));
    setStatus(OP_ERROR, true, 0, total, 0, 8);
    return;
  }

  uint32_t done = 0;
  uint16_t crc = 0xFFFF;

  setStatus(OP_PUT, true, 0, total, 0, 0);

  Serial.print(F("READY "));
  Serial.print(BLOCK_SIZE);
  Serial.write('\n');

  while (done < total) {
    uint8_t want = (uint8_t)min((uint32_t)BLOCK_SIZE, total - done);

    if (!readExact(ioBuf, want, RX_TIMEOUT_MS)) {
      file.close();
      SD.remove(path);
      Serial.print(F("ERR RX_TIMEOUT\n"));
      setStatus(OP_ERROR, true, done, total, 0, 9);
      return;
    }

    size_t written = file.write(ioBuf, want);
    if (written != want) {
      file.close();
      SD.remove(path);
      Serial.print(F("ERR WRITE\n"));
      setStatus(OP_ERROR, true, done, total, 0, 10);
      return;
    }

    for (uint8_t i = 0; i < want; ++i) crc = crc16Update(crc, ioBuf[i]);

    done += want;
    setStatus(OP_PUT, true, done, total, 0, 0);

    Serial.print(F("ACK "));
    Serial.print(done);
    Serial.write('\n');
  }

  file.close();

  Serial.print(F("OK "));
  Serial.print(total);
  Serial.write(' ');
  printHex16(crc);
  Serial.write('\n');

  setStatus(OP_IDLE, true, total, total, 0, 0);
}

static void commandRemove(char *path, bool sdReady) {
  if (!sdReady) {
    Serial.print(F("ERR SD_NOT_READY\n"));
    return;
  }

  path = skipSpaces(path);
  if (!*path) {
    Serial.print(F("ERR PATH\n"));
    return;
  }

  setStatus(OP_DELETE, true, 0, 0, 0, 0);

  if (SD.remove(path)) {
    Serial.print(F("OK\n"));
    setStatus(OP_IDLE, true, 0, 0, 0, 0);
  } else {
    Serial.print(F("ERR REMOVE\n"));
    setStatus(OP_ERROR, true, 0, 0, 0, 11);
  }
}

static void commandMkdir(char *path, bool sdReady) {
  if (!sdReady) {
    Serial.print(F("ERR SD_NOT_READY\n"));
    return;
  }

  path = skipSpaces(path);
  if (!*path) {
    Serial.print(F("ERR PATH\n"));
    return;
  }

  setStatus(OP_MKDIR, true, 0, 0, 0, 0);

  if (SD.mkdir(path)) {
    Serial.print(F("OK\n"));
    setStatus(OP_IDLE, true, 0, 0, 0, 0);
  } else {
    Serial.print(F("ERR MKDIR\n"));
    setStatus(OP_ERROR, true, 0, 0, 0, 12);
  }
}

static void commandRmdir(char *path, bool sdReady) {
  if (!sdReady) {
    Serial.print(F("ERR SD_NOT_READY\n"));
    return;
  }

  path = skipSpaces(path);
  if (!*path) {
    Serial.print(F("ERR PATH\n"));
    return;
  }

  setStatus(OP_RMDIR, true, 0, 0, 0, 0);

  if (SD.rmdir(path)) {
    Serial.print(F("OK\n"));
    setStatus(OP_IDLE, true, 0, 0, 0, 0);
  } else {
    Serial.print(F("ERR RMDIR\n"));
    setStatus(OP_ERROR, true, 0, 0, 0, 13);
  }
}

static void processCommand(char *line, bool &sdReady) {
  trimLine(line);
  if (!*line) return;

  if (strcmp(line, "PING") == 0) {
    Serial.print(F("OK PONG FRTOSFM/1\n"));
    return;
  }

  if (strcmp(line, "INFO") == 0) {
    commandInfo(sdReady);
    return;
  }

  if (strcmp(line, "MOUNT") == 0) {
    sdReady = mountSD();
    Serial.print(sdReady ? F("OK SD_READY\n") : F("ERR SD_INIT\n"));
    return;
  }

  if (strncmp(line, "LS", 2) == 0 && (line[2] == 0 || line[2] == ' ' || line[2] == '\t')) {
    commandList(line + 2, sdReady);
    return;
  }

  if (strncmp(line, "GET", 3) == 0 && (line[3] == ' ' || line[3] == '\t')) {
    commandGet(line + 3, sdReady);
    return;
  }

  if (strncmp(line, "PUT", 3) == 0 && (line[3] == ' ' || line[3] == '\t')) {
    commandPut(line + 3, sdReady);
    return;
  }

  if (strncmp(line, "RM", 2) == 0 && (line[2] == ' ' || line[2] == '\t')) {
    commandRemove(line + 2, sdReady);
    return;
  }

  if (strncmp(line, "MKDIR", 5) == 0 && (line[5] == ' ' || line[5] == '\t')) {
    commandMkdir(line + 5, sdReady);
    return;
  }

  if (strncmp(line, "RMDIR", 5) == 0 && (line[5] == ' ' || line[5] == '\t')) {
    commandRmdir(line + 5, sdReady);
    return;
  }

  Serial.print(F("ERR UNKNOWN_COMMAND\n"));
}

void setup() {
  Serial.begin(SERIAL_BAUD);

  tft.reset();
  uint16_t id = tft.readID();
  tft.begin(id);
  tft.setRotation(1);
  drawStaticScreen();

  BaseType_t hmiOK = xTaskCreate(
    TaskHMI,
    "HMI",
    180,
    NULL,
    2,
    NULL
  );

  BaseType_t fileOK = xTaskCreate(
    TaskFileServer,
    "FILE",
    256,
    NULL,
    1,
    NULL
  );

  if (hmiOK != pdPASS || fileOK != pdPASS) {
    tft.fillScreen(BLACK);
    tft.setTextColor(RED);
    tft.setTextSize(2);
    tft.setCursor(45, 95);
    tft.print(F("TASK CREATE ERROR"));
    for (;;) {}
  }
}

void loop() {
  // FreeRTOS owns execution after setup().
}

static void TaskHMI(void *pvParameters) {
  (void)pvParameters;

  UiStatus previous = {255, false, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFF, 0xFFFF};

  for (;;) {
    UiStatus current = getStatusCopy();

    if (memcmp(&current, &previous, sizeof(UiStatus)) != 0) {
      drawStatus(current);
      previous = current;
    }

    vTaskDelay(150 / portTICK_PERIOD_MS);
  }
}

static void TaskFileServer(void *pvParameters) {
  (void)pvParameters;

  bool sdReady = mountSD();

  Serial.print(F("BOOT FRTOSFM/1\n"));
  commandInfo(sdReady);

  lineLen = 0;

  for (;;) {
    while (Serial.available()) {
      char c = (char)Serial.read();

      if (c == '\r') continue;

      if (c == '\n') {
        lineBuf[lineLen] = 0;
        processCommand(lineBuf, sdReady);
        lineLen = 0;
        continue;
      }

      if (lineLen < LINE_SIZE - 1) {
        lineBuf[lineLen++] = c;
      } else {
        lineLen = 0;
        Serial.print(F("ERR CMD_TOO_LONG\n"));
      }
    }

    vTaskDelay(1);
  }
}
