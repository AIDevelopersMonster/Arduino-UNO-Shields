/*
  LAB-03 / TEST-04
  Arduino UNO + MAR2406 2.4" TFT Touch Shield

  Interactive touch calibration wizard.

  Verified touch wiring for the current tested batch:
    XP = D6
    XM = A2
    YP = A1
    YM = D7

  Seed calibration:
    TS_LEFT = 167
    TS_RT   = 931
    TS_TOP  = 964
    TS_BOT  = 190

  The wizard keeps five fixed targets safely inside the display.
  It first collects one touch at each target, computes a new affine
  calibration from all five points, then verifies them again.

  During verification:
    - if mapped touch is within +/-4 px in both axes: target PASS;
    - otherwise the new measurement is added to the calibration set,
      coefficients are recomputed immediately, and the same target
      must be touched again.

  The target itself never moves. The green dot is the mapped touch.
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
#define RED     0xF800

#define MINPRESSURE 40
#define MAXPRESSURE 2000

const int XP = 6;
const int XM = A2;
const int YP = A1;
const int YM = 7;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

static int16_t calLeft   = 167;
static int16_t calRight  = 931;
static int16_t calTop    = 964;
static int16_t calBottom = 190;

static const int16_t TARGET_R = 10;
static const int16_t MARK_R   = 2;
static const int16_t MARGIN   = 20;
static const int16_t TOL      = 4;
static const uint8_t MAX_SAMPLES = 30;

struct Target {
  int16_t x;
  int16_t y;
  const char *name;
};

struct Sample {
  int16_t screenX;
  int16_t screenY;
  int16_t rawX;
  int16_t rawY;
};

Target targets[5];
Sample samples[MAX_SAMPLES];

static uint8_t sampleCount = 0;
static uint8_t currentTarget = 0;
static bool armed = true;
static bool verificationPhase = false;
static uint8_t passedTargets = 0;

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

static void drawTargets() {
  for (uint8_t i = 0; i < 5; ++i) {
    uint16_t c = (i == currentTarget) ? YELLOW : CYAN;
    drawTarget(targets[i].x, targets[i].y, c);
  }
}

static void clearStatusLine() {
  tft.fillRect(0, 54, tft.width(), 14, BLACK);
}

static void showStatus(const __FlashStringHelper *msg, uint16_t color) {
  clearStatusLine();
  tft.setTextSize(1);
  tft.setTextColor(color);
  tft.setCursor(8, 56);
  tft.print(msg);
}

static void addSample(const Target &t, const TSPoint &p) {
  if (sampleCount >= MAX_SAMPLES) return;
  samples[sampleCount].screenX = t.x;
  samples[sampleCount].screenY = t.y;
  samples[sampleCount].rawX = p.x;
  samples[sampleCount].rawY = p.y;
  ++sampleCount;
}

static bool fitAxis(bool xAxis, int16_t screenMax, int16_t &rawAt0, int16_t &rawAtMax) {
  if (sampleCount < 2) return false;

  long n = sampleCount;
  long sumS = 0;
  long sumR = 0;
  long sumSS = 0;
  long sumSR = 0;

  for (uint8_t i = 0; i < sampleCount; ++i) {
    long s = xAxis ? samples[i].screenX : samples[i].screenY;
    long r = xAxis ? samples[i].rawX    : samples[i].rawY;
    sumS  += s;
    sumR  += r;
    sumSS += s * s;
    sumSR += s * r;
  }

  long denom = n * sumSS - sumS * sumS;
  if (denom == 0) return false;

  float slope = (float)(n * sumSR - sumS * sumR) / (float)denom;
  float intercept = ((float)sumR - slope * (float)sumS) / (float)n;

  rawAt0 = (int16_t)(intercept + (intercept >= 0 ? 0.5f : -0.5f));
  float end = intercept + slope * screenMax;
  rawAtMax = (int16_t)(end + (end >= 0 ? 0.5f : -0.5f));
  return true;
}

static void recomputeCalibration() {
  int16_t l, r, t, b;

  if (fitAxis(true, tft.width() - 1, l, r)) {
    calLeft = l;
    calRight = r;
  }

  if (fitAxis(false, tft.height() - 1, t, b)) {
    calTop = t;
    calBottom = b;
  }

  Serial.print(F("CAL LEFT="));
  Serial.print(calLeft);
  Serial.print(F(" RIGHT="));
  Serial.print(calRight);
  Serial.print(F(" TOP="));
  Serial.print(calTop);
  Serial.print(F(" BOTTOM="));
  Serial.println(calBottom);
}

static int16_t mapX(int16_t raw) {
  long v = map(raw, calLeft, calRight, 0, tft.width() - 1);
  return constrain((int16_t)v, 0, tft.width() - 1);
}

static int16_t mapY(int16_t raw) {
  long v = map(raw, calTop, calBottom, 0, tft.height() - 1);
  return constrain((int16_t)v, 0, tft.height() - 1);
}

static void printTouch(const Target &t, const TSPoint &p, int16_t sx, int16_t sy) {
  int16_t dx = sx - t.x;
  int16_t dy = sy - t.y;

  Serial.print(F("TARGET="));
  Serial.print(t.name);
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
  Serial.print(t.x);
  Serial.print(F(" Y="));
  Serial.print(t.y);
  Serial.print(F(" ERR dx="));
  Serial.print(dx);
  Serial.print(F(" dy="));
  Serial.println(dy);
}

static bool withinTolerance(const Target &t, int16_t sx, int16_t sy) {
  return abs(sx - t.x) <= TOL && abs(sy - t.y) <= TOL;
}

static void activateTarget(uint8_t index) {
  currentTarget = index;
  tft.fillScreen(BLACK);

  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(8, 8);
  tft.println(F("TOUCH CALIBRATION"));

  tft.setTextSize(1);
  tft.setCursor(8, 34);
  if (!verificationPhase) {
    tft.println(F("Phase 1: collect 5 targets"));
  } else {
    tft.println(F("Phase 2: verify / refine"));
  }

  tft.setCursor(8, 44);
  tft.print(F("Touch: "));
  tft.print(targets[currentTarget].name);

  drawTargets();
}

void setup() {
  Serial.begin(115200);

  tft.reset();
  uint16_t id = tft.readID();
  tft.begin(id);
  tft.setRotation(0);

  const int16_t w = tft.width();
  const int16_t h = tft.height();

  targets[0] = {MARGIN,           MARGIN + 50,     "TL"};
  targets[1] = {w - 1 - MARGIN,   MARGIN + 50,     "TR"};
  targets[2] = {MARGIN,           h - 1 - MARGIN,  "BL"};
  targets[3] = {w - 1 - MARGIN,   h - 1 - MARGIN,  "BR"};
  targets[4] = {w / 2,            h / 2,           "CENTER"};

  Serial.println();
  Serial.println(F("LAB-03 TEST-04 - interactive touch calibration"));
  Serial.print(F("LCD ID=0x"));
  Serial.println(id, HEX);
  Serial.println(F("Touch: XP=D6 XM=A2 YP=A1 YM=D7"));
  Serial.println(F("Seed calibration: LEFT=167 RIGHT=931 TOP=964 BOTTOM=190"));
  Serial.println(F("Tolerance: +/-4 px"));
  Serial.println(F("Phase 1: TL -> TR -> BL -> BR -> CENTER"));
  Serial.println(F("Phase 2: same targets; failed target is repeated after immediate recalibration."));

  activateTarget(0);
}

void loop() {
  TSPoint p = readTouch();
  const bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);

  if (armed && pressed) {
    Target &t = targets[currentTarget];

    int16_t sx = mapX(p.x);
    int16_t sy = mapY(p.y);

    tft.fillCircle(sx, sy, MARK_R, GREEN);
    printTouch(t, p, sx, sy);

    if (!verificationPhase) {
      addSample(t, p);

      if (currentTarget < 4) {
        ++currentTarget;
        delay(200);
        activateTarget(currentTarget);
      } else {
        recomputeCalibration();
        verificationPhase = true;
        passedTargets = 0;
        delay(400);
        activateTarget(0);
      }
    } else {
      if (withinTolerance(t, sx, sy)) {
        Serial.print(F("PASS "));
        Serial.println(t.name);
        showStatus(F("PASS - next target"), GREEN);
        ++passedTargets;

        if (currentTarget < 4) {
          ++currentTarget;
          delay(350);
          activateTarget(currentTarget);
        } else {
          tft.fillScreen(BLACK);
          tft.setTextColor(GREEN);
          tft.setTextSize(2);
          tft.setCursor(18, 80);
          tft.println(F("CALIBRATION"));
          tft.setCursor(46, 108);
          tft.println(F("COMPLETE"));

          tft.setTextSize(1);
          tft.setTextColor(WHITE);
          tft.setCursor(16, 150);
          tft.print(F("LEFT=")); tft.println(calLeft);
          tft.setCursor(16, 164);
          tft.print(F("RIGHT=")); tft.println(calRight);
          tft.setCursor(16, 178);
          tft.print(F("TOP=")); tft.println(calTop);
          tft.setCursor(16, 192);
          tft.print(F("BOTTOM=")); tft.println(calBottom);

          Serial.println(F("CALIBRATION COMPLETE"));
          recomputeCalibration();
          while (true) delay(1000);
        }
      } else {
        Serial.print(F("REFINE "));
        Serial.println(t.name);

        addSample(t, p);
        recomputeCalibration();

        showStatus(F("Adjusted - touch same target again"), RED);
        delay(450);
        activateTarget(currentTarget);
      }
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
