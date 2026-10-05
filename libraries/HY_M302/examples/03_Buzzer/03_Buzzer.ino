#include <HY_M302.h>
#include <stdlib.h>
#include <string.h>

HY_M302 shield;

static char cmd[20];
static uint8_t cmdLen = 0;

static void printHelp() {
  Serial.println(F(""));
  Serial.println(F("HY-M302 TEST-03 BUZZER MANUAL"));
  Serial.println(F("One command = one stable state."));
  Serial.println(F(""));
  Serial.println(F("  ON      active buzzer ON (constant HIGH on D5)"));
  Serial.println(F("  OFF     active buzzer OFF (LOW on D5)"));
  Serial.println(F("  F500    optional 500 Hz tone drive"));
  Serial.println(F("  F1000   optional 1000 Hz tone drive"));
  Serial.println(F("  F2000   optional 2000 Hz tone drive"));
  Serial.println(F("  F4000   optional 4000 Hz tone drive"));
  Serial.println(F("  ?       help"));
  Serial.println(F(""));
  Serial.println(F("Bench result on this sample: active/self-oscillating buzzer."));
  Serial.println(F("Constant HIGH gives the strongest sustained sound."));
  Serial.println(F(""));
}

static void stopBuzzer() {
  shield.buzzerOff();
  pinMode(shield.pins().buzzer, OUTPUT);
  digitalWrite(shield.pins().buzzer, LOW);
}

static void executeCommand(const char* s) {
  if (s[0] == 'F' && s[1] != '\0') {
    char* endPtr = nullptr;
    const long hz = strtol(s + 1, &endPtr, 10);

    if (*endPtr != '\0' || hz < 100 || hz > 5000) {
      Serial.println(F("ERR FREQ RANGE 100..5000"));
      return;
    }

    shield.buzzerTone((unsigned int)hz);
    Serial.print(F("BUZZER TONE "));
    Serial.print(hz);
    Serial.println(F(" Hz"));
    return;
  }

  if (strcmp(s, "ON") == 0 || strcmp(s, "HIGH") == 0) {
    shield.buzzerOn();
    Serial.println(F("BUZZER ON / D5 HIGH"));
  } else if (strcmp(s, "OFF") == 0 || strcmp(s, "LOW") == 0) {
    stopBuzzer();
    Serial.println(F("BUZZER OFF / D5 LOW"));
  } else if (strcmp(s, "?") == 0 || strcmp(s, "HELP") == 0) {
    printHelp();
  } else {
    Serial.print(F("ERR UNKNOWN: "));
    Serial.println(s);
  }
}

void setup() {
  Serial.begin(115200);
  shield.begin();
  stopBuzzer();

  printHelp();
  Serial.println(F("READY"));
}

void loop() {
  while (Serial.available()) {
    const char c = (char)Serial.read();

    if (c == '\r' || c == '\n') {
      if (cmdLen != 0) {
        cmd[cmdLen] = '\0';
        executeCommand(cmd);
        cmdLen = 0;
      }
      continue;
    }

    if (cmdLen < sizeof(cmd) - 1) {
      if (c >= 'a' && c <= 'z') cmd[cmdLen++] = c - ('a' - 'A');
      else cmd[cmdLen++] = c;
    }
  }
}
