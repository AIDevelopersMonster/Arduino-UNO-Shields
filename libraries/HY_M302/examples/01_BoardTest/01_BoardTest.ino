#include <HY_M302.h>
#include <string.h>

HY_M302 shield;

static char cmd[16];
static uint8_t cmdLen = 0;

static void setRgbPins(uint8_t d9, uint8_t d10, uint8_t d11) {
  const HY_M302::PinMap& p = shield.pins();
  digitalWrite(p.rgb1, d9);
  digitalWrite(p.rgb2, d10);
  digitalWrite(p.rgb3, d11);
}

static void printHelp() {
  Serial.println(F(""));
  Serial.println(F("HY-M302 TEST-01 MANUAL"));
  Serial.println(F("One command = one stable state."));
  Serial.println(F(""));
  Serial.println(F("RGB / polarity test:"));
  Serial.println(F("  9H   D9 HIGH,  D10/D11 LOW"));
  Serial.println(F("  10H  D10 HIGH, D9/D11 LOW"));
  Serial.println(F("  11H  D11 HIGH, D9/D10 LOW"));
  Serial.println(F("  9L   D9 LOW,   D10/D11 HIGH"));
  Serial.println(F("  10L  D10 LOW,  D9/D11 HIGH"));
  Serial.println(F("  11L  D11 LOW,  D9/D10 HIGH"));
  Serial.println(F(""));
  Serial.println(F("Discrete LEDs:"));
  Serial.println(F("  12H / 12L"));
  Serial.println(F("  13H / 13L"));
  Serial.println(F(""));
  Serial.println(F("Other:"));
  Serial.println(F("  LOWALL   D9..D13 LOW"));
  Serial.println(F("  HIGHALL  D9..D13 HIGH"));
  Serial.println(F("  S        print sensors once"));
  Serial.println(F("  B        print buttons once"));
  Serial.println(F("  ?        help"));
  Serial.println(F(""));
}

static void printSensors() {
  Serial.print(F("POT="));
  Serial.print(shield.readPotRaw());
  Serial.print(F(" LIGHT="));
  Serial.print(shield.readLightRaw());
  Serial.print(F(" LM35_RAW="));
  Serial.print(shield.readLm35Raw());
  Serial.print(F(" A3="));
  Serial.println(shield.readAnalog3Raw());
}

static void printButtons() {
  Serial.print(F("SW1="));
  Serial.print(shield.button1Pressed());
  Serial.print(F(" SW2="));
  Serial.println(shield.button2Pressed());
}

static void executeCommand(const char* s) {
  const HY_M302::PinMap& p = shield.pins();

  if (strcmp(s, "9H") == 0) {
    setRgbPins(HIGH, LOW, LOW);
    Serial.println(F("STATE: D9=HIGH D10=LOW D11=LOW"));
  } else if (strcmp(s, "10H") == 0) {
    setRgbPins(LOW, HIGH, LOW);
    Serial.println(F("STATE: D9=LOW D10=HIGH D11=LOW"));
  } else if (strcmp(s, "11H") == 0) {
    setRgbPins(LOW, LOW, HIGH);
    Serial.println(F("STATE: D9=LOW D10=LOW D11=HIGH"));
  } else if (strcmp(s, "9L") == 0) {
    setRgbPins(LOW, HIGH, HIGH);
    Serial.println(F("STATE: D9=LOW D10=HIGH D11=HIGH"));
  } else if (strcmp(s, "10L") == 0) {
    setRgbPins(HIGH, LOW, HIGH);
    Serial.println(F("STATE: D9=HIGH D10=LOW D11=HIGH"));
  } else if (strcmp(s, "11L") == 0) {
    setRgbPins(HIGH, HIGH, LOW);
    Serial.println(F("STATE: D9=HIGH D10=HIGH D11=LOW"));
  } else if (strcmp(s, "12H") == 0) {
    digitalWrite(p.led2, HIGH);
    Serial.println(F("STATE: D12=HIGH"));
  } else if (strcmp(s, "12L") == 0) {
    digitalWrite(p.led2, LOW);
    Serial.println(F("STATE: D12=LOW"));
  } else if (strcmp(s, "13H") == 0) {
    digitalWrite(p.led1, HIGH);
    Serial.println(F("STATE: D13=HIGH"));
  } else if (strcmp(s, "13L") == 0) {
    digitalWrite(p.led1, LOW);
    Serial.println(F("STATE: D13=LOW"));
  } else if (strcmp(s, "LOWALL") == 0) {
    setRgbPins(LOW, LOW, LOW);
    digitalWrite(p.led2, LOW);
    digitalWrite(p.led1, LOW);
    Serial.println(F("STATE: D9..D13=LOW"));
  } else if (strcmp(s, "HIGHALL") == 0) {
    setRgbPins(HIGH, HIGH, HIGH);
    digitalWrite(p.led2, HIGH);
    digitalWrite(p.led1, HIGH);
    Serial.println(F("STATE: D9..D13=HIGH"));
  } else if (strcmp(s, "S") == 0) {
    printSensors();
  } else if (strcmp(s, "B") == 0) {
    printButtons();
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

  // Stable, known starting state.
  const HY_M302::PinMap& p = shield.pins();
  setRgbPins(LOW, LOW, LOW);
  digitalWrite(p.led2, LOW);
  digitalWrite(p.led1, LOW);

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
