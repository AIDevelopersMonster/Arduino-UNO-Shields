/*
  Arduino UNO + MAR2406 2.4" TFT Touch Shield
  Tic-Tac-Toe: Player vs Arduino

  Independent MAR2406 adaptation inspired by the Dumblebots / Aditya Agarwal
  ILI9486 Tic-Tac-Toe example. No upstream source code is copied here.

  Verified LAB-03 hardware:
    LCD: ILI9341, ROT1 / 320x240
    Touch: XP=D6 XM=A2 YP=A1 YM=D7

  Certified ROT0 touch calibration:
    LEFT=153 RIGHT=930 TOP=962 BOTTOM=168

  Game model:
    - player chooses X or O;
    - X always starts;
    - Arduino uses the opposite piece;
    - Arduino uses minimax with alpha-beta pruning;
    - with legal play, the human cannot beat Arduino;
    - winning three marks blink and finish in red;
    - touch after GAME OVER returns to the piece-selection menu.

  This example intentionally does NOT use microSD so it remains much smaller
  than TEST-08 and leaves generous flash headroom on the ATmega328P.
*/

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>

MCUFRIEND_kbv tft;

#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define YELLOW  0xFFE0
#define GREY    0x8410
#define BLUE    0x001F

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

const int16_t BX = 8;
const int16_t BY = 43;
const int16_t CELL = 62;
const int16_t BS = CELL * 3;
const int16_t SX = 205;

enum {
  EMPTY = 0,
  PIECE_X = 1,
  PIECE_O = 2
};

enum {
  SCREEN_MENU = 0,
  SCREEN_PLAY = 1,
  SCREEN_OVER = 2
};

uint8_t board[9];
uint8_t playerPiece = PIECE_X;
uint8_t arduinoPiece = PIECE_O;
uint8_t turn = PIECE_X;
uint8_t moves = 0;
uint8_t screenState = SCREEN_MENU;

bool touchArmed = true;
unsigned long aiReadyAt = 0;

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

static void drawButton(int16_t x, int16_t y, int16_t w, int16_t h,
                       uint16_t color, const __FlashStringHelper *label) {
  tft.drawRect(x, y, w, h, color);
  tft.drawRect(x + 1, y + 1, w - 2, h - 2, color);
  tft.setTextColor(color);
  tft.setTextSize(2);
  tft.setCursor(x + 14, y + 14);
  tft.print(label);
}

static void drawMenu() {
  screenState = SCREEN_MENU;
  tft.fillScreen(BLACK);

  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(47, 16);
  tft.print(F("TIC TAC TOE"));

  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(70, 52);
  tft.print(F("Arduino UNO + MAR2406"));

  tft.setTextColor(WHITE);
  tft.setCursor(103, 75);
  tft.print(F("CHOOSE YOUR PIECE"));

  drawButton(55, 96, 210, 48, CYAN, F("PLAY AS X"));
  drawButton(55, 158, 210, 48, YELLOW, F("PLAY AS O"));
}

static void drawGrid() {
  tft.drawRect(BX, BY, BS + 1, BS + 1, WHITE);
  tft.drawFastVLine(BX + CELL, BY, BS, GREY);
  tft.drawFastVLine(BX + CELL * 2, BY, BS, GREY);
  tft.drawFastHLine(BX, BY + CELL, BS, GREY);
  tft.drawFastHLine(BX, BY + CELL * 2, BS, GREY);
}

static void drawSide() {
  tft.fillRect(SX, 42, 115, 145, BLACK);

  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(SX + 3, 47);
  tft.print(F("YOU: "));
  tft.setTextColor(playerPiece == PIECE_X ? CYAN : YELLOW);
  tft.print(playerPiece == PIECE_X ? 'X' : 'O');

  tft.setTextColor(WHITE);
  tft.setCursor(SX + 3, 64);
  tft.print(F("ARDUINO: "));
  tft.setTextColor(arduinoPiece == PIECE_X ? CYAN : YELLOW);
  tft.print(arduinoPiece == PIECE_X ? 'X' : 'O');

  tft.setTextColor(WHITE);
  tft.setCursor(SX + 3, 88);
  tft.print(F("MOVES: "));
  tft.print(moves);

  tft.setTextSize(2);
  tft.setCursor(SX + 3, 122);

  if (turn == playerPiece) {
    tft.setTextColor(GREEN);
    tft.print(F("YOUR"));
    tft.setCursor(SX + 3, 143);
    tft.print(F("TURN"));
  } else {
    tft.setTextColor(RED);
    tft.print(F("ARDUINO"));
    tft.setTextSize(1);
    tft.setCursor(SX + 3, 148);
    tft.print(F("thinking..."));
  }
}

static void drawGame() {
  tft.fillScreen(BLACK);

  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(8, 10);
  tft.print(F("TIC TAC TOE"));

  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(SX + 3, 11);
  tft.print(F("VS ARDUINO"));

  drawGrid();
  drawSide();
}

static uint16_t pieceColor(uint8_t piece) {
  return piece == PIECE_X ? CYAN : YELLOW;
}

static void drawMarkColor(uint8_t cell, uint8_t piece, uint16_t color) {
  uint8_t row = cell / 3;
  uint8_t col = cell % 3;
  int16_t x0 = BX + col * CELL;
  int16_t y0 = BY + row * CELL;

  if (piece == PIECE_X) {
    tft.drawLine(x0 + 13, y0 + 13, x0 + 48, y0 + 48, color);
    tft.drawLine(x0 + 14, y0 + 13, x0 + 49, y0 + 48, color);
    tft.drawLine(x0 + 48, y0 + 13, x0 + 13, y0 + 48, color);
    tft.drawLine(x0 + 49, y0 + 13, x0 + 14, y0 + 48, color);
  } else {
    tft.drawCircle(x0 + 31, y0 + 31, 20, color);
    tft.drawCircle(x0 + 31, y0 + 31, 19, color);
  }
}

static void drawMark(uint8_t cell, uint8_t piece) {
  drawMarkColor(cell, piece, pieceColor(piece));
}

static uint8_t winnerLine(uint8_t &a, uint8_t &b, uint8_t &c) {
  if (board[0] && board[0] == board[1] && board[0] == board[2]) { a=0; b=1; c=2; return board[0]; }
  if (board[3] && board[3] == board[4] && board[3] == board[5]) { a=3; b=4; c=5; return board[3]; }
  if (board[6] && board[6] == board[7] && board[6] == board[8]) { a=6; b=7; c=8; return board[6]; }

  if (board[0] && board[0] == board[3] && board[0] == board[6]) { a=0; b=3; c=6; return board[0]; }
  if (board[1] && board[1] == board[4] && board[1] == board[7]) { a=1; b=4; c=7; return board[1]; }
  if (board[2] && board[2] == board[5] && board[2] == board[8]) { a=2; b=5; c=8; return board[2]; }

  if (board[0] && board[0] == board[4] && board[0] == board[8]) { a=0; b=4; c=8; return board[0]; }
  if (board[2] && board[2] == board[4] && board[2] == board[6]) { a=2; b=4; c=6; return board[2]; }

  return EMPTY;
}

static uint8_t winner() {
  uint8_t a, b, c;
  return winnerLine(a, b, c);
}

static bool boardFull() {
  for (uint8_t i = 0; i < 9; ++i) {
    if (board[i] == EMPTY) return false;
  }
  return true;
}

static void blinkWinner(uint8_t piece) {
  uint8_t a, b, c;
  if (winnerLine(a, b, c) == EMPTY) return;

  uint16_t normal = pieceColor(piece);

  for (uint8_t i = 0; i < 3; ++i) {
    drawMarkColor(a, piece, RED);
    drawMarkColor(b, piece, RED);
    drawMarkColor(c, piece, RED);
    delay(220);

    drawMarkColor(a, piece, normal);
    drawMarkColor(b, piece, normal);
    drawMarkColor(c, piece, normal);
    delay(140);
  }

  drawMarkColor(a, piece, RED);
  drawMarkColor(b, piece, RED);
  drawMarkColor(c, piece, RED);
}

static int8_t hitCell(int16_t x, int16_t y) {
  if (x < BX || x >= BX + BS || y < BY || y >= BY + BS) return -1;
  uint8_t col = (uint8_t)((x - BX) / CELL);
  uint8_t row = (uint8_t)((y - BY) / CELL);
  return (int8_t)(row * 3 + col);
}

static void showGameOver(uint8_t w) {
  screenState = SCREEN_OVER;
  tft.fillRect(SX, 112, 115, 118, BLACK);

  tft.setTextSize(2);
  tft.setCursor(SX + 3, 122);

  if (w == EMPTY) {
    tft.setTextColor(GREY);
    tft.print(F("DRAW"));
  } else if (w == playerPiece) {
    tft.setTextColor(GREEN);
    tft.print(F("YOU WIN"));
  } else {
    tft.setTextColor(RED);
    tft.print(F("ARDUINO"));
    tft.setCursor(SX + 3, 144);
    tft.print(F("WINS"));
  }

  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(SX + 3, 181);
  tft.print(F("TOUCH"));
  tft.setCursor(SX + 3, 194);
  tft.print(F("FOR MENU"));
}

static bool placeMove(uint8_t cell, uint8_t piece) {
  if (cell > 8 || board[cell] != EMPTY) return false;

  board[cell] = piece;
  ++moves;
  drawMark(cell, piece);

  uint8_t w = winner();
  if (w != EMPTY) {
    blinkWinner(w);
    showGameOver(w);
    return true;
  }

  if (moves == 9) {
    showGameOver(EMPTY);
    return true;
  }

  turn = (piece == PIECE_X) ? PIECE_O : PIECE_X;
  drawSide();

  if (turn == arduinoPiece) aiReadyAt = millis() + 450UL;
  return true;
}

static int8_t minimax(uint8_t depth, bool aiTurn, int8_t alpha, int8_t beta) {
  uint8_t w = winner();

  if (w == arduinoPiece) return (int8_t)(10 - depth);
  if (w == playerPiece) return (int8_t)(depth - 10);
  if (boardFull()) return 0;

  if (aiTurn) {
    int8_t best = -100;

    for (uint8_t i = 0; i < 9; ++i) {
      if (board[i] != EMPTY) continue;

      board[i] = arduinoPiece;
      int8_t score = minimax(depth + 1, false, alpha, beta);
      board[i] = EMPTY;

      if (score > best) best = score;
      if (best > alpha) alpha = best;
      if (beta <= alpha) break;
    }

    return best;
  }

  int8_t best = 100;

  for (uint8_t i = 0; i < 9; ++i) {
    if (board[i] != EMPTY) continue;

    board[i] = playerPiece;
    int8_t score = minimax(depth + 1, true, alpha, beta);
    board[i] = EMPTY;

    if (score < best) best = score;
    if (best < beta) beta = best;
    if (beta <= alpha) break;
  }

  return best;
}

static uint8_t bestArduinoMove() {
  int8_t bestScore = -100;
  uint8_t bestCell = 0;

  for (uint8_t i = 0; i < 9; ++i) {
    if (board[i] != EMPTY) continue;

    board[i] = arduinoPiece;
    int8_t score = minimax(1, false, -100, 100);
    board[i] = EMPTY;

    if (score > bestScore) {
      bestScore = score;
      bestCell = i;
    }
  }

  return bestCell;
}

static void arduinoMove() {
  if (screenState != SCREEN_PLAY || turn != arduinoPiece) return;

  uint8_t cell = bestArduinoMove();

  Serial.print(F("AI MOVE="));
  Serial.println(cell);

  placeMove(cell, arduinoPiece);
}

static void startGame(uint8_t chosenPiece) {
  playerPiece = chosenPiece;
  arduinoPiece = (chosenPiece == PIECE_X) ? PIECE_O : PIECE_X;

  for (uint8_t i = 0; i < 9; ++i) board[i] = EMPTY;

  moves = 0;
  turn = PIECE_X;
  screenState = SCREEN_PLAY;

  drawGame();

  if (turn == arduinoPiece) aiReadyAt = millis() + 550UL;
}

static void handleMenuTouch(int16_t x, int16_t y) {
  if (x < 55 || x > 265) return;

  if (y >= 96 && y < 144) {
    startGame(PIECE_X);
    return;
  }

  if (y >= 158 && y < 206) {
    startGame(PIECE_O);
  }
}

void setup() {
  Serial.begin(115200);

  tft.reset();
  uint16_t id = tft.readID();
  tft.begin(id);
  tft.setRotation(1);

  Serial.println();
  Serial.println(F("MAR2406 TIC-TAC-TOE VS ARDUINO"));
  Serial.print(F("LCD ID=0x"));
  Serial.println(id, HEX);
  Serial.println(F("LCD ROT1 320x240"));
  Serial.println(F("TOUCH XP=D6 XM=A2 YP=A1 YM=D7"));
  Serial.println(F("AI=MINIMAX ALPHA-BETA"));

  drawMenu();
}

void loop() {
  if (screenState == SCREEN_PLAY &&
      turn == arduinoPiece &&
      (long)(millis() - aiReadyAt) >= 0) {
    arduinoMove();
  }

  int16_t x, y;
  bool pressed = readTouch(x, y);

  if (touchArmed && pressed) {
    if (screenState == SCREEN_MENU) {
      handleMenuTouch(x, y);
    } else if (screenState == SCREEN_PLAY && turn == playerPiece) {
      int8_t cell = hitCell(x, y);
      if (cell >= 0) placeMove((uint8_t)cell, playerPiece);
    } else if (screenState == SCREEN_OVER) {
      drawMenu();
    }

    touchArmed = false;
  }

  if (!touchArmed && !pressed) {
    delay(25);
    int16_t x2, y2;
    if (!readTouch(x2, y2)) touchArmed = true;
  }

  delay(10);
}
