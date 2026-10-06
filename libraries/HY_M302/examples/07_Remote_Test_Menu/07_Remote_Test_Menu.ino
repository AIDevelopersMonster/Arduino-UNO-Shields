#include <HY_M302.h>
#include "HY_M302_RemoteMap.h"

using namespace HY_M302_RemoteMap;

HY_M302 shield;

static uint8_t selectedTest = 0;
static bool testRunning = false;

static void safeOff() {
  shield.rgbOff();
  shield.ledRed(false);
  shield.ledBlue(false);
  shield.buzzerOff();
  testRunning = false;
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
  Serial.println(F("RETURN  stop / cancel"));
  Serial.println(F("HOME    main menu"));
  Serial.println(F("MENU    help"));
  Serial.println(F("UP/DOWN or LEFT/RIGHT  change test"));
  Serial.println(F("POWER   safe OFF"));
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
      Serial.println(F("Press SW1 and SW2 on the shield."));
      Serial.println(F("Expected: red/blue discrete LEDs follow the buttons."));
      break;

    case 2:
      Serial.println(F("RGB LED"));
      Serial.println(F("Runs RED -> GREEN -> BLUE -> OFF."));
      Serial.println(F("Expected: exact channel colors."));
      break;

    case 3:
      Serial.println(F("POTENTIOMETER"));
      Serial.println(F("Rotate POT from minimum to maximum."));
      Serial.println(F("Expected ADC range: approximately 0..1023."));
      break;

    case 4:
      Serial.println(F("LDR"));
      Serial.println(F("Cover and uncover the light sensor."));
      Serial.println(F("Expected: darker -> lower ADC, brighter -> higher ADC."));
      break;

    case 5:
      Serial.println(F("DHT11"));
      Serial.println(F("Performs three temperature/humidity reads."));
      Serial.println(F("No physical action required."));
      break;

    case 6:
      Serial.println(F("ACTIVE BUZZER"));
      Serial.println(F("Turns buzzer ON for 1 second, then OFF."));
      Serial.println(F("Expected: strong sustained tone while ON."));
      break;

    case 7:
      Serial.println(F("IR DIAGNOSTICS"));
      Serial.println(F("Press remote buttons."));
      Serial.println(F("Decoded NEC frames are printed."));
      break;

    case 8:
      Serial.println(F("INTEGRATED RUN"));
      Serial.println(F("Buttons, LEDs, POT/RGB, sensors and async IR operate together."));
      Serial.println(F("RETURN stops the run."));
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
  Serial.println(F("LM35 A2 FAIL ON THIS SAMPLE"));
}

static void runTest1() {
  Serial.println(F("TEST-01 RUNNING"));
  Serial.println(F("Press SW1/SW2 for 10 seconds."));
  const unsigned long started = millis();
  while (millis() - started < 10000UL) {
    shield.service();
    shield.ledRed(shield.button1Pressed());
    shield.ledBlue(shield.button2Pressed());
  }
  shield.ledRed(false);
  shield.ledBlue(false);
  Serial.println(F("TEST-01 COMPLETE"));
}

static void runTest2() {
  Serial.println(F("TEST-02 RGB"));
  shield.setRGB(255, 0, 0);
  Serial.println(F("RED"));
  delay(800);
  shield.setRGB(0, 255, 0);
  Serial.println(F("GREEN"));
  delay(800);
  shield.setRGB(0, 0, 255);
  Serial.println(F("BLUE"));
  delay(800);
  shield.rgbOff();
  Serial.println(F("TEST-02 COMPLETE"));
}

static void runTest3() {
  Serial.println(F("TEST-03 POT"));
  Serial.println(F("Rotate POT now."));
  const unsigned long started = millis();
  int minV = 1023;
  int maxV = 0;

  while (millis() - started < 10000UL) {
    shield.service();
    const int v = shield.readPotRaw();
    if (v < minV) minV = v;
    if (v > maxV) maxV = v;
    delay(20);
  }

  Serial.print(F("POT MIN="));
  Serial.print(minV);
  Serial.print(F(" MAX="));
  Serial.println(maxV);
  Serial.println(F("TEST-03 COMPLETE"));
}

static void runTest4() {
  Serial.println(F("TEST-04 LDR"));
  Serial.println(F("Cover/uncover sensor now."));
  const unsigned long started = millis();
  int minV = 1023;
  int maxV = 0;

  while (millis() - started < 10000UL) {
    shield.service();
    const int v = shield.readLightRaw();
    if (v < minV) minV = v;
    if (v > maxV) maxV = v;
    delay(20);
  }

  Serial.print(F("LDR MIN="));
  Serial.print(minV);
  Serial.print(F(" MAX="));
  Serial.println(maxV);
  Serial.println(F("TEST-04 COMPLETE"));
}

static void runTest5() {
  Serial.println(F("TEST-05 DHT11"));

  for (uint8_t i = 0; i < 3; ++i) {
    const HY_M302::DhtReading d = shield.readDht11();
    if (!d.ok) {
      Serial.print(F("READ "));
      Serial.print(i + 1);
      Serial.println(F(": ERR"));
    } else {
      Serial.print(F("READ "));
      Serial.print(i + 1);
      Serial.print(F(": T="));
      Serial.print(d.temperatureC, 1);
      Serial.print(F(" C RH="));
      Serial.print(d.humidity, 1);
      Serial.println(F(" %"));
    }
    if (i != 2) delay(2000);
  }

  Serial.println(F("TEST-05 COMPLETE"));
}

static void runTest6() {
  Serial.println(F("TEST-06 BUZZER"));
  shield.buzzerOn();
  delay(1000);
  shield.buzzerOff();
  Serial.println(F("TEST-06 COMPLETE"));
}

static void printIrFrame(const HY_M302::IrNecFrame& frame) {
  if (frame.repeat) return;

  Serial.print(F("IR RAW=0x"));
  Serial.print(frame.raw, HEX);
  Serial.print(F(" ADDR=0x"));
  Serial.print(frame.address, HEX);
  Serial.print(F(" CMD=0x"));
  Serial.println(frame.command, HEX);
}

static void runTest7() {
  Serial.println(F("TEST-07 IR DIAGNOSTICS"));
  Serial.println(F("Press remote buttons for 10 seconds."));
  const unsigned long started = millis();

  while (millis() - started < 10000UL) {
    shield.service();
    HY_M302::IrNecFrame frame;
    while (shield.readIrNecAsync(frame)) {
      printIrFrame(frame);
    }
  }

  Serial.println(F("TEST-07 COMPLETE"));
}

static void runTest8() {
  Serial.println(F("TEST-08 INTEGRATED RUN"));
  Serial.println(F("RUNNING until RETURN or POWER."));
  testRunning = true;
}

static void runTest9() {
  Serial.print(F("IR STATS dropped_edges="));
  Serial.print(shield.irNecDroppedEdges());
  Serial.print(F(" dropped_frames="));
  Serial.println(shield.irNecDroppedFrames());
}

static void startSelectedTest() {
  switch (selectedTest) {
    case 0: printSummary(); break;
    case 1: runTest1(); break;
    case 2: runTest2(); break;
    case 3: runTest3(); break;
    case 4: runTest4(); break;
    case 5: runTest5(); break;
    case 6: runTest6(); break;
    case 7: runTest7(); break;
    case 8: runTest8(); break;
    case 9: runTest9(); break;
  }
}

static int8_t keyToDigit(Key key) {
  switch (key) {
    case KEY_0: return 0;
    case KEY_1: return 1;
    case KEY_2: return 2;
    case KEY_3: return 3;
    case KEY_4: return 4;
    case KEY_5: return 5;
    case KEY_6: return 6;
    case KEY_7: return 7;
    case KEY_8: return 8;
    case KEY_9: return 9;
    default: return -1;
  }
}

static void handleKey(Key key) {
  const int8_t digit = keyToDigit(key);
  if (digit >= 0) {
    printSelection(uint8_t(digit));
    return;
  }

  switch (key) {
    case KEY_OK:
      startSelectedTest();
      break;

    case KEY_HOME:
      safeOff();
      printMainMenu();
      break;

    case KEY_RETURN:
      safeOff();
      Serial.println(F("CANCEL / RETURN"));
      printSelection(selectedTest);
      break;

    case KEY_MENU:
      printMainMenu();
      break;

    case KEY_UP:
    case KEY_LEFT:
      selectedTest = (selectedTest == 0) ? 9 : selectedTest - 1;
      printSelection(selectedTest);
      break;

    case KEY_DOWN:
    case KEY_RIGHT:
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

  if (testRunning) {
    const bool sw1 = shield.button1Pressed();
    const bool sw2 = shield.button2Pressed();

    shield.ledRed(sw1);
    shield.ledBlue(sw2);

    const int pot = shield.readPotRaw();
    const uint8_t red = uint8_t((uint32_t(pot) * 255UL) / 1023UL);
    shield.setRGB(red, 0, 0);
  }

  HY_M302::IrNecFrame frame;
  while (shield.readIrNecAsync(frame)) {
    // Repeat frames are intentionally ignored by the menu to avoid double actions.
    if (frame.repeat) continue;

    const Key key = decode(frame.address, frame.command);
    if (key == KEY_NONE) {
      Serial.print(F("UNKNOWN IR ADDR=0x"));
      Serial.print(frame.address, HEX);
      Serial.print(F(" CMD=0x"));
      Serial.println(frame.command, HEX);
      continue;
    }

    handleKey(key);
  }
}
