/*
  LAB-03 / TEST-04E
  Arduino UNO + MAR2406 2.4" TFT Touch Shield

  Exact raw engine from the original TEST-04, with ONLY the output behavior
  changed from continuous streaming to one-shot event capture.

  Important:
  - no Adafruit TouchScreen library;
  - same pins;
  - same GPIO direction sequence;
  - same ADC settling;
  - same restoreSharedPins() sequence;
  - same pressure proxy.

  This preserves the only method already proven to react on the physical sample.

  Physical sequence:
    1) touch TOP-LEFT, hold, release
    2) touch TOP-RIGHT, hold, release
    3) touch BOTTOM-LEFT, hold, release
    4) touch BOTTOM-RIGHT, hold, release
    5) touch CENTER, hold, release

  One press -> one line.
*/

#include <Arduino.h>

static const uint8_t LCD_RD  = A0;
static const uint8_t LCD_WR  = A1;
static const uint8_t LCD_RS  = A2;
static const uint8_t LCD_CS  = A3;
static const uint8_t LCD_RST = A4;

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

static void lcdBusIdle() {
  pinMode(LCD_RD, OUTPUT);
  pinMode(LCD_WR, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  digitalWrite(LCD_RD, HIGH);
  digitalWrite(LCD_WR, HIGH);
  digitalWrite(LCD_CS, HIGH);
}

static void restoreSharedPins() {
  pinMode(XP, OUTPUT);
  pinMode(YM, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  digitalWrite(XP, LOW);
  digitalWrite(YM, LOW);
  digitalWrite(XM, LOW);
  digitalWrite(YP, HIGH);

  lcdBusIdle();
}

static int analogReadSettled(uint8_t pin) {
  (void)analogRead(pin);
  delayMicroseconds(20);
  return analogRead(pin);
}

static TouchRaw readTouchRaw() {
  TouchRaw p;

  // EXACT TEST-04 X sequence.
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

  // EXACT TEST-04 Y sequence.
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

  // EXACT TEST-04 pressure proxy sequence.
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

  p.pressure = 1023 - (p.z2 - p.z1);

  restoreSharedPins();
  return p;
}

static TouchRaw averagePressedSample() {
  const uint8_t N = 7;
  long sx=0, sy=0, sz1=0, sz2=0, sp=0;

  for (uint8_t i=0; i<N; ++i) {
    TouchRaw p=readTouchRaw();
    sx += p.x;
    sy += p.y;
    sz1 += p.z1;
    sz2 += p.z2;
    sp += p.pressure;
    delay(20);
  }

  TouchRaw out;
  out.x=(int16_t)(sx/N);
  out.y=(int16_t)(sy/N);
  out.z1=(int16_t)(sz1/N);
  out.z2=(int16_t)(sz2/N);
  out.pressure=(int16_t)(sp/N);
  return out;
}

void setup() {
  Serial.begin(115200);
  lcdBusIdle();
  restoreSharedPins();
  delay(250);

  Serial.println();
  Serial.println(F("LAB-03 TEST-04E - exact raw engine / one-shot output"));
  Serial.println(F("No library. Raw acquisition is identical to original TEST-04."));
  Serial.println(F("Touch order: TL -> TR -> BL -> BR -> CENTER"));
  Serial.println(F("One press -> one line; release fully between points."));
}

void loop() {
  static bool armed=true;
  static uint8_t n=0;

  TouchRaw p=readTouchRaw();

  // Thresholds are based only on the actual TEST-04 bench capture:
  // idle P was about 640; clear touches were about 1090.
  if (armed && p.pressure > 900) {
    delay(60);

    TouchRaw a=averagePressedSample();
    ++n;

    Serial.print(F("POINT#"));
    Serial.print(n);
    Serial.print(F(" X="));
    Serial.print(a.x);
    Serial.print(F(" Y="));
    Serial.print(a.y);
    Serial.print(F(" Z1="));
    Serial.print(a.z1);
    Serial.print(F(" Z2="));
    Serial.print(a.z2);
    Serial.print(F(" P="));
    Serial.println(a.pressure);

    armed=false;
  }

  // Actual idle capture was around P=640, so 800 leaves a wide release margin.
  if (!armed && p.pressure < 800) {
    Serial.println(F("READY"));
    armed=true;
  }

  delay(20);
}
