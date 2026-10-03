/*
  LAB-03 / TEST-04
  Arduino UNO + MAR2406 2.4" TFT Touch Shield

  Touch certification using the working wiring/calibration from
  TFT_LCD_Break_game.ino.

  Touch wiring:
    XP = D6
    XM = A2
    YP = A1
    YM = D7

  Calibration:
    TS_LEFT = 270
    TS_RT   = 887
    TS_TOP  = 267
    TS_BOT  = 879

  Rotation:
    0 (same as the known-working game reference)

  Test:
    Five fixed targets are drawn safely inside the display:
      TL, TR, BL, BR, CENTER
    Touch each target in order.
    A small green dot marks the mapped touch position.
*/

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>

MCUFRIEND_kbv tft;

#define BLACK   0x0000
#define WHITE   0xFFFF
#define GREEN   0x07E0
#define CYAN    0x07FF
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

static const int16_t TARGET_R = 10;
static const int16_t MARK_R   = 2;
static const int16_t MARGIN   = 20;

struct Target {
  int16_t x;
  int16_t y;
  const char *name;
};

Target targets[5];

static bool armed = true;
static uint8_t currentTarget = 0;

static void restoreSharedPins() {
  pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  pinMode(YM, OUTPUT);
}

static TSPoint readTouch() {
  TSPoint p = ts.getPoint();
  restoreSharedPins();
  return p;
}

static void drawTarget(int16_t x, int16_t y, uint16_t color) {
  tft.drawCircle(x, y, TARGET_R, color);
  tft.drawFastHLine(x - 5, y, 11, color);
  tft.drawFastVLine(x, y - 5, 11, color);
}

static void drawAllTargets() {
  for (uint8_t i = 0; i < 5; ++i) {
    drawTarget(targets[i].x, targets[i].y, (i == currentTarget) ? YELLOW : CYAN);
  }
}

void setup() {
  Serial.begin(115200);

  tft.reset();
  uint16_t id = tft.readID();
  tft.begin(id);
  tft.setRotation(0);
  tft.fillScreen(BLACK);

  const int16_t w = tft.width();
  const int16_t h = tft.height();

  targets[0] = {MARGIN,         MARGIN + 50, "TL"};
  targets[1] = {w - 1 - MARGIN, MARGIN + 50, "TR"};
  targets[2] = {MARGIN,         h - 1 - MARGIN, "BL"};
  targets[3] = {w - 1 - MARGIN, h - 1 - MARGIN, "BR"};
  targets[4] = {w / 2,          h / 2, "CENTER"};

  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(8, 8);
  tft.println(F("LAB-03 TEST-04"));

  tft.setTextSize(1);
  tft.setCursor(8, 32);
  tft.println(F("Touch yellow target"));
  tft.setCursor(8, 44);
  tft.println(F("TL TR BL BR CENTER"));

  drawAllTargets();

  Serial.println();
  Serial.println(F("LAB-03 TEST-04 - five target touch check"));
  Serial.print(F("LCD ID=0x"));
  Serial.println(id, HEX);
  Serial.println(F("Touch: XP=D6 XM=A2 YP=A1 YM=D7"));
  Serial.println(F("Calibration: LEFT=270 RIGHT=887 TOP=267 BOTTOM=879"));
  Serial.println(F("Rotation=0"));
  Serial.println(F("Targets: TL -> TR -> BL -> BR -> CENTER"));
}

void loop() {
  TSPoint p = readTouch();
  const bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);

  if (armed && pressed) {
    int16_t sx = map(p.x, TS_LEFT, TS_RT, 0, tft.width() - 1);
    int16_t sy = map(p.y, TS_TOP, TS_BOT, 0, tft.height() - 1);

    sx = constrain(sx, 0, tft.width() - 1);
    sy = constrain(sy, 0, tft.height() - 1);

    // Small marker only: no long cross that can visually run outside the screen.
    tft.fillCircle(sx, sy, MARK_R, GREEN);

    Serial.print(F("TARGET="));
    Serial.print(targets[currentTarget].name);
    Serial.print(F(" RAW X="));
    Serial.print(p.x);
    Serial.print(F(" Y="));
    Serial.print(p.y);
    Serial.print(F(" Z="));
    Serial.print(p.z);
    Serial.print(F(" SCREEN X="));
    Serial.print(sx);
    Serial.print(F(" Y="));
    Serial.print(sy);
    Serial.print(F(" EXPECT X="));
    Serial.print(targets[currentTarget].x);
    Serial.print(F(" Y="));
    Serial.println(targets[currentTarget].y);

    drawTarget(targets[currentTarget].x, targets[currentTarget].y, CYAN);

    if (currentTarget < 4) {
      ++currentTarget;
      drawTarget(targets[currentTarget].x, targets[currentTarget].y, YELLOW);
    } else {
      tft.setTextColor(GREEN);
      tft.setTextSize(1);
      tft.setCursor(8, 56);
      tft.println(F("5 points captured"));
    }

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
