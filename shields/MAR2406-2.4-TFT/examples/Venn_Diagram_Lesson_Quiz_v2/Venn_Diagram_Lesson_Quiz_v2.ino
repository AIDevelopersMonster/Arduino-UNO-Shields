/*
  Arduino UNO + MAR2406 2.4" TFT Touch Shield
  Venn Diagram Lesson + 10-question Quiz — V2

  V2 keeps LEARN and QUIZ conceptually separate:

  LEARN:
    - the diagram itself is not a touch target;
    - NEXT advances through the lesson;
    - each page shades and explains one set operation.

  QUIZ:
    - the diagram is unshaded;
    - each question asks for A ONLY, A AND B, or B ONLY;
    - one touch = one answer;
    - exactly 10 questions;
    - final score is shown as SCORE n/10.

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

const uint8_t QUIZ_TOTAL = 10;

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
bool quizFinished = false;

uint8_t op = OP_A;
uint8_t quizTarget = REGION_A_ONLY;
uint8_t quizQuestion = 1;
uint8_t quizScore = 0;
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
      tft.setCursor(PANEL_X + 5, 101);
      tft.print(F("inside A"));
      break;

    case OP_B:
      tft.print(F("All points"));
      tft.setCursor(PANEL_X + 5, 101);
      tft.print(F("inside B"));
      break;

    case OP_INTERSECTION:
      tft.print(F("In A AND B"));
      tft.setCursor(PANEL_X + 5, 101);
      tft.print(F("at once"));
      break;

    case OP_UNION:
      tft.print(F("In A OR B"));
      tft.setCursor(PANEL_X + 5, 101);
      tft.print(F("or both"));
      break;

    case OP_A_ONLY:
      tft.print(F("In A,"));
      tft.setCursor(PANEL_X + 5, 101);
      tft.print(F("not in B"));
      break;

    case OP_B_ONLY:
      tft.print(F("In B,"));
      tft.setCursor(PANEL_X + 5, 101);
      tft.print(F("not in A"));
      break;

    case OP_XOR:
      tft.print(F("A or B,"));
      tft.setCursor(PANEL_X + 5, 101);
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

static void drawVenn(uint8_t operation, bool shaded) {
  tft.fillRect(4, 38, 190, 194, BLACK);

  if (shaded) {
    uint16_t color = operationColor(operation);
    int16_t minX = AX - R;
    int16_t maxX = BX + R;
    int16_t minY = CY - R;
    int16_t maxY = CY + R;

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
  }

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
  tft.print(F("LESSON "));
  tft.print(op + 1);
  tft.print('/');
  tft.print(OP_COUNT);

  tft.setTextColor(CYAN);
  tft.setCursor(PANEL_X + 5, 84);
  printOperationName(op);

  tft.setTextColor(GREY);
  tft.setCursor(PANEL_X + 5, 91);
  printOperationHint(op);

  tft.setTextColor(WHITE);
  tft.setCursor(PANEL_X + 5, 126);
  tft.print(F("NEXT = lesson"));

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
  tft.setCursor(PANEL_X + 5, 69);
  tft.print(F("QUESTION "));
  tft.print(quizQuestion);
  tft.print('/');
  tft.print(QUIZ_TOTAL);

  tft.setCursor(PANEL_X + 5, 86);
  tft.print(F("TOUCH:"));

  tft.setTextSize(2);
  tft.setTextColor(CYAN);
  tft.setCursor(PANEL_X + 5, 101);
  printRegionName(quizTarget);

  tft.setTextSize(1);
  tft.setTextColor(GREEN);
  tft.setCursor(PANEL_X + 5, 129);
  tft.print(F("SCORE "));
  tft.print(quizScore);

  tft.setTextColor(RED);
  tft.setCursor(PANEL_X + 65, 129);
  tft.print(F("BAD "));
  tft.print(quizBad);

  tft.setTextColor(GREY);
  tft.setCursor(PANEL_X + 5, 145);
  tft.print(F("TOUCH TARGET"));

  drawButton(198, GREEN, F("LEARN"));
}

static void drawResultPanel() {
  clearPanel();

  tft.setTextSize(2);
  tft.setTextColor(YELLOW);
  tft.setCursor(PANEL_X + 6, 43);
  tft.print(F("RESULT"));

  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(PANEL_X + 5, 76);
  tft.print(F("SCORE"));

  tft.setTextSize(3);
  tft.setTextColor(GREEN);
  tft.setCursor(PANEL_X + 5, 91);
  tft.print(quizScore);
  tft.print('/');
  tft.print(QUIZ_TOTAL);

  tft.setTextSize(1);
  tft.setTextColor(RED);
  tft.setCursor(PANEL_X + 5, 126);
  tft.print(F("WRONG "));
  tft.print(quizBad);

  drawButton(157, CYAN, F("AGAIN"));
  drawButton(198, GREEN, F("LEARN"));
}

static void drawHeader() {
  tft.fillRect(0, 0, 320, 35, BLACK);

  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(8, 10);
  tft.print(F("VENN DIAGRAM"));

  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(232, 12);
  tft.print(F("V2"));
}

static bool inButton(int16_t x, int16_t y, int16_t buttonY) {
  return x >= PANEL_X + 5 && x < 315 &&
         y >= buttonY && y < buttonY + 31;
}

static void showAnswer(bool correct) {
  tft.fillRect(PANEL_X + 4, 145, 114, 18, BLACK);
  tft.setTextSize(1);
  tft.setCursor(PANEL_X + 5, 149);

  if (correct) {
    tft.setTextColor(GREEN);
    tft.print(F("CORRECT!"));
  } else {
    tft.setTextColor(RED);
    tft.print(F("WRONG"));
  }
}

static void chooseNextTarget() {
  uint8_t next = (uint8_t)random(1, 4);
  if (next == quizTarget) next = (uint8_t)(next % 3 + 1);
  quizTarget = next;
}

static void startLearn() {
  quizMode = false;
  quizFinished = false;
  drawHeader();
  drawVenn(op, true);
  drawLearnPanel();
}

static void startQuiz() {
  quizMode = true;
  quizFinished = false;
  quizQuestion = 1;
  quizScore = 0;
  quizBad = 0;

  randomSeed(micros());
  chooseNextTarget();

  drawHeader();
  drawVenn(op, false);
  drawQuizPanel();
}

static void nextQuizQuestion() {
  if (quizQuestion >= QUIZ_TOTAL) {
    quizFinished = true;
    drawVenn(op, false);
    drawResultPanel();
    return;
  }

  ++quizQuestion;
  chooseNextTarget();
  drawVenn(op, false);
  drawQuizPanel();
}

static void handleLearnTouch(int16_t x, int16_t y) {
  // In V2 the diagram itself is deliberately NOT interactive in LEARN.
  // The learner observes one operation at a time and advances with NEXT.
  if (inButton(x, y, 157)) {
    op = (uint8_t)((op + 1) % OP_COUNT);
    drawVenn(op, true);
    drawLearnPanel();
    return;
  }

  if (inButton(x, y, 198)) {
    startQuiz();
  }
}

static void handleQuizTouch(int16_t x, int16_t y) {
  if (quizFinished) {
    if (inButton(x, y, 157)) {
      startQuiz();
      return;
    }

    if (inButton(x, y, 198)) {
      startLearn();
    }
    return;
  }

  if (inButton(x, y, 198)) {
    startLearn();
    return;
  }

  if (x < PANEL_X) {
    uint8_t region = regionAt(x, y);
    bool correct = region == quizTarget;

    if (correct) ++quizScore;
    else ++quizBad;

    showAnswer(correct);
    delay(550);
    nextQuizQuestion();
  }
}

void setup() {
  Serial.begin(115200);

  tft.reset();
  uint16_t id = tft.readID();
  tft.begin(id);
  tft.setRotation(1);

  Serial.println();
  Serial.println(F("MAR2406 VENN LESSON + QUIZ V2"));
  Serial.print(F("LCD ID=0x"));
  Serial.println(id, HEX);
  Serial.println(F("QUIZ=10 QUESTIONS"));

  tft.fillScreen(BLACK);
  startLearn();
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
