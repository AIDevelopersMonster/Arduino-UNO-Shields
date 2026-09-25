/*
  LAB-02 unified firmware for the classic Arduino Multi-Function Shield.

  No external Arduino libraries are required.

  Working pin map for the classic 4-LED shield family:
    D0/D1  optional APC220/Bluetooth/voice UART header (not used here)
    D2     optional IR receiver header (not used in v0.1)
    D3     buzzer
    D4     74HC595 latch
    D5/D6  free expansion pins
    D7     74HC595 clock
    D8     74HC595 data
    D9     free expansion pin
    D10    LED D4
    D11    LED D3
    D12    LED D2
    D13    LED D1
    A0     potentiometer
    A1-A3  buttons S1-S3
    A4     optional LM35/DS18B20 header (not used in v0.1)
    A5     free expansion pin

  Serial protocol: 115200 8N1, newline-terminated ASCII commands.
*/

#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

static const uint8_t PIN_BUZZER = 3;
static const uint8_t PIN_LATCH = 4;
static const uint8_t PIN_CLOCK = 7;
static const uint8_t PIN_DATA = 8;
static const uint8_t PIN_POT = A0;

static const uint8_t LED_PINS[4] = {13, 12, 11, 10};
static const uint8_t BUTTON_PINS[3] = {A1, A2, A3};

static const uint8_t SEGMENT_MAP[10] = {
  0xC0, 0xF9, 0xA4, 0xB0, 0x99,
  0x92, 0x82, 0xF8, 0x80, 0x90
};

// Standard classic-shield digit select bytes. Some clones invert these lines.
static const uint8_t DIGIT_SELECT_STD[4] = {0xF1, 0xF2, 0xF4, 0xF8};

static char displayText[5] = "0000";
static bool invertDigitSelect = false;
static uint8_t displayDigit = 0;
static unsigned long lastDisplayRefreshUs = 0;

static bool ledState[4] = {false, false, false, false};
static int potValue = 0;

static bool buttonRaw[3] = {false, false, false};
static bool buttonStable[3] = {false, false, false};
static unsigned long buttonChangedAt[3] = {0, 0, 0};
static unsigned long buttonPressCount[3] = {0, 0, 0};

static const unsigned long BUTTON_DEBOUNCE_MS = 25;
static unsigned long lastPotSampleMs = 0;
static unsigned long lastTelemetryMs = 0;

static char serialLine[80];
static uint8_t serialLineLen = 0;

enum BuzzerMode : uint8_t {
  BUZZER_OFF = 0,
  BUZZER_ACTIVE = 1,
  BUZZER_TONE = 2
};

static BuzzerMode buzzerMode = BUZZER_OFF;
static uint16_t buzzerHz = 0;
static unsigned long buzzerStopAtMs = 0;

enum TestStep : uint8_t {
  TEST_IDLE = 0,
  TEST_DISPLAY,
  TEST_LED1,
  TEST_LED2,
  TEST_LED3,
  TEST_LED4,
  TEST_BUZZER,
  TEST_POT,
  TEST_DONE
};

static TestStep testStep = TEST_IDLE;
static unsigned long testDeadlineMs = 0;

static bool deadlineReached(unsigned long now, unsigned long deadline) {
  return (long)(now - deadline) >= 0;
}

static const __FlashStringHelper *buzzerModeName() {
  switch (buzzerMode) {
    case BUZZER_ACTIVE: return F("ACTIVE");
    case BUZZER_TONE: return F("TONE");
    default: return F("OFF");
  }
}

static const __FlashStringHelper *testStepName() {
  switch (testStep) {
    case TEST_DISPLAY: return F("DISPLAY");
    case TEST_LED1: return F("LED1");
    case TEST_LED2: return F("LED2");
    case TEST_LED3: return F("LED3");
    case TEST_LED4: return F("LED4");
    case TEST_BUZZER: return F("BUZZER");
    case TEST_POT: return F("POT");
    case TEST_DONE: return F("DONE");
    default: return F("IDLE");
  }
}

static void setLed(uint8_t index, bool on) {
  if (index >= 4) return;
  ledState[index] = on;
  digitalWrite(LED_PINS[index], on ? LOW : HIGH);
}

static void setAllLeds(bool on) {
  for (uint8_t i = 0; i < 4; ++i) setLed(i, on);
}

static uint8_t segmentByteFor(char c) {
  if (c >= '0' && c <= '9') return SEGMENT_MAP[(uint8_t)(c - '0')];
  if (c == '-') return 0xBF;
  return 0xFF;
}

static void setDisplayText(const char *text) {
  char next[5] = {' ', ' ', ' ', ' ', '\0'};
  if (text == NULL) {
    memcpy(displayText, next, sizeof(displayText));
    return;
  }

  size_t n = strlen(text);
  if (n > 4) {
    text += (n - 4);
    n = 4;
  }
  uint8_t start = (uint8_t)(4 - n);
  for (uint8_t i = 0; i < n; ++i) next[start + i] = text[i];
  memcpy(displayText, next, sizeof(displayText));
}

static void setDisplayNumber(int value) {
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", value);
  setDisplayText(buf);
}

static void writeDisplayDigit(uint8_t index) {
  uint8_t digitSelect = DIGIT_SELECT_STD[index & 0x03];
  if (invertDigitSelect) digitSelect = (uint8_t)~digitSelect;

  digitalWrite(PIN_LATCH, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, segmentByteFor(displayText[index & 0x03]));
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, digitSelect);
  digitalWrite(PIN_LATCH, HIGH);
}

static void serviceDisplay() {
  unsigned long now = micros();
  if ((unsigned long)(now - lastDisplayRefreshUs) < 1800UL) return;
  lastDisplayRefreshUs = now;
  writeDisplayDigit(displayDigit);
  displayDigit = (displayDigit + 1) & 0x03;
}

static void buzzerOff() {
  noTone(PIN_BUZZER);
  digitalWrite(PIN_BUZZER, HIGH);
  buzzerMode = BUZZER_OFF;
  buzzerHz = 0;
  buzzerStopAtMs = 0;
}

static void buzzerActiveOn() {
  noTone(PIN_BUZZER);
  digitalWrite(PIN_BUZZER, LOW);
  buzzerMode = BUZZER_ACTIVE;
  buzzerHz = 0;
  buzzerStopAtMs = 0;
}

static void buzzerToneOn(uint16_t hz) {
  if (hz < 30) hz = 30;
  if (hz > 5000) hz = 5000;
  tone(PIN_BUZZER, hz);
  buzzerMode = BUZZER_TONE;
  buzzerHz = hz;
  buzzerStopAtMs = 0;
}

static void buzzerActiveBeep(uint16_t durationMs) {
  if (durationMs < 10) durationMs = 10;
  if (durationMs > 5000) durationMs = 5000;
  buzzerActiveOn();
  buzzerStopAtMs = millis() + durationMs;
}

static void serviceBuzzer() {
  if (buzzerStopAtMs != 0 && deadlineReached(millis(), buzzerStopAtMs)) buzzerOff();
}

static void serviceInputs() {
  unsigned long now = millis();

  for (uint8_t i = 0; i < 3; ++i) {
    bool rawPressed = (digitalRead(BUTTON_PINS[i]) == LOW);
    if (rawPressed != buttonRaw[i]) {
      buttonRaw[i] = rawPressed;
      buttonChangedAt[i] = now;
    }
    if (buttonStable[i] != buttonRaw[i] && (unsigned long)(now - buttonChangedAt[i]) >= BUTTON_DEBOUNCE_MS) {
      buttonStable[i] = buttonRaw[i];
      if (buttonStable[i]) ++buttonPressCount[i];
    }
  }

  if ((unsigned long)(now - lastPotSampleMs) >= 25UL) {
    lastPotSampleMs = now;
    potValue = analogRead(PIN_POT);
  }
}

static void sendInfo() {
  Serial.println(F("@SYS,MFSHIELD,LAB02,0.1,UNO,115200"));
  Serial.println(F("@PINS,BUZ=3,LATCH=4,CLK=7,DATA=8,LED=13/12/11/10,BTN=A1/A2/A3,POT=A0"));
  Serial.print(F("@CFG,DIGITSEL,"));
  Serial.println(invertDigitSelect ? F("INV") : F("STD"));
}

static void sendState() {
  Serial.print(F("@STATE,"));
  Serial.print(millis()); Serial.print(',');
  Serial.print(potValue); Serial.print(',');

  for (uint8_t i = 0; i < 3; ++i) {
    Serial.print(buttonStable[i] ? 1 : 0); Serial.print(',');
  }
  for (uint8_t i = 0; i < 3; ++i) {
    Serial.print(buttonPressCount[i]); Serial.print(',');
  }
  for (uint8_t i = 0; i < 4; ++i) {
    Serial.print(ledState[i] ? 1 : 0); Serial.print(',');
  }

  Serial.print(buzzerModeName()); Serial.print(',');
  Serial.print(buzzerHz); Serial.print(',');
  Serial.print(displayText); Serial.print(',');
  Serial.println(testStepName());
}

static void stopAutoTest() {
  testStep = TEST_IDLE;
  testDeadlineMs = 0;
  setAllLeds(false);
  buzzerOff();
  Serial.println(F("@TEST,STOPPED"));
}

static void startAutoTest() {
  setAllLeds(false);
  buzzerOff();
  setDisplayText("8888");
  testStep = TEST_DISPLAY;
  testDeadlineMs = millis() + 1000UL;
  Serial.println(F("@TEST,START"));
  Serial.println(F("@TEST,DISPLAY"));
}

static void serviceAutoTest() {
  if (testStep == TEST_IDLE) return;
  unsigned long now = millis();
  if (!deadlineReached(now, testDeadlineMs)) return;

  switch (testStep) {
    case TEST_DISPLAY:
      setAllLeds(false);
      setLed(0, true);
      testStep = TEST_LED1;
      testDeadlineMs = now + 400UL;
      Serial.println(F("@TEST,LED,1"));
      break;

    case TEST_LED1:
    case TEST_LED2:
    case TEST_LED3: {
      uint8_t current = (uint8_t)(testStep - TEST_LED1);
      setLed(current, false);
      setLed(current + 1, true);
      testStep = (TestStep)(testStep + 1);
      testDeadlineMs = now + 400UL;
      Serial.print(F("@TEST,LED,"));
      Serial.println((uint8_t)(current + 2));
      break;
    }

    case TEST_LED4:
      setAllLeds(false);
      buzzerActiveBeep(250);
      setDisplayText("0000");
      testStep = TEST_BUZZER;
      testDeadlineMs = now + 350UL;
      Serial.println(F("@TEST,BUZZER,ACTIVE"));
      break;

    case TEST_BUZZER:
      buzzerOff();
      setDisplayNumber(potValue);
      testStep = TEST_POT;
      testDeadlineMs = now + 1200UL;
      Serial.print(F("@TEST,POT,"));
      Serial.println(potValue);
      break;

    case TEST_POT:
      testStep = TEST_DONE;
      testDeadlineMs = now + 1000UL;
      Serial.println(F("@TEST,DONE"));
      break;

    case TEST_DONE:
      testStep = TEST_IDLE;
      testDeadlineMs = 0;
      break;

    default:
      testStep = TEST_IDLE;
      break;
  }
}

static bool tokenIsOn(const char *token) {
  return token != NULL && (strcmp(token, "ON") == 0 || strcmp(token, "1") == 0);
}

static void ack(const __FlashStringHelper *what) {
  Serial.print(F("@ACK,"));
  Serial.println(what);
}

static void handleCommand(char *line) {
  char *tokens[5] = {NULL, NULL, NULL, NULL, NULL};
  uint8_t count = 0;

  char *save = NULL;
  char *token = strtok_r(line, ",", &save);
  while (token != NULL && count < 5) {
    tokens[count++] = token;
    token = strtok_r(NULL, ",", &save);
  }
  if (count == 0) return;

  if (strcmp(tokens[0], "PING") == 0) {
    Serial.println(F("@PONG"));
    return;
  }

  if (strcmp(tokens[0], "INFO") == 0) {
    sendInfo();
    return;
  }

  if (strcmp(tokens[0], "STATE?") == 0) {
    sendState();
    return;
  }

  if (strcmp(tokens[0], "LED") == 0 && count >= 3) {
    bool on = tokenIsOn(tokens[2]);
    if (strcmp(tokens[1], "ALL") == 0) {
      setAllLeds(on);
      ack(F("LED"));
      return;
    }
    int n = atoi(tokens[1]);
    if (n >= 1 && n <= 4) {
      setLed((uint8_t)(n - 1), on);
      ack(F("LED"));
      return;
    }
  }

  if (strcmp(tokens[0], "DISP") == 0 && count >= 2) {
    setDisplayText(tokens[1]);
    ack(F("DISP"));
    return;
  }

  if (strcmp(tokens[0], "BUZ") == 0 && count >= 2) {
    if (strcmp(tokens[1], "ON") == 0) buzzerActiveOn();
    else if (strcmp(tokens[1], "OFF") == 0) buzzerOff();
    else {
      Serial.println(F("@ERR,BUZ,ARG"));
      return;
    }
    ack(F("BUZ"));
    return;
  }

  if (strcmp(tokens[0], "BEEP") == 0 && count >= 2) {
    int durationMs = atoi(tokens[1]);
    buzzerActiveBeep((uint16_t)durationMs);
    ack(F("BEEP"));
    return;
  }

  if (strcmp(tokens[0], "TONE") == 0 && count >= 2) {
    if (strcmp(tokens[1], "OFF") == 0) buzzerOff();
    else buzzerToneOn((uint16_t)atoi(tokens[1]));
    ack(F("TONE"));
    return;
  }

  if (strcmp(tokens[0], "TEST") == 0 && count >= 2) {
    if (strcmp(tokens[1], "ALL") == 0) {
      startAutoTest();
      return;
    }
    if (strcmp(tokens[1], "STOP") == 0) {
      stopAutoTest();
      return;
    }
  }

  if (strcmp(tokens[0], "CFG") == 0 && count >= 3 && strcmp(tokens[1], "DIGITSEL") == 0) {
    if (strcmp(tokens[2], "STD") == 0) invertDigitSelect = false;
    else if (strcmp(tokens[2], "INV") == 0) invertDigitSelect = true;
    else {
      Serial.println(F("@ERR,CFG,DIGITSEL"));
      return;
    }
    Serial.print(F("@CFG,DIGITSEL,"));
    Serial.println(invertDigitSelect ? F("INV") : F("STD"));
    return;
  }

  Serial.println(F("@ERR,UNKNOWN"));
}

static void serviceSerial() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r') continue;

    if (c == '\n') {
      serialLine[serialLineLen] = '\0';
      if (serialLineLen > 0) handleCommand(serialLine);
      serialLineLen = 0;
      continue;
    }

    if (serialLineLen < sizeof(serialLine) - 1) {
      serialLine[serialLineLen++] = c;
    } else {
      serialLineLen = 0;
      Serial.println(F("@ERR,LINE_TOO_LONG"));
    }
  }
}

void setup() {
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);

  // Set buzzer OFF before switching the pin to output to avoid a startup chirp.
  digitalWrite(PIN_BUZZER, HIGH);
  pinMode(PIN_BUZZER, OUTPUT);

  for (uint8_t i = 0; i < 4; ++i) {
    digitalWrite(LED_PINS[i], HIGH);
    pinMode(LED_PINS[i], OUTPUT);
    ledState[i] = false;
  }

  for (uint8_t i = 0; i < 3; ++i) pinMode(BUTTON_PINS[i], INPUT_PULLUP);

  Serial.begin(115200);
  delay(250);
  Serial.println(F("@SYS,READY,MFSHIELD,LAB02,0.1"));
  sendInfo();
}

void loop() {
  serviceDisplay();
  serviceSerial();
  serviceInputs();
  serviceBuzzer();
  serviceAutoTest();

  unsigned long now = millis();
  if ((unsigned long)(now - lastTelemetryMs) >= 100UL) {
    lastTelemetryMs = now;
    sendState();
  }
}
