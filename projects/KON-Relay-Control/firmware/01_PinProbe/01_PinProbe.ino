/*
 KON Relay & Control Lab — Stage 01 pin inventory sketch.
 DEFAULT: PASSIVE, NO GPIO DRIVING. Outputs not enabled.
 Danger: enable the stimulus ONLY after continuity mapping, confirmation
 of active levels, no load on any contact and supervised bench setup.
 No pin-level observations from this sketch prove contact health.
*/
#include <Arduino.h>
#define ENABLE_STIMULUS 0
const uint8_t candidatePins[4] = {4, 5, 6, 7}; // CANDIDATES ONLY
const uint8_t outputOffLevel = LOW; // PLACEHOLDER; DO NOT ENABLE UNTIL VERIFIED
bool armed = false;
char cmd[20];
uint8_t len = 0;
unsigned long armDeadline = 0;

void allToInput() {
  for (uint8_t i = 0; i < 4; ++i) {
    digitalWrite(candidatePins[i], LOW); // disable pull-up
    pinMode(candidatePins[i], INPUT);
  }
}
void disarm() {
#if ENABLE_STIMULUS
  for (uint8_t i = 0; i < 4; ++i) {
    digitalWrite(candidatePins[i], outputOffLevel);
  }
#endif
  armed = false;
}
void handle(const char *p) {
  if (!strcmp(p, "HELP")) {
    Serial.println(F("HELP PINS STATUS DISARM"));
#if ENABLE_STIMULUS
    Serial.println(F("ARM ON1..ON4 OFF1..OFF4 (temporary, MAX 10s arm)"));
#endif
    return;
  }
  if (!strcmp(p, "PINS")) {
    Serial.println(F("CANDIDATE D4 D5 D6 D7 (unverified)"));
    return;
  }
  if (!strcmp(p, "STATUS")) {
    Serial.println(F("PHYSICAL_PASS=NO"));
    Serial.println(armed ? F("ARMED") : F("PASSIVE"));
    return;
  }
  if (!strcmp(p, "DISARM")) {
    disarm(); Serial.println(F("DISARMED")); return;
  }
#if ENABLE_STIMULUS
  if (!strcmp(p, "ARM")) {
    armed = true;
    armDeadline = millis() + 10000UL;
    Serial.println(F("ARMED 10s"));
    return;
  }
  const bool turnOn = strlen(p)==3 && p[0]=='O' && p[1]=='N' && p[2]>='1' && p[2]<='4';
  const bool turnOff = strlen(p)==4 && p[0]=='O' && p[1]=='F' && p[2]=='F' && p[3]>='1' && p[3]<='4';
  if (armed && (turnOn || turnOff)) {
    uint8_t index=(uint8_t)((turnOn ? p[2] : p[3])-'1');
    digitalWrite(candidatePins[index],
       turnOn ? (outputOffLevel == HIGH ? LOW : HIGH) : outputOffLevel);
    Serial.println(F("GPIO COMMAND SENT -- NOT RELAY CONTACT CONFIRMATION"));
    return;
  }
#endif
  Serial.println(F("UNKNOWN OR LOCKED"));
}
void setup() {
  allToInput();
  Serial.begin(115200);
  Serial.println(F("KON RELAY STAGE01 PASSIVE PROBE / NO BENCH PASS"));
  Serial.println(F("TYPE HELP"));
#if ENABLE_STIMULUS
  // NOT SAFE to change this switch without confirming pins and polarity:
  // even initial outputOffLevel must be established from real hardware.
  for (uint8_t i=0; i<4; ++i) {
    digitalWrite(candidatePins[i], outputOffLevel);
    pinMode(candidatePins[i], OUTPUT);
  }
#endif
}
void loop() {
#if ENABLE_STIMULUS
  if (armed && (int32_t)(millis() - armDeadline) >= 0) {
    disarm();
    Serial.println(F("ARM EXPIRED"));
  }
#endif
  while (Serial.available()) {
    char c=(char)Serial.read();
    if (c=='\r' || c=='\n') {
      if (len) { cmd[len]=0; handle(cmd); len=0; }
    } else if (c>=32 && c<127 && len < sizeof(cmd)-1) {
      cmd[len++] = c;
    } else if (len >= sizeof(cmd)-1) {
      len=0; Serial.println(F("COMMAND TOO LONG"));
    }
  }
}
