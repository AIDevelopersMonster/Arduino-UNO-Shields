/*
  Arduino UNO + MAR2406 2.4" TFT Touch Shield
  BOOLEAN ALGEBRA HMI v0.1

  Verified hardware:
    LCD: ILI9341, ROT1 / 320x240
    Touch: XP=D6 XM=A2 YP=A1 YM=D7

  Certified ROT0 touch calibration:
    LEFT=153 RIGHT=930 TOP=962 BOTTOM=168
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

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

enum {
  MODE_LEARN = 0,
  MODE_QUIZ  = 1
};

enum {
  OP_NOTA = 0,
  OP_AND,
  OP_OR,
  OP_XOR,
  OP_NAND,
  OP_NOR,
  OP_XNOR,
  LESSON_COUNT
};

uint8_t mode = MODE_LEARN;
uint8_t lesson = 0;

uint8_t valA = 0;
uint8_t valB = 0;

uint8_t quizA = 0;
uint8_t quizB = 0;
uint8_t quizOp = 0;
uint8_t quizAnswer = 0;
uint8_t quizState = 0;   // 0 none, 1 correct, 2 wrong

bool touchArmed = true;

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

static bool inRect(int16_t x, int16_t y, int16_t rx, int16_t ry, int16_t rw, int16_t rh) {
  return (x >= rx && x < rx + rw && y >= ry && y < ry + rh);
}

static void drawButton(int16_t x, int16_t y, int16_t w, int16_t h,
                       uint16_t color, const __FlashStringHelper *label, uint8_t textSize) {
  tft.drawRect(x, y, w, h, color);
  tft.drawRect(x + 1, y + 1, w - 2, h - 2, color);
  tft.setTextColor(color);
  tft.setTextSize(textSize);
  tft.setCursor(x + 12, y + (h / 2) - (textSize * 4));
  tft.print(label);
}

static const __FlashStringHelper* opName(uint8_t op) {
  switch (op) {
    case OP_NOTA: return F("NOT A");
    case OP_AND:  return F("A AND B");
    case OP_OR:   return F("A OR B");
    case OP_XOR:  return F("A XOR B");
    case OP_NAND: return F("A NAND B");
    case OP_NOR:  return F("A NOR B");
    case OP_XNOR: return F("A XNOR B");
  }
  return F("?");
}

static const __FlashStringHelper* opHint(uint8_t op) {
  switch (op) {
    case OP_NOTA: return F("Invert A");
    case OP_AND:  return F("1 only if both 1");
    case OP_OR:   return F("1 if any input is 1");
    case OP_XOR:  return F("1 if inputs differ");
    case OP_NAND: return F("NOT(AND)");
    case OP_NOR:  return F("NOT(OR)");
    case OP_XNOR: return F("1 if inputs equal");
  }
  return F("");
}

static uint8_t evalOp(uint8_t op, uint8_t a, uint8_t b) {
  switch (op) {
    case OP_NOTA: return a ? 0 : 1;
    case OP_AND:  return (a && b) ? 1 : 0;
    case OP_OR:   return (a || b) ? 1 : 0;
    case OP_XOR:  return (a != b) ? 1 : 0;
    case OP_NAND: return (a && b) ? 0 : 1;
    case OP_NOR:  return (a || b) ? 0 : 1;
    case OP_XNOR: return (a == b) ? 1 : 0;
  }
  return 0;
}

static void drawHeader() {
  tft.fillRect(0, 0, 320, 28, BLACK);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(8, 7);
  tft.print(F("BOOLEAN ALGEBRA"));

  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(286, 8);
  tft.print(F("v1"));
}

static void drawSidebar() {
  tft.drawFastVLine(218, 30, 205, GREY);

  tft.setTextSize(2);
  tft.setCursor(230, 40);
  if (mode == MODE_LEARN) {
    tft.setTextColor(GREEN);
    tft.print(F("LEARN"));
  } else {
    tft.setTextColor(YELLOW);
    tft.print(F("QUIZ"));
  }

  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(230, 67);
  if (mode == MODE_LEARN) {
    tft.print(F("LESSON "));
    tft.print(lesson + 1);
    tft.print(F("/"));
    tft.print(LESSON_COUNT);
  } else {
    tft.print(F("QUESTION"));
  }

  drawButton(226, 100, 84, 28, CYAN, F("PREV"), 1);
  drawButton(226, 136, 84, 28, CYAN, F("NEXT"), 1);

  if (mode == MODE_LEARN) {
    drawButton(226, 176, 84, 34, YELLOW, F("QUIZ"), 2);
  } else {
    drawButton(226, 176, 84, 34, GREEN, F("LEARN"), 1);
  }
}

static void drawABControls() {
  drawButton(18, 194, 84, 30, CYAN, F("A"), 2);
  drawButton(116, 194, 84, 30, YELLOW, F("B"), 2);

  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(54, 201);
  tft.print(valA);

  tft.setCursor(152, 201);
  tft.print(valB);
}

static void drawTruthTable(uint8_t op) {
  tft.setTextSize(1);
  tft.setTextColor(WHITE);

  int16_t x0 = 18;
  int16_t y0 = 106;

  if (op == OP_NOTA) {
    tft.setCursor(x0, y0);
    tft.print(F("A   Q"));

    for (uint8_t r = 0; r < 2; ++r) {
      uint8_t a = r;
      uint8_t q = evalOp(op, a, 0);

      if (a == valA) tft.setTextColor(CYAN);
      else tft.setTextColor(WHITE);

      tft.setCursor(x0, y0 + 18 + r * 16);
      tft.print(a);
      tft.setCursor(x0 + 24, y0 + 18 + r * 16);
      tft.print(q);
    }
  } else {
    tft.setCursor(x0, y0);
    tft.print(F("A   B   Q"));

    for (uint8_t r = 0; r < 4; ++r) {
      uint8_t a = (r >> 1) & 1;
      uint8_t b = r & 1;
      uint8_t q = evalOp(op, a, b);

      if (a == valA && b == valB) tft.setTextColor(CYAN);
      else tft.setTextColor(WHITE);

      tft.setCursor(x0, y0 + 18 + r * 16);
      tft.print(a);
      tft.setCursor(x0 + 24, y0 + 18 + r * 16);
      tft.print(b);
      tft.setCursor(x0 + 50, y0 + 18 + r * 16);
      tft.print(q);
    }
  }
}

static void drawLearnScreen() {
  tft.fillScreen(BLACK);
  drawHeader();
  drawSidebar();

  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(18, 38);
  tft.print(opName(lesson));

  tft.setTextSize(1);
  tft.setTextColor(GREY);
  tft.setCursor(18, 62);
  tft.print(opHint(lesson));

  tft.setTextColor(WHITE);
  tft.setCursor(18, 82);
  tft.print(F("A="));
  tft.print(valA);
  tft.print(F("   B="));
  tft.print(valB);
  tft.print(F("   RESULT="));
  tft.print(evalOp(lesson, valA, valB));

  drawTruthTable(lesson);
  drawABControls();
}

static void newQuiz() {
  quizOp = (uint8_t)random(LESSON_COUNT);
  quizA = (uint8_t)random(2);
  quizB = (uint8_t)random(2);
  quizAnswer = evalOp(quizOp, quizA, quizB);
  quizState = 0;
}

static void drawQuizScreen() {
  tft.fillScreen(BLACK);
  drawHeader();
  drawSidebar();

  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(18, 38);
  tft.print(F("SOLVE"));

  tft.setTextSize(2);
  tft.setCursor(18, 72);
  tft.setTextColor(CYAN);
  tft.print(F("A="));
  tft.print(quizA);

  tft.setTextColor(YELLOW);
  tft.setCursor(80, 72);
  tft.print(F("B="));
  tft.print(quizB);

  tft.setTextColor(WHITE);
  tft.setCursor(18, 104);
  tft.print(opName(quizOp));

  tft.setTextSize(1);
  tft.setTextColor(GREY);
  tft.setCursor(18, 130);
  tft.print(F("What is the result?"));

  drawButton(22, 170, 80, 42, CYAN, F("0"), 2);
  drawButton(118, 170, 80, 42, CYAN, F("1"), 2);

  if (quizState == 1) {
    tft.setTextColor(GREEN);
    tft.setTextSize(2);
    tft.setCursor(20, 220);
    tft.print(F("CORRECT"));
  } else if (quizState == 2) {
    tft.setTextColor(RED);
    tft.setTextSize(2);
    tft.setCursor(20, 220);
    tft.print(F("TRY AGAIN"));
  }
}

static void redraw() {
  if (mode == MODE_LEARN) drawLearnScreen();
  else drawQuizScreen();
}

static void handleLearnTouch(int16_t x, int16_t y) {
  if (inRect(x, y, 226, 100, 84, 28)) {
    if (lesson == 0) lesson = LESSON_COUNT - 1;
    else --lesson;
    redraw();
    return;
  }

  if (inRect(x, y, 226, 136, 84, 28)) {
    lesson = (lesson + 1) % LESSON_COUNT;
    redraw();
    return;
  }

  if (inRect(x, y, 226, 176, 84, 34)) {
    mode = MODE_QUIZ;
    newQuiz();
    redraw();
    return;
  }

  if (inRect(x, y, 18, 194, 84, 30)) {
    valA ^= 1;
    redraw();
    return;
  }

  if (inRect(x, y, 116, 194, 84, 30)) {
    valB ^= 1;
    redraw();
    return;
  }
}

static void handleQuizTouch(int16_t x, int16_t y) {
  if (inRect(x, y, 226, 100, 84, 28) || inRect(x, y, 226, 136, 84, 28)) {
    newQuiz();
    redraw();
    return;
  }

  if (inRect(x, y, 226, 176, 84, 34)) {
    mode = MODE_LEARN;
    redraw();
    return;
  }

  if (inRect(x, y, 22, 170, 80, 42)) {
    quizState = (quizAnswer == 0) ? 1 : 2;
    redraw();
    return;
  }

  if (inRect(x, y, 118, 170, 80, 42)) {
    quizState = (quizAnswer == 1) ? 1 : 2;
    redraw();
    return;
  }
}

void setup() {
  Serial.begin(115200);
  randomSeed(micros());

  tft.reset();
  uint16_t id = tft.readID();
  tft.begin(id);
  tft.setRotation(1);
  tft.fillScreen(BLACK);

  Serial.println(F("BOOLEAN ALGEBRA HMI"));
  Serial.print(F("LCD ID=0x"));
  Serial.println(id, HEX);
  Serial.println(F("ROT1 320x240"));
  Serial.println(F("TOUCH XP=D6 XM=A2 YP=A1 YM=D7"));

  redraw();
}

void loop() {
  int16_t x, y;
  bool pressed = readTouch(x, y);

  if (touchArmed && pressed) {
    if (mode == MODE_LEARN) handleLearnTouch(x, y);
    else handleQuizTouch(x, y);

    touchArmed = false;
  }

  if (!touchArmed && !pressed) {
    delay(25);
    int16_t x2, y2;
    if (!readTouch(x2, y2)) touchArmed = true;
  }

  delay(10);
}
