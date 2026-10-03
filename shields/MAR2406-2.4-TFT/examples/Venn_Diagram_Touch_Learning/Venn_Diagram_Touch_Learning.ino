/*
  Arduino UNO + MAR2406 2.4" TFT Touch Shield
  Interactive Venn Diagram Learning Demo

  Two intersecting circles teach:
    A
    B
    A AND B   (intersection)
    A OR B    (union)
    A - B
    B - A
    A XOR B   (symmetric difference)

  QUIZ mode asks the student to touch:
    A ONLY
    A AND B
    B ONLY

  Verified LAB-03 hardware:
    LCD: ILI9341, ROT1 / 320x240
    Touch: XP=D6 XM=A2 YP=A1 YM=D7

  Certified ROT0 touch calibration:
    LEFT=153 RIGHT=930 TOP=962 BOTTOM=168
*/

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>

MCUFRIEND_kbv tft;

#define BLACK    0x0000
#define WHITE    0xFFFF
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define YELLOW   0xFFE0
#define MAGENTA  0xF81F
#define GREY     0x8410

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

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// Venn geometry in canonical 320x240 landscape.
const int16_t AX = 70;
const int16_t BX = 132;
const int16_t CY = 122;
const int16_t R  = 56;

const int16_t PANEL_X = 198;
const int16_t PANEL_W = 122;

enum {
  OP_A = 0,
  OP_B,
  OP_INTERSECTION,
  OP_UNION,
  OP_A_ONLY,
  OP_B_ONLY,
  OP_XOR,
  OP_COUNT
};

enum {
  REGION_OUTSIDE = 0,
  REGION_A_ONLY = 1,
  REGION_INTERSECTION = 2,
  REGION_B_ONLY = 3
};

bool touchArmed = true;
bool quizMode = false;

uint8_t op = OP_A;
uint8_t quizTarget = REGION_A_ONLY;
uint8_t quizOK = 0;
uint8_t quizBad = 0;

static void restoreSharedPins() {
  pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  pinMode(YM, OUTPUT);
}

static bool readTouch(int16_t &x, int16_t &y) {
  TSPoint p = ts.getPoint();
  restoreSharedPins();

  if (p.z <= MINPRESSURE || p.z >= MAXPRESSURE) return false;

  long px = map(p.x, TS_LEFT, TS_RT, 0, 239);
  long py = map(p.y, TS_TOP, TS_BOT, 0, 319);

  px = constrain(px, 0, 239);
  py = constrain(py, 0, 319);

  x = constrain((int16_t)py, 0, 319);
  y = constrain((int16_t)(239 - px), 0, 239);
  return true;
}

static bool insideCircle(int16_t x, int16_t y, int16_t cx) {
  int32_t dx = (int32_t)x - cx;
  int32_t dy = (int32_t)y - CY;
  return dx * dx + dy * dy <= (int32_t)R * R;
}

static uint8_t regionAt(int16_t x, int16_t y) {
  bool a = insideCircle(x, y, AX);
  bool b = insideCircle(x, y, BX);

  if (a && b) return REGION_INTERSECTION;
  if (a) return REGION_A_ONLY;
  if (b) return REGION_B_ONLY;
  return REGION_OUTSIDE;
}

static bool selectedPixel(uint8_t operation, bool a, bool b) {
  switch (operation) {
    case OP_A:            return a;
    case OP_B:            return b;
    case OP_INTERSECTION: return a && b;
    case OP_UNION:        return a || b;
    case OP_A_ONLY:       return a && !b;
    case OP_B_ONLY:       return b && !a;
    case OP_XOR:          return a != b;
  }
  return false;
}

static uint16_t operationColor(uint8_t operation) {
  switch (operation) {
    case OP_A:            return CYAN;
    case OP_B:            return YELLOW;
    case OP_INTERSECTION: return GREEN;
    case OP_UNION:        return MAGENTA;
    case OP_A_ONLY:       return CYAN;
    case OP_B_ONLY:       return YELLOW;
    default:               return RED;
  }
}

static void printOperationName(uint8_t operation) {
  switch (operation) {
    case OP_A:            tft.print(F("SET A")); break;
    case OP_B:            tft.print(F("SET B")); break;
    case OP_INTERSECTION: tft.print(F("A AND B")); break;
    case OP_UNION:        tft.print(F("A OR B")); break;
    case OP_A_ONLY:       tft.print(F("A - B")); break;
    case OP_B_ONLY:       tft.print(F("B - A")); break;
    case OP_XOR:          tft.print(F("A XOR B")); break;
  }
}

static void printOperationHint(uint8_t operation) {
  switch (operation) {
    case OP_A:
      tft.print(F("All points"));
      tft.setCursor(PANEL_X + 5, 93);
      tft.print(F("inside A"));
      break;

    case OP_B:
      tft.print(F("All points"));
      tft.setCursor(PANEL_X + 5, 93);
      tft.print(F("inside B"));
      break;

    case OP_INTERSECTION:
      tft.print(F("In A and B"));
      tft.setCursor(PANEL_X + 5, 93);
      tft.print(F("at once"));
      break;

    case OP_UNION:
      tft.print(F("In A or B"));
      tft.setCursor(PANEL_X + 5, 93);
      tft.print(F("(or both)"));
      break;

    case OP_A_ONLY:
      tft.print(F("In A, not B"));
      break;

    case OP_B_ONLY:
      tft.print(F("In B, not A"));
      break;

    case OP_XOR:
      tft.print(F("A or B,"));
      tft.setCursor(PANEL_X + 5, 93);
      tft.print(F("not both"));
      break;
  }
}

static void printRegionName(uint8_t region) {
  switch (region) {
    case REGION_A_ONLY:       tft.print(F("A ONLY")); break;
    case REGION_INTERSECTION: tft.print(F("A AND B")); break;
    case REGION_B_ONLY:       tft.print(F("B ONLY")); break;
    default:                  tft.print(F("OUTSIDE")); break;
  }
}

static void drawButton(int16_t y, uint16_t color,
                       const __FlashStringHelper *label) {
  tft.drawRect(PANEL_X + 5, y, 112, 31, color);
  tft.drawRect(PANEL_X + 6, y + 1, 110, 29, color);
  tft.setTextSize(1);
  tft.setTextColor(color);
  tft.setCursor(PANEL_X + 37, y + 12);
  tft.print(label);
}

static void drawVenn(uint8_t operation) {
  // Clear only the diagram area.
  tft.fillRect(4, 38, 190, 194, BLACK);

  uint16_t color = operationColor(operation);
  int16_t minX = AX - R;
  int16_t maxX = BX + R;
  int16_t minY = CY - R;
  int16_t maxY = CY + R;

  // Scanline fill: much faster than calling drawPixel for the whole box.
  for (int16_t y = minY; y <= maxY; ++y) {
    bool run = false;
    int16_t start = 0;

    for (int16_t x = minX; x <= maxX + 1; ++x) {
      bool active = false;

      if (x <= maxX) {
        bool a = insideCircle(x, y, AX);
        bool b = insideCircle(x, y, BX);
        active = selectedPixel(operation, a, b);
      }

      if (active && !run) {
        start = x;
        run = true;
      } else if (!active && run) {
        tft.drawFastHLine(start, y, x - start, color);
        run = false;
      }
    }
  }

  // Always keep both set boundaries visible.
  tft.drawCircle(AX, CY, R, WHITE);
  tft.drawCircle(AX, CY, R - 1, WHITE);
  tft.drawCircle(BX, CY, R, WHITE);
  tft.drawCircle(BX, CY, R - 1, WHITE);

  tft.setTextSize(2);
  tft.setTextColor(WHITE, BLACK);
  tft.setCursor(34, 48);
  tft.print('A');
  tft.setCursor(158, 48);
  tft.print('B');

  tft.setTextSize(1);
  tft.setTextColor(GREY, BLACK);
  tft.setCursor(24, 200);
  tft.print(F("TOUCH A REGION"));
}

static void clearPanel() {
  tft.fillRect(PANEL_X, 35, PANEL_W, 205, BLACK);
  tft.drawFastVLine(PANEL_X, 35, 205, GREY);
}

static void drawLearnPanel() {
  clearPanel();

  tft.setTextSize(2);
  tft.setTextColor(GREEN);
  tft.setCursor(PANEL_X + 6, 43);
  tft.print(F("LEARN"));

  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(PANEL_X + 5, 70);
  printOperationName(op);

  tft.setTextColor(GREY);
  tft.setCursor(PANEL_X + 5, 81);
  printOperationHint(op);

  tft.setTextColor(WHITE);
  tft.setCursor(PANEL_X + 5, 119);
  tft.print(F("TAP REGION:"));
  tft.setCursor(PANEL_X + 5, 131);
  tft.print(F("explore sets"));

  drawButton(157, CYAN, F("NEXT"));
  drawButton(198, YELLOW, F("QUIZ"));
}

static void drawQuizPanel() {
  clearPanel();

  tft.setTextSize(2);
  tft.setTextColor(YELLOW);
  tft.setCursor(PANEL_X + 6, 43);
  tft.print(F("QUIZ"));

  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(PANEL_X + 5, 70);
  tft.print(F("TOUCH:"));

  tft.setTextSize(2);
  tft.setTextColor(CYAN);
  tft.setCursor(PANEL_X + 5, 85);
  printRegionName(quizTarget);

  tft.setTextSize(1);
  tft.setTextColor(GREEN);
  tft.setCursor(PANEL_X + 5, 119);
  tft.print(F("OK "));
  tft.print(quizOK);

  tft.setTextColor(RED);
  tft.setCursor(PANEL_X + 62, 119);
  tft.print(F("BAD "));
  tft.print(quizBad);

  drawButton(157, CYAN, F("SKIP"));
  drawButton(198, GREEN, F("LEARN"));
}

static void showRegion(uint8_t region) {
  tft.fillRect(PANEL_X + 4, 118, 114, 32, BLACK);

  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(PANEL_X + 5, 119);
  tft.print(F("YOU TOUCHED:"));

  tft.setTextColor(CYAN);
  tft.setCursor(PANEL_X + 5, 133);
  printRegionName(region);
}

static void newQuizQuestion() {
  uint8_t next = (uint8_t)random(1, 4);

  if (next == quizTarget) {
    next = (uint8_t)(next % 3 + 1);
  }

  quizTarget = next;
  drawQuizPanel();
}

static void quizFeedback(bool correct, uint8_t region) {
  tft.fillRect(PANEL_X + 4, 132, 114, 21, BLACK);
  tft.setTextSize(1);
  tft.setCursor(PANEL_X + 5, 137);

  if (correct) {
    tft.setTextColor(GREEN);
    tft.print(F("CORRECT!"));
  } else {
    tft.setTextColor(RED);
    tft.print(F("TRY AGAIN: "));
    printRegionName(region);
  }
}

static void drawScreen() {
  tft.fillScreen(BLACK);

  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(8, 10);
  tft.print(F("VENN DIAGRAM"));

  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(205, 12);
  tft.print(F("TOUCH LEARNING"));

  drawVenn(op);

  if (quizMode) drawQuizPanel();
  else drawLearnPanel();
}

static bool inButton(int16_t x, int16_t y, int16_t buttonY) {
  return x >= PANEL_X + 5 && x < 315 &&
         y >= buttonY && y < buttonY + 31;
}

static void handleLearnTouch(int16_t x, int16_t y) {
  if (inButton(x, y, 157)) {
    op = (uint8_t)((op + 1) % OP_COUNT);
    drawVenn(op);
    drawLearnPanel();
    return;
  }

  if (inButton(x, y, 198)) {
    quizMode = true;
    randomSeed(micros());
    newQuizQuestion();
    return;
  }

  if (x < PANEL_X) {
    showRegion(regionAt(x, y));
  }
}

static void handleQuizTouch(int16_t x, int16_t y) {
  if (inButton(x, y, 157)) {
    newQuizQuestion();
    return;
  }

  if (inButton(x, y, 198)) {
    quizMode = false;
    drawVenn(op);
    drawLearnPanel();
    return;
  }

  if (x < PANEL_X) {
    uint8_t region = regionAt(x, y);

    if (region == quizTarget) {
      ++quizOK;
      quizFeedback(true, region);
      delay(500);
      newQuizQuestion();
    } else {
      ++quizBad;
      quizFeedback(false, region);
    }
  }
}

void setup() {
  Serial.begin(115200);

  tft.reset();
  uint16_t id = tft.readID();
  tft.begin(id);
  tft.setRotation(1);

  Serial.println();
  Serial.println(F("MAR2406 VENN DIAGRAM TOUCH LEARNING"));
  Serial.print(F("LCD ID=0x"));
  Serial.println(id, HEX);

  drawScreen();
}

void loop() {
  int16_t x, y;
  bool pressed = readTouch(x, y);

  if (touchArmed && pressed) {
    if (quizMode) handleQuizTouch(x, y);
    else handleLearnTouch(x, y);

    touchArmed = false;
  }

  if (!touchArmed && !pressed) {
    delay(25);
    int16_t x2, y2;
    if (!readTouch(x2, y2)) touchArmed = true;
  }

  delay(10);
}
