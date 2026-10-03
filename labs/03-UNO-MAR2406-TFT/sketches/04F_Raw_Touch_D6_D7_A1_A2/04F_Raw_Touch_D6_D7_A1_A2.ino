/*
  LAB-03 / TEST-04F
  Arduino UNO + MAR2406 2.4" TFT Touch Shield

  Revised raw touch probe for candidate pins inferred from the actual shield
  layout and common 2.4" UNO resistive-touch topology.

  Candidate:
    XP = D6
    XM = A2
    YP = A1
    YM = D7

  No external library.

  Behavior:
    - no continuous idle stream
    - waits for a touch-like event
    - prints one averaged record
    - waits for release before rearming

  Physical order:
    TL -> TR -> BL -> BR -> CENTER
*/

#include <Arduino.h>

static const uint8_t XP = 6;
static const uint8_t XM = A2;
static const uint8_t YP = A1;
static const uint8_t YM = 7;

struct TouchRaw {
  int16_t x;
  int16_t y;
  int16_t z1;
  int16_t z2;
  int16_t pressure;
};

static int analogReadSettled(uint8_t pin) {
  (void)analogRead(pin);
  delayMicroseconds(20);
  int a = analogRead(pin);
  int b = analogRead(pin);
  return (a + b) >> 1;
}

static void releasePins() {
  pinMode(XP, INPUT);
  pinMode(XM, INPUT);
  pinMode(YP, INPUT);
  pinMode(YM, INPUT);

  digitalWrite(XP, LOW);
  digitalWrite(XM, LOW);
  digitalWrite(YP, LOW);
  digitalWrite(YM, LOW);
}

static TouchRaw readTouchRaw() {
  TouchRaw p;

  // X axis: drive XP/XM, sense YP.
  releasePins();

  pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  digitalWrite(XP, HIGH);
  digitalWrite(XM, LOW);

  pinMode(YP, INPUT);
  pinMode(YM, INPUT);

  delayMicroseconds(30);
  p.x = analogReadSettled(YP);

  // Y axis: drive YP/YM, sense XM.
  releasePins();

  pinMode(YP, OUTPUT);
  pinMode(YM, OUTPUT);
  digitalWrite(YP, HIGH);
  digitalWrite(YM, LOW);

  pinMode(XP, INPUT);
  pinMode(XM, INPUT);

  delayMicroseconds(30);
  p.y = analogReadSettled(XM);

  // Pressure proxy.
  releasePins();

  pinMode(XP, OUTPUT);
  digitalWrite(XP, LOW);

  pinMode(YM, OUTPUT);
  digitalWrite(YM, HIGH);

  pinMode(XM, INPUT);
  pinMode(YP, INPUT);

  delayMicroseconds(30);
  p.z1 = analogReadSettled(XM);
  p.z2 = analogReadSettled(YP);
  p.pressure = 1023 - (p.z2 - p.z1);

  releasePins();
  return p;
}

static TouchRaw averageTouch(uint8_t n=7) {
  long sx=0, sy=0, sz1=0, sz2=0, sp=0;

  for (uint8_t i=0; i<n; ++i) {
    TouchRaw p=readTouchRaw();
    sx += p.x;
    sy += p.y;
    sz1 += p.z1;
    sz2 += p.z2;
    sp += p.pressure;
    delay(15);
  }

  TouchRaw out;
  out.x = (int16_t)(sx/n);
  out.y = (int16_t)(sy/n);
  out.z1 = (int16_t)(sz1/n);
  out.z2 = (int16_t)(sz2/n);
  out.pressure = (int16_t)(sp/n);
  return out;
}

void setup() {
  Serial.begin(115200);
  releasePins();
  delay(250);

  Serial.println();
  Serial.println(F("LAB-03 TEST-04F - revised raw touch candidate"));
  Serial.println(F("Candidate: XP=D6 XM=A2 YP=A1 YM=D7"));
  Serial.println(F("Touch order: TL -> TR -> BL -> BR -> CENTER"));
  Serial.println(F("One press -> one line."));
}

void loop() {
  static bool armed = true;
  static uint8_t seq = 0;

  TouchRaw p = readTouchRaw();

  // Broad event threshold only. If this candidate is correct, pressure
  // should move strongly on contact.
  if (armed && p.pressure > 850) {
    delay(60);

    TouchRaw a = averageTouch();
    ++seq;

    Serial.print(F("POINT#"));
    Serial.print(seq);
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

    armed = false;
  }

  if (!armed && p.pressure < 750) {
    Serial.println(F("READY"));
    armed = true;
  }

  delay(20);
}
