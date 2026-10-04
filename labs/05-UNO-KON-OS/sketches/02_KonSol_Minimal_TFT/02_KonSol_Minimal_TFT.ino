/*
  KonSol 0.2
  LAB-05 / TEST-02
  Arduino UNO R3 + MAR2406 2.4" TFT + microSD

  Goal:
    Keep the verified KonSol 0.1 cooperative kernel + shell + SD services,
    but replace MCUFRIEND_kbv / Adafruit_GFX with a tiny direct ILI9341 driver.

  Architecture:
    - no FreeRTOS
    - no Arduino String
    - no malloc/new in KonSol code
    - one shared AVR stack
    - cooperative task dispatcher
    - Serial shell
    - microSD filesystem
    - direct 8-bit parallel ILI9341 system display

  Verified MAR2406 LCD wiring:
    LCD D0..D7 = UNO D8,D9,D2,D3,D4,D5,D6,D7
    RD  = A0
    WR  = A1
    RS  = A2
    CS  = A3
    RST = A4

  microSD:
    CS   = D10
    MOSI = D11
    MISO = D12
    SCK  = D13

  IMPORTANT:
    Direct LCD writes preserve D0/D1 (Serial) and D10..D13 (SPI).
    Touch is not enabled in TEST-02 because A1/A2/D6/D7 are shared with LCD.
*/

#include <SPI.h>
#include <SD.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>

#define KONSOL_VERSION "0.2"

const uint8_t SD_CS = 10;
const uint32_t SERIAL_BAUD = 115200UL;
const uint8_t CMD_SIZE = 88;

static char commandLine[CMD_SIZE];
static uint8_t commandLen = 0;
static bool sdReady = false;

// -----------------------------------------------------------------------------
// Minimal direct ILI9341 driver
// -----------------------------------------------------------------------------

#define LCD_RD_MASK   _BV(PC0)
#define LCD_WR_MASK   _BV(PC1)
#define LCD_RS_MASK   _BV(PC2)
#define LCD_CS_MASK   _BV(PC3)
#define LCD_RST_MASK  _BV(PC4)

#define LCD_BLACK   0x0000
#define LCD_WHITE   0xFFFF
#define LCD_RED     0xF800
#define LCD_GREEN   0x07E0
#define LCD_CYAN    0x07FF
#define LCD_YELLOW  0xFFE0
#define LCD_BLUE    0x001F
#define LCD_GREY    0x8410

static inline void lcdWrLow()  { PORTC &= (uint8_t)~LCD_WR_MASK; }
static inline void lcdWrHigh() { PORTC |= LCD_WR_MASK; }
static inline void lcdRsLow()  { PORTC &= (uint8_t)~LCD_RS_MASK; }
static inline void lcdRsHigh() { PORTC |= LCD_RS_MASK; }
static inline void lcdCsLow()  { PORTC &= (uint8_t)~LCD_CS_MASK; }
static inline void lcdCsHigh() { PORTC |= LCD_CS_MASK; }

static inline void lcdWrite8(uint8_t value) {
  // LCD bits 2..7 are UNO D2..D7 = PORTD bits 2..7.
  // Preserve D0/D1 because they are the hardware UART.
  PORTD = (PORTD & 0x03) | (value & 0xFC);

  // LCD bits 0..1 are UNO D8..D9 = PORTB bits 0..1.
  // Preserve D10..D13 because they are SD/SPI.
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
  lcdCommand(0x2A); // CASET
  lcdData16(x0);
  lcdData16(x1);

  lcdCommand(0x2B); // PASET
  lcdData16(y0);
  lcdData16(y1);

  lcdCommand(0x2C); // RAMWR
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
  // Data bus outputs: D2..D7 and D8..D9.
  DDRD |= 0xFC;
  DDRB |= 0x03;

  // Control bus A0..A4 = PORTC bits 0..4.
  DDRC |= LCD_RD_MASK | LCD_WR_MASK | LCD_RS_MASK | LCD_CS_MASK | LCD_RST_MASK;

  PORTC |= LCD_RD_MASK | LCD_WR_MASK | LCD_RS_MASK | LCD_CS_MASK | LCD_RST_MASK;

  // Hardware reset.
  PORTC &= (uint8_t)~LCD_RST_MASK;
  delay(20);
  PORTC |= LCD_RST_MASK;
  delay(120);

  lcdCommand(0x01); // software reset
  delay(10);

  lcdCommand(0x28); // display off

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

  // Landscape, BGR.
  lcdCommand(0x36); lcdData(0x28);
  lcdCommand(0x3A); lcdData(0x55); // RGB565

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

  lcdCommand(0x11); // sleep out
  delay(120);
  lcdCommand(0x29); // display on
  delay(20);

  lcdFillScreen(LCD_BLACK);
}

// -----------------------------------------------------------------------------
// Tiny 3x5 uppercase font
// -----------------------------------------------------------------------------

// A-Z followed by 0-9. Each byte is one 3-bit row.
static const uint8_t font3x5[36][5] PROGMEM = {
  {2,5,7,5,5}, // A
  {6,5,6,5,6}, // B
  {3,4,4,4,3}, // C
  {6,5,5,5,6}, // D
  {7,4,6,4,7}, // E
  {7,4,6,4,4}, // F
  {3,4,5,5,3}, // G
  {5,5,7,5,5}, // H
  {7,2,2,2,7}, // I
  {1,1,1,5,2}, // J
  {5,5,6,5,5}, // K
  {4,4,4,4,7}, // L
  {5,7,7,5,5}, // M
  {5,7,7,7,5}, // N
  {2,5,5,5,2}, // O
  {6,5,6,4,4}, // P
  {2,5,5,3,1}, // Q
  {6,5,6,5,5}, // R
  {3,4,2,1,6}, // S
  {7,2,2,2,2}, // T
  {5,5,5,5,7}, // U
  {5,5,5,5,2}, // V
  {5,5,7,7,5}, // W
  {5,5,2,5,5}, // X
  {5,5,2,2,2}, // Y
  {7,1,2,4,7}, // Z

  {7,5,5,5,7}, // 0
  {2,6,2,2,7}, // 1
  {6,1,7,4,7}, // 2
  {6,1,3,1,6}, // 3
  {5,5,7,1,1}, // 4
  {7,4,6,1,6}, // 5
  {3,4,7,5,7}, // 6
  {7,1,2,2,2}, // 7
  {7,5,7,5,7}, // 8
  {7,5,7,1,6}  // 9
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
    case '=': return (row == 1 || row == 3) ? 7 : 0;
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
          lcdFillRect(x + col * scale, y + row * scale, scale, scale, color);
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
// KonSol TFT system dashboard
// -----------------------------------------------------------------------------

static void lcdDrawStaticDashboard() {
  lcdFillScreen(LCD_BLACK);

  lcdTextP(12, 10, PSTR("KONSOL 0.2"), 3, LCD_CYAN);
  lcdTextP(12, 32, PSTR("MINIMAL TFT SYSTEM DRIVER"), 1, LCD_GREY);

  lcdHLine(10, 43, 300, LCD_BLUE);

  lcdTextP(18, 58, PSTR("KERNEL"), 2, LCD_WHITE);
  lcdTextP(18, 84, PSTR("SD"), 2, LCD_WHITE);
  lcdTextP(18, 110, PSTR("RAM"), 2, LCD_WHITE);
  lcdTextP(18, 136, PSTR("UPTIME"), 2, LCD_WHITE);
  lcdTextP(18, 162, PSTR("SHELL"), 2, LCD_WHITE);

  lcdRect(10, 188, 300, 39, LCD_GREY);
  lcdTextP(20, 198, PSTR("DIRECT ILI9341 8-BIT"), 2, LCD_YELLOW);
  lcdTextP(20, 216, PSTR("NO MCUFRIEND / NO GFX"), 1, LCD_GREY);
}

static void lcdClearValue(uint16_t y) {
  lcdFillRect(132, y, 174, 18, LCD_BLACK);
}

static void lcdUpdateDashboard() {
  char buf[14];

  lcdClearValue(56);
  lcdTextP(136, 58, PSTR("RUN"), 2, LCD_GREEN);

  lcdClearValue(82);
  lcdTextP(136, 84, sdReady ? PSTR("READY") : PSTR("NOT READY"),
           2, sdReady ? LCD_GREEN : LCD_RED);

  lcdClearValue(108);
  itoa(freeRam(), buf, 10);
  lcdText(136, 110, buf, 2, LCD_CYAN);
  lcdTextP(192, 110, PSTR("B"), 2, LCD_CYAN);

  lcdClearValue(134);
  ultoa(millis() / 1000UL, buf, 10);
  lcdText(136, 136, buf, 2, LCD_WHITE);
  lcdTextP(216, 136, PSTR("S"), 2, LCD_WHITE);

  lcdClearValue(160);
  lcdTextP(136, 162, PSTR("A:/>"), 2, LCD_YELLOW);
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

static KernelTask tasks[] = {
  { taskSerial,  1,    0, 0, true },
  { taskClock,   100,  0, 0, true },
  { taskDisplay, 1000, 0, 0, true }
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
  lcdUpdateDashboard();
}

// -----------------------------------------------------------------------------
// Shell helpers
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
  Serial.println(F("KonSol 0.2"));
  Serial.println(F("Arduino UNO / ATmega328P / 16 MHz"));
  Serial.println(F("Cooperative kernel + SD + direct ILI9341"));
  Serial.println(F("No MCUFRIEND_kbv / No Adafruit_GFX"));
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
  Serial.println(F("TFT               reinitialize display"));
  Serial.println(F("CLS               clear ANSI terminal"));
  Serial.println(F("REBOOT            watchdog reset"));
}

static void cmdInfo() {
  Serial.println(F("KonSol 0.2"));
  Serial.println(F("CPU: ATmega328P @ 16 MHz"));
  Serial.println(F("FLASH: 32 KB"));
  Serial.println(F("SRAM: 2 KB"));
  Serial.println(F("SCHED: cooperative"));
  Serial.print(F("TASKS: "));
  Serial.println(TASK_COUNT);
  Serial.println(F("TFT: ILI9341 direct 8-bit"));
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

  Serial.print(F("0   SERIAL    "));
  Serial.print(tasks[0].periodMs);
  Serial.print(F(" ms    "));
  Serial.println(tasks[0].runs);

  Serial.print(F("1   CLOCK     "));
  Serial.print(tasks[1].periodMs);
  Serial.print(F(" ms  "));
  Serial.println(tasks[1].runs);

  Serial.print(F("2   DISPLAY   "));
  Serial.print(tasks[2].periodMs);
  Serial.print(F(" ms "));
  Serial.println(tasks[2].runs);
}

static void cmdMount() {
  Serial.println(F("MOUNT BEGIN"));
  sdReady = mountSD();
  Serial.println(sdReady ? F("MOUNT PASS") : F("MOUNT FAIL"));
  lcdUpdateDashboard();
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

  while (f.available()) {
    Serial.write((uint8_t)f.read());
  }

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

static void cmdTft() {
  Serial.println(F("TFT INIT"));
  lcdInit();
  lcdDrawStaticDashboard();
  lcdUpdateDashboard();
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

// -----------------------------------------------------------------------------
// Serial task
// -----------------------------------------------------------------------------

static void taskSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      commandLine[commandLen] = 0;

      if (commandLen) {
        executeCommand(commandLine);
      }

      commandLen = 0;
      printPrompt();
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
  lcdDrawStaticDashboard();

  Serial.println(F("BOOT: kernel init"));
  kernelInit();

  Serial.println(F("BOOT: SD mount"));
  sdReady = mountSD();
  printSdStatus();

  lcdUpdateDashboard();

  Serial.print(F("FREE RAM: "));
  Serial.print(freeRam());
  Serial.println(F(" B"));

  Serial.println(F("Type HELP"));
  printPrompt();
}

void loop() {
  kernelDispatch();
}
