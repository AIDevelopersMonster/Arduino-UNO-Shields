#include <HY_M302.h>
#include <HY_M302_Remote.h>
#include <HY_M302_Remote_iDroid_OrangePi.h>
#include <string.h>

using namespace HY_M302_Remote;
using HY_M302_Remote::IDroidOrangePi::decode;

HY_M302 shield;

enum ActiveTest : uint8_t {
  ACTIVE_NONE = 0,
  ACTIVE_1_BUTTONS,
  ACTIVE_2_RGB,
  ACTIVE_3_POT,
  ACTIVE_4_LDR,
  ACTIVE_5_DHT11,
  ACTIVE_6_BUZZER,
  ACTIVE_7_IR,
  ACTIVE_8_INTEGRATED
};

static uint8_t selectedTest = 0;
static ActiveTest activeTest = ACTIVE_NONE;
static unsigned long stateStartedMs = 0;
static uint8_t statePhase = 0;
static uint8_t dhtReadCount = 0;
static int sampleMin = 1023;
static int sampleMax = 0;

static char serialCmd[20];
static uint8_t serialCmdLen = 0;

static void safeOff() {
  shield.rgbOff();
  shield.ledRed(false);
  shield.ledBlue(false);
  shield.buzzerOff();
  activeTest = ACTIVE_NONE;
}

static void printMainMenu() {
  Serial.println(F(""));
  Serial.println(F("HY-M302 REMOTE TEST MENU"));
  Serial.println(F(""));
  Serial.println(F("0  SUMMARY / ALL STATUS"));
  Serial.println(F("1  BUTTONS + DISCRETE LEDS"));
  Serial.println(F("2  RGB LED"));
  Serial.println(F("3  POTENTIOMETER"));
  Serial.println(F("4  LDR"));
  Serial.println(F("5  DHT11"));
  Serial.println(F("6  ACTIVE BUZZER"));
  Serial.println(F("7  IR DIAGNOSTICS"));
  Serial.println(F("8  INTEGRATED RUN"));
  Serial.println(F("9  STATS"));
  Serial.println(F(""));
  Serial.println(F("OK      start / confirm selected test"));
  Serial.println(F("RETURN  stop / cancel immediately"));
  Serial.println(F("HOME    stop + main menu"));
  Serial.println(F("MENU    help"));
  Serial.println(F("UP/DOWN or LEFT/RIGHT  change test"));
  Serial.println(F("POWER   immediate safe OFF"));
  Serial.println(F(""));
  Serial.println(F("SERIAL / CLI mirrors the remote:"));
  Serial.println(F("0..9, OK/RUN, RETURN/STOP/X, HOME, MENU/HELP/?"));
  Serial.println(F("UP, DOWN, LEFT, RIGHT, POWER/OFF, STATS, ZERO"));
  Serial.println(F(""));
}

static void printSelection(uint8_t n) {
  selectedTest = n;

  Serial.print(F("SELECTED TEST-0"));
  Serial.println(n);

  switch (n) {
    case 0:
      Serial.println(F("SUMMARY"));
      Serial.println(F("Shows current verified board status."));
      break;

    case 1:
      Serial.println(F("BUTTONS + DISCRETE LEDS"));
      Serial.println(F("Press SW1 and SW2 on the shield for up to 10 seconds."));
      Serial.println(F("Expected: red/blue discrete LEDs follow the buttons."));
      break;

    case 2:
      Serial.println(F("RGB LED"));
      Serial.println(F("Runs RED -> GREEN -> BLUE -> OFF."));
      Serial.println(F("Expected: exact channel colors."));
      break;

    case 3:
      Serial.println(F("POTENTIOMETER"));
      Serial.println(F("Rotate POT from minimum to maximum for up to 10 seconds."));
      Serial.println(F("Expected ADC range: approximately 0..1023."));
      break;

    case 4:
      Serial.println(F("LDR"));
      Serial.println(F("Cover and uncover the light sensor for up to 10 seconds."));
      Serial.println(F("Expected: darker -> lower ADC, brighter -> higher ADC."));
      break;

    case 5:
      Serial.println(F("DHT11"));
      Serial.println(F("Performs three temperature/humidity reads about 2 s apart."));
      Serial.println(F("No physical action required."));
      break;

    case 6:
      Serial.println(F("ACTIVE BUZZER"));
      Serial.println(F("Turns buzzer ON for 1 second, then OFF."));
      Serial.println(F("Expected: strong sustained tone while ON."));
      break;

    case 7:
      Serial.println(F("IR DIAGNOSTICS"));
      Serial.println(F("Press remote buttons for up to 10 seconds."));
      Serial.println(F("Decoded NEC frames are printed."));
      break;

    case 8:
      Serial.println(F("INTEGRATED RUN"));
      Serial.println(F("Buttons, LEDs, POT/RGB and async IR operate together."));
      Serial.println(F("Runs until RETURN, HOME, POWER or another selection."));
      break;

    case 9:
      Serial.println(F("STATS"));
      Serial.println(F("Shows IR dropped-edge/frame counters."));
      break;
  }

  Serial.println(F("Press OK to start/confirm, RETURN to cancel."));
}

static void printSummary() {
  Serial.println(F("SUMMARY"));
  Serial.println(F("SW1/SW2 PASS"));
  Serial.println(F("RGB D9/D10/D11 PASS"));
  Serial.println(F("LED D12/D13 PASS"));
  Serial.println(F("POT A0 PASS"));
  Serial.println(F("LDR A1 PASS"));
  Serial.println(F("DHT11 D4 PASS"));
  Serial.println(F("BUZZER D5 ACTIVE PASS"));
  Serial.println(F("IR D6 ASYNC NEC PASS"));
  Serial.println(F("REMOTE TEST MENU PHYSICAL PASS"));
  Serial.println(F("LM35 A2 FAIL ON THIS SAMPLE"));
}

static void printIrStats() {
  Serial.print(F("IR STATS dropped_edges="));
  Serial.print(shield.irNecDroppedEdges());
  Serial.print(F(" dropped_frames="));
  Serial.println(shield.irNecDroppedFrames());
}

static void completeActiveTest(uint8_t number) {
  safeOff();
  Serial.print(F("TEST-0"));
  Serial.print(number);
  Serial.println(F(" COMPLETE"));
}

static void startSelectedTest() {
  if (activeTest != ACTIVE_NONE) {
    Serial.println(F("TEST ALREADY RUNNING - RETURN TO STOP"));
    return;
  }

  const unsigned long now = millis();

  switch (selectedTest) {
    case 0:
      printSummary();
      break;

    case 1:
      Serial.println(F("TEST-01 RUNNING"));
      Serial.println(F("Press SW1/SW2. RETURN stops immediately."));
      stateStartedMs = now;
      activeTest = ACTIVE_1_BUTTONS;
      break;

    case 2:
      Serial.println(F("TEST-02 RGB"));
      Serial.println(F("RED"));
      shield.setRGB(255, 0, 0);
      stateStartedMs = now;
      statePhase = 0;
      activeTest = ACTIVE_2_RGB;
      break;

    case 3:
      Serial.println(F("TEST-03 POT"));
      Serial.println(F("Rotate POT now. RETURN stops immediately."));
      sampleMin = 1023;
      sampleMax = 0;
      stateStartedMs = now;
      activeTest = ACTIVE_3_POT;
      break;

    case 4:
      Serial.println(F("TEST-04 LDR"));
      Serial.println(F("Cover/uncover sensor now. RETURN stops immediately."));
      sampleMin = 1023;
      sampleMax = 0;
      stateStartedMs = now;
      activeTest = ACTIVE_4_LDR;
      break;

    case 5:
      Serial.println(F("TEST-05 DHT11"));
      dhtReadCount = 0;
      statePhase = 0;
      stateStartedMs = now;
      activeTest = ACTIVE_5_DHT11;
      break;

    case 6:
      Serial.println(F("TEST-06 BUZZER"));
      shield.buzzerOn();
      stateStartedMs = now;
      activeTest = ACTIVE_6_BUZZER;
      break;

    case 7:
      Serial.println(F("TEST-07 IR DIAGNOSTICS"));
      Serial.println(F("Press remote buttons. RETURN stops immediately."));
      stateStartedMs = now;
      activeTest = ACTIVE_7_IR;
      break;

    case 8:
      Serial.println(F("TEST-08 INTEGRATED RUN"));
      Serial.println(F("RUNNING until RETURN / HOME / POWER."));
      activeTest = ACTIVE_8_INTEGRATED;
      break;

    case 9:
      printIrStats();
      break;
  }
}

static void serviceActiveTest() {
  const unsigned long now = millis();

  switch (activeTest) {
    case ACTIVE_NONE:
      break;

    case ACTIVE_1_BUTTONS:
      shield.ledRed(shield.button1Pressed());
      shield.ledBlue(shield.button2Pressed());

      if (now - stateStartedMs >= 10000UL) {
        completeActiveTest(1);
      }
      break;

    case ACTIVE_2_RGB:
      if (now - stateStartedMs < 800UL) break;

      stateStartedMs = now;
      ++statePhase;

      if (statePhase == 1) {
        Serial.println(F("GREEN"));
        shield.setRGB(0, 255, 0);
      } else if (statePhase == 2) {
        Serial.println(F("BLUE"));
        shield.setRGB(0, 0, 255);
      } else {
        completeActiveTest(2);
      }
      break;

    case ACTIVE_3_POT: {
      const int v = shield.readPotRaw();
      if (v < sampleMin) sampleMin = v;
      if (v > sampleMax) sampleMax = v;

      if (now - stateStartedMs >= 10000UL) {
        Serial.print(F("POT MIN="));
        Serial.print(sampleMin);
        Serial.print(F(" MAX="));
        Serial.println(sampleMax);
        completeActiveTest(3);
      }
      break;
    }

    case ACTIVE_4_LDR: {
      const int v = shield.readLightRaw();
      if (v < sampleMin) sampleMin = v;
      if (v > sampleMax) sampleMax = v;

      if (now - stateStartedMs >= 10000UL) {
        Serial.print(F("LDR MIN="));
        Serial.print(sampleMin);
        Serial.print(F(" MAX="));
        Serial.println(sampleMax);
        completeActiveTest(4);
      }
      break;
    }

    case ACTIVE_5_DHT11:
      if (statePhase == 0 || now - stateStartedMs >= 2000UL) {
        const HY_M302::DhtReading d = shield.readDht11();
        ++dhtReadCount;

        Serial.print(F("READ "));
        Serial.print(dhtReadCount);

        if (!d.ok) {
          Serial.println(F(": ERR"));
        } else {
          Serial.print(F(": T="));
          Serial.print(d.temperatureC, 1);
          Serial.print(F(" C RH="));
          Serial.print(d.humidity, 1);
          Serial.println(F(" %"));
        }

        if (dhtReadCount >= 3) {
          completeActiveTest(5);
        } else {
          statePhase = 1;
          stateStartedMs = millis();
        }
      }
      break;

    case ACTIVE_6_BUZZER:
      if (now - stateStartedMs >= 1000UL) {
        completeActiveTest(6);
      }
      break;

    case ACTIVE_7_IR:
      if (now - stateStartedMs >= 10000UL) {
        completeActiveTest(7);
      }
      break;

    case ACTIVE_8_INTEGRATED: {
      const bool sw1 = shield.button1Pressed();
      const bool sw2 = shield.button2Pressed();

      shield.ledRed(sw1);
      shield.ledBlue(sw2);

      const int pot = shield.readPotRaw();
      const uint8_t red = uint8_t((uint32_t(pot) * 255UL) / 1023UL);
      shield.setRGB(red, 0, 0);
      break;
    }
  }
}


static void stopForSelection() {
  if (activeTest != ACTIVE_NONE) {
    safeOff();
    Serial.println(F("ACTIVE TEST STOPPED"));
  }
}

static void handleKey(Key key) {
  const int8_t digit = HY_M302_Remote::digit(key);

  if (digit >= 0) {
    stopForSelection();
    printSelection(uint8_t(digit));
    return;
  }

  switch (key) {
    case KEY_OK:
      startSelectedTest();
      break;

    case KEY_HOME:
      safeOff();
      Serial.println(F("HOME / SAFE OFF"));
      printMainMenu();
      printSelection(selectedTest);
      break;

    case KEY_RETURN:
      if (activeTest != ACTIVE_NONE) {
        safeOff();
        Serial.println(F("TEST CANCELLED"));
      } else {
        Serial.println(F("CANCEL / RETURN"));
      }
      printSelection(selectedTest);
      break;

    case KEY_MENU:
      printMainMenu();
      break;

    case KEY_UP:
    case KEY_LEFT:
      stopForSelection();
      selectedTest = (selectedTest == 0) ? 9 : selectedTest - 1;
      printSelection(selectedTest);
      break;

    case KEY_DOWN:
    case KEY_RIGHT:
      stopForSelection();
      selectedTest = (selectedTest >= 9) ? 0 : selectedTest + 1;
      printSelection(selectedTest);
      break;

    case KEY_POWER:
      safeOff();
      Serial.println(F("SAFE OFF"));
      break;

    default:
      break;
  }
}


static void resetIrStatsAndQueue() {
  shield.service();

  HY_M302::IrNecFrame stale;
  while (shield.readIrNecAsync(stale)) {
    // Flush frames captured before the reset command.
  }

  shield.resetIrNecStats();
  Serial.println(F("IR COUNTERS RESET / QUEUE FLUSHED"));
}

static void executeSerialCommand(const char* s) {
  if (s[0] >= '0' && s[0] <= '9' && s[1] == '\0') {
    const uint8_t n = static_cast<uint8_t>(s[0] - '0');
    handleKey(static_cast<Key>(static_cast<uint8_t>(KEY_0) + n));
    return;
  }

  if (strcmp(s, "OK") == 0 || strcmp(s, "RUN") == 0) {
    handleKey(KEY_OK);
  } else if (strcmp(s, "RETURN") == 0 ||
             strcmp(s, "STOP") == 0 ||
             strcmp(s, "X") == 0) {
    handleKey(KEY_RETURN);
  } else if (strcmp(s, "HOME") == 0) {
    handleKey(KEY_HOME);
  } else if (strcmp(s, "MENU") == 0 ||
             strcmp(s, "HELP") == 0 ||
             strcmp(s, "?") == 0) {
    handleKey(KEY_MENU);
  } else if (strcmp(s, "UP") == 0) {
    handleKey(KEY_UP);
  } else if (strcmp(s, "DOWN") == 0) {
    handleKey(KEY_DOWN);
  } else if (strcmp(s, "LEFT") == 0) {
    handleKey(KEY_LEFT);
  } else if (strcmp(s, "RIGHT") == 0) {
    handleKey(KEY_RIGHT);
  } else if (strcmp(s, "POWER") == 0 ||
             strcmp(s, "OFF") == 0) {
    handleKey(KEY_POWER);
  } else if (strcmp(s, "STATS") == 0 ||
             strcmp(s, "S") == 0) {
    printIrStats();
  } else if (strcmp(s, "ZERO") == 0 ||
             strcmp(s, "Z") == 0) {
    resetIrStatsAndQueue();
  } else {
    Serial.print(F("ERR UNKNOWN SERIAL COMMAND: "));
    Serial.println(s);
  }
}

static void serviceSerialCommands() {
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());

    if (c == '\r' || c == '\n') {
      if (serialCmdLen != 0) {
        serialCmd[serialCmdLen] = '\0';
        executeSerialCommand(serialCmd);
        serialCmdLen = 0;
      }
      continue;
    }

    if (serialCmdLen >= sizeof(serialCmd) - 1) {
      continue;
    }

    if (c >= 'a' && c <= 'z') {
      serialCmd[serialCmdLen++] = c - ('a' - 'A');
    } else {
      serialCmd[serialCmdLen++] = c;
    }
  }
}

static void printIrFrame(const HY_M302::IrNecFrame& frame) {
  Serial.print(F("IR RAW=0x"));
  Serial.print(frame.raw, HEX);
  Serial.print(F(" ADDR=0x"));
  Serial.print(frame.address, HEX);
  Serial.print(F(" CMD=0x"));
  Serial.println(frame.command, HEX);
}

void setup() {
  Serial.begin(115200);
  shield.begin();

  if (!shield.beginIrNecAsync()) {
    Serial.println(F("IR ASYNC INIT FAIL"));
    return;
  }

  printMainMenu();
  printSelection(0);
  Serial.println(F("READY"));
}

void loop() {
  shield.service();
  serviceSerialCommands();
  serviceActiveTest();

  HY_M302::IrNecFrame frame;

  while (shield.readIrNecAsync(frame)) {
    // Repeat frames must not trigger menu actions twice.
    if (frame.repeat) continue;

    if (activeTest == ACTIVE_7_IR) {
      printIrFrame(frame);
    }

    const Key key = decode(frame.address, frame.command);

    if (key == KEY_NONE) {
      if (activeTest != ACTIVE_7_IR) {
        Serial.print(F("UNKNOWN IR ADDR=0x"));
        Serial.print(frame.address, HEX);
        Serial.print(F(" CMD=0x"));
        Serial.println(frame.command, HEX);
      }
      continue;
    }

    handleKey(key);
  }
}
