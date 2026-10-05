#include <HY_M302.h>
#include <string.h>

HY_M302 shield;

static char cmd[12];
static uint8_t cmdLen = 0;
static bool liveMode = false;

static void printFrame(const HY_M302::IrNecFrame& frame) {
  if (frame.repeat) {
    Serial.print(F("IR NEC REPEAT"));
    if (frame.ok) {
      Serial.print(F(" LAST_RAW=0x"));
      Serial.print(frame.raw, HEX);
      Serial.print(F(" ADDR=0x"));
      Serial.print(frame.address, HEX);
      Serial.print(F(" CMD=0x"));
      Serial.print(frame.command, HEX);
    }
    Serial.println();
    return;
  }

  Serial.print(F("IR NEC OK RAW=0x"));
  Serial.print(frame.raw, HEX);
  Serial.print(F(" ADDR=0x"));
  Serial.print(frame.address, HEX);
  Serial.print(F(" CMD=0x"));
  Serial.println(frame.command, HEX);
}

static void printStats() {
  Serial.print(F("IR STATS dropped_edges="));
  Serial.print(shield.irNecDroppedEdges());
  Serial.print(F(" dropped_frames="));
  Serial.println(shield.irNecDroppedFrames());
}

static void printHelp() {
  Serial.println(F(""));
  Serial.println(F("HY-M302 TEST-04 IR NEC ASYNC"));
  Serial.println(F("D6 -> PCINT22 edge capture -> NEC state machine"));
  Serial.println(F(""));
  Serial.println(F("  LIVE ON   print every decoded full/repeat frame"));
  Serial.println(F("  LIVE OFF  stop live printing"));
  Serial.println(F("  ONE       wait for one full NEC frame without blocking IR capture"));
  Serial.println(F("  STATS     print dropped edge/frame counters"));
  Serial.println(F("  ZERO      reset counters"));
  Serial.println(F("  L         print current D6 logic level"));
  Serial.println(F("  ?         help"));
  Serial.println(F(""));
}

static void serviceAndPrintIfLive() {
  shield.serviceIrNec();

  if (!liveMode) return;

  HY_M302::IrNecFrame frame;
  while (shield.readIrNecAsync(frame)) {
    printFrame(frame);
  }
}

static void waitOneFullFrame() {
  Serial.println(F("IR ASYNC ARMED 10 s - press one remote button"));
  const unsigned long started = millis();

  while (millis() - started < 10000UL) {
    shield.serviceIrNec();

    HY_M302::IrNecFrame frame;
    while (shield.readIrNecAsync(frame)) {
      if (frame.repeat) continue;
      printFrame(frame);
      return;
    }

    // Deliberately do other cooperative work here. IR capture continues in ISR.
    // A real KonSol task scheduler can use this time for other drivers/tasks.
  }

  Serial.println(F("IR TIMEOUT"));
}

static void executeCommand(const char* s) {
  if (strcmp(s, "LIVE ON") == 0 || strcmp(s, "ON") == 0) {
    liveMode = true;
    Serial.println(F("IR LIVE ON"));
  } else if (strcmp(s, "LIVE OFF") == 0 || strcmp(s, "OFF") == 0) {
    liveMode = false;
    Serial.println(F("IR LIVE OFF"));
  } else if (strcmp(s, "ONE") == 0 || strcmp(s, "R") == 0) {
    waitOneFullFrame();
  } else if (strcmp(s, "STATS") == 0 || strcmp(s, "S") == 0) {
    printStats();
  } else if (strcmp(s, "ZERO") == 0 || strcmp(s, "Z") == 0) {
    shield.resetIrNecStats();
    Serial.println(F("IR STATS RESET"));
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

  if (!shield.beginIrNecAsync()) {
    Serial.println(F("IR ASYNC INIT FAIL"));
    Serial.println(F("This optimized receiver requires UNO-class AVR and IR on D6."));
    return;
  }

  Serial.println(F("IR ASYNC INIT OK"));
  Serial.println(F("READY"));
}

void loop() {
  serviceAndPrintIfLive();

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
