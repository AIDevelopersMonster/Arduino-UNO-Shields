/*
  LAB-03 / TEST-07
  Arduino UNO + MAR2406 2.4" TFT Touch Shield

  Integrated LCD + resistive touch + microSD certification test.

  Verified hardware:
    LCD: ILI9341, canonical ROT1 / 320x240
    Touch: XP=D6 XM=A2 YP=A1 YM=D7
    microSD: CS=D10, MOSI=D11, MISO=D12, SCK=D13

  Certified ROT0 touch calibration:
    LEFT   = 153
    RIGHT  = 930
    TOP    = 962
    BOTTOM = 168

  Test sequence:
    1. Initialize LCD.
    2. Initialize microSD and create T07LOG.TXT.
    3. Touch five fixed targets in order:
         TL -> TR -> BL -> BR -> CENTER
    4. Every accepted touch is appended to the SD log.
    5. After the fifth point, reopen the file and verify that
       exactly five point records are present.
    6. Show TEST-07 PASS only if display, touch, write, reopen,
       and file verification all succeed together.
*/

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <SPI.h>
#include <SD.h>

MCUFRIEND_kbv tft;

#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define YELLOW  0xFFE0
#define GREY    0x8410

#define MINPRESSURE 40
#define MAXPRESSURE 2000

const int XP = 6;
const int XM = A2;
const int YP = A1;
const int YM = 7;

const int TS_LEFT = 153;
const int TS_RT   = 930;
const int TS_TOP  = 962;
const int TS_BOT  = 168;

const uint8_t SD_CS = 10;
const char LOG_FILE[] = "T07LOG.TXT";

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

struct Target {
  int16_t x;
  int16_t y;
  const char *name;
};

static const Target targets[5] = {
  { 35,  70, "TL" },
  {285,  70, "TR" },
  { 35, 190, "BL" },
  {285, 190, "BR" },
  {160, 130, "CENTER" }
};

static const int16_t TARGET_R = 14;
static const int16_t HIT_TOL  = 22;

static uint8_t currentTarget = 0;
static bool armed = true;
static bool finished = false;

static void restoreSharedPins() {
  pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  pinMode(YM, OUTPUT);
}

static TSPoint readTouchRaw() {
  TSPoint p = ts.getPoint();
  restoreSharedPins();
  return p;
}

static bool rawToLandscape(const TSPoint &p, int16_t &x, int16_t &y) {
  if (p.z <= MINPRESSURE || p.z >= MAXPRESSURE) return false;

  long portraitX = map(p.x, TS_LEFT, TS_RT, 0, 239);
  long portraitY = map(p.y, TS_TOP, TS_BOT, 0, 319);

  portraitX = constrain(portraitX, 0, 239);
  portraitY = constrain(portraitY, 0, 319);

  x = (int16_t)portraitY;
  y = (int16_t)(239 - portraitX);

  x = constrain(x, 0, 319);
  y = constrain(y, 0, 239);
  return true;
}

static void drawTarget(uint8_t i, uint16_t color) {
  int16_t x = targets[i].x;
  int16_t y = targets[i].y;
  tft.drawCircle(x, y, TARGET_R, color);
  tft.drawFastHLine(x - 7, y, 15, color);
  tft.drawFastVLine(x, y - 7, 15, color);
}

static void drawScreen() {
  tft.fillScreen(BLACK);

  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(8, 8);
  tft.print(F("LAB-03 TEST-07"));

  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(8, 31);
  tft.print(F("LCD + TOUCH + microSD"));

  tft.setTextColor(WHITE);
  tft.setCursor(8, 43);
  tft.print(F("Touch yellow target"));

  for (uint8_t i = 0; i < 5; ++i) {
    drawTarget(i, (i == currentTarget) ? YELLOW : GREY);
  }
}

static void showFatal(const __FlashStringHelper *line1,
                      const __FlashStringHelper *line2) {
  tft.fillScreen(BLACK);
  tft.setTextColor(RED);
  tft.setTextSize(2);
  tft.setCursor(20, 75);
  tft.println(line1);
  tft.setCursor(20, 105);
  tft.println(line2);
}

static bool createLog() {
  if (SD.exists(LOG_FILE)) {
    if (!SD.remove(LOG_FILE)) return false;
  }

  File f = SD.open(LOG_FILE, FILE_WRITE);
  if (!f) return false;

  f.println(F("LAB03 TEST07"));
  f.println(F("N,TARGET,X,Y,RAWX,RAWY,Z"));
  f.close();
  return true;
}

static bool appendPoint(uint8_t n, const Target &t, int16_t x, int16_t y,
                        const TSPoint &p) {
  File f = SD.open(LOG_FILE, FILE_WRITE);
  if (!f) return false;

  f.print('P');
  f.print(n);
  f.print(',');
  f.print(t.name);
  f.print(',');
  f.print(x);
  f.print(',');
  f.print(y);
  f.print(',');
  f.print(p.x);
  f.print(',');
  f.print(p.y);
  f.print(',');
  f.println(p.z);

  bool ok = (bool)f;
  f.close();
  return ok;
}

static uint8_t countPointRecords() {
  File f = SD.open(LOG_FILE, FILE_READ);
  if (!f) return 0;

  uint8_t count = 0;
  bool lineStart = true;

  while (f.available()) {
    char c = (char)f.read();

    if (lineStart && c == 'P') {
      ++count;
    }

    if (c == '\n' || c == '\r') {
      lineStart = true;
    } else {
      lineStart = false;
    }
  }

  f.close();
  return count;
}

static bool hitTarget(uint8_t i, int16_t x, int16_t y) {
  return abs(x - targets[i].x) <= HIT_TOL &&
         abs(y - targets[i].y) <= HIT_TOL;
}

static void showPass() {
  tft.fillScreen(BLACK);

  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(42, 28);
  tft.println(F("LAB-03 TEST-07"));

  tft.setTextColor(GREEN);
  tft.setCursor(36, 75);
  tft.println(F("LCD PASS"));
  tft.setCursor(36, 105);
  tft.println(F("TOUCH PASS"));
  tft.setCursor(36, 135);
  tft.println(F("SD LOG PASS"));

  tft.setTextSize(3);
  tft.setCursor(47, 184);
  tft.println(F("TEST-07 PASS"));
}

void setup() {
  Serial.begin(115200);

  tft.reset();
  uint16_t id = tft.readID();
  tft.begin(id);
  tft.setRotation(1);

  Serial.println();
  Serial.println(F("LAB-03 TEST-07 - LCD + Touch + microSD"));
  Serial.print(F("LCD ID=0x"));
  Serial.println(id, HEX);
  Serial.println(F("Display: ROT1 / 320x240"));
  Serial.println(F("Touch: XP=D6 XM=A2 YP=A1 YM=D7"));
  Serial.println(F("Touch calibration ROT0: LEFT=153 RIGHT=930 TOP=962 BOTTOM=168"));
  Serial.println(F("microSD: CS=D10 MOSI=D11 MISO=D12 SCK=D13"));

  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(30, 82);
  tft.println(F("TEST-07 INIT"));

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  if (!SD.begin(SD_CS)) {
    Serial.println(F("SD INIT FAIL"));
    showFatal(F("SD INIT FAIL"), F("TEST-07 FAIL"));
    finished = true;
    return;
  }
  Serial.println(F("SD INIT PASS"));

  if (!createLog()) {
    Serial.println(F("LOG CREATE FAIL"));
    showFatal(F("LOG CREATE FAIL"), F("TEST-07 FAIL"));
    finished = true;
    return;
  }
  Serial.println(F("LOG CREATE PASS"));

  delay(400);
  drawScreen();
}

void loop() {
  if (finished) {
    delay(100);
    return;
  }

  TSPoint p = readTouchRaw();
  int16_t x, y;
  bool pressed = rawToLandscape(p, x, y);

  if (armed && pressed) {
    const Target &t = targets[currentTarget];

    Serial.print(F("TOUCH "));
    Serial.print(t.name);
    Serial.print(F(" X="));
    Serial.print(x);
    Serial.print(F(" Y="));
    Serial.print(y);
    Serial.print(F(" RAW="));
    Serial.print(p.x);
    Serial.print(',');
    Serial.print(p.y);
    Serial.print(F(" Z="));
    Serial.println(p.z);

    if (hitTarget(currentTarget, x, y)) {
      if (!appendPoint(currentTarget + 1, t, x, y, p)) {
        Serial.println(F("SD WRITE FAIL"));
        showFatal(F("SD WRITE FAIL"), F("TEST-07 FAIL"));
        finished = true;
        return;
      }

      Serial.println(F("SD WRITE PASS"));

      tft.fillCircle(x, y, 3, GREEN);
      drawTarget(currentTarget, GREEN);

      ++currentTarget;

      if (currentTarget < 5) {
        drawTarget(currentTarget, YELLOW);
      } else {
        Serial.println(F("REOPEN / VERIFY"));
        uint8_t records = countPointRecords();

        Serial.print(F("POINT RECORDS="));
        Serial.println(records);

        if (records == 5) {
          Serial.println(F("VERIFY PASS"));
          Serial.println(F("TEST-07 PASS"));
          showPass();
        } else {
          Serial.println(F("VERIFY FAIL"));
          showFatal(F("VERIFY FAIL"), F("TEST-07 FAIL"));
        }

        finished = true;
      }
    } else {
      Serial.println(F("MISS - touch current yellow target"));
      tft.fillCircle(x, y, 2, RED);
    }

    armed = false;
  }

  if (!armed && !pressed) {
    delay(25);
    TSPoint q = readTouchRaw();
    int16_t qx, qy;
    if (!rawToLandscape(q, qx, qy)) {
      armed = true;
      Serial.println(F("READY"));
    }
  }

  delay(10);
}
