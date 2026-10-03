/*
  LAB-03 / TEST-06
  Arduino UNO + MAR2406 2.4" TFT Touch Shield

  microSD read/write certification.

  Verified shield SD wiring:
    SD_SS / CS   = D10
    SD_DI / MOSI = D11
    SD_DO / MISO = D12
    SD_SCK       = D13

  TEST-06 intentionally tests SD independently from touch.
  The TFT is used only to show visible PASS/FAIL status.

  Test sequence:
    1. initialize SD on CS=D10;
    2. list the root directory to Serial;
    3. create LAB03.TST;
    4. write 512 deterministic bytes;
    5. reopen and verify every byte;
    6. delete the temporary file.

  PASS requires SD init + write + reopen + exact byte verification.
*/

#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>

MCUFRIEND_kbv tft;

#define BLACK 0x0000
#define WHITE 0xFFFF
#define GREEN 0x07E0
#define RED   0xF800
#define CYAN  0x07FF

static const uint8_t SD_CS = 10;
static const char TEST_FILE[] = "LAB03.TST";
static const uint16_t TEST_BYTES = 512;

static uint8_t expectedByte(uint16_t i) {
  return (uint8_t)(((uint32_t)i * 37UL + 11UL) & 0xFF);
}

static void screenLine(uint8_t row, const __FlashStringHelper *label, bool ok) {
  tft.setTextSize(2);
  tft.setTextColor(ok ? GREEN : RED, BLACK);
  tft.setCursor(12, 52 + row * 30);
  tft.print(label);
  tft.print(ok ? F(" PASS") : F(" FAIL"));
}

static void listRoot() {
  Serial.println(F("ROOT DIRECTORY:"));
  File root = SD.open("/");
  if (!root) {
    Serial.println(F("  <cannot open root>"));
    return;
  }

  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;

    Serial.print(F("  "));
    Serial.print(entry.name());

    if (entry.isDirectory()) {
      Serial.println(F("/"));
    } else {
      Serial.print(F("  "));
      Serial.print(entry.size());
      Serial.println(F(" bytes"));
    }
    entry.close();
  }
  root.close();
}

void setup() {
  Serial.begin(115200);

  tft.reset();
  uint16_t id = tft.readID();
  tft.begin(id);
  tft.setRotation(1);
  tft.fillScreen(BLACK);

  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(18, 12);
  tft.println(F("LAB-03 TEST-06"));
  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(18, 36);
  tft.println(F("microSD R/W certification"));

  Serial.println();
  Serial.println(F("LAB-03 TEST-06 - microSD R/W certification"));
  Serial.print(F("LCD ID=0x"));
  Serial.println(id, HEX);
  Serial.println(F("SD pins: CS=D10 MOSI=D11 MISO=D12 SCK=D13"));

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  bool initOK = SD.begin(SD_CS);
  screenLine(0, F("SD INIT"), initOK);
  Serial.print(F("SD INIT: "));
  Serial.println(initOK ? F("PASS") : F("FAIL"));

  if (!initOK) {
    tft.setTextColor(RED);
    tft.setTextSize(1);
    tft.setCursor(12, 185);
    tft.println(F("Check card / format / contacts"));
    while (true) delay(1000);
  }

  listRoot();

  if (SD.exists(TEST_FILE)) {
    SD.remove(TEST_FILE);
  }

  File f = SD.open(TEST_FILE, FILE_WRITE);
  bool writeOK = (bool)f;

  if (writeOK) {
    for (uint16_t i = 0; i < TEST_BYTES; ++i) {
      if (f.write(expectedByte(i)) != 1) {
        writeOK = false;
        break;
      }
    }
    f.flush();
    f.close();
  }

  screenLine(1, F("WRITE"), writeOK);
  Serial.print(F("WRITE 512 B: "));
  Serial.println(writeOK ? F("PASS") : F("FAIL"));

  bool openOK = false;
  bool verifyOK = false;

  if (writeOK) {
    f = SD.open(TEST_FILE, FILE_READ);
    openOK = (bool)f;

    if (openOK) {
      verifyOK = (f.size() == TEST_BYTES);

      for (uint16_t i = 0; verifyOK && i < TEST_BYTES; ++i) {
        int v = f.read();
        if (v < 0 || (uint8_t)v != expectedByte(i)) {
          verifyOK = false;
        }
      }

      if (f.available()) {
        verifyOK = false;
      }
      f.close();
    }
  }

  screenLine(2, F("REOPEN"), openOK);
  screenLine(3, F("VERIFY"), verifyOK);

  Serial.print(F("REOPEN: "));
  Serial.println(openOK ? F("PASS") : F("FAIL"));
  Serial.print(F("VERIFY 512 B: "));
  Serial.println(verifyOK ? F("PASS") : F("FAIL"));

  bool removeOK = false;
  if (SD.exists(TEST_FILE)) {
    removeOK = SD.remove(TEST_FILE);
  } else {
    removeOK = true;
  }

  Serial.print(F("REMOVE TEMP: "));
  Serial.println(removeOK ? F("PASS") : F("FAIL"));

  bool pass = initOK && writeOK && openOK && verifyOK;

  tft.setTextSize(2);
  tft.setTextColor(pass ? GREEN : RED, BLACK);
  tft.setCursor(58, 188);
  tft.println(pass ? F("TEST-06 PASS") : F("TEST-06 FAIL"));

  Serial.println();
  Serial.println(pass ? F("TEST-06 PASS") : F("TEST-06 FAIL"));
}

void loop() {
}
