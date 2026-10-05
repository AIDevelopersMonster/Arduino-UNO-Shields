#include <HY_M302.h>
#include <string.h>

HY_M302 shield;

static char cmd[12];
static uint8_t cmdLen = 0;

static void printHelp() {
  Serial.println(F(""));
  Serial.println(F("HY-M302 TEST-04 IR RECEIVER MANUAL"));
  Serial.println(F("IR receiver = D6"));
  Serial.println(F(""));
  Serial.println(F("  R   wait up to 10 s for one NEC frame"));
  Serial.println(F("  L   print current D6 logic level"));
  Serial.println(F("  ?   help"));
  Serial.println(F(""));
}

static void printFrame(const HY_M302::IrNecFrame& frame) {
  if (frame.repeat) {
    Serial.println(F("IR NEC REPEAT"));
    return;
  }

  Serial.print(F("IR NEC OK RAW=0x"));
  Serial.print(frame.raw, HEX);
  Serial.print(F(" ADDR=0x"));
  Serial.print(frame.address, HEX);
  Serial.print(F(" CMD=0x"));
  Serial.println(frame.command, HEX);
}

static void waitOneFrame() {
  Serial.println(F("IR ARMED 10 s - press one remote button"));

  const unsigned long started = millis();

  while (millis() - started < 10000UL) {
    HY_M302::IrNecFrame frame;

    // Short per-attempt timeout keeps the wait responsive while allowing
    // a complete NEC frame to be captured when it starts.
    if (shield.readIrNec(frame, 5000UL)) {
      printFrame(frame);
      return;
    }
  }

  Serial.println(F("IR TIMEOUT"));
}

static void executeCommand(const char* s) {
  if (strcmp(s, "R") == 0) {
    waitOneFrame();
  } else if (strcmp(s, "L") == 0) {
    Serial.print(F("D6 LEVEL="));
    Serial.println(digitalRead(shield.irPin()));
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
