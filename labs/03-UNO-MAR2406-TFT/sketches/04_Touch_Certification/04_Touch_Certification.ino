/*
  LAB-03 / TEST-04
  Arduino UNO + MAR2406 2.4" TFT Touch Shield

  Canonical touch certification test derived from the user-provided
  working TFT_LCD_Break_game.ino.

  Libraries:
    Adafruit_GFX
    MCUFRIEND_kbv
    TouchScreen

  Verified working touch wiring from the reference sketch:
    XP = D6
    XM = A2
    YP = A1
    YM = D7

  Calibration from the reference sketch:
    TS_LEFT = 270
    TS_RT   = 887
    TS_TOP  = 267
    TS_BOT  = 879

  IMPORTANT:
  The reference sketch uses rotation 0. TEST-04 intentionally keeps
  rotation 0 so that touch wiring/calibration is certified before any
  landscape coordinate transform is introduced.

  Behavior:
    - one touch -> one Serial record
    - draws a cross at the mapped screen coordinate
    - waits for release before recording the next point
*/

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>

MCUFRIEND_kbv tft;

#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define YELLOW  0xFFE0

#define MINPRESSURE 40
#define MAXPRESSURE 2000

const int XP = 6;
const int XM = A2;
const int YP = A1;
const int YM = 7;

const int TS_LEFT = 270;
const int TS_RT   = 887;
const int TS_TOP  = 267;
const int TS_BOT  = 879;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

static bool armed = true;
static uint8_t pointNo = 0;
static int16_t lastX = -1;
static int16_t lastY = -1;

static void restoreSharedPins() {
  // This is intentionally identical in principle to the working reference:
  // all four touch/shared lines are returned to OUTPUT after getPoint().
  pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  pinMode(YM, OUTPUT);
}

static void drawCross(int16_t x, int16_t y, uint16_t color) {
  const int16_t r = 8;
  tft.drawFastHLine(max(0, x - r), y,
                    min((int16_t)(2 * r + 1), (int16_t)(tft.width() - max(0, x - r))),
                    color);
  tft.drawFastVLine(x, max(0, y - r),
                    min((int16_t)(2 * r + 1), (int16_t)(tft.height() - max(0, y - r))),
                    color);
  tft.drawCircle(x, y, 3, color);
}

static TSPoint readTouch() {
  TSPoint p = ts.getPoint();
  restoreSharedPins();
  return p;
}

void setup() {
  Serial.begin(115200);

  tft.reset();
  uint16_t id = tft.readID();
  tft.begin(id);

  // Keep the same orientation used by the known-working reference sketch.
  tft.setRotation(0);
  tft.fillScreen(BLACK);

  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(8, 10);
  tft.println(F("LAB-03 TEST-04"));

  tft.setTextSize(1);
  tft.setCursor(8, 38);
  tft.println(F("Touch certification"));
  tft.setCursor(8, 52);
  tft.println(F("XP=D6 XM=A2 YP=A1 YM=D7"));
  tft.setCursor(8, 66);
  tft.println(F("Touch 5 points:"));
  tft.setCursor(8, 78);
  tft.println(F("TL TR BL BR CENTER"));

  Serial.println();
  Serial.println(F("LAB-03 TEST-04 - canonical touch certification"));
  Serial.print(F("LCD ID=0x"));
  Serial.println(id, HEX);
  Serial.println(F("Touch: XP=D6 XM=A2 YP=A1 YM=D7"));
  Serial.println(F("Calibration: LEFT=270 RIGHT=887 TOP=267 BOTTOM=879"));
  Serial.println(F("Rotation=0 (same as known-working reference)"));
  Serial.println(F("Touch order: TL -> TR -> BL -> BR -> CENTER"));
  Serial.println(F("One press -> one record."));
}

void loop() {
  TSPoint p = readTouch();
  const bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);

  if (armed && pressed) {
    int16_t sx = map(p.x, TS_LEFT, TS_RT, 0, tft.width() - 1);
    int16_t sy = map(p.y, TS_TOP, TS_BOT, 0, tft.height() - 1);

    sx = constrain(sx, 0, tft.width() - 1);
    sy = constrain(sy, 0, tft.height() - 1);

    if (lastX >= 0 && lastY >= 0) {
      drawCross(lastX, lastY, BLACK);
    }

    drawCross(sx, sy, GREEN);
    lastX = sx;
    lastY = sy;

    ++pointNo;

    Serial.print(F("POINT#"));
    Serial.print(pointNo);
    Serial.print(F(" RAW X="));
    Serial.print(p.x);
    Serial.print(F(" Y="));
    Serial.print(p.y);
    Serial.print(F(" Z="));
    Serial.print(p.z);
    Serial.print(F(" SCREEN X="));
    Serial.print(sx);
    Serial.print(F(" Y="));
    Serial.println(sy);

    armed = false;
  }

  if (!armed && !pressed) {
    delay(30);
    TSPoint q = readTouch();
    if (!(q.z > MINPRESSURE && q.z < MAXPRESSURE)) {
      Serial.println(F("READY"));
      armed = true;
    }
  }

  delay(15);
}
