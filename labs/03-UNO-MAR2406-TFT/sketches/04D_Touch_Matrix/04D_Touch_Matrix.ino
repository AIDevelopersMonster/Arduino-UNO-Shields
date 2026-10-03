/*
  LAB-03 / TEST-04D
  Arduino UNO + MAR2406 2.4" TFT Touch Shield

  Event-driven raw touch matrix probe.
  No external libraries.

  Rationale:
  TEST-04A direct raw code clearly reacted to touch.
  TEST-04C library-based guided capture did not react on this sample.
  Therefore this test returns to direct GPIO/ADC control, but removes
  the continuous idle stream.

  Behavior:
  - wait silently for a touch-like event using the working raw detector;
  - once detected, take several direct measurements with different
    drive/sense combinations;
  - print ONE block per touch;
  - wait for release before arming again.

  Pins under investigation:
    D8, D9, A2, A3
*/

#include <Arduino.h>

static const uint8_t P_D8 = 8;
static const uint8_t P_D9 = 9;
static const uint8_t P_A2 = A2;
static const uint8_t P_A3 = A3;

static const uint8_t LCD_RD = A0;
static const uint8_t LCD_WR = A1;
static const uint8_t LCD_RS = A2;
static const uint8_t LCD_CS = A3;

static void lcdIdle() {
  pinMode(LCD_RD, OUTPUT);
  pinMode(LCD_WR, OUTPUT);
  pinMode(LCD_RS, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  digitalWrite(LCD_RD, HIGH);
  digitalWrite(LCD_WR, HIGH);
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, HIGH);
}

static int readStable(uint8_t pin) {
  (void)analogRead(pin);
  delayMicroseconds(20);
  int a=analogRead(pin);
  int b=analogRead(pin);
  int c=analogRead(pin);
  return (a+b+c)/3;
}

static void allInput() {
  pinMode(P_D8, INPUT);
  pinMode(P_D9, INPUT);
  pinMode(P_A2, INPUT);
  pinMode(P_A3, INPUT);
  digitalWrite(P_D8, LOW);
  digitalWrite(P_D9, LOW);
  digitalWrite(P_A2, LOW);
  digitalWrite(P_A3, LOW);
}

static int m1() {
  // Original TEST-04A X-like measurement:
  // D8=HIGH, A2=LOW, read A3
  allInput();
  pinMode(P_D8, OUTPUT); digitalWrite(P_D8, HIGH);
  pinMode(P_A2, OUTPUT); digitalWrite(P_A2, LOW);
  pinMode(P_A3, INPUT);
  pinMode(P_D9, INPUT);
  delayMicroseconds(30);
  return readStable(P_A3);
}

static int m2() {
  // Original TEST-04A Y-like measurement:
  // A3=HIGH, D9=LOW, read A2
  allInput();
  pinMode(P_A3, OUTPUT); digitalWrite(P_A3, HIGH);
  pinMode(P_D9, OUTPUT); digitalWrite(P_D9, LOW);
  pinMode(P_A2, INPUT);
  pinMode(P_D8, INPUT);
  delayMicroseconds(30);
  return readStable(P_A2);
}

static int m3() {
  // Alternate: D9=HIGH, A2=LOW, read A3
  allInput();
  pinMode(P_D9, OUTPUT); digitalWrite(P_D9, HIGH);
  pinMode(P_A2, OUTPUT); digitalWrite(P_A2, LOW);
  pinMode(P_A3, INPUT);
  pinMode(P_D8, INPUT);
  delayMicroseconds(30);
  return readStable(P_A3);
}

static int m4() {
  // Alternate: A3=HIGH, D8=LOW, read A2
  allInput();
  pinMode(P_A3, OUTPUT); digitalWrite(P_A3, HIGH);
  pinMode(P_D8, OUTPUT); digitalWrite(P_D8, LOW);
  pinMode(P_A2, INPUT);
  pinMode(P_D9, INPUT);
  delayMicroseconds(30);
  return readStable(P_A2);
}

static int detector() {
  // Use the one channel that definitely changed in TEST-04A.
  return m2();
}

static int median5(int (*fn)()) {
  int v[5];
  for (uint8_t i=0;i<5;++i) {
    v[i]=fn();
    delay(8);
  }
  for (uint8_t i=1;i<5;++i) {
    int key=v[i];
    int8_t j=i-1;
    while(j>=0 && v[j]>key) {
      v[j+1]=v[j];
      --j;
    }
    v[j+1]=key;
  }
  return v[2];
}

void setup() {
  Serial.begin(115200);
  lcdIdle();
  delay(250);

  Serial.println();
  Serial.println(F("LAB-03 TEST-04D - event-driven raw touch matrix"));
  Serial.println(F("No external library. No continuous stream."));
  Serial.println(F("Touch one point, hold briefly, release, then move to next point."));
  Serial.println(F("Suggested order: TL -> TR -> BL -> BR -> CENTER"));
  Serial.println(F("One TOUCH block will be printed per press."));
}

void loop() {
  static bool armed=true;
  static uint8_t seq=0;

  int d=detector();
  lcdIdle();

  // TEST-04A showed idle near ~75 and pressed near ~890 on this channel.
  // Keep wide margins; these are only event thresholds, not calibration constants.
  if (armed && d > 500) {
    delay(80);

    int a=median5(m1);
    int b=median5(m2);
    int c=median5(m3);
    int e=median5(m4);
    lcdIdle();

    ++seq;
    Serial.print(F("TOUCH #"));
    Serial.println(seq);
    Serial.print(F("  M1 D8=H A2=L read A3 : ")); Serial.println(a);
    Serial.print(F("  M2 A3=H D9=L read A2 : ")); Serial.println(b);
    Serial.print(F("  M3 D9=H A2=L read A3 : ")); Serial.println(c);
    Serial.print(F("  M4 A3=H D8=L read A2 : ")); Serial.println(e);
    Serial.println(F("RELEASE"));

    armed=false;
  }

  if (!armed && d < 250) {
    Serial.println(F("READY"));
    armed=true;
  }

  delay(25);
}
