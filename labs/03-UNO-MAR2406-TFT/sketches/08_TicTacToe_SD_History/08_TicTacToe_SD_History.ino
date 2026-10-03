/*
  LAB-03 / TEST-08
  Arduino UNO + MAR2406 2.4" TFT Touch Shield

  Original project demonstrator: Tic-Tac-Toe with persistent SD history.

  Design goals:
    - use the already certified LCD + touch + microSD configuration;
    - keep the game playable entirely from the TFT touch screen;
    - append every finished game to an immutable log;
    - rebuild statistics from that log after reset / power loss;
    - store the complete move sequence for later replay or analysis;
    - reject damaged / truncated log records with CRC-8;
    - avoid Arduino String and keep SRAM use predictable on ATmega328P.

  Verified hardware:
    LCD: ILI9341, canonical ROT1 / 320x240
    Touch: XP=D6 XM=A2 YP=A1 YM=D7
    microSD: CS=D10, MOSI=D11, MISO=D12, SCK=D13

  Certified ROT0 touch calibration:
    LEFT   = 153
    RIGHT  = 930
    TOP    = 962
    BOTTOM = 168

  Persistent file:
    XOLOG.CSV

  Record format:
    G,<game>,<result>,<moves>,<sequence>,<crc8>

  Example:
    G,12,X,5,0-3-1-4-2,6A

  The CRC-8 covers all characters before the final comma.
  A damaged or incomplete record is ignored when history is rebuilt.
*/

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <SPI.h>
#include <SD.h>
#include <avr/pgmspace.h>

MCUFRIEND_kbv tft;

#define BLACK    0x0000
#define WHITE    0xFFFF
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define YELLOW   0xFFE0
#define GREY     0x8410
#define DARKGREY 0x4208

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
const char LOG_FILE[] = "XOLOG.CSV";

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

static const int16_t BOARD_X = 8;
static const int16_t BOARD_Y = 42;
static const int16_t CELL    = 63;
static const int16_t BOARD_S = CELL * 3;

static const int16_t SIDE_X = 205;
static const int16_t NEW_X  = 211;
static const int16_t NEW_Y  = 194;
static const int16_t NEW_W  = 100;
static const int16_t NEW_H  = 34;

enum UiState : uint8_t {
  UI_BOOT_HISTORY = 0,
  UI_PLAYING      = 1,
  UI_GAME_OVER    = 2
};

struct History {
  uint32_t games;
  uint32_t xWins;
  uint32_t oWins;
  uint32_t draws;
  uint32_t lastGame;
  uint16_t badRecords;
  char lastResult;
};

static History historyData;
static uint8_t board[9];
static uint8_t moveSeq[9];
static uint8_t moveCount = 0;
static uint8_t currentPlayer = 1;  // 1 = X, 2 = O
static UiState uiState = UI_BOOT_HISTORY;
static bool touchArmed = true;
static bool sdReady = false;
static bool lastSaveOK = true;

static const uint8_t WIN_LINES[8][3] PROGMEM = {
  {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
  {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
  {0, 4, 8}, {2, 4, 6}
};

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

static uint8_t crc8Update(uint8_t crc, uint8_t data) {
  crc ^= data;
  for (uint8_t i = 0; i < 8; ++i) {
    if (crc & 0x80) {
      crc = (uint8_t)((crc << 1) ^ 0x07);
    } else {
      crc <<= 1;
    }
  }
  return crc;
}

static uint8_t crc8Buffer(const char *s, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; ++i) {
    crc = crc8Update(crc, (uint8_t)s[i]);
  }
  return crc;
}

static char hexDigit(uint8_t v) {
  v &= 0x0F;
  return (v < 10) ? (char)('0' + v) : (char)('A' + v - 10);
}

static int8_t hexValue(char c) {
  if (c >= '0' && c <= '9') return (int8_t)(c - '0');
  if (c >= 'A' && c <= 'F') return (int8_t)(c - 'A' + 10);
  if (c >= 'a' && c <= 'f') return (int8_t)(c - 'a' + 10);
  return -1;
}

static bool appendChar(char *buf, uint8_t &len, uint8_t cap, char c) {
  if (len + 1 >= cap) return false;
  buf[len++] = c;
  buf[len] = '\0';
  return true;
}

static bool appendUInt(char *buf, uint8_t &len, uint8_t cap, uint32_t value) {
  char tmp[10];
  uint8_t n = 0;

  do {
    tmp[n++] = (char)('0' + (value % 10));
    value /= 10;
  } while (value && n < sizeof(tmp));

  while (n) {
    if (!appendChar(buf, len, cap, tmp[--n])) return false;
  }
  return true;
}

static uint8_t buildRecord(char *buf, uint8_t cap, uint32_t gameNo, char result) {
  uint8_t len = 0;
  buf[0] = '\0';

  if (!appendChar(buf, len, cap, 'G')) return 0;
  if (!appendChar(buf, len, cap, ',')) return 0;
  if (!appendUInt(buf, len, cap, gameNo)) return 0;
  if (!appendChar(buf, len, cap, ',')) return 0;
  if (!appendChar(buf, len, cap, result)) return 0;
  if (!appendChar(buf, len, cap, ',')) return 0;
  if (!appendUInt(buf, len, cap, moveCount)) return 0;
  if (!appendChar(buf, len, cap, ',')) return 0;

  for (uint8_t i = 0; i < moveCount; ++i) {
    if (i && !appendChar(buf, len, cap, '-')) return 0;
    if (!appendChar(buf, len, cap, (char)('0' + moveSeq[i]))) return 0;
  }

  uint8_t crc = crc8Buffer(buf, len);

  if (!appendChar(buf, len, cap, ',')) return 0;
  if (!appendChar(buf, len, cap, hexDigit(crc >> 4))) return 0;
  if (!appendChar(buf, len, cap, hexDigit(crc))) return 0;
  if (!appendChar(buf, len, cap, '\n')) return 0;

  return len;
}

static bool parseRecord(char *line, uint32_t &gameNo, char &result) {
  if (line[0] != 'G' || line[1] != ',') return false;

  char *lastComma = strrchr(line, ',');
  if (!lastComma) return false;

  char *crcText = lastComma + 1;
  if (!crcText[0] || !crcText[1] || crcText[2]) return false;

  int8_t hi = hexValue(crcText[0]);
  int8_t lo = hexValue(crcText[1]);
  if (hi < 0 || lo < 0) return false;

  uint8_t expected = (uint8_t)((hi << 4) | lo);
  uint8_t payloadLen = (uint8_t)(lastComma - line);
  uint8_t actual = crc8Buffer(line, payloadLen);
  if (actual != expected) return false;

  const char *p = line + 2;
  if (*p < '0' || *p > '9') return false;

  uint32_t n = 0;
  while (*p >= '0' && *p <= '9') {
    n = n * 10UL + (uint8_t)(*p - '0');
    ++p;
  }

  if (*p != ',') return false;
  ++p;

  char r = *p++;
  if (r != 'X' && r != 'O' && r != 'D') return false;
  if (*p != ',') return false;

  gameNo = n;
  result = r;
  return true;
}

static void applyHistoryRecord(uint32_t gameNo, char result) {
  ++historyData.games;

  if (result == 'X') ++historyData.xWins;
  else if (result == 'O') ++historyData.oWins;
  else ++historyData.draws;

  if (gameNo >= historyData.lastGame) {
    historyData.lastGame = gameNo;
    historyData.lastResult = result;
  }
}

static void processHistoryLine(char *line, uint8_t len) {
  if (!len) return;
  line[len] = '\0';

  if (line[0] == '#') return;

  uint32_t gameNo = 0;
  char result = 0;

  if (parseRecord(line, gameNo, result)) {
    applyHistoryRecord(gameNo, result);
  } else {
    ++historyData.badRecords;
  }
}

static bool ensureLogFile() {
  if (SD.exists(LOG_FILE)) return true;

  File f = SD.open(LOG_FILE, FILE_WRITE);
  if (!f) return false;

  f.println(F("# LAB03 TEST08 TIC-TAC-TOE HISTORY"));
  f.println(F("# G,GAME,RESULT,MOVES,SEQUENCE,CRC8"));
  f.close();
  return true;
}

static bool loadHistory() {
  historyData.games = 0;
  historyData.xWins = 0;
  historyData.oWins = 0;
  historyData.draws = 0;
  historyData.lastGame = 0;
  historyData.badRecords = 0;
  historyData.lastResult = 0;

  File f = SD.open(LOG_FILE, FILE_READ);
  if (!f) return false;

  char line[56];
  uint8_t len = 0;
  bool overflow = false;

  while (f.available()) {
    char c = (char)f.read();

    if (c == '\r') continue;

    if (c == '\n') {
      if (overflow) {
        ++historyData.badRecords;
      } else {
        processHistoryLine(line, len);
      }
      len = 0;
      overflow = false;
      continue;
    }

    if (!overflow) {
      if (len + 1 < sizeof(line)) {
        line[len++] = c;
      } else {
        overflow = true;
      }
    }
  }

  if (len || overflow) {
    if (overflow) ++historyData.badRecords;
    else processHistoryLine(line, len);
  }

  f.close();
  return true;
}

static bool appendGameRecord(char result) {
  char record[56];
  uint32_t gameNo = historyData.lastGame + 1UL;
  uint8_t len = buildRecord(record, sizeof(record), gameNo, result);
  if (!len) return false;

  File f = SD.open(LOG_FILE, FILE_WRITE);
  if (!f) return false;

  size_t written = f.write((const uint8_t *)record, len);
  f.flush();
  f.close();

  if (written != len) return false;

  applyHistoryRecord(gameNo, result);

  Serial.print(F("APPEND "));
  Serial.write((const uint8_t *)record, len);
  return true;
}

static void printHistorySerial() {
  Serial.print(F("HISTORY games="));
  Serial.print(historyData.games);
  Serial.print(F(" X="));
  Serial.print(historyData.xWins);
  Serial.print(F(" O="));
  Serial.print(historyData.oWins);
  Serial.print(F(" D="));
  Serial.print(historyData.draws);
  Serial.print(F(" last="));
  Serial.print(historyData.lastGame);
  Serial.print(F(" result="));
  if (historyData.lastResult) Serial.print(historyData.lastResult);
  else Serial.print('-');
  Serial.print(F(" bad="));
  Serial.println(historyData.badRecords);
}

static void drawButton(int16_t x, int16_t y, int16_t w, int16_t h,
                       uint16_t border, const __FlashStringHelper *label) {
  tft.drawRect(x, y, w, h, border);
  tft.drawRect(x + 1, y + 1, w - 2, h - 2, border);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.setCursor(x + 18, y + 13);
  tft.print(label);
}

static void drawBootHistory() {
  tft.fillScreen(BLACK);

  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(35, 12);
  tft.print(F("TIC-TAC-TOE"));

  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(65, 43);
  tft.print(F("LAB-03 TEST-08 / SD HISTORY"));

  tft.setTextSize(2);
  tft.setTextColor(WHITE);

  tft.setCursor(50, 68);
  tft.print(F("GAMES : "));
  tft.print(historyData.games);

  tft.setTextColor(CYAN);
  tft.setCursor(50, 94);
  tft.print(F("X WINS: "));
  tft.print(historyData.xWins);

  tft.setTextColor(YELLOW);
  tft.setCursor(50, 120);
  tft.print(F("O WINS: "));
  tft.print(historyData.oWins);

  tft.setTextColor(GREEN);
  tft.setCursor(50, 146);
  tft.print(F("DRAWS : "));
  tft.print(historyData.draws);

  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(50, 174);
  tft.print(F("LAST: "));
  if (!historyData.lastResult) {
    tft.print(F("NONE"));
  } else {
    tft.print(F("GAME "));
    tft.print(historyData.lastGame);
    tft.print(' ');
    if (historyData.lastResult == 'D') tft.print(F("DRAW"));
    else {
      tft.print(historyData.lastResult);
      tft.print(F(" WIN"));
    }
  }

  tft.setCursor(50, 187);
  if (historyData.badRecords == 0) {
    tft.setTextColor(GREEN);
    tft.print(F("LOG: VALID"));
  } else {
    tft.setTextColor(RED);
    tft.print(F("LOG: ignored "));
    tft.print(historyData.badRecords);
    tft.print(F(" damaged line(s)"));
  }

  drawButton(85, 204, 150, 28, GREEN, F("TAP TO PLAY"));
}

static void drawMarkX(uint8_t cell) {
  uint8_t row = cell / 3;
  uint8_t col = cell % 3;

  int16_t x0 = BOARD_X + col * CELL;
  int16_t y0 = BOARD_Y + row * CELL;
  int16_t m = 13;

  tft.drawLine(x0 + m, y0 + m, x0 + CELL - m, y0 + CELL - m, CYAN);
  tft.drawLine(x0 + m + 1, y0 + m, x0 + CELL - m + 1, y0 + CELL - m, CYAN);
  tft.drawLine(x0 + CELL - m, y0 + m, x0 + m, y0 + CELL - m, CYAN);
  tft.drawLine(x0 + CELL - m + 1, y0 + m, x0 + m + 1, y0 + CELL - m, CYAN);
}

static void drawMarkO(uint8_t cell) {
  uint8_t row = cell / 3;
  uint8_t col = cell % 3;

  int16_t cx = BOARD_X + col * CELL + CELL / 2;
  int16_t cy = BOARD_Y + row * CELL + CELL / 2;

  tft.drawCircle(cx, cy, 20, YELLOW);
  tft.drawCircle(cx, cy, 19, YELLOW);
}

static void drawBoardGrid() {
  tft.drawRect(BOARD_X, BOARD_Y, BOARD_S + 1, BOARD_S + 1, WHITE);

  for (uint8_t i = 1; i < 3; ++i) {
    int16_t x = BOARD_X + i * CELL;
    int16_t y = BOARD_Y + i * CELL;
    tft.drawFastVLine(x, BOARD_Y, BOARD_S, GREY);
    tft.drawFastHLine(BOARD_X, y, BOARD_S, GREY);
  }
}

static void drawSideStats() {
  tft.fillRect(SIDE_X, 37, 115, 151, BLACK);

  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(SIDE_X + 4, 42);
  tft.print(F("HISTORY"));

  tft.setTextColor(CYAN);
  tft.setCursor(SIDE_X + 4, 60);
  tft.print(F("X: "));
  tft.print(historyData.xWins);

  tft.setTextColor(YELLOW);
  tft.setCursor(SIDE_X + 4, 74);
  tft.print(F("O: "));
  tft.print(historyData.oWins);

  tft.setTextColor(GREEN);
  tft.setCursor(SIDE_X + 4, 88);
  tft.print(F("D: "));
  tft.print(historyData.draws);

  tft.setTextColor(WHITE);
  tft.setCursor(SIDE_X + 4, 102);
  tft.print(F("G: "));
  tft.print(historyData.games);

  tft.drawFastHLine(SIDE_X + 4, 119, 101, DARKGREY);
}

static void drawTurn() {
  tft.fillRect(SIDE_X, 124, 115, 28, BLACK);
  tft.setTextSize(2);
  tft.setTextColor(currentPlayer == 1 ? CYAN : YELLOW);
  tft.setCursor(SIDE_X + 4, 130);
  tft.print(F("TURN "));
  tft.print(currentPlayer == 1 ? 'X' : 'O');
}

static void drawGameScreen() {
  tft.fillScreen(BLACK);

  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(8, 8);
  tft.print(F("TIC-TAC-TOE"));

  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(SIDE_X + 4, 9);
  tft.print(F("LAB-03"));
  tft.setCursor(SIDE_X + 4, 20);
  tft.print(F("TEST-08"));

  drawBoardGrid();
  drawSideStats();
  drawTurn();
}

static void drawGameOver(char result) {
  tft.fillRect(SIDE_X, 124, 115, 64, BLACK);

  tft.setTextSize(2);
  if (result == 'X') {
    tft.setTextColor(CYAN);
    tft.setCursor(SIDE_X + 4, 128);
    tft.print(F("X WINS"));
  } else if (result == 'O') {
    tft.setTextColor(YELLOW);
    tft.setCursor(SIDE_X + 4, 128);
    tft.print(F("O WINS"));
  } else {
    tft.setTextColor(GREEN);
    tft.setCursor(SIDE_X + 4, 128);
    tft.print(F("DRAW"));
  }

  tft.setTextSize(1);
  tft.setCursor(SIDE_X + 4, 155);
  if (lastSaveOK) {
    tft.setTextColor(GREEN);
    tft.print(F("SAVED TO SD"));
  } else {
    tft.setTextColor(RED);
    tft.print(F("SD SAVE FAIL"));
  }

  tft.setTextColor(WHITE);
  tft.setCursor(SIDE_X + 4, 170);
  tft.print(F("MOVES: "));
  tft.print(moveCount);

  drawButton(NEW_X, NEW_Y, NEW_W, NEW_H, GREEN, F("PLAY AGAIN"));
}

static void resetGame() {
  for (uint8_t i = 0; i < 9; ++i) {
    board[i] = 0;
    moveSeq[i] = 0;
  }

  moveCount = 0;
  currentPlayer = 1;
  lastSaveOK = true;
  uiState = UI_PLAYING;
  drawGameScreen();

  Serial.println(F("NEW GAME - X starts"));
}

static int8_t touchedCell(int16_t x, int16_t y) {
  if (x < BOARD_X || y < BOARD_Y) return -1;
  if (x >= BOARD_X + BOARD_S || y >= BOARD_Y + BOARD_S) return -1;

  uint8_t col = (uint8_t)((x - BOARD_X) / CELL);
  uint8_t row = (uint8_t)((y - BOARD_Y) / CELL);
  if (col > 2 || row > 2) return -1;

  return (int8_t)(row * 3 + col);
}

static uint8_t winner() {
  for (uint8_t i = 0; i < 8; ++i) {
    uint8_t a = pgm_read_byte(&WIN_LINES[i][0]);
    uint8_t b = pgm_read_byte(&WIN_LINES[i][1]);
    uint8_t c = pgm_read_byte(&WIN_LINES[i][2]);

    if (board[a] && board[a] == board[b] && board[a] == board[c]) {
      return board[a];
    }
  }
  return 0;
}

static void finishGame(char result) {
  lastSaveOK = appendGameRecord(result);
  if (!lastSaveOK) {
    Serial.println(F("SD SAVE FAIL"));
  }

  printHistorySerial();
  uiState = UI_GAME_OVER;
  drawSideStats();
  drawGameOver(result);
}

static void playCell(uint8_t cell) {
  if (board[cell]) {
    Serial.print(F("CELL "));
    Serial.print(cell);
    Serial.println(F(" ALREADY USED"));
    return;
  }

  board[cell] = currentPlayer;
  moveSeq[moveCount++] = cell;

  if (currentPlayer == 1) drawMarkX(cell);
  else drawMarkO(cell);

  Serial.print(F("MOVE "));
  Serial.print(moveCount);
  Serial.print(F(" player="));
  Serial.print(currentPlayer == 1 ? 'X' : 'O');
  Serial.print(F(" cell="));
  Serial.println(cell);

  uint8_t w = winner();

  if (w == 1) {
    finishGame('X');
    return;
  }

  if (w == 2) {
    finishGame('O');
    return;
  }

  if (moveCount == 9) {
    finishGame('D');
    return;
  }

  currentPlayer = (currentPlayer == 1) ? 2 : 1;
  drawTurn();
}

static bool insideNewButton(int16_t x, int16_t y) {
  return x >= NEW_X && x < NEW_X + NEW_W &&
         y >= NEW_Y && y < NEW_Y + NEW_H;
}

static void handleTouch(int16_t x, int16_t y) {
  Serial.print(F("TOUCH X="));
  Serial.print(x);
  Serial.print(F(" Y="));
  Serial.println(y);

  if (uiState == UI_BOOT_HISTORY) {
    resetGame();
    return;
  }

  if (uiState == UI_PLAYING) {
    int8_t cell = touchedCell(x, y);
    if (cell >= 0) playCell((uint8_t)cell);
    return;
  }

  if (uiState == UI_GAME_OVER && insideNewButton(x, y)) {
    resetGame();
  }
}

static void showFatal(const __FlashStringHelper *line1,
                      const __FlashStringHelper *line2) {
  tft.fillScreen(BLACK);
  tft.setTextColor(RED);
  tft.setTextSize(2);
  tft.setCursor(42, 82);
  tft.println(line1);
  tft.setCursor(42, 114);
  tft.println(line2);

  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(42, 154);
  tft.println(F("microSD is required for TEST-08"));
}

void setup() {
  Serial.begin(115200);

  tft.reset();
  uint16_t id = tft.readID();
  tft.begin(id);
  tft.setRotation(1);

  Serial.println();
  Serial.println(F("LAB-03 TEST-08 - Tic-Tac-Toe persistent SD history"));
  Serial.print(F("LCD ID=0x"));
  Serial.println(id, HEX);
  Serial.println(F("Display: ROT1 / 320x240"));
  Serial.println(F("Touch: XP=D6 XM=A2 YP=A1 YM=D7"));
  Serial.println(F("Touch calibration ROT0: LEFT=153 RIGHT=930 TOP=962 BOTTOM=168"));
  Serial.println(F("microSD: CS=D10 MOSI=D11 MISO=D12 SCK=D13"));
  Serial.print(F("History file: "));
  Serial.println(LOG_FILE);

  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(58, 94);
  tft.println(F("TEST-08 INIT"));

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  sdReady = SD.begin(SD_CS);
  if (!sdReady) {
    Serial.println(F("SD INIT FAIL"));
    showFatal(F("SD INIT FAIL"), F("TEST-08 STOP"));
    return;
  }

  if (!ensureLogFile()) {
    Serial.println(F("LOG CREATE FAIL"));
    showFatal(F("LOG CREATE FAIL"), F("TEST-08 STOP"));
    sdReady = false;
    return;
  }

  if (!loadHistory()) {
    Serial.println(F("LOG READ FAIL"));
    showFatal(F("LOG READ FAIL"), F("TEST-08 STOP"));
    sdReady = false;
    return;
  }

  Serial.println(F("SD HISTORY READY"));
  printHistorySerial();

  drawBootHistory();
}

void loop() {
  if (!sdReady) {
    delay(100);
    return;
  }

  TSPoint p = readTouchRaw();
  int16_t x, y;
  bool pressed = rawToLandscape(p, x, y);

  if (touchArmed && pressed) {
    handleTouch(x, y);
    touchArmed = false;
  }

  if (!touchArmed && !pressed) {
    delay(25);

    TSPoint q = readTouchRaw();
    int16_t qx, qy;

    if (!rawToLandscape(q, qx, qy)) {
      touchArmed = true;
    }
  }

  delay(10);
}
