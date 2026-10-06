#include <HY_M302.h>
#include <string.h>

HY_M302 shield;

static char cmd[16];
static uint8_t cmdLen = 0;

static void printIdentity() {
  Serial.println(F("HY_M302_REMOTE_MAPPER 0.1"));
  Serial.println(F("PROTO NEC"));
  Serial.println(F("PIN D6"));
  Serial.println(F("READY"));
}

static void printFrame(const HY_M302::IrNecFrame& frame) {
  if (frame.repeat) {
    Serial.print(F("REPEAT ADDR=0x"));
    Serial.print(frame.address, HEX);
    Serial.print(F(" CMD=0x"));
    Serial.println(frame.command, HEX);
    return;
  }

  Serial.print(F("FRAME RAW=0x"));
  Serial.print(frame.raw, HEX);
  Serial.print(F(" ADDR=0x"));
  Serial.print(frame.address, HEX);
  Serial.print(F(" CMD=0x"));
  Serial.println(frame.command, HEX);
}

static void executeCommand(const char* s) {
  if (strcmp(s, "ID") == 0) {
    printIdentity();
  } else if (strcmp(s, "STATS") == 0) {
    Serial.print(F("STATS dropped_edges="));
    Serial.print(shield.irNecDroppedEdges());
    Serial.print(F(" dropped_frames="));
    Serial.println(shield.irNecDroppedFrames());
  } else if (strcmp(s, "ZERO") == 0) {
    shield.service();
    HY_M302::IrNecFrame stale;
    while (shield.readIrNecAsync(stale)) {
      // Flush queued frames.
    }
    shield.resetIrNecStats();
    Serial.println(F("OK ZERO"));
  } else {
    Serial.print(F("ERR UNKNOWN "));
    Serial.println(s);
  }
}

void setup() {
  Serial.begin(115200);
  shield.begin();

  if (!shield.beginIrNecAsync()) {
    Serial.println(F("HY_M302_REMOTE_MAPPER 0.1"));
    Serial.println(F("ERROR IR_INIT"));
    return;
  }

  printIdentity();
}

void loop() {
  shield.service();

  HY_M302::IrNecFrame frame;
  while (shield.readIrNecAsync(frame)) {
    printFrame(frame);
  }

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
