/**
 * APP-01 — 4-Relay Serial Controller
 *
 * Arduino UNO, USB/COM control only.
 * Relay inputs:
 *   R1 -> D4
 *   R2 -> D5
 *   R3 -> D6
 *   R4 -> D7
 *
 * Bench polarity: ACTIVE HIGH.
 * Serial: 115200 8N1.
 *
 * Commands:
 *   HELP
 *   PING
 *   STATUS
 *   R1 ON | R1 OFF | R1 TOGGLE
 *   ...
 *   R4 ON | R4 OFF | R4 TOGGLE
 *   ALL ON | ALL OFF
 */

#include <Arduino.h>
#include <string.h>
#include <ctype.h>

const uint8_t RELAY_PINS[4] = {4, 5, 6, 7};
const bool ACTIVE_HIGH = true;

bool relayState[4] = {false, false, false, false};

char lineBuf[32];
uint8_t lineLen = 0;

uint8_t outputLevel(bool on) {
  return ACTIVE_HIGH ? (on ? HIGH : LOW) : (on ? LOW : HIGH);
}

void setRelay(uint8_t index, bool on) {
  if (index >= 4) return;
  relayState[index] = on;
  digitalWrite(RELAY_PINS[index], outputLevel(on));
}

void setAll(bool on) {
  for (uint8_t i = 0; i < 4; ++i) setRelay(i, on);
}

void toggleRelay(uint8_t index) {
  if (index < 4) setRelay(index, !relayState[index]);
}

void printState() {
  Serial.print(F("STATE "));
  for (uint8_t i = 0; i < 4; ++i) {
    Serial.print(F("R"));
    Serial.print(i + 1);
    Serial.print('=');
    Serial.print(relayState[i] ? F("ON") : F("OFF"));
    if (i != 3) Serial.print(' ');
  }
  Serial.println();
}

void printHelp() {
  Serial.println(F("HELP: PING | STATUS | R1..R4 ON|OFF|TOGGLE | ALL ON|OFF"));
}

void toUpper(char *s) {
  while (*s) {
    *s = (char)toupper((unsigned char)*s);
    ++s;
  }
}

void execute(char *line) {
  toUpper(line);

  if (!strcmp(line, "PING")) {
    Serial.println(F("PONG"));
    return;
  }

  if (!strcmp(line, "HELP") || !strcmp(line, "?")) {
    printHelp();
    return;
  }

  if (!strcmp(line, "STATUS")) {
    printState();
    return;
  }

  if (!strcmp(line, "ALL ON")) {
    setAll(true);
    Serial.println(F("OK ALL ON"));
    printState();
    return;
  }

  if (!strcmp(line, "ALL OFF")) {
    setAll(false);
    Serial.println(F("OK ALL OFF"));
    printState();
    return;
  }

  if (line[0] == 'R' &&
      line[1] >= '1' && line[1] <= '4' &&
      line[2] == ' ') {

    uint8_t index = (uint8_t)(line[1] - '1');
    const char *action = line + 3;

    if (!strcmp(action, "ON")) setRelay(index, true);
    else if (!strcmp(action, "OFF")) setRelay(index, false);
    else if (!strcmp(action, "TOGGLE")) toggleRelay(index);
    else {
      Serial.println(F("ERR ACTION"));
      return;
    }

    Serial.print(F("OK R"));
    Serial.print(index + 1);
    Serial.print(' ');
    Serial.println(relayState[index] ? F("ON") : F("OFF"));
    printState();
    return;
  }

  Serial.println(F("ERR COMMAND"));
  printHelp();
}

void setup() {
  Serial.begin(115200);

  // Safe startup: force logical OFF before switching pins to OUTPUT.
  for (uint8_t i = 0; i < 4; ++i) {
    digitalWrite(RELAY_PINS[i], outputLevel(false));
    pinMode(RELAY_PINS[i], OUTPUT);
    setRelay(i, false);
  }

  delay(150);
  Serial.println(F("KON APP-01 / 4-RELAY SERIAL CONTROLLER"));
  Serial.println(F("LOGIC ACTIVE HIGH"));
  printState();
  printHelp();
  Serial.println(F("READY"));
}

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\r' || c == '\n') {
      if (lineLen) {
        lineBuf[lineLen] = 0;
        execute(lineBuf);
        lineLen = 0;
      }
      continue;
    }

    if (c >= 32 && c <= 126 && lineLen < sizeof(lineBuf) - 1) {
      lineBuf[lineLen++] = c;
    }
  }
}
