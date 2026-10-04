/*
  LAB-04 / TEST-02
  Arduino UNO + MAR2406 microSD + FreeRTOS
  PC <-> microSD file manager over USB Serial

  IMPORTANT:
  The first TEST-02 build combined FreeRTOS + SD + MCUFRIEND_kbv +
  Adafruit_GFX and exceeded the ATmega328P program Flash limit.

  This revision deliberately removes TFT/GFX from the file-transfer firmware.
  The goal is to prove a reliable FreeRTOS + SD + Serial file manager first.
  The display can be reintroduced later with a small direct ILI9341 driver
  instead of the full graphics stack.

  No LED heartbeat is used:
    D13 = SPI SCK for the microSD card.

  Protocol: FRTOSFM/1
    PING
    INFO
    MOUNT
    LS [path]
    GET <path>
    PUT <size> <path>

  Transfers use stop-and-wait blocks of 32 bytes to protect the small AVR
  serial RX buffer while SD writes are in progress.

  Hardware:
    Arduino UNO R3 / ATmega328P
    MAR2406 microSD:
      CS   = D10
      MOSI = D11
      MISO = D12
      SCK  = D13
*/

#include <Arduino_FreeRTOS.h>
#include <SPI.h>
#include <SD.h>

const uint8_t SD_CS = 10;
const uint32_t SERIAL_BAUD = 115200UL;

const uint8_t BLOCK_SIZE = 32;
const uint8_t LINE_SIZE = 72;

const unsigned long RX_TIMEOUT_MS = 5000UL;
const unsigned long ACK_TIMEOUT_MS = 5000UL;

static uint8_t ioBuf[BLOCK_SIZE];
static char lineBuf[LINE_SIZE];
static uint8_t lineLen = 0;

static void TaskFileServer(void *pvParameters);

static uint16_t crc16Update(uint16_t crc, uint8_t data) {
  crc ^= (uint16_t)data << 8;

  for (uint8_t i = 0; i < 8; ++i) {
    if (crc & 0x8000) {
      crc = (uint16_t)((crc << 1) ^ 0x1021);
    } else {
      crc <<= 1;
    }
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

static char *skipSpaces(char *p) {
  while (*p == ' ' || *p == '\t') ++p;
  return p;
}

static bool mountSD() {
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  return SD.begin(SD_CS);
}

static bool readExact(uint8_t *dst, uint8_t count) {
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

    if ((unsigned long)(millis() - lastData) > RX_TIMEOUT_MS) {
      return false;
    }

    vTaskDelay(1);
  }

  return true;
}

static bool waitForAck() {
  char ack[5];
  uint8_t n = 0;
  unsigned long started = millis();

  while ((unsigned long)(millis() - started) <= ACK_TIMEOUT_MS) {
    while (Serial.available()) {
      char c = (char)Serial.read();

      if (c == '\r') continue;

      if (c == '\n') {
        ack[n] = 0;
        return strcmp(ack, "ACK") == 0;
      }

      if (n < sizeof(ack) - 1) {
        ack[n++] = c;
      }
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
  Serial.println(SERIAL_BAUD);
}

static void commandList(char *path, bool sdReady) {
  if (!sdReady) {
    Serial.println(F("ERR SD_NOT_READY"));
    return;
  }

  path = skipSpaces(path);
  if (!*path) path = (char *)"/";

  File dir = SD.open(path);

  if (!dir) {
    Serial.println(F("ERR OPEN"));
    return;
  }

  if (!dir.isDirectory()) {
    dir.close();
    Serial.println(F("ERR NOT_DIR"));
    return;
  }

  Serial.println(F("BEGIN LS"));

  for (;;) {
    File entry = dir.openNextFile();
    if (!entry) break;

    Serial.write(entry.isDirectory() ? 'D' : 'F');
    Serial.write('\t');
    Serial.print((uint32_t)entry.size());
    Serial.write('\t');
    Serial.println(entry.name());

    entry.close();
    vTaskDelay(1);
  }

  dir.close();
  Serial.println(F("END LS"));
}

static void commandGet(char *path, bool sdReady) {
  if (!sdReady) {
    Serial.println(F("ERR SD_NOT_READY"));
    return;
  }

  path = skipSpaces(path);

  if (!*path) {
    Serial.println(F("ERR PATH"));
    return;
  }

  File file = SD.open(path, FILE_READ);

  if (!file) {
    Serial.println(F("ERR OPEN"));
    return;
  }

  if (file.isDirectory()) {
    file.close();
    Serial.println(F("ERR IS_DIR"));
    return;
  }

  uint32_t total = file.size();
  uint32_t done = 0;
  uint16_t crc = 0xFFFF;

  Serial.print(F("DATA "));
  Serial.print(total);
  Serial.write(' ');
  Serial.println(BLOCK_SIZE);

  while (done < total) {
    uint8_t want = (uint8_t)min((uint32_t)BLOCK_SIZE, total - done);
    int got = file.read(ioBuf, want);

    if (got != want) {
      file.close();
      Serial.println(F("ERR READ"));
      return;
    }

    for (uint8_t i = 0; i < want; ++i) {
      crc = crc16Update(crc, ioBuf[i]);
    }

    Serial.write(ioBuf, want);
    Serial.flush();

    if (!waitForAck()) {
      file.close();
      Serial.println(F("ERR ACK_TIMEOUT"));
      return;
    }

    done += want;
  }

  file.close();

  Serial.print(F("END "));
  printHex16(crc);
  Serial.write('\n');
}

static void commandPut(char *args, bool sdReady) {
  if (!sdReady) {
    Serial.println(F("ERR SD_NOT_READY"));
    return;
  }

  args = skipSpaces(args);

  char *endNum = NULL;
  uint32_t total = strtoul(args, &endNum, 10);

  if (endNum == args) {
    Serial.println(F("ERR SIZE"));
    return;
  }

  char *path = skipSpaces(endNum);

  if (!*path) {
    Serial.println(F("ERR PATH"));
    return;
  }

  if (SD.exists(path) && !SD.remove(path)) {
    Serial.println(F("ERR REMOVE_OLD"));
    return;
  }

  File file = SD.open(path, FILE_WRITE);

  if (!file) {
    Serial.println(F("ERR CREATE"));
    return;
  }

  uint32_t done = 0;
  uint16_t crc = 0xFFFF;

  Serial.print(F("READY "));
  Serial.println(BLOCK_SIZE);

  while (done < total) {
    uint8_t want = (uint8_t)min((uint32_t)BLOCK_SIZE, total - done);

    if (!readExact(ioBuf, want)) {
      file.close();
      SD.remove(path);
      Serial.println(F("ERR RX_TIMEOUT"));
      return;
    }

    if (file.write(ioBuf, want) != want) {
      file.close();
      SD.remove(path);
      Serial.println(F("ERR WRITE"));
      return;
    }

    for (uint8_t i = 0; i < want; ++i) {
      crc = crc16Update(crc, ioBuf[i]);
    }

    done += want;

    Serial.print(F("ACK "));
    Serial.println(done);
  }

  file.close();

  Serial.print(F("OK "));
  Serial.print(total);
  Serial.write(' ');
  printHex16(crc);
  Serial.write('\n');
}

static void processCommand(char *line, bool &sdReady) {
  if (!*line) return;

  if (strcmp(line, "PING") == 0) {
    Serial.println(F("OK PONG FRTOSFM/1"));
    return;
  }

  if (strcmp(line, "INFO") == 0) {
    commandInfo(sdReady);
    return;
  }

  if (strcmp(line, "MOUNT") == 0) {
    sdReady = mountSD();
    Serial.println(sdReady ? F("OK SD_READY") : F("ERR SD_INIT"));
    return;
  }

  if (strncmp(line, "LS", 2) == 0 &&
      (line[2] == 0 || line[2] == ' ' || line[2] == '\t')) {
    commandList(line + 2, sdReady);
    return;
  }

  if (strncmp(line, "GET", 3) == 0 &&
      (line[3] == ' ' || line[3] == '\t')) {
    commandGet(line + 3, sdReady);
    return;
  }

  if (strncmp(line, "PUT", 3) == 0 &&
      (line[3] == ' ' || line[3] == '\t')) {
    commandPut(line + 3, sdReady);
    return;
  }

  Serial.println(F("ERR UNKNOWN_COMMAND"));
}

void setup() {
  Serial.begin(SERIAL_BAUD);

  BaseType_t ok = xTaskCreate(
    TaskFileServer,
    "FILE",
    320,
    NULL,
    1,
    NULL
  );

  if (ok != pdPASS) {
    Serial.println(F("ERR TASK_CREATE"));
    for (;;) {}
  }
}

void loop() {
  // FreeRTOS owns execution after setup().
}

static void TaskFileServer(void *pvParameters) {
  (void)pvParameters;

  bool sdReady = mountSD();

  Serial.println(F("BOOT FRTOSFM/1"));
  commandInfo(sdReady);

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
        Serial.println(F("ERR CMD_TOO_LONG"));
      }
    }

    vTaskDelay(1);
  }
}
