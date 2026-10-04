/*
  Arduino UNO + MAR2406 2.4" TFT Touch Shield
  ARKANOID / BREAKOUT TOUCH

  Touch-controlled paddle:
    - drag a finger across the lower part of the screen;
    - the paddle follows the finger continuously.

  Game:
    - ball bounces from walls, paddle and bricks;
    - 5 x 8 brick field;
    - score +10 per brick;
    - 3 lives;
    - paddle hit position changes ball direction;
    - win / game-over screen and touch restart.

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

const int16_t SCREEN_W = 320;
const int16_t SCREEN_H = 240;

const int16_t HUD_H = 31;
const int16_t WALL_L = 3;
const int16_t WALL_R = 316;
const int16_t WALL_T = 32;

const uint8_t BRICK_ROWS = 5;
const uint8_t BRICK_COLS = 8;
const int16_t BRICK_X = 8;
const int16_t BRICK_Y = 43;
const int16_t BRICK_W = 36;
const int16_t BRICK_H = 13;
const int16_t BRICK_GAP_X = 3;
const int16_t BRICK_GAP_Y = 4;

const int16_t PADDLE_Y = 219;
const int16_t PADDLE_W = 58;
const int16_t PADDLE_H = 6;

const int16_t BALL_R = 4;
const unsigned long FRAME_MS = 18;
const unsigned long SERVE_MS = 650;

enum {
  SCREEN_TITLE = 0,
  SCREEN_GAME = 1,
  SCREEN_RESULT = 2
};

uint8_t screenState = SCREEN_TITLE;
bool lastPressed = false;

uint8_t bricks[BRICK_ROWS];
uint8_t bricksLeft = BRICK_ROWS * BRICK_COLS;

int16_t paddleX = (SCREEN_W - PADDLE_W) / 2;

int16_t ballX = SCREEN_W / 2;
int16_t ballY = PADDLE_Y - BALL_R - 2;
int8_t ballDX = 2;
int8_t ballDY = -2;

uint16_t score = 0;
uint8_t lives = 3;
bool winState = false;

bool serving = false;
unsigned long serveUntil = 0;
unsigned long lastFrame = 0;

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

static uint16_t rowColor(uint8_t row) {
  switch (row) {
    case 0: return RED;
    case 1: return YELLOW;
    case 2: return GREEN;
    case 3: return CYAN;
    default: return MAGENTA;
  }
}

static void drawHud() {
  tft.fillRect(0, 0, SCREEN_W, HUD_H, BLACK);
  tft.drawFastHLine(0, HUD_H - 1, SCREEN_W, GREY);

  tft.setTextSize(1);

  tft.setTextColor(WHITE);
  tft.setCursor(8, 10);
  tft.print(F("SCORE "));

  tft.setTextColor(YELLOW);
  tft.print(score);

  tft.setTextColor(WHITE);
  tft.setCursor(132, 10);
  tft.print(F("BRICKS "));

  tft.setTextColor(CYAN);
  tft.print(bricksLeft);

  tft.setTextColor(WHITE);
  tft.setCursor(247, 10);
  tft.print(F("LIVES "));

  tft.setTextColor(GREEN);
  tft.print(lives);
}

static void drawBrick(uint8_t row, uint8_t col, uint16_t color) {
  int16_t x = BRICK_X + col * (BRICK_W + BRICK_GAP_X);
  int16_t y = BRICK_Y + row * (BRICK_H + BRICK_GAP_Y);

  if (color == BLACK) {
    tft.fillRect(x, y, BRICK_W, BRICK_H, BLACK);
    return;
  }

  tft.fillRect(x, y, BRICK_W, BRICK_H, color);
  tft.drawRect(x, y, BRICK_W, BRICK_H, WHITE);
}

static void drawAllBricks() {
  for (uint8_t r = 0; r < BRICK_ROWS; ++r) {
    for (uint8_t c = 0; c < BRICK_COLS; ++c) {
      if (bricks[r] & (1 << c)) drawBrick(r, c, rowColor(r));
    }
  }
}

static void drawPaddle() {
  tft.fillRect(paddleX, PADDLE_Y, PADDLE_W, PADDLE_H, CYAN);
  tft.drawFastHLine(paddleX + 3, PADDLE_Y, PADDLE_W - 6, WHITE);
}

static void erasePaddle(int16_t oldX) {
  tft.fillRect(oldX, PADDLE_Y, PADDLE_W, PADDLE_H, BLACK);
}

static void movePaddleTo(int16_t touchX) {
  int16_t next = touchX - PADDLE_W / 2;

  if (next < WALL_L + 1) next = WALL_L + 1;
  if (next > WALL_R - PADDLE_W) next = WALL_R - PADDLE_W;

  if (next == paddleX) return;

  int16_t old = paddleX;
  paddleX = next;

  erasePaddle(old);
  drawPaddle();

  if (serving) {
    tft.fillCircle(ballX, ballY, BALL_R, BLACK);
    ballX = paddleX + PADDLE_W / 2;
    ballY = PADDLE_Y - BALL_R - 2;
    tft.fillCircle(ballX, ballY, BALL_R, WHITE);
  }
}

static void resetBall() {
  ballX = paddleX + PADDLE_W / 2;
  ballY = PADDLE_Y - BALL_R - 2;

  ballDX = (lives & 1) ? 2 : -2;
  ballDY = -2;

  serving = true;
  serveUntil = millis() + SERVE_MS;

  tft.fillCircle(ballX, ballY, BALL_R, WHITE);
}

static void initBricks() {
  for (uint8_t r = 0; r < BRICK_ROWS; ++r) bricks[r] = 0xFF;
  bricksLeft = BRICK_ROWS * BRICK_COLS;
}

static void drawPlayfield() {
  tft.fillScreen(BLACK);
  drawHud();

  tft.drawFastVLine(WALL_L, WALL_T, SCREEN_H - WALL_T, GREY);
  tft.drawFastVLine(WALL_R, WALL_T, SCREEN_H - WALL_T, GREY);
  tft.drawFastHLine(WALL_L, WALL_T, WALL_R - WALL_L + 1, GREY);

  drawAllBricks();
  drawPaddle();
  tft.fillCircle(ballX, ballY, BALL_R, WHITE);

  tft.setTextSize(1);
  tft.setTextColor(GREY);
  tft.setCursor(87, 234);
  tft.print(F("DRAG PADDLE WITH TOUCH"));
}

static void startGame() {
  score = 0;
  lives = 3;
  paddleX = (SCREEN_W - PADDLE_W) / 2;
  initBricks();

  ballX = paddleX + PADDLE_W / 2;
  ballY = PADDLE_Y - BALL_R - 2;

  screenState = SCREEN_GAME;
  winState = false;

  drawPlayfield();
  resetBall();

  lastFrame = millis();
}

static void showTitle() {
  screenState = SCREEN_TITLE;
  tft.fillScreen(BLACK);

  tft.setTextColor(YELLOW);
  tft.setTextSize(4);
  tft.setCursor(54, 35);
  tft.print(F("ARKANOID"));

  tft.setTextColor(CYAN);
  tft.setTextSize(2);
  tft.setCursor(67, 84);
  tft.print(F("BREAKOUT TOUCH"));

  for (uint8_t r = 0; r < 3; ++r) {
    for (uint8_t c = 0; c < 7; ++c) {
      uint16_t color = (r == 0) ? RED : (r == 1 ? YELLOW : GREEN);
      tft.fillRect(43 + c * 34, 121 + r * 15, 30, 10, color);
    }
  }

  tft.fillCircle(160, 178, 5, WHITE);
  tft.fillRect(126, 194, 68, 6, CYAN);

  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.setCursor(104, 216);
  tft.print(F("TOUCH TO START"));
}

static void showResult(bool won) {
  screenState = SCREEN_RESULT;
  winState = won;

  tft.fillRect(35, 73, 250, 104, BLACK);
  tft.drawRect(35, 73, 250, 104, won ? GREEN : RED);
  tft.drawRect(36, 74, 248, 102, won ? GREEN : RED);

  tft.setTextSize(3);
  tft.setTextColor(won ? GREEN : RED);
  tft.setCursor(won ? 76 : 64, 91);
  if (won) tft.print(F("YOU WIN!"));
  else tft.print(F("GAME OVER"));

  tft.setTextSize(2);
  tft.setTextColor(YELLOW);
  tft.setCursor(91, 129);
  tft.print(F("SCORE "));
  tft.print(score);

  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(101, 157);
  tft.print(F("TOUCH TO RESTART"));
}

static bool ballHitsBrick(uint8_t row, uint8_t col, int16_t x, int16_t y) {
  if (!(bricks[row] & (1 << col))) return false;

  int16_t bx = BRICK_X + col * (BRICK_W + BRICK_GAP_X);
  int16_t by = BRICK_Y + row * (BRICK_H + BRICK_GAP_Y);

  return x + BALL_R >= bx &&
         x - BALL_R <  bx + BRICK_W &&
         y + BALL_R >= by &&
         y - BALL_R <  by + BRICK_H;
}

static void hitBrick(uint8_t row, uint8_t col) {
  bricks[row] &= ~(1 << col);
  drawBrick(row, col, BLACK);

  if (bricksLeft) --bricksLeft;
  score += 10;
  drawHud();
}

static void loseLife() {
  if (lives) --lives;
  drawHud();

  if (lives == 0) {
    showResult(false);
    return;
  }

  resetBall();
}

static void updateBall() {
  if (serving) {
    if ((long)(millis() - serveUntil) < 0) return;
    serving = false;
  }

  tft.fillCircle(ballX, ballY, BALL_R, BLACK);

  int16_t nextX = ballX + ballDX;
  int16_t nextY = ballY + ballDY;

  if (nextX - BALL_R <= WALL_L) {
    nextX = WALL_L + BALL_R + 1;
    ballDX = -ballDX;
  } else if (nextX + BALL_R >= WALL_R) {
    nextX = WALL_R - BALL_R - 1;
    ballDX = -ballDX;
  }

  if (nextY - BALL_R <= WALL_T) {
    nextY = WALL_T + BALL_R + 1;
    ballDY = -ballDY;
  }

  bool brickHit = false;

  for (uint8_t r = 0; r < BRICK_ROWS && !brickHit; ++r) {
    for (uint8_t c = 0; c < BRICK_COLS; ++c) {
      if (ballHitsBrick(r, c, nextX, nextY)) {
        hitBrick(r, c);
        ballDY = -ballDY;
        nextY = ballY + ballDY;
        brickHit = true;
        break;
      }
    }
  }

  if (bricksLeft == 0) {
    ballX = nextX;
    ballY = nextY;
    tft.fillCircle(ballX, ballY, BALL_R, WHITE);
    showResult(true);
    return;
  }

  if (ballDY > 0 &&
      nextY + BALL_R >= PADDLE_Y &&
      nextY - BALL_R <= PADDLE_Y + PADDLE_H &&
      nextX >= paddleX - BALL_R &&
      nextX <= paddleX + PADDLE_W + BALL_R) {

    nextY = PADDLE_Y - BALL_R - 1;
    ballDY = -2;

    int16_t offset = nextX - (paddleX + PADDLE_W / 2);

    if (offset < -20) ballDX = -3;
    else if (offset < -7) ballDX = -2;
    else if (offset > 20) ballDX = 3;
    else if (offset > 7) ballDX = 2;
    else ballDX = (ballDX < 0) ? -1 : 1;
  }

  ballX = nextX;
  ballY = nextY;

  if (ballY - BALL_R > SCREEN_H) {
    loseLife();
    return;
  }

  tft.fillCircle(ballX, ballY, BALL_R, WHITE);
  drawPaddle();
}

void setup() {
  Serial.begin(115200);

  tft.reset();
  uint16_t id = tft.readID();
  tft.begin(id);
  tft.setRotation(1);

  Serial.println();
  Serial.println(F("MAR2406 ARKANOID / BREAKOUT TOUCH"));
  Serial.print(F("LCD ID=0x"));
  Serial.println(id, HEX);
  Serial.println(F("TOUCH PADDLE CONTROL"));

  showTitle();
}

void loop() {
  int16_t tx, ty;
  bool pressed = readTouch(tx, ty);

  bool newPress = pressed && !lastPressed;

  if (screenState == SCREEN_TITLE) {
    if (newPress) startGame();
  } else if (screenState == SCREEN_RESULT) {
    if (newPress) startGame();
  } else {
    if (pressed && ty >= 165) movePaddleTo(tx);

    unsigned long now = millis();
    if ((unsigned long)(now - lastFrame) >= FRAME_MS) {
      lastFrame = now;
      updateBall();
    }
  }

  lastPressed = pressed;
}
