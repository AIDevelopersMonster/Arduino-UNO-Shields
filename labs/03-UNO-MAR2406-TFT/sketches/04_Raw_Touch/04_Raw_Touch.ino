/*
  LAB-03 / TEST-04
  Arduino UNO + MAR2406 2.4" TFT Touch Shield
  Raw resistive-touch diagnostic.

  Goal:
    1) verify the likely 4-wire resistive touch pin assignment;
    2) print raw X/Y/pressure-related ADC values;
    3) observe min/max ranges before calibration.

  No external libraries are required.

  Candidate touch wiring for this shield family:
    XP = D8
    XM = A2
    YP = A3
    YM = D9

  IMPORTANT:
  These pins are shared with the TFT parallel bus.
  The sketch disables LCD access while sampling touch and restores
  the shared pins afterward.

  Serial:
    115200 baud

  Commands:
    r = reset observed min/max statistics
    ? = help
*/

#include <Arduino.h>

static const uint8_t LCD_RD  = A0;
static const uint8_t LCD_WR  = A1;
static const uint8_t LCD_RS  = A2;
static const uint8_t LCD_CS  = A3;
static const uint8_t LCD_RST = A4;

// Candidate 4-wire resistive touch mapping.
static const uint8_t XP = 8;
static const uint8_t XM = A2;
static const uint8_t YP = A3;
static const uint8_t YM = 9;

struct TouchRaw {
  int16_t x;
  int16_t y;
  int16_t z1;
  int16_t z2;
  int16_t pressure;
};

static int16_t minX = 1023;
static int16_t maxX = 0;
static int16_t minY = 1023;
static int16_t maxY = 0;
static int16_t minP = 32767;
static int16_t maxP = -32768;

static void lcdBusIdle() {
  // Keep the TFT deselected and its strobes inactive while touch shares pins.
  pinMode(LCD_RD, OUTPUT);
  pinMode(LCD_WR, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  digitalWrite(LCD_RD, HIGH);
  digitalWrite(LCD_WR, HIGH);
  digitalWrite(LCD_CS, HIGH);
}

// Restore shared TFT pins after each touch conversion.
// Full TFT data/control direction restoration is intentionally minimal here:
// TEST-04 is a raw touch test, not a simultaneous graphics test.
static void restoreSharedPins() {
  pinMode(XP, OUTPUT);
  pinMode(YM, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  digitalWrite(XP, LOW);
  digitalWrite(YM, LOW);
  digitalWrite(XM, LOW);
  digitalWrite(YP, HIGH);  // LCD_CS idle HIGH

  lcdBusIdle();
}

static int analogReadSettled(uint8_t pin) {
  // First conversion after mux/source changes can be less stable on AVR.
  (void)analogRead(pin);
  delayMicroseconds(20);
  return analogRead(pin);
}

static TouchRaw readTouchRaw() {
  TouchRaw p;

  // ---- X coordinate ----
  // Drive X layer: XP=HIGH, XM=LOW.
  // Read voltage from YP.
  pinMode(YP, INPUT);
  pinMode(YM, INPUT);
  digitalWrite(YP, LOW);
  digitalWrite(YM, LOW);

  pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  digitalWrite(XP, HIGH);
  digitalWrite(XM, LOW);

  delayMicroseconds(30);
  p.x = analogReadSettled(YP);

  // ---- Y coordinate ----
  // Drive Y layer: YP=HIGH, YM=LOW.
  // Read voltage from XM.
  pinMode(XP, INPUT);
  pinMode(XM, INPUT);
  digitalWrite(XP, LOW);
  digitalWrite(XM, LOW);

  pinMode(YP, OUTPUT);
  pinMode(YM, OUTPUT);
  digitalWrite(YP, HIGH);
  digitalWrite(YM, LOW);

  delayMicroseconds(30);
  p.y = analogReadSettled(XM);

  // ---- Pressure proxy ----
  // Similar to the classic 4-wire resistive method:
  // XP=LOW, YM=HIGH, read the two opposite sense nodes.
  pinMode(XP, OUTPUT);
  digitalWrite(XP, LOW);

  pinMode(YM, OUTPUT);
  digitalWrite(YM, HIGH);

  pinMode(YP, INPUT);
  digitalWrite(YP, LOW);

  pinMode(XM, INPUT);
  digitalWrite(XM, LOW);

  delayMicroseconds(30);
  p.z1 = analogReadSettled(XM);
  p.z2 = analogReadSettled(YP);

  // Simple pressure proxy: higher positive values should correlate
  // with contact on this topology. Exact threshold is determined on bench.
  p.pressure = 1023 - (p.z2 - p.z1);

  restoreSharedPins();
  return p;
}

static void resetStats() {
  minX = 1023;
  maxX = 0;
  minY = 1023;
  maxY = 0;
  minP = 32767;
  maxP = -32768;
  Serial.println(F("STATS RESET"));
}

static void updateStats(const TouchRaw &p) {
  if (p.x < minX) minX = p.x;
  if (p.x > maxX) maxX = p.x;
  if (p.y < minY) minY = p.y;
  if (p.y > maxY) maxY = p.y;
  if (p.pressure < minP) minP = p.pressure;
  if (p.pressure > maxP) maxP = p.pressure;
}

static void printHelp() {
  Serial.println();
  Serial.println(F("LAB-03 TEST-04 - raw resistive touch probe"));
  Serial.println(F("Candidate wiring: XP=D8 XM=A2 YP=A3 YM=D9"));
  Serial.println(F("Press and release the panel at corners and center."));
  Serial.println(F("Watch X, Y and P change together."));
  Serial.println(F("Commands: r=reset stats, ?=help"));
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  lcdBusIdle();
  restoreSharedPins();

  delay(250);
  printHelp();

  Serial.println(F("FORMAT: X=<raw> Y=<raw> Z1=<raw> Z2=<raw> P=<proxy> | ranges"));
}

void loop() {
  if (Serial.available()) {
    const char c = (char)Serial.read();
    if (c == 'r' || c == 'R') {
      resetStats();
    } else if (c == '?') {
      printHelp();
    }
  }

  TouchRaw p = readTouchRaw();
  updateStats(p);

  Serial.print(F("X="));
  Serial.print(p.x);
  Serial.print(F(" Y="));
  Serial.print(p.y);
  Serial.print(F(" Z1="));
  Serial.print(p.z1);
  Serial.print(F(" Z2="));
  Serial.print(p.z2);
  Serial.print(F(" P="));
  Serial.print(p.pressure);

  Serial.print(F(" | X["));
  Serial.print(minX);
  Serial.print(',');
  Serial.print(maxX);
  Serial.print(F("] Y["));
  Serial.print(minY);
  Serial.print(',');
  Serial.print(maxY);
  Serial.print(F("] P["));
  Serial.print(minP);
  Serial.print(',');
  Serial.print(maxP);
  Serial.println(']');

  delay(120);
}
