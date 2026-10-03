/*
  LAB-03 / TEST-01
  Arduino UNO + MAR2406 2.4" TFT Touch Shield
  Low-level LCD controller ID probe.

  No external libraries are required.

  Shield bus map:
    LCD_D0  -> D8
    LCD_D1  -> D9
    LCD_D2  -> D2
    LCD_D3  -> D3
    LCD_D4  -> D4
    LCD_D5  -> D5
    LCD_D6  -> D6
    LCD_D7  -> D7

    LCD_RD  -> A0
    LCD_WR  -> A1
    LCD_RS  -> A2
    LCD_CS  -> A3
    LCD_RST -> A4

  The sketch intentionally does not initialize the display.
  A white or unchanged screen during this test is normal.

  Serial:
    115200 baud
    r = repeat register probe
    x = hardware reset and repeat probe
*/

#include <Arduino.h>

static const uint8_t LCD_RD  = A0;
static const uint8_t LCD_WR  = A1;
static const uint8_t LCD_RS  = A2;
static const uint8_t LCD_CS  = A3;
static const uint8_t LCD_RST = A4;

// Bit number in the byte -> Arduino UNO pin.
static const uint8_t LCD_DATA[8] = {
  8,  // D0
  9,  // D1
  2,  // D2
  3,  // D3
  4,  // D4
  5,  // D5
  6,  // D6
  7   // D7
};

static void dataBusOutput() {
  for (uint8_t i = 0; i < 8; ++i) {
    pinMode(LCD_DATA[i], OUTPUT);
    digitalWrite(LCD_DATA[i], LOW);
  }
}

static void dataBusInput() {
  for (uint8_t i = 0; i < 8; ++i) {
    // Disable the AVR input pull-up before allowing the LCD to drive the bus.
    digitalWrite(LCD_DATA[i], LOW);
    pinMode(LCD_DATA[i], INPUT);
  }
}

static void write8(uint8_t value) {
  for (uint8_t bit = 0; bit < 8; ++bit) {
    digitalWrite(LCD_DATA[bit], (value & (1u << bit)) ? HIGH : LOW);
  }

  digitalWrite(LCD_WR, LOW);
  delayMicroseconds(1);
  digitalWrite(LCD_WR, HIGH);
  delayMicroseconds(1);
}

static uint8_t read8() {
  uint8_t value = 0;

  for (uint8_t bit = 0; bit < 8; ++bit) {
    if (digitalRead(LCD_DATA[bit])) {
      value |= (1u << bit);
    }
  }

  return value;
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

// Reads raw bus bytes returned after an 8-bit command.
// The buffer contains every RD cycle, including any protocol dummy byte.
static void readRegisterRaw(uint8_t command, uint8_t *buffer, uint8_t count) {
  dataBusOutput();

  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_RS, LOW);
  write8(command);

  digitalWrite(LCD_RS, HIGH);
  dataBusInput();

  for (uint8_t i = 0; i < count; ++i) {
    digitalWrite(LCD_RD, LOW);
    delayMicroseconds(3);
    buffer[i] = read8();
    digitalWrite(LCD_RD, HIGH);
    delayMicroseconds(3);
  }

  digitalWrite(LCD_CS, HIGH);
  dataBusOutput();
}

static void printHexByte(uint8_t value) {
  if (value < 0x10) {
    Serial.print('0');
  }
  Serial.print(value, HEX);
}

static void printRegister(const __FlashStringHelper *name,
                          uint8_t command,
                          uint8_t count,
                          uint8_t *capture) {
  readRegisterRaw(command, capture, count);

  Serial.print(name);
  Serial.print(F(" [0x"));
  printHexByte(command);
  Serial.print(F("] :"));

  for (uint8_t i = 0; i < count; ++i) {
    Serial.print(' ');
    printHexByte(capture[i]);
  }

  Serial.println();
}

static bool contains9341(const uint8_t *data, uint8_t count) {
  if (count < 2) {
    return false;
  }

  for (uint8_t i = 0; i + 1 < count; ++i) {
    if (data[i] == 0x93 && data[i + 1] == 0x41) {
      return true;
    }
  }

  return false;
}

static bool allSame(const uint8_t *data, uint8_t count, uint8_t value) {
  for (uint8_t i = 0; i < count; ++i) {
    if (data[i] != value) {
      return false;
    }
  }
  return true;
}

static void probeController() {
  uint8_t id4[4];
  uint8_t rddid[4];
  uint8_t status[5];
  uint8_t id1[2];
  uint8_t id2[2];
  uint8_t id3[2];

  Serial.println();
  Serial.println(F("=== MAR2406 LCD REGISTER PROBE ==="));

  printRegister(F("D3 ID4 "), 0xD3, sizeof(id4), id4);
  printRegister(F("04 RDDID"), 0x04, sizeof(rddid), rddid);
  printRegister(F("09 RDDST"), 0x09, sizeof(status), status);
  printRegister(F("DA ID1  "), 0xDA, sizeof(id1), id1);
  printRegister(F("DB ID2  "), 0xDB, sizeof(id2), id2);
  printRegister(F("DC ID3  "), 0xDC, sizeof(id3), id3);

  Serial.println();

  if (contains9341(id4, sizeof(id4))) {
    Serial.println(F("RESULT: ILI9341 signature 0x9341 detected in D3 response."));
    Serial.println(F("TEST-01: candidate PASS; record the full capture in the lab result."));
  } else if (allSame(id4, sizeof(id4), 0x00) ||
             allSame(id4, sizeof(id4), 0xFF)) {
    Serial.println(F("RESULT: D3 response is stuck at 00/FF; identification is inconclusive."));
    Serial.println(F("Do not assume a bad LCD yet. Save the complete serial output for diagnosis."));
  } else {
    Serial.println(F("RESULT: no 0x9341 signature found in the D3 capture."));
    Serial.println(F("The raw bytes may identify another controller/revision; save all output."));
  }

  Serial.println(F("Commands: r=repeat, x=reset+repeat"));
}

static void printBanner() {
  Serial.println();
  Serial.println(F("LAB-03 TEST-01 - MAR2406 LCD ID Probe v0.1"));
  Serial.println(F("Arduino UNO, direct 8-bit parallel register read"));
  Serial.println(F("No external display library is used."));
  Serial.println(F("A white/uninitialized LCD during this test is normal."));
  Serial.println();
  Serial.println(F("Bus map: D0=D8 D1=D9 D2=D2 D3=D3 D4=D4 D5=D5 D6=D6 D7=D7"));
  Serial.println(F("Control: RD=A0 WR=A1 RS=A2 CS=A3 RST=A4"));
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
  printBanner();

  Serial.println(F("Hardware reset..."));
  hardwareReset();
  Serial.println(F("Reset complete."));

  probeController();
}

void loop() {
  if (!Serial.available()) {
    return;
  }

  const char c = (char)Serial.read();

  if (c == 'r' || c == 'R') {
    probeController();
  } else if (c == 'x' || c == 'X') {
    Serial.println(F("Hardware reset..."));
    hardwareReset();
    Serial.println(F("Reset complete."));
    probeController();
  }
}
