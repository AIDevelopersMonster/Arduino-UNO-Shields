/*
  LAB-03 / TEST-04C
  Arduino UNO + MAR2406 2.4" TFT Touch Shield
  Guided point capture for resistive touch.

  This version intentionally prints NOTHING continuously.
  The operator holds the stylus on a requested point and presses
  a serial command to capture one averaged/median-like result.

  Touch mapping used:
    XP = D8
    XM = A2
    YP = A3
    YM = D9

  This is the standard mapping for many 2.4" UNO ILI9341 shields
  and matches the tested shield family.

  Commands:
    1 = capture TOP-LEFT
    2 = capture TOP-RIGHT
    3 = capture BOTTOM-LEFT
    4 = capture BOTTOM-RIGHT
    5 = capture CENTER
    ? = help

  Requires library:
    Adafruit TouchScreen
*/

#include <Arduino.h>
#include <TouchScreen.h>

static const uint8_t XP = 8;
static const uint8_t XM = A2;
static const uint8_t YP = A3;
static const uint8_t YM = 9;

static const int RXPLATE_OHMS = 300;
static TouchScreen ts(XP, YP, XM, YM, RXPLATE_OHMS);

static const uint8_t LCD_RS = A2;
static const uint8_t LCD_CS = A3;

struct Sample {
  int16_t x;
  int16_t y;
  int16_t z;
};

static const uint8_t N = 15;

static void restoreTftPins() {
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, HIGH);
}

static void sort16(int16_t *v, uint8_t n) {
  for (uint8_t i = 1; i < n; ++i) {
    int16_t key = v[i];
    int8_t j = i - 1;
    while (j >= 0 && v[j] > key) {
      v[j + 1] = v[j];
      --j;
    }
    v[j + 1] = key;
  }
}

static Sample capturePoint() {
  int16_t xs[N];
  int16_t ys[N];
  int16_t zs[N];

  delay(120);

  for (uint8_t i = 0; i < N; ++i) {
    TSPoint p = ts.getPoint();
    xs[i] = p.x;
    ys[i] = p.y;
    zs[i] = p.z;
    delay(15);
  }

  restoreTftPins();

  sort16(xs, N);
  sort16(ys, N);
  sort16(zs, N);

  Sample s;
  s.x = xs[N / 2];
  s.y = ys[N / 2];
  s.z = zs[N / 2];
  return s;
}

static const __FlashStringHelper *pointName(char c) {
  switch (c) {
    case '1': return F("TOP-LEFT");
    case '2': return F("TOP-RIGHT");
    case '3': return F("BOTTOM-LEFT");
    case '4': return F("BOTTOM-RIGHT");
    case '5': return F("CENTER");
    default:  return F("UNKNOWN");
  }
}

static void printHelp() {
  Serial.println();
  Serial.println(F("LAB-03 TEST-04C - guided touch capture"));
  Serial.println(F("Touch map: XP=D8 XM=A2 YP=A3 YM=D9"));
  Serial.println(F("Hold stylus on requested point, then send:"));
  Serial.println(F("  1 = TOP-LEFT"));
  Serial.println(F("  2 = TOP-RIGHT"));
  Serial.println(F("  3 = BOTTOM-LEFT"));
  Serial.println(F("  4 = BOTTOM-RIGHT"));
  Serial.println(F("  5 = CENTER"));
  Serial.println(F("  ? = help"));
  Serial.println();
  Serial.println(F("One command -> one captured point. No continuous stream."));
}

void setup() {
  Serial.begin(115200);
  restoreTftPins();
  delay(250);
  printHelp();
}

void loop() {
  if (!Serial.available()) {
    return;
  }

  char c = (char)Serial.read();

  if (c == '?') {
    printHelp();
    return;
  }

  if (c < '1' || c > '5') {
    return;
  }

  Serial.print(F("CAPTURE "));
  Serial.print(pointName(c));
  Serial.println(F(" ... keep stylus pressed"));

  Sample s = capturePoint();

  Serial.print(F("POINT "));
  Serial.print(pointName(c));
  Serial.print(F(" : X="));
  Serial.print(s.x);
  Serial.print(F(" Y="));
  Serial.print(s.y);
  Serial.print(F(" Z="));
  Serial.println(s.z);
}
