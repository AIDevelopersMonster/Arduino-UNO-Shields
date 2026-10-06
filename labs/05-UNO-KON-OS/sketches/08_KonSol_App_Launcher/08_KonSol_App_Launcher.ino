/*
  KonSol 0.8
  LAB-05 / TEST-11
  Arduino UNO R3 + MAR2406 2.4" TFT Touch + microSD

  Goal:
    Preserve the physically certified KonSol 0.7 HOST1/KAP2 runtime and add
    the first on-device application launcher without requiring the PC manager.

    TEST-11 / KonSol 0.8 adds:
      - APPS button on the resident dashboard
      - KAP-only application view on microSD
      - Touch launch of an external KAP1/KAP2 application
      - separate FILES browser retained for general filesystem access
      - no new external hardware assumptions

  Existing KAP1/KAP2 bytecode, HOST1 and the normal file browser are retained.

  No MCUFRIEND_kbv.
  No Adafruit_GFX.
  No TouchScreen library.
  No Arduino String.
  No malloc/new in KonSol code.

  Verified MAR2406:
    LCD D0..D7 = UNO D8,D9,D2,D3,D4,D5,D6,D7
    RD=A0 WR=A1 RS=A2 CS=A3 RST=A4

    Touch:
      XP=D6 XM=A2 YP=A1 YM=D7
      certified calibration:
      LEFT=153 RIGHT=930 TOP=962 BOTTOM=168

    microSD:
      CS=D10 MOSI=D11 MISO=D12 SCK=D13
*/

#include <SPI.h>
#include <SD.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>

#define KONSOL_VERSION "0.8"

const uint8_t SD_CS = 10;
const uint32_t SERIAL_BAUD = 115200UL;
const uint8_t CMD_SIZE = 88;

static char commandLine[CMD_SIZE];
static uint8_t commandLen = 0;
static bool sdReady = false;

// -----------------------------------------------------------------------------
// Direct ILI9341 driver
// -----------------------------------------------------------------------------

#define LCD_RD_MASK   _BV(PC0)
#define LCD_WR_MASK   _BV(PC1)
#define LCD_RS_MASK   _BV(PC2)
#define LCD_CS_MASK   _BV(PC3)
#define LCD_RST_MASK  _BV(PC4)

#define LCD_BLACK    0x0000
#define LCD_WHITE    0xFFFF
#define LCD_RED      0xF800
#define LCD_GREEN    0x07E0
#define LCD_CYAN     0x07FF
#define LCD_YELLOW   0xFFE0
#define LCD_BLUE     0x001F
#define LCD_GREY     0x8410
#define LCD_DARKGREY 0x4208

static inline void lcdWrLow()  { PORTC &= (uint8_t)~LCD_WR_MASK; }
static inline void lcdWrHigh() { PORTC |= LCD_WR_MASK; }
static inline void lcdRsLow()  { PORTC &= (uint8_t)~LCD_RS_MASK; }
static inline void lcdRsHigh() { PORTC |= LCD_RS_MASK; }
static inline void lcdCsLow()  { PORTC &= (uint8_t)~LCD_CS_MASK; }
static inline void lcdCsHigh() { PORTC |= LCD_CS_MASK; }

static void lcdBusRestore() {
  // D2..D7 and D8..D9 are LCD data outputs.
  DDRD |= 0xFC;
  DDRB |= 0x03;

  // A0..A4 are LCD control outputs.
  DDRC |= LCD_RD_MASK | LCD_WR_MASK | LCD_RS_MASK | LCD_CS_MASK | LCD_RST_MASK;

  // Keep LCD inactive until the next explicit transfer.
  PORTC |= LCD_RD_MASK | LCD_WR_MASK | LCD_RS_MASK | LCD_CS_MASK | LCD_RST_MASK;
}

static inline void lcdWrite8(uint8_t value) {
  // Preserve D0/D1: hardware UART.
  PORTD = (PORTD & 0x03) | (value & 0xFC);

  // Preserve D10..D13: SD/SPI.
  PORTB = (PORTB & 0xFC) | (value & 0x03);

  lcdWrLow();
  lcdWrHigh();
}

static void lcdCommand(uint8_t cmd) {
  lcdCsLow();
  lcdRsLow();
  lcdWrite8(cmd);
  lcdCsHigh();
}

static void lcdData(uint8_t data) {
  lcdCsLow();
  lcdRsHigh();
  lcdWrite8(data);
  lcdCsHigh();
}

static void lcdData16(uint16_t data) {
  lcdCsLow();
  lcdRsHigh();
  lcdWrite8((uint8_t)(data >> 8));
  lcdWrite8((uint8_t)data);
  lcdCsHigh();
}

static void lcdSetAddressWindow(uint16_t x0, uint16_t y0,
                                uint16_t x1, uint16_t y1) {
  lcdCommand(0x2A);
  lcdData16(x0);
  lcdData16(x1);

  lcdCommand(0x2B);
  lcdData16(y0);
  lcdData16(y1);

  lcdCommand(0x2C);
}

static void lcdFillRect(uint16_t x, uint16_t y,
                        uint16_t w, uint16_t h,
                        uint16_t color) {
  if (!w || !h || x >= 320 || y >= 240) return;

  if ((uint32_t)x + w > 320) w = 320 - x;
  if ((uint32_t)y + h > 240) h = 240 - y;

  lcdSetAddressWindow(x, y, x + w - 1, y + h - 1);

  uint32_t count = (uint32_t)w * h;
  uint8_t hi = (uint8_t)(color >> 8);
  uint8_t lo = (uint8_t)color;

  lcdCsLow();
  lcdRsHigh();

  while (count--) {
    lcdWrite8(hi);
    lcdWrite8(lo);
  }

  lcdCsHigh();
}

static void lcdFillScreen(uint16_t color) {
  lcdFillRect(0, 0, 320, 240, color);
}

static void lcdPixel(uint16_t x, uint16_t y, uint16_t color) {
  if (x >= 320 || y >= 240) return;

  lcdSetAddressWindow(x, y, x, y);
  lcdCsLow();
  lcdRsHigh();
  lcdWrite8((uint8_t)(color >> 8));
  lcdWrite8((uint8_t)color);
  lcdCsHigh();
}

static void lcdHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color) {
  lcdFillRect(x, y, w, 1, color);
}

static void lcdRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                    uint16_t color) {
  if (!w || !h) return;

  lcdHLine(x, y, w, color);
  lcdHLine(x, y + h - 1, w, color);
  lcdFillRect(x, y, 1, h, color);
  lcdFillRect(x + w - 1, y, 1, h, color);
}

static void lcdInit() {
  lcdBusRestore();

  PORTC &= (uint8_t)~LCD_RST_MASK;
  delay(20);
  PORTC |= LCD_RST_MASK;
  delay(120);

  lcdCommand(0x01);
  delay(10);
  lcdCommand(0x28);

  lcdCommand(0xCF); lcdData(0x00); lcdData(0x83); lcdData(0x30);
  lcdCommand(0xED); lcdData(0x64); lcdData(0x03); lcdData(0x12); lcdData(0x81);
  lcdCommand(0xE8); lcdData(0x85); lcdData(0x01); lcdData(0x79);
  lcdCommand(0xCB); lcdData(0x39); lcdData(0x2C); lcdData(0x00); lcdData(0x34); lcdData(0x02);
  lcdCommand(0xF7); lcdData(0x20);
  lcdCommand(0xEA); lcdData(0x00); lcdData(0x00);

  lcdCommand(0xC0); lcdData(0x26);
  lcdCommand(0xC1); lcdData(0x11);
  lcdCommand(0xC5); lcdData(0x35); lcdData(0x3E);
  lcdCommand(0xC7); lcdData(0xBE);

  // Landscape / BGR / RGB565.
  lcdCommand(0x36); lcdData(0x28);
  lcdCommand(0x3A); lcdData(0x55);

  lcdCommand(0xB1); lcdData(0x00); lcdData(0x1B);
  lcdCommand(0xF2); lcdData(0x08);
  lcdCommand(0x26); lcdData(0x01);
  lcdCommand(0xB7); lcdData(0x07);
  lcdCommand(0xB6); lcdData(0x0A); lcdData(0x82); lcdData(0x27); lcdData(0x00);

  lcdCommand(0xE0);
  lcdData(0x1F); lcdData(0x1A); lcdData(0x18); lcdData(0x0A); lcdData(0x0F);
  lcdData(0x06); lcdData(0x45); lcdData(0x87); lcdData(0x32); lcdData(0x0A);
  lcdData(0x07); lcdData(0x02); lcdData(0x07); lcdData(0x05); lcdData(0x00);

  lcdCommand(0xE1);
  lcdData(0x00); lcdData(0x25); lcdData(0x27); lcdData(0x05); lcdData(0x10);
  lcdData(0x09); lcdData(0x3A); lcdData(0x78); lcdData(0x4D); lcdData(0x05);
  lcdData(0x18); lcdData(0x0D); lcdData(0x38); lcdData(0x3A); lcdData(0x1F);

  lcdCommand(0x11);
  delay(120);
  lcdCommand(0x29);
  delay(20);

  lcdFillScreen(LCD_BLACK);
}

// -----------------------------------------------------------------------------
// Tiny 3x5 uppercase font
// -----------------------------------------------------------------------------

static const uint8_t font3x5[36][5] PROGMEM = {
  {2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},{6,5,5,5,6},{7,4,6,4,7},
  {7,4,6,4,4},{3,4,5,5,3},{5,5,7,5,5},{7,2,2,2,7},{1,1,1,5,2},
  {5,5,6,5,5},{4,4,4,4,7},{5,7,7,5,5},{5,7,7,7,5},{2,5,5,5,2},
  {6,5,6,4,4},{2,5,5,3,1},{6,5,6,5,5},{3,4,2,1,6},{7,2,2,2,2},
  {5,5,5,5,7},{5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},{5,5,2,2,2},
  {7,1,2,4,7},

  {7,5,5,5,7},{2,6,2,2,7},{6,1,7,4,7},{6,1,3,1,6},{5,5,7,1,1},
  {7,4,6,1,6},{3,4,7,5,7},{7,1,2,2,2},{7,5,7,5,7},{7,5,7,1,6}
};

static uint8_t glyphRow(char c, uint8_t row) {
  if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');

  if (c >= 'A' && c <= 'Z') {
    return pgm_read_byte(&font3x5[c - 'A'][row]);
  }

  if (c >= '0' && c <= '9') {
    return pgm_read_byte(&font3x5[26 + c - '0'][row]);
  }

  switch (c) {
    case '-': return row == 2 ? 7 : 0;
    case '_': return row == 4 ? 7 : 0;
    case '.': return row == 4 ? 2 : 0;
    case ':': return (row == 1 || row == 3) ? 2 : 0;
    case '/': {
      static const uint8_t s[5] = {1,1,2,4,4};
      return s[row];
    }
    case '>': {
      static const uint8_t s[5] = {4,2,1,2,4};
      return s[row];
    }
    case '<': {
      static const uint8_t s[5] = {1,2,4,2,1};
      return s[row];
    }
    case '=': return (row == 1 || row == 3) ? 7 : 0;
    case '+': return (row == 1 || row == 2 || row == 3) ?
                     (row == 2 ? 7 : 2) : 0;
    default:  return 0;
  }
}

static void lcdChar(uint16_t x, uint16_t y, char c,
                    uint8_t scale, uint16_t color) {
  for (uint8_t row = 0; row < 5; ++row) {
    uint8_t bits = glyphRow(c, row);

    for (uint8_t col = 0; col < 3; ++col) {
      if (bits & (1 << (2 - col))) {
        if (scale == 1) {
          lcdPixel(x + col, y + row, color);
        } else {
          lcdFillRect(x + col * scale, y + row * scale,
                      scale, scale, color);
        }
      }
    }
  }
}

static void lcdText(uint16_t x, uint16_t y, const char *text,
                    uint8_t scale, uint16_t color) {
  uint16_t step = 4U * scale;

  while (*text) {
    lcdChar(x, y, *text++, scale, color);
    x += step;
  }
}

static void lcdTextP(uint16_t x, uint16_t y, PGM_P text,
                     uint8_t scale, uint16_t color) {
  uint16_t step = 4U * scale;

  for (;;) {
    char c = (char)pgm_read_byte(text++);
    if (!c) break;
    lcdChar(x, y, c, scale, color);
    x += step;
  }
}

// -----------------------------------------------------------------------------
// Direct resistive-touch service
// -----------------------------------------------------------------------------

const uint8_t TOUCH_XP = 6;
const uint8_t TOUCH_XM = A2;
const uint8_t TOUCH_YP = A1;
const uint8_t TOUCH_YM = 7;

const int16_t TS_LEFT = 153;
const int16_t TS_RT   = 930;
const int16_t TS_TOP  = 962;
const int16_t TS_BOT  = 168;

const int16_t MINPRESSURE = 40;
const int16_t MAXPRESSURE = 2000;
const uint16_t RXPLATE = 300;

static bool touchDown = false;
static bool touchWasDown = false;
static int16_t touchX = 0;
static int16_t touchY = 0;
static int16_t touchZ = 0;

static bool touchRead(int16_t &sx, int16_t &sy, int16_t &pressure) {
  int16_t samples[2];
  bool valid = true;

  // Never allow a touch measurement while the LCD is selected.
  lcdCsHigh();
  PORTC |= LCD_RD_MASK | LCD_WR_MASK;

  // X measurement: X+ high, X- low, read Y+.
  pinMode(TOUCH_YP, INPUT);
  pinMode(TOUCH_YM, INPUT);
  pinMode(TOUCH_XP, OUTPUT);
  pinMode(TOUCH_XM, OUTPUT);
  digitalWrite(TOUCH_XP, HIGH);
  digitalWrite(TOUCH_XM, LOW);

  samples[0] = analogRead(TOUCH_YP);
  samples[1] = analogRead(TOUCH_YP);

  if (abs(samples[0] - samples[1]) > 4) valid = false;
  int16_t rawX = 1023 - ((samples[0] + samples[1]) >> 1);

  // Y measurement: Y+ high, Y- low, read X-.
  pinMode(TOUCH_XP, INPUT);
  pinMode(TOUCH_XM, INPUT);
  pinMode(TOUCH_YP, OUTPUT);
  pinMode(TOUCH_YM, OUTPUT);
  digitalWrite(TOUCH_YM, LOW);
  digitalWrite(TOUCH_YP, HIGH);

  samples[0] = analogRead(TOUCH_XM);
  samples[1] = analogRead(TOUCH_XM);

  if (abs(samples[0] - samples[1]) > 4) valid = false;
  int16_t rawY = 1023 - ((samples[0] + samples[1]) >> 1);

  // Pressure measurement: X+ low, Y- high, X- and Y+ high impedance.
  pinMode(TOUCH_XP, OUTPUT);
  digitalWrite(TOUCH_XP, LOW);
  pinMode(TOUCH_YM, OUTPUT);
  digitalWrite(TOUCH_YM, HIGH);

  digitalWrite(TOUCH_XM, LOW);
  pinMode(TOUCH_XM, INPUT);
  digitalWrite(TOUCH_YP, LOW);
  pinMode(TOUCH_YP, INPUT);

  int16_t z1 = analogRead(TOUCH_XM);
  int16_t z2 = analogRead(TOUCH_YP);

  int32_t z = 0;

  if (z1 > 0 && z2 >= z1) {
    z = ((int32_t)(z2 - z1) * rawX * RXPLATE) /
        ((int32_t)z1 * 1024L);
  }

  // Restore every shared pin to the LCD bus before any graphics operation.
  lcdBusRestore();

  if (!valid || z <= MINPRESSURE || z >= MAXPRESSURE) {
    pressure = (int16_t)z;
    return false;
  }

  long px = map(rawX, TS_LEFT, TS_RT, 0, 239);
  long py = map(rawY, TS_TOP, TS_BOT, 0, 319);

  px = constrain(px, 0, 239);
  py = constrain(py, 0, 319);

  sx = constrain((int16_t)py, 0, 319);
  sy = constrain((int16_t)(239 - px), 0, 239);
  pressure = (int16_t)z;
  return true;
}

// -----------------------------------------------------------------------------
// Memory diagnostics
// -----------------------------------------------------------------------------

extern int __heap_start;
extern void *__brkval;

static int freeRam() {
  int stackTop;
  int heapTop = (__brkval == 0) ? (int)&__heap_start : (int)__brkval;
  return (int)&stackTop - heapTop;
}

// -----------------------------------------------------------------------------
// UI: dashboard, file browser, text viewer
// -----------------------------------------------------------------------------

enum UiMode {
  UI_DASHBOARD = 0,
  UI_BROWSER = 1,
  UI_VIEWER = 2,
  UI_APP = 3
};

static UiMode uiMode = UI_DASHBOARD;

const uint8_t BROWSER_ROWS = 5;
const uint8_t BROWSER_ROW_H = 28;
const uint8_t BROWSER_Y = 48;
static uint8_t browserPage = 0;
static bool browserHasNext = false;
static bool browserKapOnly = false;
static char currentPath[40] = "/";

static bool appStart(const char *path);
static bool appRunning = false;
static bool appTouchEvent = false;

static bool isKapPath(const char *path) {
  size_t n = strlen(path);
  if (n < 4) return false;

  const char *p = path + n - 4;
  return p[0] == '.' &&
         (p[1] == 'K' || p[1] == 'k') &&
         (p[2] == 'A' || p[2] == 'a') &&
         (p[3] == 'P' || p[3] == 'p');
}

static void drawButton(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                       PGM_P label, uint16_t color) {
  lcdRect(x, y, w, h, color);
  lcdTextP(x + 7, y + 8, label, 2, color);
}

static void drawDashboardStatic() {
  uiMode = UI_DASHBOARD;
  lcdFillScreen(LCD_BLACK);

  lcdTextP(12, 10, PSTR("KONSOL 0.8"), 3, LCD_CYAN);
  lcdTextP(12, 32, PSTR("KAP2 + HOST1 + TOUCH"), 1, LCD_GREY);
  drawButton(172, 7, 66, 27, PSTR("APPS"), LCD_GREEN);
  drawButton(244, 7, 66, 27, PSTR("FILES"), LCD_YELLOW);

  lcdHLine(10, 43, 300, LCD_BLUE);

  lcdTextP(18, 58, PSTR("KERNEL"), 2, LCD_WHITE);
  lcdTextP(18, 84, PSTR("SD"), 2, LCD_WHITE);
  lcdTextP(18, 110, PSTR("RAM"), 2, LCD_WHITE);
  lcdTextP(18, 136, PSTR("UPTIME"), 2, LCD_WHITE);
  lcdTextP(18, 162, PSTR("TOUCH"), 2, LCD_WHITE);

  lcdRect(10, 188, 300, 39, LCD_GREY);
  lcdTextP(20, 198, PSTR("KERNEL + KAP2 + HOST1"), 2, LCD_YELLOW);
  lcdTextP(20, 216, PSTR("HOST INSTALL + RUN FROM SD"), 1, LCD_GREY);
}

static void clearDashValue(uint16_t y) {
  lcdFillRect(132, y, 104, 18, LCD_BLACK);
}

static void updateDashboard() {
  if (uiMode != UI_DASHBOARD) return;

  char buf[14];

  clearDashValue(56);
  lcdTextP(136, 58, PSTR("RUN"), 2, LCD_GREEN);

  clearDashValue(82);
  lcdTextP(136, 84, sdReady ? PSTR("READY") : PSTR("NOT READY"),
           2, sdReady ? LCD_GREEN : LCD_RED);

  clearDashValue(108);
  itoa(freeRam(), buf, 10);
  lcdText(136, 110, buf, 2, LCD_CYAN);
  lcdTextP(192, 110, PSTR("B"), 2, LCD_CYAN);

  clearDashValue(134);
  ultoa(millis() / 1000UL, buf, 10);
  lcdText(136, 136, buf, 2, LCD_WHITE);
  lcdTextP(216, 136, PSTR("S"), 2, LCD_WHITE);

  clearDashValue(160);
  lcdTextP(136, 162, touchDown ? PSTR("DOWN") : PSTR("READY"),
           2, touchDown ? LCD_GREEN : LCD_YELLOW);
}

static void browserFooter() {
  drawButton(0,   202, 78, 37, PSTR("DASH"), LCD_CYAN);
  drawButton(80,  202, 78, 37, PSTR("PREV"), LCD_WHITE);
  drawButton(160, 202, 78, 37, PSTR("NEXT"), LCD_WHITE);
  drawButton(240, 202, 79, 37,
             browserKapOnly ? PSTR("FILES") : PSTR("UP"), LCD_YELLOW);
}

static bool browserEntryVisible(File &entry) {
  if (!browserKapOnly) return true;
  if (entry.isDirectory()) return false;
  return isKapPath(entry.name());
}

static void browserDraw() {
  uiMode = UI_BROWSER;
  lcdFillScreen(LCD_BLACK);

  lcdTextP(8, 7,
           browserKapOnly ? PSTR("KONSOL 0.8 APP LAUNCHER")
                          : PSTR("KONSOL 0.8 FILE BROWSER"),
           2, LCD_CYAN);
  if (browserKapOnly) {
    lcdTextP(8, 28, PSTR("KAP APPLICATIONS"), 1, LCD_YELLOW);
  } else {
    lcdText(8, 28, currentPath, 1, LCD_YELLOW);
  }
  lcdHLine(6, 40, 308, LCD_BLUE);

  browserHasNext = false;

  if (!sdReady) {
    lcdTextP(18, 80, PSTR("SD NOT READY"), 3, LCD_RED);
    browserFooter();
    return;
  }

  File dir = SD.open(currentPath);

  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    lcdTextP(18, 80, PSTR("DIR OPEN ERROR"), 2, LCD_RED);
    browserFooter();
    return;
  }

  uint16_t skip = (uint16_t)browserPage * BROWSER_ROWS;
  uint16_t index = 0;
  uint8_t shown = 0;

  for (;;) {
    File entry = dir.openNextFile();
    if (!entry) break;

    if (!browserEntryVisible(entry)) {
      entry.close();
      continue;
    }

    if (index++ < skip) {
      entry.close();
      continue;
    }

    if (shown >= BROWSER_ROWS) {
      browserHasNext = true;
      entry.close();
      break;
    }

    uint16_t y = BROWSER_Y + shown * BROWSER_ROW_H;
    uint16_t color = entry.isDirectory() ? LCD_YELLOW : LCD_WHITE;

    lcdRect(5, y, 310, BROWSER_ROW_H - 2, LCD_DARKGREY);
    lcdTextP(11, y + 8, entry.isDirectory() ? PSTR("D") : PSTR("F"),
             2, color);
    lcdText(31, y + 8, entry.name(), 2, color);

    if (!entry.isDirectory()) {
      char sizeBuf[11];
      ultoa((uint32_t)entry.size(), sizeBuf, 10);
      lcdText(244, y + 8, sizeBuf, 1, LCD_CYAN);
    }

    entry.close();
    ++shown;
  }

  dir.close();

  if (!shown) {
    lcdTextP(18, 88,
             browserKapOnly ? PSTR("NO KAP APPS") : PSTR("EMPTY"),
             3, LCD_GREY);
  }

  char pageBuf[5];
  utoa(browserPage + 1, pageBuf, 10);
  lcdTextP(274, 29, PSTR("P"), 1, LCD_GREY);
  lcdText(282, 29, pageBuf, 1, LCD_GREY);

  browserFooter();
}

static bool makeChildPath(char *dst, uint8_t dstSize,
                          const char *base, const char *name) {
  uint8_t n = 0;

  while (*base && n < dstSize - 1) dst[n++] = *base++;

  if (n == 0) {
    dst[n++] = '/';
  } else if (n > 1 && dst[n - 1] != '/' && n < dstSize - 1) {
    dst[n++] = '/';
  }

  while (*name && n < dstSize - 1) dst[n++] = *name++;
  dst[n] = 0;

  return *name == 0;
}

static void viewerDraw(File &f, const char *name) {
  uiMode = UI_VIEWER;
  lcdFillScreen(LCD_BLACK);

  lcdTextP(7, 7, PSTR("VIEW"), 2, LCD_CYAN);
  lcdText(52, 7, name, 2, LCD_YELLOW);
  lcdHLine(5, 25, 310, LCD_BLUE);

  uint16_t x = 7;
  uint16_t y = 34;
  uint8_t col = 0;
  uint8_t line = 0;

  while (f.available() && line < 19) {
    char c = (char)f.read();

    if (c == '\r') continue;

    if (c == '\n') {
      col = 0;
      ++line;
      x = 7;
      y += 8;
      continue;
    }

    if (c < 32 || c > 126) c = '.';

    lcdChar(x, y, c, 1, LCD_WHITE);
    x += 8;
    ++col;

    if (col >= 38) {
      col = 0;
      ++line;
      x = 7;
      y += 8;
    }
  }

  lcdRect(0, 210, 319, 29, LCD_CYAN);
  lcdTextP(124, 219, PSTR("BACK"), 2, LCD_CYAN);
}

static void browserOpenRow(uint8_t row) {
  if (!sdReady || row >= BROWSER_ROWS) return;

  File dir = SD.open(currentPath);
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    return;
  }

  uint16_t wanted = (uint16_t)browserPage * BROWSER_ROWS + row;
  uint16_t index = 0;

  for (;;) {
    File entry = dir.openNextFile();
    if (!entry) break;

    if (!browserEntryVisible(entry)) {
      entry.close();
      continue;
    }

    if (index++ != wanted) {
      entry.close();
      continue;
    }

    char child[40];

    if (!makeChildPath(child, sizeof(child), currentPath, entry.name())) {
      entry.close();
      dir.close();
      return;
    }

    if (entry.isDirectory()) {
      entry.close();
      dir.close();

      strncpy(currentPath, child, sizeof(currentPath) - 1);
      currentPath[sizeof(currentPath) - 1] = 0;
      browserPage = 0;
      browserDraw();
      return;
    }

    if (isKapPath(child)) {
      entry.close();
      dir.close();
      appStart(child);
      return;
    }

    // Non-KAP files use the one-page text preview.
    viewerDraw(entry, entry.name());
    entry.close();
    dir.close();
    return;
  }

  dir.close();
}

static void browserUp() {
  if (strcmp(currentPath, "/") == 0) {
    browserPage = 0;
    browserDraw();
    return;
  }

  int8_t i = (int8_t)strlen(currentPath) - 1;

  while (i > 0 && currentPath[i] != '/') --i;

  if (i <= 0) {
    currentPath[0] = '/';
    currentPath[1] = 0;
  } else {
    currentPath[i] = 0;
  }

  browserPage = 0;
  browserDraw();
}

static void uiHandlePress(int16_t x, int16_t y) {
  if (uiMode == UI_DASHBOARD) {
    if (y <= 42 && x >= 166 && x < 238) {
      browserKapOnly = true;
      browserPage = 0;
      currentPath[0] = '/';
      currentPath[1] = 0;
      browserDraw();
    } else if (x >= 238 && y <= 42) {
      browserKapOnly = false;
      browserPage = 0;
      browserDraw();
    }
    return;
  }

  if (uiMode == UI_VIEWER) {
    if (y >= 200) browserDraw();
    return;
  }

  if (uiMode != UI_BROWSER) return;

  if (y >= BROWSER_Y &&
      y < BROWSER_Y + BROWSER_ROWS * BROWSER_ROW_H) {
    uint8_t row = (uint8_t)((y - BROWSER_Y) / BROWSER_ROW_H);
    browserOpenRow(row);
    return;
  }

  if (y >= 195) {
    if (x < 80) {
      drawDashboardStatic();
      updateDashboard();
    } else if (x < 160) {
      if (browserPage) --browserPage;
      browserDraw();
    } else if (x < 240) {
      if (browserHasNext) ++browserPage;
      browserDraw();
    } else {
      if (browserKapOnly) {
        browserKapOnly = false;
        browserPage = 0;
        browserDraw();
      } else {
        browserUp();
      }
    }
  }
}

// -----------------------------------------------------------------------------
// KAP1 / KAP2 external application VM
// -----------------------------------------------------------------------------
//
// Storage remains ASCII hexadecimal and is streamed directly from microSD.
//
// Common KAP1/KAP2 opcodes:
//   10 cc                         CLS color
//   11 xx yy ss cc nn <nn bytes> TEXT
//   20 ll hh                      WAIT milliseconds (little endian)
//   21                            WAIT_TOUCH
//   30 nn <nn bytes>              SERIAL text
//   FF                            EXIT
//
// KAP2-only opcodes:
//   40 rr ll hh                   MOVI Rr, imm16
//   41 rr                         INC Rr
//   42 rr                         DEC Rr
//   43 rr ll hh                   CMPI Rr, imm16 -> zero flag
//   44                            MARK current stream position
//   45                            JNZ MARK
//   46 rr                         GET_TOUCH_X -> Rr
//   47 rr                         GET_TOUCH_Y -> Rr
//   48 xx yy ss cc rr             DRAW_REG
//   49                            JZ MARK
//   4A ii                         LABEL id (0..7)
//   4B ii                         JMP label id
//   4C ii                         JZ label id
//   4D ii                         JNZ label id
//
// LABEL/JMP/JZ/JNZ are the TEST-08 multi-label extension. KASM maps symbolic
// source names to compact IDs. Legacy MARK opcodes remain supported.
//
// Registers: R0..R3, unsigned 16-bit.
//
// MARK stores the current raw SD-file position immediately after the MARK
// opcode. JNZ/JZ seek back to that position. Because the ASCII-hex decoder
// ignores whitespace, a MARK placed on its own line naturally targets the next
// instruction.
//
// Color indices:
//   0 BLACK, 1 WHITE, 2 CYAN, 3 YELLOW,
//   4 GREEN, 5 RED, 6 BLUE, 7 GREY
//
// The resident kernel keeps scheduling Serial, Clock, Display and Touch while
// the application task interprets at most one VM instruction per dispatch.

static File appFile;
static bool appWaitTouch = false;
static uint32_t appWaitUntil = 0;
static uint8_t appExitCode = 0;
static uint8_t appFormatVersion = 0;
static uint16_t appRegs[4] = {0, 0, 0, 0};
static bool appZeroFlag = false;
static bool appMarkValid = false;
static uint32_t appMarkPos = 0;

const uint8_t APP_LABEL_SLOTS = 8;
static uint16_t appLabelPos[APP_LABEL_SLOTS] = {0};
static uint8_t appLabelMask = 0;

static uint16_t vmColor(uint8_t index) {
  switch (index) {
    case 1: return LCD_WHITE;
    case 2: return LCD_CYAN;
    case 3: return LCD_YELLOW;
    case 4: return LCD_GREEN;
    case 5: return LCD_RED;
    case 6: return LCD_BLUE;
    case 7: return LCD_GREY;
    default: return LCD_BLACK;
  }
}

static int8_t vmHexNibble(char c) {
  if (c >= '0' && c <= '9') return (int8_t)(c - '0');
  if (c >= 'A' && c <= 'F') return (int8_t)(c - 'A' + 10);
  if (c >= 'a' && c <= 'f') return (int8_t)(c - 'a' + 10);
  return -1;
}

static int16_t vmReadNibble() {
  while (appFile && appFile.available()) {
    char c = (char)appFile.read();
    int8_t n = vmHexNibble(c);
    if (n >= 0) return n;

    // Whitespace is allowed between encoded bytecode chunks.
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;

    return -2;
  }

  return -1;
}

static int16_t vmReadByte() {
  int16_t hi = vmReadNibble();
  if (hi < 0) return hi;

  int16_t lo = vmReadNibble();
  if (lo < 0) return lo;

  return (hi << 4) | lo;
}

static bool vmReadU8(uint8_t &value) {
  int16_t v = vmReadByte();
  if (v < 0) return false;
  value = (uint8_t)v;
  return true;
}

static bool vmReadU16LE(uint16_t &value) {
  uint8_t lo, hi;
  if (!vmReadU8(lo) || !vmReadU8(hi)) return false;
  value = (uint16_t)lo | ((uint16_t)hi << 8);
  return true;
}

static bool vmSkipBytes(uint16_t count) {
  uint8_t value;
  while (count--) {
    if (!vmReadU8(value)) return false;
  }
  return true;
}

static void vmClearLabels() {
  appLabelMask = 0;
  for (uint8_t i = 0; i < APP_LABEL_SLOTS; ++i) appLabelPos[i] = 0;
}

static bool vmIndexLabels(uint32_t programStart) {
  vmClearLabels();

  // Positions are stored as uint16_t to keep the UNO SRAM cost at 17 bytes.
  // Multi-label KAP2 files are therefore deliberately limited to 65535 raw
  // ASCII-file bytes.
  if (appFile.size() > 65535UL) return false;
  if (!appFile.seek(programStart)) return false;

  for (;;) {
    int16_t opRead = vmReadByte();
    if (opRead < 0) return false;

    uint8_t op = (uint8_t)opRead;

    if (op == 0x10) {
      if (!vmSkipBytes(1)) return false;
      continue;
    }

    if (op == 0x11) {
      uint8_t xUnit, y, scale, color, len;
      if (!vmReadU8(xUnit) ||
          !vmReadU8(y) ||
          !vmReadU8(scale) ||
          !vmReadU8(color) ||
          !vmReadU8(len) ||
          !vmSkipBytes(len)) return false;
      continue;
    }

    if (op == 0x20) {
      if (!vmSkipBytes(2)) return false;
      continue;
    }

    if (op == 0x21) continue;

    if (op == 0x30) {
      uint8_t len;
      if (!vmReadU8(len) || !vmSkipBytes(len)) return false;
      continue;
    }

    if (op == 0x40 || op == 0x43) {
      if (!vmSkipBytes(3)) return false;
      continue;
    }

    if (op == 0x41 || op == 0x42 || op == 0x46 || op == 0x47) {
      if (!vmSkipBytes(1)) return false;
      continue;
    }

    if (op == 0x44 || op == 0x45 || op == 0x49) continue;

    if (op == 0x48) {
      if (!vmSkipBytes(5)) return false;
      continue;
    }

    if (op == 0x4A) {
      uint8_t id;
      if (!vmReadU8(id) || id >= APP_LABEL_SLOTS) return false;

      uint8_t bit = (uint8_t)(1U << id);
      if (appLabelMask & bit) return false;

      uint32_t pos = appFile.position();
      if (pos > 65535UL) return false;

      appLabelPos[id] = (uint16_t)pos;
      appLabelMask |= bit;
      continue;
    }

    if (op == 0x4B || op == 0x4C || op == 0x4D) {
      if (!vmSkipBytes(1)) return false;
      continue;
    }

    if (op == 0xFF) {
      return appFile.seek(programStart);
    }

    return false;
  }
}

static bool vmJumpLabel(uint8_t id) {
  if (id >= APP_LABEL_SLOTS) return false;

  uint8_t bit = (uint8_t)(1U << id);
  if (!(appLabelMask & bit)) return false;

  return appFile.seek((uint32_t)appLabelPos[id]);
}

static void appReturnToSystem(uint8_t code) {
  if (appFile) appFile.close();

  appRunning = false;
  appWaitTouch = false;
  appTouchEvent = false;
  appWaitUntil = 0;
  appExitCode = code;
  appFormatVersion = 0;
  appZeroFlag = false;
  appMarkValid = false;
  appMarkPos = 0;
  vmClearLabels();
  for (uint8_t i = 0; i < 4; ++i) appRegs[i] = 0;

  Serial.print(F("APP EXIT "));
  Serial.println(code);

  drawDashboardStatic();
  updateDashboard();
}

static void appFault(uint8_t code) {
  Serial.print(F("APP FAULT "));
  Serial.println(code);
  appReturnToSystem(code);
}

static bool appStart(const char *path) {
  if (!sdReady) {
    Serial.println(F("ERR SD_NOT_READY"));
    return false;
  }

  if (appRunning) {
    Serial.println(F("ERR APP_BUSY"));
    return false;
  }

  appFile = SD.open(path, FILE_READ);

  if (!appFile || appFile.isDirectory()) {
    if (appFile) appFile.close();
    Serial.println(F("ERR APP_OPEN"));
    return false;
  }

  uint8_t header[4];

  for (uint8_t i = 0; i < 4; ++i) {
    int16_t b = vmReadByte();
    if (b < 0) {
      appFile.close();
      Serial.println(F("ERR APP_HEADER"));
      return false;
    }
    header[i] = (uint8_t)b;
  }

  if (header[0] != 'K' || header[1] != 'A' || header[2] != 'P' ||
      (header[3] != '1' && header[3] != '2')) {
    appFile.close();
    Serial.println(F("ERR APP_HEADER"));
    return false;
  }

  appFormatVersion = (uint8_t)(header[3] - '0');

  if (appFormatVersion == 2) {
    uint32_t programStart = appFile.position();

    if (!vmIndexLabels(programStart)) {
      appFile.close();
      appFormatVersion = 0;
      Serial.println(F("ERR APP_LABELS"));
      return false;
    }
  } else {
    vmClearLabels();
  }

  appRunning = true;
  appWaitTouch = false;
  appTouchEvent = false;
  appWaitUntil = 0;
  appExitCode = 0;
  appZeroFlag = false;
  appMarkValid = false;
  appMarkPos = 0;
  for (uint8_t i = 0; i < 4; ++i) appRegs[i] = 0;
  uiMode = UI_APP;

  lcdFillScreen(LCD_BLACK);
  if (appFormatVersion == 2)
    lcdTextP(8, 8, PSTR("KONSOL KAP2 APP"), 2, LCD_GREY);
  else
    lcdTextP(8, 8, PSTR("KONSOL KAP1 APP"), 2, LCD_GREY);

  Serial.print(F("APP RUN "));
  Serial.println(path);
  return true;
}

static void taskApp() {
  if (!appRunning) return;

  if (appWaitUntil) {
    if ((int32_t)(millis() - appWaitUntil) < 0) return;
    appWaitUntil = 0;
  }

  if (appWaitTouch) {
    if (!appTouchEvent) return;
    appTouchEvent = false;
    appWaitTouch = false;
    return;
  }

  int16_t opRead = vmReadByte();

  if (opRead < 0) {
    appFault(3);
    return;
  }

  uint8_t op = (uint8_t)opRead;

  if (op == 0x10) {
    uint8_t color;
    if (!vmReadU8(color)) {
      appFault(5);
      return;
    }

    lcdFillScreen(vmColor(color));
    return;
  }

  if (op == 0x11) {
    uint8_t xUnit, y, scale, color, len;

    if (!vmReadU8(xUnit) ||
        !vmReadU8(y) ||
        !vmReadU8(scale) ||
        !vmReadU8(color) ||
        !vmReadU8(len)) {
      appFault(5);
      return;
    }

    if (scale < 1) scale = 1;
    if (scale > 4) scale = 4;

    uint16_t x = (uint16_t)xUnit * 2U;
    uint16_t fg = vmColor(color);
    uint16_t step = 4U * scale;

    for (uint8_t i = 0; i < len; ++i) {
      uint8_t ch;
      if (!vmReadU8(ch)) {
        appFault(5);
        return;
      }

      if (ch < 32 || ch > 126) ch = '.';
      lcdChar(x, y, (char)ch, scale, fg);
      x += step;
    }

    return;
  }

  if (op == 0x20) {
    uint8_t lo, hi;

    if (!vmReadU8(lo) || !vmReadU8(hi)) {
      appFault(5);
      return;
    }

    uint16_t waitMs = (uint16_t)lo | ((uint16_t)hi << 8);
    appWaitUntil = millis() + waitMs;
    return;
  }

  if (op == 0x21) {
    appTouchEvent = false;
    appWaitTouch = true;
    return;
  }

  if (op == 0x30) {
    uint8_t len;
    if (!vmReadU8(len)) {
      appFault(5);
      return;
    }

    for (uint8_t i = 0; i < len; ++i) {
      uint8_t ch;
      if (!vmReadU8(ch)) {
        appFault(5);
        return;
      }
      Serial.write(ch);
    }

    Serial.println();
    return;
  }

  if (op >= 0x40 && op <= 0x4D && appFormatVersion != 2) {
    appFault(4);
    return;
  }

  if (op == 0x40) {
    uint8_t reg;
    uint16_t value;
    if (!vmReadU8(reg) || !vmReadU16LE(value) || reg >= 4) {
      appFault(5);
      return;
    }
    appRegs[reg] = value;
    return;
  }

  if (op == 0x41 || op == 0x42) {
    uint8_t reg;
    if (!vmReadU8(reg) || reg >= 4) {
      appFault(5);
      return;
    }

    if (op == 0x41) ++appRegs[reg];
    else --appRegs[reg];

    appZeroFlag = (appRegs[reg] == 0);
    return;
  }

  if (op == 0x43) {
    uint8_t reg;
    uint16_t value;
    if (!vmReadU8(reg) || !vmReadU16LE(value) || reg >= 4) {
      appFault(5);
      return;
    }

    appZeroFlag = (appRegs[reg] == value);
    return;
  }

  if (op == 0x44) {
    appMarkPos = appFile.position();
    appMarkValid = true;
    return;
  }

  if (op == 0x45 || op == 0x49) {
    if (!appMarkValid) {
      appFault(6);
      return;
    }

    bool jump = (op == 0x45) ? !appZeroFlag : appZeroFlag;
    if (jump && !appFile.seek(appMarkPos)) {
      appFault(7);
      return;
    }
    return;
  }

  if (op == 0x46 || op == 0x47) {
    uint8_t reg;
    if (!vmReadU8(reg) || reg >= 4) {
      appFault(5);
      return;
    }

    appRegs[reg] = (uint16_t)((op == 0x46) ? touchX : touchY);
    appZeroFlag = (appRegs[reg] == 0);
    return;
  }

  if (op == 0x48) {
    uint8_t xUnit, y, scale, color, reg;
    if (!vmReadU8(xUnit) ||
        !vmReadU8(y) ||
        !vmReadU8(scale) ||
        !vmReadU8(color) ||
        !vmReadU8(reg) ||
        reg >= 4) {
      appFault(5);
      return;
    }

    if (scale < 1) scale = 1;
    if (scale > 4) scale = 4;

    char buf[6];
    utoa(appRegs[reg], buf, 10);
    lcdText((uint16_t)xUnit * 2U, y, buf, scale, vmColor(color));
    return;
  }

  if (op == 0x4A) {
    uint8_t id;
    if (!vmReadU8(id) || id >= APP_LABEL_SLOTS) {
      appFault(5);
      return;
    }
    return;
  }

  if (op == 0x4B || op == 0x4C || op == 0x4D) {
    uint8_t id;
    if (!vmReadU8(id)) {
      appFault(5);
      return;
    }

    bool jump = (op == 0x4B) ||
                (op == 0x4C && appZeroFlag) ||
                (op == 0x4D && !appZeroFlag);

    if (jump && !vmJumpLabel(id)) {
      appFault(8);
      return;
    }
    return;
  }

  if (op == 0xFF) {
    appReturnToSystem(0);
    return;
  }

  appFault(4);
}

// -----------------------------------------------------------------------------
// Cooperative kernel
// -----------------------------------------------------------------------------

typedef void (*TaskFunction)();

struct KernelTask {
  TaskFunction fn;
  uint16_t periodMs;
  uint32_t nextRun;
  uint32_t runs;
  bool enabled;
};

static void taskSerial();
static void taskClock();
static void taskDisplay();
static void taskTouch();
static void taskApp();

static KernelTask tasks[] = {
  { taskSerial,  1,    0, 0, true },
  { taskClock,   100,  0, 0, true },
  { taskDisplay, 1000, 0, 0, true },
  { taskTouch,   30,   0, 0, true },
  { taskApp,     10,   0, 0, true }
};

const uint8_t TASK_COUNT = sizeof(tasks) / sizeof(tasks[0]);
static uint32_t kernelTicks = 0;

static void kernelInit() {
  uint32_t now = millis();

  for (uint8_t i = 0; i < TASK_COUNT; ++i) {
    tasks[i].nextRun = now + tasks[i].periodMs;
  }
}

static void kernelDispatch() {
  uint32_t now = millis();

  for (uint8_t i = 0; i < TASK_COUNT; ++i) {
    KernelTask &t = tasks[i];

    if (!t.enabled) continue;

    if ((int32_t)(now - t.nextRun) >= 0) {
      t.nextRun += t.periodMs;
      ++t.runs;
      t.fn();
    }
  }
}

static void taskClock() {
  ++kernelTicks;
}

static void taskDisplay() {
  updateDashboard();
}

static void taskTouch() {
  int16_t x, y, z;
  bool pressed = touchRead(x, y, z);

  touchDown = pressed;
  touchZ = z;

  if (pressed) {
    touchX = x;
    touchY = y;
  }

  bool newPress = pressed && !touchWasDown;
  touchWasDown = pressed;

  if (newPress) {
    if (appRunning) appTouchEvent = true;
    else uiHandlePress(touchX, touchY);
  }
}

// -----------------------------------------------------------------------------
// Shell
// -----------------------------------------------------------------------------

static char *skipSpaces(char *p) {
  while (*p == ' ' || *p == '\t') ++p;
  return p;
}

static void toUpperCommand(char *p) {
  while (*p && *p != ' ' && *p != '\t') {
    if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
    ++p;
  }
}

static char *splitCommand(char *line) {
  char *p = line;

  while (*p && *p != ' ' && *p != '\t') ++p;

  if (*p) {
    *p++ = 0;
    p = skipSpaces(p);
  }

  return p;
}

static void printPrompt() {
  Serial.print(F("A:/> "));
}

static void printBanner() {
  Serial.println();
  Serial.println(F("KonSol 0.8"));
  Serial.println(F("Arduino UNO / ATmega328P / 16 MHz"));
  Serial.println(F("Kernel + SD + direct ILI9341 + direct Touch"));
  Serial.println(F("KAP1/KAP2 VM + HOST1 + app launcher + touch browser"));
  Serial.println();
}

static bool mountSD() {
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  return SD.begin(SD_CS);
}

static void printSdStatus() {
  Serial.print(F("SD: "));
  Serial.println(sdReady ? F("READY") : F("NOT READY"));
}

static void cmdHelp() {
  Serial.println(F("HELP              command list"));
  Serial.println(F("INFO              system information"));
  Serial.println(F("MEM               free SRAM"));
  Serial.println(F("UPTIME            milliseconds since boot"));
  Serial.println(F("PS                cooperative task table"));
  Serial.println(F("MOUNT             initialize microSD"));
  Serial.println(F("DIR [path]        list directory"));
  Serial.println(F("TYPE <file>       print text file"));
  Serial.println(F("WRITE <file> txt  replace file with text"));
  Serial.println(F("APPEND <file> txt append text"));
  Serial.println(F("DEL <file>        delete file"));
  Serial.println(F("MKDIR <path>      create directory"));
  Serial.println(F("RMDIR <path>      remove empty directory"));
  Serial.println(F("FILES             touch file browser"));
  Serial.println(F("HOME              system dashboard"));
  Serial.println(F("TOUCH             last touch state"));
  Serial.println(F("RUN <file.KAP>    launch KAP1/KAP2 app"));
  Serial.println(F("APP               application status"));
  Serial.println(F("TFT               reinitialize display"));
  Serial.println(F("CLS               clear ANSI terminal"));
  Serial.println(F("REBOOT            watchdog reset"));
  Serial.println(F("@PING             HOST1 machine protocol"));
}

static void cmdInfo() {
  Serial.println(F("KonSol 0.8"));
  Serial.println(F("CPU: ATmega328P @ 16 MHz"));
  Serial.println(F("FLASH: 32 KB"));
  Serial.println(F("SRAM: 2 KB"));
  Serial.println(F("SCHED: cooperative"));
  Serial.print(F("TASKS: "));
  Serial.println(TASK_COUNT);
  Serial.println(F("TFT: ILI9341 direct 8-bit"));
  Serial.println(F("TOUCH: direct resistive"));
  Serial.println(F("APP VM: KAP1/KAP2 streamed from SD"));
  printSdStatus();
  Serial.print(F("FREE RAM: "));
  Serial.print(freeRam());
  Serial.println(F(" B"));
}

static void cmdMem() {
  Serial.print(F("FREE RAM: "));
  Serial.print(freeRam());
  Serial.println(F(" B"));
}

static void cmdUptime() {
  Serial.print(F("UPTIME: "));
  Serial.print(millis());
  Serial.println(F(" ms"));
  Serial.print(F("KERNEL TICKS: "));
  Serial.println(kernelTicks);
}

static void cmdPs() {
  Serial.println(F("ID  TASK      PERIOD  RUNS"));

  Serial.print(F("0   SERIAL    1 ms    "));
  Serial.println(tasks[0].runs);

  Serial.print(F("1   CLOCK     100 ms  "));
  Serial.println(tasks[1].runs);

  Serial.print(F("2   DISPLAY   1000 ms "));
  Serial.println(tasks[2].runs);

  Serial.print(F("3   TOUCH     30 ms   "));
  Serial.println(tasks[3].runs);

  Serial.print(F("4   APP       10 ms   "));
  Serial.println(tasks[4].runs);
}

static void cmdMount() {
  Serial.println(F("MOUNT BEGIN"));
  sdReady = mountSD();
  Serial.println(sdReady ? F("MOUNT PASS") : F("MOUNT FAIL"));

  if (uiMode == UI_DASHBOARD) updateDashboard();
  else if (uiMode == UI_BROWSER) browserDraw();
}

static void cmdDir(char *args) {
  if (!sdReady) {
    Serial.println(F("ERR SD_NOT_READY"));
    return;
  }

  char *path = skipSpaces(args);
  if (!*path) path = (char *)"/";

  Serial.print(F("DIR "));
  Serial.println(path);

  File dir = SD.open(path);

  if (!dir) {
    Serial.println(F("ERR OPEN"));
    return;
  }

  if (!dir.isDirectory()) {
    dir.close();
    Serial.println(F("ERR NOT_DIR"));
    return;
  }

  uint16_t count = 0;

  for (;;) {
    File entry = dir.openNextFile();
    if (!entry) break;

    Serial.write(entry.isDirectory() ? 'D' : 'F');
    Serial.write(' ');
    Serial.print((uint32_t)entry.size());
    Serial.write(' ');
    Serial.println(entry.name());

    entry.close();
    ++count;
  }

  dir.close();

  Serial.print(F("FILES: "));
  Serial.println(count);
}

static void cmdType(char *args) {
  if (!sdReady) {
    Serial.println(F("ERR SD_NOT_READY"));
    return;
  }

  char *path = skipSpaces(args);

  if (!*path) {
    Serial.println(F("ERR PATH"));
    return;
  }

  File f = SD.open(path, FILE_READ);

  if (!f) {
    Serial.println(F("ERR OPEN"));
    return;
  }

  if (f.isDirectory()) {
    f.close();
    Serial.println(F("ERR IS_DIR"));
    return;
  }

  Serial.println(F("-----"));

  while (f.available()) Serial.write((uint8_t)f.read());

  f.close();
  Serial.println();
  Serial.println(F("-----"));
}

static void writeText(char *args, bool appendMode) {
  if (!sdReady) {
    Serial.println(F("ERR SD_NOT_READY"));
    return;
  }

  args = skipSpaces(args);

  if (!*args) {
    Serial.println(F("ERR PATH"));
    return;
  }

  char *p = args;
  while (*p && *p != ' ' && *p != '\t') ++p;

  if (!*p) {
    Serial.println(F("ERR TEXT"));
    return;
  }

  *p++ = 0;
  char *text = skipSpaces(p);

  if (!appendMode && SD.exists(args)) {
    if (!SD.remove(args)) {
      Serial.println(F("ERR REMOVE_OLD"));
      return;
    }
  }

  File f = SD.open(args, FILE_WRITE);

  if (!f) {
    Serial.println(F("ERR CREATE"));
    return;
  }

  size_t n = f.print(text);
  f.write('\n');
  f.close();

  Serial.print(F("OK "));
  Serial.print((uint16_t)n);
  Serial.println(F(" B"));
}

static void cmdDelete(char *args) {
  if (!sdReady) {
    Serial.println(F("ERR SD_NOT_READY"));
    return;
  }

  char *path = skipSpaces(args);

  if (!*path) {
    Serial.println(F("ERR PATH"));
    return;
  }

  Serial.println(SD.remove(path) ? F("OK") : F("ERR DELETE"));
}

static void cmdMkdir(char *args) {
  if (!sdReady) {
    Serial.println(F("ERR SD_NOT_READY"));
    return;
  }

  char *path = skipSpaces(args);

  if (!*path) {
    Serial.println(F("ERR PATH"));
    return;
  }

  Serial.println(SD.mkdir(path) ? F("OK") : F("ERR MKDIR"));
}

static void cmdRmdir(char *args) {
  if (!sdReady) {
    Serial.println(F("ERR SD_NOT_READY"));
    return;
  }

  char *path = skipSpaces(args);

  if (!*path) {
    Serial.println(F("ERR PATH"));
    return;
  }

  Serial.println(SD.rmdir(path) ? F("OK") : F("ERR RMDIR"));
}

static void cmdFiles() {
  if (appRunning) {
    Serial.println(F("ERR APP_RUNNING"));
    return;
  }

  browserPage = 0;
  browserDraw();
}

static void cmdHome() {
  if (appRunning) {
    Serial.println(F("ERR APP_RUNNING"));
    return;
  }

  drawDashboardStatic();
  updateDashboard();
}

static void cmdTouch() {
  Serial.print(F("TOUCH: "));
  Serial.print(touchDown ? F("DOWN") : F("UP"));
  Serial.print(F(" X="));
  Serial.print(touchX);
  Serial.print(F(" Y="));
  Serial.print(touchY);
  Serial.print(F(" Z="));
  Serial.println(touchZ);
}

static void cmdRun(char *args) {
  char *path = skipSpaces(args);

  if (!*path) {
    Serial.println(F("ERR PATH"));
    return;
  }

  if (!isKapPath(path)) {
    Serial.println(F("ERR APP_EXT"));
    return;
  }

  appStart(path);
}

static void cmdApp() {
  Serial.print(F("APP: "));
  Serial.println(appRunning ? F("RUNNING") : F("IDLE"));

  if (appRunning) {
    Serial.print(F("FORMAT: KAP"));
    Serial.println(appFormatVersion);

    if (appFormatVersion == 2) {
      Serial.print(F("REGS: "));
      for (uint8_t i = 0; i < 4; ++i) {
        if (i) Serial.write(' ');
        Serial.write('R');
        Serial.print(i);
        Serial.write('=');
        Serial.print(appRegs[i]);
      }
      Serial.println();

      uint8_t labelCount = 0;
      for (uint8_t i = 0; i < APP_LABEL_SLOTS; ++i) {
        if (appLabelMask & (uint8_t)(1U << i)) ++labelCount;
      }
      Serial.print(F("LABELS: "));
      Serial.println(labelCount);
    }
  }

  Serial.print(F("LAST EXIT: "));
  Serial.println(appExitCode);
}

static void cmdTft() {
  if (appRunning) {
    Serial.println(F("ERR APP_RUNNING"));
    return;
  }

  Serial.println(F("TFT INIT"));
  lcdInit();

  if (uiMode == UI_BROWSER) browserDraw();
  else if (uiMode == UI_VIEWER) {
    drawDashboardStatic();
    updateDashboard();
  } else {
    drawDashboardStatic();
    updateDashboard();
  }

  Serial.println(F("TFT OK"));
}

static void cmdCls() {
  Serial.write(27);
  Serial.print(F("[2J"));
  Serial.write(27);
  Serial.print(F("[H"));
}

static void cmdReboot() {
  Serial.println(F("REBOOT"));
  Serial.flush();
  wdt_enable(WDTO_15MS);
  for (;;) {}
}

// -----------------------------------------------------------------------------
// HOST1 machine protocol over the existing USB-TTL Serial link
//
// Lines beginning with '@' are machine commands. They do not emit the A:/>
// shell prompt. Human shell commands remain unchanged.
//
// File transfer is ASCII-hex framed to keep the protocol deterministic on the
// ATmega328P without dynamic allocation or binary parser state.
// -----------------------------------------------------------------------------

static uint16_t hostCrc16Update(uint16_t crc, uint8_t data) {
  crc ^= (uint16_t)data << 8;
  for (uint8_t i = 0; i < 8; ++i) {
    if (crc & 0x8000U) crc = (uint16_t)((crc << 1) ^ 0x1021U);
    else crc <<= 1;
  }
  return crc;
}

static void hostPrintHexNibble(uint8_t v) {
  v &= 0x0F;
  Serial.write((char)(v < 10 ? ('0' + v) : ('A' + v - 10)));
}

static void hostPrintHexByte(uint8_t v) {
  hostPrintHexNibble(v >> 4);
  hostPrintHexNibble(v);
}

static void hostPrintHex16(uint16_t v) {
  hostPrintHexByte((uint8_t)(v >> 8));
  hostPrintHexByte((uint8_t)v);
}

static int8_t hostHexNibble(char c) {
  if (c >= '0' && c <= '9') return (int8_t)(c - '0');
  if (c >= 'a' && c <= 'f') return (int8_t)(c - 'a' + 10);
  if (c >= 'A' && c <= 'F') return (int8_t)(c - 'A' + 10);
  return -1;
}

static char *hostNextToken(char **cursor) {
  char *p = skipSpaces(*cursor);
  if (!*p) {
    *cursor = p;
    return p;
  }

  char *start = p;
  while (*p && *p != ' ' && *p != '\t') ++p;

  if (*p) {
    *p++ = 0;
    p = skipSpaces(p);
  }

  *cursor = p;
  return start;
}

static bool hostParseU32(const char *s, uint32_t &value) {
  if (!s || !*s) return false;
  uint32_t v = 0;

  while (*s) {
    if (*s < '0' || *s > '9') return false;
    uint8_t d = (uint8_t)(*s - '0');
    if (v > 429496729UL) return false;
    v = v * 10UL + d;
    ++s;
  }

  value = v;
  return true;
}

static bool hostParseHex16(const char *s, uint16_t &value) {
  if (!s || !*s) return false;
  uint16_t v = 0;
  uint8_t digits = 0;

  while (*s) {
    int8_t n = hostHexNibble(*s++);
    if (n < 0 || digits >= 4) return false;
    v = (uint16_t)((v << 4) | (uint8_t)n);
    ++digits;
  }

  if (!digits) return false;
  value = v;
  return true;
}

static bool hostFsReady() {
  if (!sdReady) {
    Serial.println(F("@ERR SD"));
    return false;
  }
  if (appRunning) {
    Serial.println(F("@ERR APP_BUSY"));
    return false;
  }
  return true;
}

static void hostInfo() {
  Serial.print(F("@OK INFO V="));
  Serial.print(F(KONSOL_VERSION));
  Serial.print(F(" HOST=1 SD="));
  Serial.print(sdReady ? 1 : 0);
  Serial.print(F(" APP="));
  Serial.print(appRunning ? 1 : 0);
  Serial.print(F(" RAM="));
  Serial.print(freeRam());
  Serial.print(F(" TASKS="));
  Serial.println(TASK_COUNT);
}

static void hostPs() {
  static const char * const names[] = {
    "SERIAL", "CLOCK", "DISPLAY", "TOUCH", "APP"
  };

  for (uint8_t i = 0; i < TASK_COUNT; ++i) {
    Serial.print(F("@TASK "));
    Serial.print(i);
    Serial.write(' ');
    Serial.print(names[i]);
    Serial.write(' ');
    Serial.print(tasks[i].periodMs);
    Serial.write(' ');
    Serial.println(tasks[i].runs);
  }
  Serial.print(F("@END PS "));
  Serial.println(TASK_COUNT);
}

static void hostLs(char *path) {
  if (!hostFsReady()) return;
  path = skipSpaces(path);
  if (!*path) path = (char *)"/";

  File dir = SD.open(path);
  if (!dir) {
    Serial.println(F("@ERR OPEN"));
    return;
  }
  if (!dir.isDirectory()) {
    dir.close();
    Serial.println(F("@ERR NOT_DIR"));
    return;
  }

  uint16_t count = 0;
  for (;;) {
    File entry = dir.openNextFile();
    if (!entry) break;

    if (entry.isDirectory()) {
      Serial.print(F("@D "));
      Serial.println(entry.name());
    } else {
      Serial.print(F("@F "));
      Serial.print((uint32_t)entry.size());
      Serial.write(' ');
      Serial.println(entry.name());
    }

    entry.close();
    ++count;
  }

  dir.close();
  Serial.print(F("@END LS "));
  Serial.println(count);
}

static void hostGet(char *path) {
  if (!hostFsReady()) return;
  path = skipSpaces(path);
  if (!*path) {
    Serial.println(F("@ERR PATH"));
    return;
  }

  File f = SD.open(path, FILE_READ);
  if (!f) {
    Serial.println(F("@ERR OPEN"));
    return;
  }
  if (f.isDirectory()) {
    f.close();
    Serial.println(F("@ERR IS_DIR"));
    return;
  }

  Serial.print(F("@BEGIN GET "));
  Serial.println((uint32_t)f.size());

  uint32_t count = 0;
  uint16_t crc = 0xFFFFU;
  uint8_t onLine = 0;

  while (f.available()) {
    int b = f.read();
    if (b < 0) break;

    if (!onLine) Serial.print(F("@DATA "));
    hostPrintHexByte((uint8_t)b);
    crc = hostCrc16Update(crc, (uint8_t)b);
    ++count;
    ++onLine;

    if (onLine >= 24) {
      Serial.println();
      onLine = 0;
    }
  }

  if (onLine) Serial.println();
  f.close();

  Serial.print(F("@END GET "));
  Serial.print(count);
  Serial.write(' ');
  hostPrintHex16(crc);
  Serial.println();
}

static void hostPutBegin(char *path) {
  if (!hostFsReady()) return;
  path = skipSpaces(path);
  if (!*path) {
    Serial.println(F("@ERR PATH"));
    return;
  }

  if (SD.exists(path) && !SD.remove(path)) {
    Serial.println(F("@ERR REMOVE"));
    return;
  }

  File f = SD.open(path, FILE_WRITE);
  if (!f) {
    Serial.println(F("@ERR CREATE"));
    return;
  }
  f.close();

  Serial.println(F("@OK PUTB"));
}

static void hostPutData(char *args) {
  if (!hostFsReady()) return;

  char *cursor = args;
  char *path = hostNextToken(&cursor);
  char *hex = hostNextToken(&cursor);

  if (!*path || !*hex || *cursor) {
    Serial.println(F("@ERR ARGS"));
    return;
  }

  uint8_t hexLen = (uint8_t)strlen(hex);
  if (!hexLen || (hexLen & 1U)) {
    Serial.println(F("@ERR HEX"));
    return;
  }

  for (uint8_t i = 0; i < hexLen; ++i) {
    if (hostHexNibble(hex[i]) < 0) {
      Serial.println(F("@ERR HEX"));
      return;
    }
  }

  File f = SD.open(path, FILE_WRITE);
  if (!f) {
    Serial.println(F("@ERR OPEN"));
    return;
  }

  uint8_t bytes = 0;
  for (uint8_t i = 0; i < hexLen; i += 2) {
    uint8_t hi = (uint8_t)hostHexNibble(hex[i]);
    uint8_t lo = (uint8_t)hostHexNibble(hex[i + 1]);
    f.write((uint8_t)((hi << 4) | lo));
    ++bytes;
  }
  f.close();

  Serial.print(F("@OK PUTD "));
  Serial.println(bytes);
}

static void hostPutEnd(char *args) {
  if (!hostFsReady()) return;

  char *cursor = args;
  char *path = hostNextToken(&cursor);
  char *sizeText = hostNextToken(&cursor);
  char *crcText = hostNextToken(&cursor);

  uint32_t expectedSize;
  uint16_t expectedCrc;

  if (!*path || !*sizeText || !*crcText || *cursor ||
      !hostParseU32(sizeText, expectedSize) ||
      !hostParseHex16(crcText, expectedCrc)) {
    Serial.println(F("@ERR ARGS"));
    return;
  }

  File f = SD.open(path, FILE_READ);
  if (!f || f.isDirectory()) {
    if (f) f.close();
    Serial.println(F("@ERR OPEN"));
    return;
  }

  uint32_t count = 0;
  uint16_t crc = 0xFFFFU;

  while (f.available()) {
    int b = f.read();
    if (b < 0) break;
    crc = hostCrc16Update(crc, (uint8_t)b);
    ++count;
  }
  f.close();

  if (count != expectedSize || crc != expectedCrc) {
    Serial.print(F("@ERR VERIFY "));
    Serial.print(count);
    Serial.write(' ');
    hostPrintHex16(crc);
    Serial.println();
    return;
  }

  Serial.print(F("@OK PUTE "));
  Serial.print(count);
  Serial.write(' ');
  hostPrintHex16(crc);
  Serial.println();
}

static void hostDelete(char *path) {
  if (!hostFsReady()) return;
  path = skipSpaces(path);
  if (!*path) {
    Serial.println(F("@ERR PATH"));
    return;
  }
  Serial.println(SD.remove(path) ? F("@OK DEL") : F("@ERR DEL"));
}

static void hostRun(char *path) {
  path = skipSpaces(path);
  if (!*path) {
    Serial.println(F("@ERR PATH"));
    return;
  }
  if (!isKapPath(path)) {
    Serial.println(F("@ERR APP_EXT"));
    return;
  }

  if (appStart(path)) Serial.println(F("@OK RUN"));
  else Serial.println(F("@ERR RUN"));
}

static void hostApp() {
  Serial.print(F("@OK APP "));
  Serial.print(appRunning ? F("RUNNING") : F("IDLE"));
  Serial.print(F(" FORMAT="));
  Serial.print(appRunning ? appFormatVersion : 0);
  Serial.print(F(" EXIT="));
  Serial.println(appExitCode);
}

static void hostStop() {
  if (!appRunning) {
    Serial.println(F("@ERR NO_APP"));
    return;
  }
  appReturnToSystem(254);
  Serial.println(F("@OK STOP"));
}

static void executeHostCommand(char *line) {
  line = skipSpaces(line);
  if (*line == '@') ++line;
  line = skipSpaces(line);
  if (!*line) {
    Serial.println(F("@ERR EMPTY"));
    return;
  }

  toUpperCommand(line);
  char *args = splitCommand(line);

  if (strcmp(line, "PING") == 0) {
    Serial.println(F("@OK PONG HOST1"));
  } else if (strcmp(line, "INFO") == 0) {
    hostInfo();
  } else if (strcmp(line, "MEM") == 0) {
    Serial.print(F("@OK MEM "));
    Serial.println(freeRam());
  } else if (strcmp(line, "PS") == 0) {
    hostPs();
  } else if (strcmp(line, "LS") == 0) {
    hostLs(args);
  } else if (strcmp(line, "GET") == 0) {
    hostGet(args);
  } else if (strcmp(line, "PUTB") == 0) {
    hostPutBegin(args);
  } else if (strcmp(line, "PUTD") == 0) {
    hostPutData(args);
  } else if (strcmp(line, "PUTE") == 0) {
    hostPutEnd(args);
  } else if (strcmp(line, "DEL") == 0) {
    hostDelete(args);
  } else if (strcmp(line, "RUN") == 0) {
    hostRun(args);
  } else if (strcmp(line, "APP") == 0) {
    hostApp();
  } else if (strcmp(line, "STOP") == 0) {
    hostStop();
  } else {
    Serial.println(F("@ERR COMMAND"));
  }
}

static void executeCommand(char *line) {
  line = skipSpaces(line);
  if (!*line) return;

  toUpperCommand(line);
  char *args = splitCommand(line);

  if (strcmp(line, "HELP") == 0) {
    cmdHelp();
  } else if (strcmp(line, "INFO") == 0) {
    cmdInfo();
  } else if (strcmp(line, "MEM") == 0) {
    cmdMem();
  } else if (strcmp(line, "UPTIME") == 0) {
    cmdUptime();
  } else if (strcmp(line, "PS") == 0) {
    cmdPs();
  } else if (strcmp(line, "MOUNT") == 0) {
    cmdMount();
  } else if (strcmp(line, "DIR") == 0 || strcmp(line, "LS") == 0) {
    cmdDir(args);
  } else if (strcmp(line, "TYPE") == 0 || strcmp(line, "CAT") == 0) {
    cmdType(args);
  } else if (strcmp(line, "WRITE") == 0) {
    writeText(args, false);
  } else if (strcmp(line, "APPEND") == 0) {
    writeText(args, true);
  } else if (strcmp(line, "DEL") == 0 || strcmp(line, "RM") == 0) {
    cmdDelete(args);
  } else if (strcmp(line, "MKDIR") == 0) {
    cmdMkdir(args);
  } else if (strcmp(line, "RMDIR") == 0) {
    cmdRmdir(args);
  } else if (strcmp(line, "FILES") == 0 || strcmp(line, "BROWSER") == 0) {
    cmdFiles();
  } else if (strcmp(line, "HOME") == 0) {
    cmdHome();
  } else if (strcmp(line, "TOUCH") == 0) {
    cmdTouch();
  } else if (strcmp(line, "RUN") == 0) {
    cmdRun(args);
  } else if (strcmp(line, "APP") == 0) {
    cmdApp();
  } else if (strcmp(line, "TFT") == 0) {
    cmdTft();
  } else if (strcmp(line, "CLS") == 0) {
    cmdCls();
  } else if (strcmp(line, "REBOOT") == 0) {
    cmdReboot();
  } else {
    Serial.println(F("ERR UNKNOWN COMMAND"));
  }
}

static void taskSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      commandLine[commandLen] = 0;

      bool hostLine = commandLen && commandLine[0] == '@';

      if (commandLen) {
        if (hostLine) executeHostCommand(commandLine);
        else executeCommand(commandLine);
      }

      commandLen = 0;
      if (!hostLine) printPrompt();
      continue;
    }

    if (c == 8 || c == 127) {
      if (commandLen) --commandLen;
      continue;
    }

    if (commandLen < CMD_SIZE - 1) {
      commandLine[commandLen++] = c;
    }
  }
}

// -----------------------------------------------------------------------------
// Arduino entry points
// -----------------------------------------------------------------------------

void setup() {
  MCUSR = 0;
  wdt_disable();

  Serial.begin(SERIAL_BAUD);
  printBanner();

  Serial.println(F("BOOT: TFT init"));
  lcdInit();
  drawDashboardStatic();

  Serial.println(F("BOOT: kernel init"));
  kernelInit();

  Serial.println(F("BOOT: SD mount"));
  sdReady = mountSD();
  printSdStatus();

  updateDashboard();

  Serial.print(F("FREE RAM: "));
  Serial.print(freeRam());
  Serial.println(F(" B"));

  Serial.println(F("Type HELP"));
  printPrompt();
}

void loop() {
  kernelDispatch();
}
