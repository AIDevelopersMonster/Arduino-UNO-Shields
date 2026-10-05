#include <HY_M302.h>
#include <string.h>

HY_M302 shield;

static char cmd[20];
static uint8_t cmdLen = 0;

static bool runMode = false;
static unsigned long lastSensorsMs = 0;
static unsigned long lastDhtMs = 0;
static uint32_t irFullFrames = 0;
static uint32_t irRepeatFrames = 0;
static uint32_t dhtOkCount = 0;
static uint32_t dhtErrCount = 0;

static void printHelp() {
  Serial.println(F(""));
  Serial.println(F("HY-M302 TEST-05 INTEGRATED BOARD"));
  Serial.println(F("All verified onboard services run through one HY_M302 instance."));
  Serial.println(F(""));
  Serial.println(F("  RUN       start integrated continuous test"));
  Serial.println(F("  STOP      stop continuous sensor/RGB updates"));
  Serial.println(F("  DHT       read DHT11 now"));
  Serial.println(F("  SNAP      print buttons + analog snapshot"));
  Serial.println(F("  BUZZ ON   active buzzer ON"));
  Serial.println(F("  BUZZ OFF  active buzzer OFF"));
  Serial.println(F("  STATS     integration + IR counters"));
  Serial.println(F("  ZERO      reset integration + IR counters"));
  Serial.println(F("  ?         help"));
  Serial.println(F(""));
  Serial.println(F("RUN behavior:"));
  Serial.println(F("  SW1 -> red discrete LED"));
  Serial.println(F("  SW2 -> blue discrete LED"));
  Serial.println(F("  POT -> RGB red brightness"));
  Serial.println(F("  POT/LIGHT snapshot every 1 s"));
  Serial.println(F("  DHT11 read every 3 s"));
  Serial.println(F("  async NEC IR remains active continuously"));
  Serial.println(F(""));
}

static void printSnapshot() {
  Serial.print(F("SNAP SW1="));
  Serial.print(shield.button1Pressed() ? 1 : 0);
  Serial.print(F(" SW2="));
  Serial.print(shield.button2Pressed() ? 1 : 0);
  Serial.print(F(" POT="));
  Serial.print(shield.readPotRaw());
  Serial.print(F(" LIGHT="));
  Serial.print(shield.readLightRaw());
  Serial.print(F(" LM35_RAW="));
  Serial.print(shield.readLm35Raw());
  Serial.print(F(" A3="));
  Serial.println(shield.readAnalog3Raw());
}

static void readDhtNow() {
  Serial.println(F("DHT BEGIN"));
  const HY_M302::DhtReading dht = shield.readDht11();

  if (!dht.ok) {
    ++dhtErrCount;
    Serial.println(F("DHT ERR"));
    return;
  }

  ++dhtOkCount;
  Serial.print(F("DHT OK T="));
  Serial.print(dht.temperatureC, 1);
  Serial.print(F(" C RH="));
  Serial.print(dht.humidity, 1);
  Serial.println(F(" %"));
}

static void printIrFrame(const HY_M302::IrNecFrame& frame) {
  if (frame.repeat) {
    ++irRepeatFrames;
    Serial.print(F("IR REPEAT CMD=0x"));
    Serial.println(frame.command, HEX);
    return;
  }

  ++irFullFrames;
  Serial.print(F("IR OK RAW=0x"));
  Serial.print(frame.raw, HEX);
  Serial.print(F(" ADDR=0x"));
  Serial.print(frame.address, HEX);
  Serial.print(F(" CMD=0x"));
  Serial.println(frame.command, HEX);
}

static void drainIrFrames() {
  HY_M302::IrNecFrame frame;
  while (shield.readIrNecAsync(frame)) {
    printIrFrame(frame);
  }
}

static void printStats() {
  Serial.print(F("STATS ir_full="));
  Serial.print(irFullFrames);
  Serial.print(F(" ir_repeat="));
  Serial.print(irRepeatFrames);
  Serial.print(F(" dropped_edges="));
  Serial.print(shield.irNecDroppedEdges());
  Serial.print(F(" dropped_frames="));
  Serial.print(shield.irNecDroppedFrames());
  Serial.print(F(" dht_ok="));
  Serial.print(dhtOkCount);
  Serial.print(F(" dht_err="));
  Serial.println(dhtErrCount);
}

static void resetStats() {
  shield.service();
  HY_M302::IrNecFrame stale;
  while (shield.readIrNecAsync(stale)) {
    // Flush queued frames so the next run starts clean.
  }

  shield.resetIrNecStats();
  irFullFrames = 0;
  irRepeatFrames = 0;
  dhtOkCount = 0;
  dhtErrCount = 0;

  Serial.println(F("COUNTERS RESET / IR QUEUE FLUSHED"));
}

static void executeCommand(const char* s) {
  if (strcmp(s, "RUN") == 0) {
    runMode = true;
    lastSensorsMs = millis();
    lastDhtMs = millis();
    Serial.println(F("INTEGRATED RUN ON"));
  } else if (strcmp(s, "STOP") == 0) {
    runMode = false;
    shield.rgbOff();
    shield.ledRed(false);
    shield.ledBlue(false);
    Serial.println(F("INTEGRATED RUN OFF"));
  } else if (strcmp(s, "DHT") == 0) {
    readDhtNow();
  } else if (strcmp(s, "SNAP") == 0) {
    printSnapshot();
  } else if (strcmp(s, "BUZZ ON") == 0) {
    shield.buzzerOn();
    Serial.println(F("BUZZER ON"));
  } else if (strcmp(s, "BUZZ OFF") == 0) {
    shield.buzzerOff();
    Serial.println(F("BUZZER OFF"));
  } else if (strcmp(s, "STATS") == 0) {
    printStats();
  } else if (strcmp(s, "ZERO") == 0) {
    resetStats();
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
  } else {
    Serial.println(F("IR ASYNC INIT OK"));
  }

  Serial.println(F("READY"));
}

void loop() {
  // One common cooperative service call for asynchronous drivers.
  shield.service();
  drainIrFrames();

  if (runMode) {
    const bool sw1 = shield.button1Pressed();
    const bool sw2 = shield.button2Pressed();

    shield.ledRed(sw1);
    shield.ledBlue(sw2);

    const int pot = shield.readPotRaw();
    const uint8_t red = uint8_t((uint32_t(pot) * 255UL) / 1023UL);
    shield.setRGB(red, 0, 0);

    const unsigned long now = millis();

    if (now - lastSensorsMs >= 1000UL) {
      lastSensorsMs = now;
      printSnapshot();
    }

    if (now - lastDhtMs >= 3000UL) {
      lastDhtMs = now;
      readDhtNow();

      // DHT11 is timing-critical and briefly disables interrupts.
      // Service IR immediately afterwards so any captured edges are decoded.
      shield.service();
      drainIrFrames();
    }
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
