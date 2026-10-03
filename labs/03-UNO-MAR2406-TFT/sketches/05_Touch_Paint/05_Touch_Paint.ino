/*
  LAB-03 / TEST-05
  Arduino UNO + MAR2406 2.4" TFT Touch Shield

  Touch Paint / HMI test inspired by the factory "touch_pen" demo,
  but rewritten for the verified current-batch touch wiring and the
  project canonical display orientation ROT1 / 320x240.

  Verified current-batch touch wiring:
    XP = D6
    XM = A2
    YP = A1
    YM = D7

  Certified portrait (ROT0) calibration seed:
    LEFT   = 151
    RIGHT  = 920
    TOP    = 964
    BOTTOM = 169

  Touch is first mapped into the certified ROT0 portrait coordinate
  system (240x320), then transformed into ROT1 landscape:
    landscapeX = portraitY
    landscapeY = 239 - portraitX

  UI:
    - six colors
    - three pen sizes
    - CLEAR button
    - free drawing area

  Purpose:
    Verify that calibrated touch follows the stylus across the full
    320x240 canonical HMI coordinate system without dead zones,
    axis swap, mirroring, or gross nonlinearity.
*/

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>

MCUFRIEND_kbv tft;

#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define GREY    0x8410

#define MINPRESSURE 40
#define MAXPRESSURE 2000

const int XP = 6;
const int XM = A2;
const int YP = A1;
const int YM = 7;

const int TS_LEFT = 151;
const int TS_RT   = 920;
const int TS_TOP  = 964;
const int TS_BOT  = 169;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

static const int16_t TOOLBAR_H = 40;
static const uint8_t COLOR_COUNT = 6;
static const uint16_t COLORS[COLOR_COUNT] = {
  RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA
};

static uint16_t currentColor = RED;
static uint8_t currentRadius = 2;
static bool wasPressed = false;
static int16_t lastX = -1;
static int16_t lastY = -1;

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

static void drawToolbar() {
  tft.fillRect(0, 0, 320, TOOLBAR_H, BLACK);
  tft.drawFastHLine(0, TOOLBAR_H - 1, 320, WHITE);

  // Six 28 px color swatches: x = 0..167
  for (uint8_t i = 0; i < COLOR_COUNT; ++i) {
    int16_t x = i * 28;
    tft.fillRect(x + 2, 4, 24, 28, COLORS[i]);
    if (COLORS[i] == currentColor) {
      tft.drawRect(x, 2, 28, 32, WHITE);
    }
  }

  // Pen size buttons: 172..267
  const int16_t penX[3] = {172, 204, 236};
  const uint8_t radii[3] = {1, 2, 4};
  for (uint8_t i = 0; i < 3; ++i) {
    tft.drawRect(penX[i], 4, 28, 28,
                 (currentRadius == radii[i]) ? YELLOW : GREY);
    tft.fillCircle(penX[i] + 14, 18, radii[i], WHITE);
  }

  // Clear button: 272..319
  tft.fillRect(272, 4, 46, 28, GREY);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.setCursor(280, 14);
  tft.print(F("CLEAR"));
}

static void clearCanvas() {
  tft.fillRect(0, TOOLBAR_H, 320, 240 - TOOLBAR_H, BLACK);

  // Subtle reference frame helps reveal edge/dead-zone problems.
  tft.drawRect(1, TOOLBAR_H + 1, 318, 198, GREY);

  // Five tiny reference marks, deliberately away from toolbar.
  const int16_t pts[5][2] = {
    {12, TOOLBAR_H + 12},
    {307, TOOLBAR_H + 12},
    {12, 227},
    {307, 227},
    {160, 140}
  };
  for (uint8_t i = 0; i < 5; ++i) {
    int16_t x = pts[i][0], y = pts[i][1];
    tft.drawFastHLine(x - 3, y, 7, GREY);
    tft.drawFastVLine(x, y - 3, 7, GREY);
  }
}

static void selectColor(uint8_t index) {
  if (index >= COLOR_COUNT) return;
  currentColor = COLORS[index];
  drawToolbar();
  Serial.print(F("COLOR="));
  Serial.println(index);
}

static void selectPen(uint8_t radius) {
  currentRadius = radius;
  drawToolbar();
  Serial.print(F("PEN R="));
  Serial.println(radius);
}

static void handleToolbar(int16_t x, int16_t y) {
  (void)y;

  if (x < 168) {
    uint8_t idx = x / 28;
    if (idx < COLOR_COUNT) selectColor(idx);
    return;
  }

  if (x >= 172 && x < 200) {
    selectPen(1);
    return;
  }
  if (x >= 204 && x < 232) {
    selectPen(2);
    return;
  }
  if (x >= 236 && x < 264) {
    selectPen(4);
    return;
  }

  if (x >= 272) {
    clearCanvas();
    drawToolbar();
    Serial.println(F("CLEAR"));
    return;
  }
}

static void drawStrokePoint(int16_t x, int16_t y) {
  if (currentRadius <= 1) {
    tft.drawPixel(x, y, currentColor);
  } else {
    tft.fillCircle(x, y, currentRadius, currentColor);
  }
}

void setup() {
  Serial.begin(115200);

  tft.reset();
  uint16_t id = tft.readID();
  tft.begin(id);
  tft.setRotation(1);  // Project canonical HMI orientation: 320x240

  tft.fillScreen(BLACK);
  clearCanvas();
  drawToolbar();

  Serial.println();
  Serial.println(F("LAB-03 TEST-05 - Touch Paint"));
  Serial.print(F("LCD ID=0x"));
  Serial.println(id, HEX);
  Serial.println(F("Display: ROT1 / 320x240"));
  Serial.println(F("Touch: XP=D6 XM=A2 YP=A1 YM=D7"));
  Serial.println(F("Calibration ROT0: LEFT=151 RIGHT=920 TOP=964 BOTTOM=169"));
  Serial.println(F("Transform: ROT0 touch -> ROT1 HMI"));
  Serial.println(F("Draw across all edges and center. Use palette / pen / CLEAR."));
}

void loop() {
  TSPoint p = readTouchRaw();

  int16_t x, y;
  bool pressed = rawToLandscape(p, x, y);

  if (pressed) {
    if (!wasPressed) {
      Serial.print(F("DOWN X="));
      Serial.print(x);
      Serial.print(F(" Y="));
      Serial.print(y);
      Serial.print(F(" RAW="));
      Serial.print(p.x);
      Serial.print(',');
      Serial.print(p.y);
      Serial.print(F(" Z="));
      Serial.println(p.z);
    }

    if (y < TOOLBAR_H) {
      if (!wasPressed) handleToolbar(x, y);
      lastX = -1;
      lastY = -1;
    } else {
      // Draw a continuous stroke. Interpolate with a line for fast moves.
      if (lastX >= 0 && lastY >= TOOLBAR_H) {
        tft.drawLine(lastX, lastY, x, y, currentColor);
      }
      drawStrokePoint(x, y);
      lastX = x;
      lastY = y;
    }

    wasPressed = true;
  } else {
    if (wasPressed) {
      Serial.println(F("UP"));
    }
    wasPressed = false;
    lastX = -1;
    lastY = -1;
  }

  delay(8);
}
