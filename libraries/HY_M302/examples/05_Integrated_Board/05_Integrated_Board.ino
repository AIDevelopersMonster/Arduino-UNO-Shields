#include <HY_M302.h>
#include <string.h>

HY_M302 shield;

static char cmd[20];
static uint8_t cmdLen = 0;

static bool runMode = false;
static bool liveOutput = false;

static bool statsTriggerActive = false;
static bool statsTriggerFired = false;
static unsigned long statsTriggerStartedMs = 0;

static unsigned long lastSensorsMs = 0;
static unsigned long lastDhtMs = 0;

static uint32_t irFullFrames = 0;
static uint32_t irRepeatFrames = 0;
static uint32_t dhtOkCount = 0;
static uint32_t dhtErrCount = 0;

static int lastPot = 0;
static int lastLight = 0;

static void printStats() {
  Serial.print(F("STATS run="));
  Serial.print(runMode ? 1 : 0);
  Serial.print(F(" ir_full="));
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
  Serial.print(dhtErrCount);
  Serial.print(F(" pot="));
  Serial.print(lastPot);
  Serial.print(F(" light="));
  Serial.println(lastLight);
}

static void printHelp() {
  Serial.println(F(""));
  Serial.println(F("HY-M302 TEST-05 INTEGRATED BOARD"));
  Serial.println(F("Quiet integrated test by default; hardware STATS trigger = SW1+SW2."));
  Serial.println(F(""));
  Serial.println(F("  RUN       start integrated test"));
  Serial.println(F("  SW1+SW2   hold both ~150 ms -> print STATS, RUN continues"));
  Serial.println(F("  X / STOP  stop RUN and print STATS"));
  Serial.println(F("  S / STATS print STATS"));
  Serial.println(F("  LIVE ON   enable continuous telemetry"));
  Serial.println(F("  LIVE OFF  disable continuous telemetry"));
  Serial.println(F("  DHT       read DHT11 now"));
  Serial.println(F("  SNAP      print buttons + analog snapshot"));
  Serial.println(F("  BUZZ ON   active buzzer ON"));
  Serial.println(F("  BUZZ OFF  active buzzer OFF"));
  Serial.println(F("  ZERO      reset counters + flush IR queue"));
  Serial.println(F("  ?         help"));
  Serial.println(F(""));
}

static void sampleAnalog() {
  lastPot = shield.readPotRaw();
  lastLight = shield.readLightRaw();
}

static void printSnapshot() {
  sampleAnalog();

  Serial.print(F("SNAP SW1="));
  Serial.print(shield.button1Pressed() ? 1 : 0);
  Serial.print(F(" SW2="));
  Serial.print(shield.button2Pressed() ? 1 : 0);
  Serial.print(F(" POT="));
  Serial.print(lastPot);
  Serial.print(F(" LIGHT="));
  Serial.print(lastLight);
  Serial.print(F(" LM35_RAW="));
  Serial.print(shield.readLm35Raw());
  Serial.print(F(" A3="));
  Serial.println(shield.readAnalog3Raw());
}

static void readDhtNow(bool verbose) {
  if (verbose) Serial.println(F("DHT BEGIN"));

  const HY_M302::DhtReading dht = shield.readDht11();

  if (!dht.ok) {
    ++dhtErrCount;
    if (verbose) Serial.println(F("DHT ERR"));
    return;
  }

  ++dhtOkCount;

  if (verbose) {
    Serial.print(F("DHT OK T="));
    Serial.print(dht.temperatureC, 1);
    Serial.print(F(" C RH="));
    Serial.print(dht.humidity, 1);
    Serial.println(F(" %"));
  }
}

static void drainIrFrames() {
  HY_M302::IrNecFrame frame;

  while (shield.readIrNecAsync(frame)) {
    if (frame.repeat) {
      ++irRepeatFrames;

      if (liveOutput) {
        Serial.print(F("IR REPEAT CMD=0x"));
        Serial.println(frame.command, HEX);
      }
      continue;
    }

    ++irFullFrames;

    if (liveOutput) {
      Serial.print(F("IR OK RAW=0x"));
      Serial.print(frame.raw, HEX);
      Serial.print(F(" ADDR=0x"));
      Serial.print(frame.address, HEX);
      Serial.print(F(" CMD=0x"));
      Serial.println(frame.command, HEX);
    }
  }
}

static void resetStats() {
  shield.service();
  HY_M302::IrNecFrame stale;

  while (shield.readIrNecAsync(stale)) {
    // Flush queued frames.
  }

  shield.resetIrNecStats();
  irFullFrames = 0;
  irRepeatFrames = 0;
  dhtOkCount = 0;
  dhtErrCount = 0;

  sampleAnalog();

  Serial.println(F("COUNTERS RESET / IR QUEUE FLUSHED"));
}

static void serviceStatsTrigger() {
  const bool bothPressed =
      shield.button1Pressed() && shield.button2Pressed();

  if (!bothPressed) {
    statsTriggerActive = false;
    statsTriggerFired = false;
    return;
  }

  if (!statsTriggerActive) {
    statsTriggerActive = true;
    statsTriggerStartedMs = millis();
    return;
  }

  if (!statsTriggerFired &&
      millis() - statsTriggerStartedMs >= 150UL) {
    statsTriggerFired = true;
    Serial.println(F("TRIGGER SW1+SW2"));
    printStats();
  }
}

static void executeCommand(const char* s) {
  if (strcmp(s, "RUN") == 0) {
    runMode = true;
    lastSensorsMs = millis();
    lastDhtMs = millis();
    sampleAnalog();
    Serial.println(F("INTEGRATED RUN ON / QUIET"));
  } else if (strcmp(s, "STOP") == 0 || strcmp(s, "X") == 0) {
    runMode = false;
    shield.rgbOff();
    shield.ledRed(false);
    shield.ledBlue(false);
    Serial.println(F("INTEGRATED RUN OFF"));
    printStats();
  } else if (strcmp(s, "LIVE ON") == 0) {
    liveOutput = true;
    Serial.println(F("LIVE OUTPUT ON"));
  } else if (strcmp(s, "LIVE OFF") == 0) {
    liveOutput = false;
    Serial.println(F("LIVE OUTPUT OFF"));
  } else if (strcmp(s, "DHT") == 0) {
    readDhtNow(true);
  } else if (strcmp(s, "SNAP") == 0) {
    printSnapshot();
  } else if (strcmp(s, "BUZZ ON") == 0) {
    shield.buzzerOn();
    Serial.println(F("BUZZER ON"));
  } else if (strcmp(s, "BUZZ OFF") == 0) {
    shield.buzzerOff();
    Serial.println(F("BUZZER OFF"));
  } else if (strcmp(s, "STATS") == 0 || strcmp(s, "S") == 0) {
    printStats();
  } else if (strcmp(s, "ZERO") == 0 || strcmp(s, "Z") == 0) {
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

  sampleAnalog();
  Serial.println(F("READY"));
}

void loop() {
  shield.service();
  drainIrFrames();
  serviceStatsTrigger();

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
      sampleAnalog();

      if (liveOutput) {
        printSnapshot();
      }
    }

    if (now - lastDhtMs >= 3000UL) {
      lastDhtMs = now;
      readDhtNow(liveOutput);

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
      // Empty CR/LF is intentionally ignored. This also makes CR+LF from
      // arduino-cli monitor a single logical line terminator.
      continue;
    }

    if (cmdLen < sizeof(cmd) - 1) {
      if (c >= 'a' && c <= 'z') cmd[cmdLen++] = c - ('a' - 'A');
      else cmd[cmdLen++] = c;
    }
  }
}
