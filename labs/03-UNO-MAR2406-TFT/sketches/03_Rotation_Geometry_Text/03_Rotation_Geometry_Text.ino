/*
  LAB-03 / TEST-03
  Arduino UNO + MAR2406 2.4" TFT Touch Shield
  ILI9341 rotation + geometry + built-in text diagnostic.

  No external libraries are required.

  Serial commands at 115200 baud:
    0..3 = select rotation
    n    = next rotation
    a    = autoplay all four rotations
    ?    = print help

  Rotation map used here:
    0 -> MADCTL 0x48 -> 240 x 320
    1 -> MADCTL 0x28 -> 320 x 240
    2 -> MADCTL 0x88 -> 240 x 320
    3 -> MADCTL 0xE8 -> 320 x 240

  TEST-03 is intended to choose the canonical 320x240 landscape
  orientation for the rest of LAB-03.
*/

#include <Arduino.h>
#include <avr/pgmspace.h>

static const uint8_t LCD_RD  = A0;
static const uint8_t LCD_WR  = A1;
static const uint8_t LCD_RS  = A2;
static const uint8_t LCD_CS  = A3;
static const uint8_t LCD_RST = A4;

static uint16_t screenW = 240;
static uint16_t screenH = 320;
static uint8_t currentRotation = 0;

static const uint16_t BLACK   = 0x0000;
static const uint16_t WHITE   = 0xFFFF;
static const uint16_t RED     = 0xF800;
static const uint16_t GREEN   = 0x07E0;
static const uint16_t BLUE    = 0x001F;
static const uint16_t YELLOW  = 0xFFE0;
static const uint16_t MAGENTA = 0xF81F;
static const uint16_t CYAN    = 0x07FF;
static const uint16_t GRAY    = 0x7BEF;

static inline void dataBusOutput() {
  DDRD |= 0xFC;   // UNO D2..D7 = LCD D2..D7
  DDRB |= 0x03;   // UNO D8..D9 = LCD D0..D1
}

static inline void write8(uint8_t value) {
  PORTD = (PORTD & 0x03) | (value & 0xFC);
  PORTB = (PORTB & 0xFC) | (value & 0x03);

  digitalWrite(LCD_WR, LOW);
  digitalWrite(LCD_WR, HIGH);
}

static void writeCommand(uint8_t command) {
  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_RS, LOW);
  write8(command);
  digitalWrite(LCD_CS, HIGH);
}

static void writeCommandData(uint8_t command, const uint8_t *data, uint8_t count) {
  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_RS, LOW);
  write8(command);
  digitalWrite(LCD_RS, HIGH);

  for (uint8_t i = 0; i < count; ++i) {
    write8(data[i]);
  }

  digitalWrite(LCD_CS, HIGH);
}

static void hardwareReset() {
  digitalWrite(LCD_CS, HIGH);
  digitalWrite(LCD_RD, HIGH);
  digitalWrite(LCD_WR, HIGH);
  digitalWrite(LCD_RS, HIGH);

  digitalWrite(LCD_RST, HIGH);
  delay(5);
  digitalWrite(LCD_RST, LOW);
  delay(20);
  digitalWrite(LCD_RST, HIGH);
  delay(150);
}

static void ili9341Init() {
  writeCommand(0x01);
  delay(120);

  writeCommand(0x28);

  const uint8_t cf[] = {0x00, 0xC1, 0x30};
  writeCommandData(0xCF, cf, sizeof(cf));

  const uint8_t ed[] = {0x64, 0x03, 0x12, 0x81};
  writeCommandData(0xED, ed, sizeof(ed));

  const uint8_t e8[] = {0x85, 0x00, 0x78};
  writeCommandData(0xE8, e8, sizeof(e8));

  const uint8_t cb[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
  writeCommandData(0xCB, cb, sizeof(cb));

  const uint8_t f7[] = {0x20};
  writeCommandData(0xF7, f7, sizeof(f7));

  const uint8_t ea[] = {0x00, 0x00};
  writeCommandData(0xEA, ea, sizeof(ea));

  const uint8_t c0[] = {0x23};
  writeCommandData(0xC0, c0, sizeof(c0));

  const uint8_t c1[] = {0x10};
  writeCommandData(0xC1, c1, sizeof(c1));

  const uint8_t c5[] = {0x3E, 0x28};
  writeCommandData(0xC5, c5, sizeof(c5));

  const uint8_t c7[] = {0x86};
  writeCommandData(0xC7, c7, sizeof(c7));

  const uint8_t pixfmt[] = {0x55};  // RGB565
  writeCommandData(0x3A, pixfmt, sizeof(pixfmt));

  const uint8_t b1[] = {0x00, 0x18};
  writeCommandData(0xB1, b1, sizeof(b1));

  const uint8_t b6[] = {0x08, 0x82, 0x27};
  writeCommandData(0xB6, b6, sizeof(b6));

  const uint8_t f2[] = {0x00};
  writeCommandData(0xF2, f2, sizeof(f2));

  const uint8_t gammaSet[] = {0x01};
  writeCommandData(0x26, gammaSet, sizeof(gammaSet));

  const uint8_t gammaPos[] = {
    0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
    0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00
  };
  writeCommandData(0xE0, gammaPos, sizeof(gammaPos));

  const uint8_t gammaNeg[] = {
    0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
    0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F
  };
  writeCommandData(0xE1, gammaNeg, sizeof(gammaNeg));

  writeCommand(0x11);
  delay(120);

  writeCommand(0x29);
  delay(20);
}

static void setRotation(uint8_t rotation) {
  static const uint8_t madctl[4] = {0x48, 0x28, 0x88, 0xE8};

  currentRotation = rotation & 3;

  const uint8_t value = madctl[currentRotation];
  writeCommandData(0x36, &value, 1);

  if (currentRotation & 1) {
    screenW = 320;
    screenH = 240;
  } else {
    screenW = 240;
    screenH = 320;
  }
}

static void setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  uint8_t col[] = {
    (uint8_t)(x0 >> 8), (uint8_t)x0,
    (uint8_t)(x1 >> 8), (uint8_t)x1
  };
  writeCommandData(0x2A, col, sizeof(col));

  uint8_t row[] = {
    (uint8_t)(y0 >> 8), (uint8_t)y0,
    (uint8_t)(y1 >> 8), (uint8_t)y1
  };
  writeCommandData(0x2B, row, sizeof(row));
}

static void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if (w <= 0 || h <= 0) {
    return;
  }

  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x >= (int16_t)screenW || y >= (int16_t)screenH) {
    return;
  }
  if (x + w > (int16_t)screenW) {
    w = screenW - x;
  }
  if (y + h > (int16_t)screenH) {
    h = screenH - y;
  }
  if (w <= 0 || h <= 0) {
    return;
  }

  setAddressWindow((uint16_t)x, (uint16_t)y,
                   (uint16_t)(x + w - 1), (uint16_t)(y + h - 1));

  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_RS, LOW);
  write8(0x2C);
  digitalWrite(LCD_RS, HIGH);

  const uint8_t hi = color >> 8;
  const uint8_t lo = color & 0xFF;
  uint32_t pixels = (uint32_t)w * (uint32_t)h;

  while (pixels--) {
    write8(hi);
    write8(lo);
  }

  digitalWrite(LCD_CS, HIGH);
}

static void fillScreen(uint16_t color) {
  fillRect(0, 0, screenW, screenH, color);
}

static void drawPixel(int16_t x, int16_t y, uint16_t color) {
  fillRect(x, y, 1, 1, color);
}

static void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  fillRect(x, y, w, 1, color);
}

static void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  fillRect(x, y, 1, h, color);
}

static void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if (w <= 0 || h <= 0) {
    return;
  }
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y + h - 1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x + w - 1, y, h, color);
}

static void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  int16_t dx = abs(x1 - x0);
  int16_t sx = x0 < x1 ? 1 : -1;
  int16_t dy = -abs(y1 - y0);
  int16_t sy = y0 < y1 ? 1 : -1;
  int16_t err = dx + dy;

  for (;;) {
    drawPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) {
      break;
    }

    const int16_t e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

// Small 5x7 font: only glyphs required by TEST-03.
static const uint8_t GLYPH_0[5] PROGMEM = {0x3E,0x51,0x49,0x45,0x3E};
static const uint8_t GLYPH_1[5] PROGMEM = {0x00,0x42,0x7F,0x40,0x00};
static const uint8_t GLYPH_2[5] PROGMEM = {0x42,0x61,0x51,0x49,0x46};
static const uint8_t GLYPH_3[5] PROGMEM = {0x21,0x41,0x45,0x4B,0x31};
static const uint8_t GLYPH_4[5] PROGMEM = {0x18,0x14,0x12,0x7F,0x10};

static const uint8_t GLYPH_B[5] PROGMEM = {0x7F,0x49,0x49,0x49,0x36};
static const uint8_t GLYPH_H[5] PROGMEM = {0x7F,0x08,0x08,0x08,0x7F};
static const uint8_t GLYPH_L[5] PROGMEM = {0x7F,0x40,0x40,0x40,0x40};
static const uint8_t GLYPH_O[5] PROGMEM = {0x3E,0x41,0x41,0x41,0x3E};
static const uint8_t GLYPH_R[5] PROGMEM = {0x7F,0x09,0x19,0x29,0x46};
static const uint8_t GLYPH_T[5] PROGMEM = {0x01,0x01,0x7F,0x01,0x01};
static const uint8_t GLYPH_W[5] PROGMEM = {0x7F,0x20,0x18,0x20,0x7F};
static const uint8_t GLYPH_X[5] PROGMEM = {0x63,0x14,0x08,0x14,0x63};

static const uint8_t *glyphFor(char c) {
  switch (c) {
    case '0': return GLYPH_0;
    case '1': return GLYPH_1;
    case '2': return GLYPH_2;
    case '3': return GLYPH_3;
    case '4': return GLYPH_4;
    case 'B': return GLYPH_B;
    case 'H': return GLYPH_H;
    case 'L': return GLYPH_L;
    case 'O': return GLYPH_O;
    case 'R': return GLYPH_R;
    case 'T': return GLYPH_T;
    case 'W': return GLYPH_W;
    case 'X': return GLYPH_X;
    default:  return NULL;
  }
}

static void drawChar(int16_t x, int16_t y, char c,
                     uint16_t color, uint8_t scale) {
  if (c == ' ') {
    return;
  }

  const uint8_t *glyph = glyphFor(c);
  if (!glyph) {
    return;
  }

  for (uint8_t col = 0; col < 5; ++col) {
    uint8_t bits = pgm_read_byte(glyph + col);

    for (uint8_t row = 0; row < 7; ++row) {
      if (bits & (1u << row)) {
        fillRect(x + col * scale,
                 y + row * scale,
                 scale,
                 scale,
                 color);
      }
    }
  }
}

static void drawText(int16_t x, int16_t y, const char *text,
                     uint16_t color, uint8_t scale) {
  while (*text) {
    drawChar(x, y, *text, color, scale);
    x += 6 * scale;
    ++text;
  }
}

static void drawRotationScreen(uint8_t rotation) {
  setRotation(rotation);
  fillScreen(BLACK);

  // Full logical-frame border plus two inset rectangles.
  drawRect(0, 0, screenW, screenH, WHITE);
  drawRect(8, 8, screenW - 16, screenH - 16, GRAY);
  drawRect(20, 20, screenW - 40, screenH - 40, CYAN);

  // Diagonals make swapped/truncated axes obvious.
  drawLine(1, 1, screenW - 2, screenH - 2, GRAY);
  drawLine(screenW - 2, 1, 1, screenH - 2, GRAY);

  // Logical-corner markers.
  fillRect(4, 4, 16, 16, RED);                         // TL
  fillRect(screenW - 20, 4, 16, 16, GREEN);            // TR
  fillRect(4, screenH - 20, 16, 16, BLUE);             // BL
  fillRect(screenW - 20, screenH - 20, 16, 16, YELLOW);// BR

  drawText(24, 5, "TL", WHITE, 1);
  drawText(screenW - 48, 5, "TR", WHITE, 1);
  drawText(24, screenH - 13, "BL", WHITE, 1);
  drawText(screenW - 48, screenH - 13, "BR", WHITE, 1);

  const int16_t cx = screenW / 2;
  const int16_t cy = screenH / 2;

  drawFastHLine(cx - 30, cy, 61, MAGENTA);
  drawFastVLine(cx, cy - 30, 61, MAGENTA);

  char rotText[] = "ROT 0";
  rotText[4] = '0' + rotation;

  const uint8_t titleScale = 3;
  const int16_t titleW = 5 * 6 * titleScale;
  drawText((screenW - titleW) / 2, cy - 34, rotText, WHITE, titleScale);

  if (screenW == 320) {
    drawText((screenW - 9 * 12) / 2, cy + 8, "W320 H240", CYAN, 2);
  } else {
    drawText((screenW - 9 * 12) / 2, cy + 8, "W240 H320", CYAN, 2);
  }

  Serial.print(F("ROT "));
  Serial.print(rotation);
  Serial.print(F("  MADCTL=0x"));
  const uint8_t madctl[4] = {0x48, 0x28, 0x88, 0xE8};
  if (madctl[rotation] < 0x10) Serial.print('0');
  Serial.print(madctl[rotation], HEX);
  Serial.print(F("  "));
  Serial.print(screenW);
  Serial.print('x');
  Serial.println(screenH);
}

static void autoplay() {
  Serial.println(F("Autoplay: ROT 0 -> 1 -> 2 -> 3"));
  for (uint8_t r = 0; r < 4; ++r) {
    drawRotationScreen(r);
    delay(1800);
  }

  Serial.println(F("Autoplay complete. Returning to ROT 1 candidate landscape."));
  drawRotationScreen(1);
}

static void printHelp() {
  Serial.println();
  Serial.println(F("Commands:"));
  Serial.println(F("  0  ROT0  240x320  MADCTL 0x48"));
  Serial.println(F("  1  ROT1  320x240  MADCTL 0x28"));
  Serial.println(F("  2  ROT2  240x320  MADCTL 0x88"));
  Serial.println(F("  3  ROT3  320x240  MADCTL 0xE8"));
  Serial.println(F("  n  next rotation"));
  Serial.println(F("  a  autoplay all rotations"));
  Serial.println(F("  ?  help"));
  Serial.println();
  Serial.println(F("For LAB-03 choose the landscape rotation where"));
  Serial.println(F("ROT text and TL/TR/BL/BR are upright in the normal board position."));
}

void setup() {
  Serial.begin(115200);

  pinMode(LCD_RD, OUTPUT);
  pinMode(LCD_WR, OUTPUT);
  pinMode(LCD_RS, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_RST, OUTPUT);

  dataBusOutput();

  digitalWrite(LCD_RD, HIGH);
  digitalWrite(LCD_WR, HIGH);
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, HIGH);
  digitalWrite(LCD_RST, HIGH);

  delay(250);

  Serial.println();
  Serial.println(F("LAB-03 TEST-03 - rotation / geometry / text v0.1"));
  Serial.println(F("ILI9341 already confirmed by TEST-01."));
  Serial.println(F("Initializing display..."));

  hardwareReset();
  ili9341Init();

  Serial.println(F("Display initialized."));
  printHelp();
  autoplay();
}

void loop() {
  if (!Serial.available()) {
    return;
  }

  const char c = (char)Serial.read();

  if (c >= '0' && c <= '3') {
    drawRotationScreen((uint8_t)(c - '0'));
  } else if (c == 'n' || c == 'N') {
    drawRotationScreen((currentRotation + 1) & 3);
  } else if (c == 'a' || c == 'A') {
    autoplay();
  } else if (c == '?') {
    printHelp();
  }
}
