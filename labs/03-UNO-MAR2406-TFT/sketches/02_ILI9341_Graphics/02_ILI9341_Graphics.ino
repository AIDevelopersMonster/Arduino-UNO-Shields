/*
  LAB-03 / TEST-02
  Arduino UNO + MAR2406 2.4" TFT Touch Shield
  Direct ILI9341 graphics smoke test.

  No external libraries are required.

  Expected sequence:
    RED -> GREEN -> BLUE -> WHITE -> BLACK
  Then the display remains on a six-bar RGB565 color pattern.

  This sketch is intentionally AVR/UNO-specific for fast writes.
*/

#include <Arduino.h>

static const uint8_t LCD_RD  = A0;
static const uint8_t LCD_WR  = A1;
static const uint8_t LCD_RS  = A2;
static const uint8_t LCD_CS  = A3;
static const uint8_t LCD_RST = A4;

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

static void writeData8(uint8_t value) {
  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_RS, HIGH);
  write8(value);
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
  writeCommand(0x01); // Software reset
  delay(120);

  writeCommand(0x28); // Display OFF

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

  // Portrait 240x320, BGR color order.
  const uint8_t madctl[] = {0x48};
  writeCommandData(0x36, madctl, sizeof(madctl));

  // 16-bit RGB565 pixels.
  const uint8_t pixfmt[] = {0x55};
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

  writeCommand(0x11); // Sleep OUT
  delay(120);

  writeCommand(0x29); // Display ON
  delay(20);
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

static void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  if (w == 0 || h == 0) {
    return;
  }

  setAddressWindow(x, y, x + w - 1, y + h - 1);

  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_RS, LOW);
  write8(0x2C); // Memory write
  digitalWrite(LCD_RS, HIGH);

  const uint8_t hi = color >> 8;
  const uint8_t lo = color & 0xFF;
  uint32_t pixels = (uint32_t)w * h;

  while (pixels--) {
    write8(hi);
    write8(lo);
  }

  digitalWrite(LCD_CS, HIGH);
}

static void fillScreen(uint16_t color) {
  fillRect(0, 0, 240, 320, color);
}

static void showColorBars() {
  const uint16_t colors[6] = {
    0xF800, // red
    0x07E0, // green
    0x001F, // blue
    0xFFFF, // white
    0xFFE0, // yellow
    0xF81F  // magenta
  };

  for (uint8_t i = 0; i < 6; ++i) {
    fillRect(i * 40, 0, 40, 320, colors[i]);
  }
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
  Serial.println(F("LAB-03 TEST-02 - ILI9341 graphics smoke test v0.1"));
  Serial.println(F("TEST-01 already confirmed controller ID 0x9341."));
  Serial.println(F("Initializing display..."));

  hardwareReset();
  ili9341Init();

  Serial.println(F("Display initialized."));
  Serial.println(F("Expected: RED -> GREEN -> BLUE -> WHITE -> BLACK -> color bars"));

  fillScreen(0xF800);
  Serial.println(F("RED"));
  delay(1200);

  fillScreen(0x07E0);
  Serial.println(F("GREEN"));
  delay(1200);

  fillScreen(0x001F);
  Serial.println(F("BLUE"));
  delay(1200);

  fillScreen(0xFFFF);
  Serial.println(F("WHITE"));
  delay(1200);

  fillScreen(0x0000);
  Serial.println(F("BLACK"));
  delay(1200);

  showColorBars();
  Serial.println(F("COLOR BARS"));
  Serial.println(F("TEST-02 visual result now requires operator observation."));
}

void loop() {
}
