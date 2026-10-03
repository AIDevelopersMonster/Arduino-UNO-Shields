/*
  LAB-03 / TEST-08
  Arduino UNO + MAR2406 2.4" TFT Touch Shield

  Tic-Tac-Toe + persistent microSD history.
  Optimized for ATmega328P / Arduino UNO flash limits.

  Verified hardware:
    LCD: ILI9341, ROT1 / 320x240
    Touch: XP=D6 XM=A2 YP=A1 YM=D7
    SD: CS=D10 MOSI=D11 MISO=D12 SCK=D13

  Certified ROT0 touch calibration:
    LEFT=153 RIGHT=930 TOP=962 BOTTOM=168

  History file:
    XOLOG.TXT

  One finished game = one appended text line:
    X,03142
    O,041328
    D,041235786

  First character is the result: X, O or D.
  Remaining digits are board cells 0..8 in move order.

  At startup the file is scanned and valid complete lines rebuild
  X wins, O wins, draws, total games and the last result.

  Incomplete/truncated lines are ignored.
  No Arduino String objects are used.
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
const char LOG_FILE[] = "XOLOG.TXT";

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

const int16_t BX = 8;
const int16_t BY = 42;
const int16_t CELL = 63;
const int16_t BS = 189;
const int16_t SX = 207;

uint8_t board[9];
uint8_t seq[9];
uint8_t moves = 0;
uint8_t turn = 1;              // 1=X, 2=O
uint8_t screenState = 0;       // 0=history, 1=play, 2=game over
bool armed = true;
bool sdOK = false;

uint16_t games = 0;
uint16_t xWins = 0;
uint16_t oWins = 0;
uint16_t draws = 0;
char lastResult = '-';

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

static void addStat(char r) {
  ++games;
  if (r == 'X') ++xWins;
  else if (r == 'O') ++oWins;
  else ++draws;
  lastResult = r;
}

static bool validRecord(char r, uint8_t n, bool ok) {
  if (!ok) return false;
  if (r == 'X') return n >= 5 && n <= 9 && (n & 1);
  if (r == 'O') return n >= 6 && n <= 8 && !(n & 1);
  if (r == 'D') return n == 9;
  return false;
}

static bool loadHistory() {
  File f = SD.open(LOG_FILE, FILE_READ);
  if (!f) return false;

  char r = 0;
  uint8_t pos = 0;
  uint8_t n = 0;
  uint16_t used = 0;
  bool ok = true;

  while (f.available()) {
    char c = (char)f.read();

    if (c == '\r') continue;

    if (c == '\n') {
      if (validRecord(r, n, ok)) addStat(r);
      r = 0;
      pos = 0;
      n = 0;
      used = 0;
      ok = true;
      continue;
    }

    if (pos == 0) {
      r = c;
      if (c != 'X' && c != 'O' && c != 'D') ok = false;
    } else if (pos == 1) {
      if (c != ',') ok = false;
    } else {
      if (c < '0' || c > '8' || n >= 9) {
        ok = false;
      } else {
        uint8_t cell = (uint8_t)(c - '0');
        uint16_t bit = (uint16_t)1 << cell;
        if (used & bit) ok = false;
        else used |= bit;
        ++n;
      }
    }
    ++pos;
  }

  f.close();
  return true;
}

static bool appendGame(char r) {
  File f = SD.open(LOG_FILE, FILE_WRITE);
  if (!f) return false;

  bool ok = true;
  if (f.write((uint8_t)r) != 1) ok = false;
  if (f.write((uint8_t)',') != 1) ok = false;

  for (uint8_t i = 0; i < moves; ++i) {
    if (f.write((uint8_t)('0' + seq[i])) != 1) ok = false;
  }

  if (f.write((uint8_t)'\n') != 1) ok = false;
  f.close();

  if (ok) addStat(r);
  return ok;
}

static void showFatal() {
  tft.fillScreen(BLACK);
  tft.setTextColor(RED);
  tft.setTextSize(2);
  tft.setCursor(60, 86);
  tft.print(F("SD ERROR"));
  tft.setCursor(38, 118);
  tft.print(F("TEST-08 STOP"));
}

static void showHistory() {
  tft.fillScreen(BLACK);

  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(48, 14);
  tft.print(F("TIC TAC TOE"));

  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(88, 47);
  tft.print(F("LAB-03 TEST-08"));

  tft.setTextSize(2);

  tft.setTextColor(WHITE);
  tft.setCursor(56, 76);
  tft.print(F("GAMES "));
  tft.print(games);

  tft.setTextColor(CYAN);
  tft.setCursor(56, 104);
  tft.print(F("X WINS "));
  tft.print(xWins);

  tft.setTextColor(YELLOW);
  tft.setCursor(56, 132);
  tft.print(F("O WINS "));
  tft.print(oWins);

  tft.setTextColor(GREEN);
  tft.setCursor(56, 160);
  tft.print(F("DRAWS  "));
  tft.print(draws);

  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(56, 190);
  tft.print(F("LAST: "));
  if (lastResult == '-') tft.print(F("NONE"));
  else if (lastResult == 'D') tft.print(F("DRAW"));
  else {
    tft.print(lastResult);
    tft.print(F(" WIN"));
  }

  tft.drawRect(84, 207, 152, 25, GREEN);
  tft.setCursor(112, 216);
  tft.print(F("TOUCH TO PLAY"));
}

static void drawGrid() {
  tft.drawRect(BX, BY, BS + 1, BS + 1, WHITE);
  tft.drawFastVLine(BX + CELL, BY, BS, GREY);
  tft.drawFastVLine(BX + CELL * 2, BY, BS, GREY);
  tft.drawFastHLine(BX, BY + CELL, BS, GREY);
  tft.drawFastHLine(BX, BY + CELL * 2, BS, GREY);
}

static void drawSide() {
  tft.fillRect(SX, 40, 113, 148, BLACK);

  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(SX + 3, 48);
  tft.print(F("X "));
  tft.print(xWins);

  tft.setTextColor(YELLOW);
  tft.setCursor(SX + 3, 64);
  tft.print(F("O "));
  tft.print(oWins);

  tft.setTextColor(GREEN);
  tft.setCursor(SX + 3, 80);
  tft.print(F("D "));
  tft.print(draws);

  tft.setTextColor(WHITE);
  tft.setCursor(SX + 3, 96);
  tft.print(F("G "));
  tft.print(games);

  tft.setTextSize(2);
  tft.setTextColor(turn == 1 ? CYAN : YELLOW);
  tft.setCursor(SX + 3, 126);
  tft.print(F("TURN "));
  tft.print(turn == 1 ? 'X' : 'O');
}

static void drawGame() {
  tft.fillScreen(BLACK);

  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(8, 9);
  tft.print(F("TIC TAC TOE"));

  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(SX + 3, 10);
  tft.print(F("TEST-08"));

  drawGrid();
  drawSide();
}

static void drawMark(uint8_t cell, uint8_t player) {
  uint8_t row = cell / 3;
  uint8_t col = cell % 3;
  int16_t x0 = BX + col * CELL;
  int16_t y0 = BY + row * CELL;

  if (player == 1) {
    tft.drawLine(x0 + 14, y0 + 14, x0 + 49, y0 + 49, CYAN);
    tft.drawLine(x0 + 49, y0 + 14, x0 + 14, y0 + 49, CYAN);
  } else {
    tft.drawCircle(x0 + 31, y0 + 31, 20, YELLOW);
    tft.drawCircle(x0 + 31, y0 + 31, 19, YELLOW);
  }
}

static uint8_t winner() {
  if (board[0] && board[0] == board[1] && board[0] == board[2]) return board[0];
  if (board[3] && board[3] == board[4] && board[3] == board[5]) return board[3];
  if (board[6] && board[6] == board[7] && board[6] == board[8]) return board[6];
  if (board[0] && board[0] == board[3] && board[0] == board[6]) return board[0];
  if (board[1] && board[1] == board[4] && board[1] == board[7]) return board[1];
  if (board[2] && board[2] == board[5] && board[2] == board[8]) return board[2];
  if (board[0] && board[0] == board[4] && board[0] == board[8]) return board[0];
  if (board[2] && board[2] == board[4] && board[2] == board[6]) return board[2];
  return 0;
}

static void newGame() {
  for (uint8_t i = 0; i < 9; ++i) board[i] = 0;
  moves = 0;
  turn = 1;
  screenState = 1;
  drawGame();
}

static int8_t hitCell(int16_t x, int16_t y) {
  if (x < BX || x >= BX + BS || y < BY || y >= BY + BS) return -1;
  return (int8_t)(((y - BY) / CELL) * 3 + ((x - BX) / CELL));
}

static void gameOver(char r) {
  bool saved = appendGame(r);
  screenState = 2;

  drawSide();
  tft.fillRect(SX, 118, 113, 113, BLACK);

  tft.setTextSize(2);
  tft.setTextColor(r == 'X' ? CYAN : (r == 'O' ? YELLOW : GREEN));
  tft.setCursor(SX + 3, 125);

  if (r == 'D') tft.print(F("DRAW"));
  else {
    tft.print(r);
    tft.print(F(" WINS"));
  }

  tft.setTextSize(1);
  tft.setCursor(SX + 3, 157);
  tft.setTextColor(saved ? GREEN : RED);
  tft.print(saved ? F("SD SAVE OK") : F("SD SAVE FAIL"));

  tft.setTextColor(WHITE);
  tft.setCursor(SX + 3, 178);
  tft.print(F("TOUCH"));
  tft.setCursor(SX + 3, 190);
  tft.print(F("FOR NEW GAME"));
}

static void play(uint8_t cell) {
  if (cell > 8 || board[cell]) return;

  board[cell] = turn;
  seq[moves++] = cell;
  drawMark(cell, turn);

  uint8_t w = winner();

  if (w == 1) {
    gameOver('X');
    return;
  }

  if (w == 2) {
    gameOver('O');
    return;
  }

  if (moves == 9) {
    gameOver('D');
    return;
  }

  turn = (turn == 1) ? 2 : 1;
  drawSide();
}

void setup() {
  Serial.begin(115200);

  tft.reset();
  tft.begin(tft.readID());
  tft.setRotation(1);
  tft.fillScreen(BLACK);

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  if (!SD.begin(SD_CS)) {
    showFatal();
    return;
  }

  if (!SD.exists(LOG_FILE)) {
    File f = SD.open(LOG_FILE, FILE_WRITE);
    if (!f) {
      showFatal();
      return;
    }
    f.close();
  }

  if (!loadHistory()) {
    showFatal();
    return;
  }

  sdOK = true;

  Serial.println(F("LAB-03 TEST-08 READY"));
  Serial.print(F("GAMES="));
  Serial.println(games);

  showHistory();
}

void loop() {
  if (!sdOK) {
    delay(100);
    return;
  }

  int16_t x, y;
  bool pressed = readTouch(x, y);

  if (armed && pressed) {
    if (screenState == 0 || screenState == 2) {
      newGame();
    } else {
      int8_t cell = hitCell(x, y);
      if (cell >= 0) play((uint8_t)cell);
    }
    armed = false;
  }

  if (!armed && !pressed) {
    delay(25);
    int16_t x2, y2;
    if (!readTouch(x2, y2)) armed = true;
  }

  delay(10);
}
