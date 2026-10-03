/*
  LAB-03 / TEST-04B
  Arduino UNO + MAR2406 2.4" TFT Touch Shield
  Alternate raw resistive-touch diagnostic.

  Candidate B:
    XP = D9
    YP = A2
    XM = A3
    YM = D8

  This arrangement matches the common Arduino 2.4" resistive-touch demo
  topology where YP and XM are the two ADC sense pins.

  No external library required.
*/

#include <Arduino.h>

static const uint8_t LCD_RD  = A0;
static const uint8_t LCD_WR  = A1;
static const uint8_t LCD_RS  = A2;
static const uint8_t LCD_CS  = A3;

static const uint8_t XP = 9;
static const uint8_t YP = A2;
static const uint8_t XM = A3;
static const uint8_t YM = 8;

struct TouchRaw {
  int16_t x;
  int16_t y;
  int16_t z1;
  int16_t z2;
  int16_t pressure;
};

static int16_t minX=1023,maxX=0,minY=1023,maxY=0,minP=32767,maxP=-32768;

static void lcdIdle() {
  pinMode(LCD_RD,OUTPUT);
  pinMode(LCD_WR,OUTPUT);
  pinMode(LCD_RS,OUTPUT);
  pinMode(LCD_CS,OUTPUT);
  digitalWrite(LCD_RD,HIGH);
  digitalWrite(LCD_WR,HIGH);
  digitalWrite(LCD_RS,HIGH);
  digitalWrite(LCD_CS,HIGH);
}

static int readAvg(uint8_t pin) {
  (void)analogRead(pin);
  delayMicroseconds(20);
  int a=analogRead(pin);
  int b=analogRead(pin);
  return (a+b)>>1;
}

static TouchRaw readTouchRaw() {
  TouchRaw p;

  // X: YP/YM high-Z, drive XP high and XM low, read YP.
  pinMode(YP,INPUT);
  pinMode(YM,INPUT);
  digitalWrite(YP,LOW);
  digitalWrite(YM,LOW);

  pinMode(XP,OUTPUT);
  pinMode(XM,OUTPUT);
  digitalWrite(XP,HIGH);
  digitalWrite(XM,LOW);

  delayMicroseconds(30);
  p.x = 1023 - readAvg(YP);

  // Y: XP/XM high-Z, drive YP high and YM low, read XM.
  pinMode(XP,INPUT);
  pinMode(XM,INPUT);
  digitalWrite(XP,LOW);
  digitalWrite(XM,LOW);

  pinMode(YP,OUTPUT);
  pinMode(YM,OUTPUT);
  digitalWrite(YP,HIGH);
  digitalWrite(YM,LOW);

  delayMicroseconds(30);
  p.y = 1023 - readAvg(XM);

  // Pressure proxy, same topology as common TouchScreen implementations.
  pinMode(XP,OUTPUT);
  digitalWrite(XP,LOW);

  pinMode(YM,OUTPUT);
  digitalWrite(YM,HIGH);

  pinMode(XM,INPUT);
  digitalWrite(XM,LOW);

  pinMode(YP,INPUT);
  digitalWrite(YP,LOW);

  delayMicroseconds(30);
  p.z1 = readAvg(XM);
  p.z2 = readAvg(YP);
  p.pressure = 1023 - (p.z2 - p.z1);

  lcdIdle();
  return p;
}

static void resetStats() {
  minX=1023; maxX=0; minY=1023; maxY=0; minP=32767; maxP=-32768;
  Serial.println(F("STATS RESET"));
}

static void updateStats(const TouchRaw &p) {
  if(p.x<minX)minX=p.x; if(p.x>maxX)maxX=p.x;
  if(p.y<minY)minY=p.y; if(p.y>maxY)maxY=p.y;
  if(p.pressure<minP)minP=p.pressure; if(p.pressure>maxP)maxP=p.pressure;
}

void setup() {
  Serial.begin(115200);
  lcdIdle();
  delay(250);
  Serial.println();
  Serial.println(F("LAB-03 TEST-04B - alternate raw touch probe"));
  Serial.println(F("Candidate B: XP=D9 YP=A2 XM=A3 YM=D8"));
  Serial.println(F("Press TL, TR, BL, BR, CENTER; hold each about 1 second."));
  Serial.println(F("Command r = reset ranges"));
}

void loop() {
  if(Serial.available()) {
    char c=(char)Serial.read();
    if(c=='r'||c=='R') resetStats();
  }

  TouchRaw p=readTouchRaw();
  updateStats(p);

  Serial.print(F("X=")); Serial.print(p.x);
  Serial.print(F(" Y=")); Serial.print(p.y);
  Serial.print(F(" Z1=")); Serial.print(p.z1);
  Serial.print(F(" Z2=")); Serial.print(p.z2);
  Serial.print(F(" P=")); Serial.print(p.pressure);
  Serial.print(F(" | X[")); Serial.print(minX); Serial.print(','); Serial.print(maxX);
  Serial.print(F("] Y[")); Serial.print(minY); Serial.print(','); Serial.print(maxY);
  Serial.print(F("] P[")); Serial.print(minP); Serial.print(','); Serial.print(maxP);
  Serial.println(']');

  delay(120);
}
