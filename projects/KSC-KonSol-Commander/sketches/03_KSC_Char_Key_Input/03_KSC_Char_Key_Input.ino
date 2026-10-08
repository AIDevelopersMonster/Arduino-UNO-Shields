#include <Arduino.h>
#include <HY_M302.h>
#include <HY_M302_Remote.h>
#include <HY_M302_Remote_iDroid_OrangePi.h>

using namespace HY_M302_Remote;
using HY_M302_Remote::IDroidOrangePi::decode;

HY_M302 shield;

extern unsigned int __heap_start;
extern void *__brkval;

enum KscInputType : uint8_t {
  KSC_INPUT_CHAR = 1,
  KSC_INPUT_KEY  = 2
};

enum KscInputSource : uint8_t {
  KSC_SRC_TTY = 1,
  KSC_SRC_IR  = 2
};

enum KscSpecialKey : uint8_t {
  KSC_KEY_NONE = 0,
  KSC_KEY_UP,
  KSC_KEY_DOWN,
  KSC_KEY_LEFT,
  KSC_KEY_RIGHT,
  KSC_KEY_ENTER,
  KSC_KEY_BACK,
  KSC_KEY_HOME,
  KSC_KEY_END,
  KSC_KEY_DELETE,
  KSC_KEY_MENU,
  KSC_KEY_POWER
};

struct KscInputEvent {
  uint8_t type;
  uint8_t source;
  uint8_t code;
};

static const uint8_t EVENT_QUEUE_SIZE = 8;
static KscInputEvent eventQueue[EVENT_QUEUE_SIZE];
static uint8_t eventHead = 0;
static uint8_t eventTail = 0;
static uint8_t eventCount = 0;
static uint16_t eventDrops = 0;
static uint32_t eventSequence = 0;

enum TtyState : uint8_t {
  TTY_NORMAL = 0,
  TTY_ESC,
  TTY_CSI,
  TTY_SS3
};

static TtyState ttyState = TTY_NORMAL;
static unsigned long ttyStateMs = 0;
static uint16_t csiParam = 0;
static bool csiHaveParam = false;
static bool lastWasCR = false;

static int freeRam() {
  int v;
  return (int)&v -
         (__brkval == 0
            ? (int)&__heap_start
            : (int)__brkval);
}

static bool queueEvent(uint8_t type, uint8_t source, uint8_t code) {
  if (eventCount >= EVENT_QUEUE_SIZE) {
    ++eventDrops;
    return false;
  }

  eventQueue[eventHead].type = type;
  eventQueue[eventHead].source = source;
  eventQueue[eventHead].code = code;

  eventHead = (uint8_t)((eventHead + 1) % EVENT_QUEUE_SIZE);
  ++eventCount;
  return true;
}

static bool queueChar(KscInputSource source, uint8_t value) {
  return queueEvent(KSC_INPUT_CHAR, source, value);
}

static bool queueKey(KscInputSource source, KscSpecialKey key) {
  if (key == KSC_KEY_NONE) return false;
  return queueEvent(KSC_INPUT_KEY, source, (uint8_t)key);
}

static bool popEvent(KscInputEvent &event) {
  if (eventCount == 0) return false;

  event = eventQueue[eventTail];
  eventTail = (uint8_t)((eventTail + 1) % EVENT_QUEUE_SIZE);
  --eventCount;
  return true;
}

static const __FlashStringHelper *sourceName(uint8_t source) {
  switch (source) {
    case KSC_SRC_TTY: return F("TTY");
    case KSC_SRC_IR:  return F("IR");
    default:          return F("?");
  }
}

static const __FlashStringHelper *keyName(uint8_t code) {
  switch ((KscSpecialKey)code) {
    case KSC_KEY_UP:     return F("UP");
    case KSC_KEY_DOWN:   return F("DOWN");
    case KSC_KEY_LEFT:   return F("LEFT");
    case KSC_KEY_RIGHT:  return F("RIGHT");
    case KSC_KEY_ENTER:  return F("ENTER");
    case KSC_KEY_BACK:   return F("BACK");
    case KSC_KEY_HOME:   return F("HOME");
    case KSC_KEY_END:    return F("END");
    case KSC_KEY_DELETE: return F("DELETE");
    case KSC_KEY_MENU:   return F("MENU");
    case KSC_KEY_POWER:  return F("POWER");
    default:             return F("UNKNOWN");
  }
}

static void printEvent(const KscInputEvent &event) {
  ++eventSequence;

  Serial.print(F("EVT #"));
  Serial.print(eventSequence);
  Serial.print(F(" SRC="));
  Serial.print(sourceName(event.source));

  if (event.type == KSC_INPUT_CHAR) {
    Serial.print(F(" TYPE=CHAR CHAR='"));

    if (event.code >= 32 && event.code <= 126) {
      Serial.write(event.code);
    } else {
      Serial.print('?');
    }

    Serial.print(F("' HEX=0x"));
    if (event.code < 0x10) Serial.print('0');
    Serial.println(event.code, HEX);
    return;
  }

  Serial.print(F(" TYPE=KEY KEY="));
  Serial.println(keyName(event.code));
}

static void serviceEventConsumer() {
  KscInputEvent event;
  while (popEvent(event)) {
    printEvent(event);
  }
}

static void queueRemoteKey(Key remoteKey) {
  const int8_t digit = HY_M302_Remote::digit(remoteKey);

  if (digit >= 0 && digit <= 9) {
    queueChar(KSC_SRC_IR, (uint8_t)('0' + digit));
    return;
  }

  switch (remoteKey) {
    case KEY_UP:
      queueKey(KSC_SRC_IR, KSC_KEY_UP);
      break;
    case KEY_DOWN:
      queueKey(KSC_SRC_IR, KSC_KEY_DOWN);
      break;
    case KEY_LEFT:
      queueKey(KSC_SRC_IR, KSC_KEY_LEFT);
      break;
    case KEY_RIGHT:
      queueKey(KSC_SRC_IR, KSC_KEY_RIGHT);
      break;
    case KEY_OK:
      queueKey(KSC_SRC_IR, KSC_KEY_ENTER);
      break;
    case KEY_RETURN:
      queueKey(KSC_SRC_IR, KSC_KEY_BACK);
      break;
    case KEY_HOME:
      queueKey(KSC_SRC_IR, KSC_KEY_HOME);
      break;
    case KEY_MENU:
      queueKey(KSC_SRC_IR, KSC_KEY_MENU);
      break;
    case KEY_POWER:
      queueKey(KSC_SRC_IR, KSC_KEY_POWER);
      break;
    default:
      break;
  }
}

static void serviceIrInput() {
  HY_M302::IrNecFrame frame;

  while (shield.readIrNecAsync(frame)) {
    if (frame.repeat) continue;

    const Key remoteKey = decode(frame.address, frame.command);

    if (remoteKey == KEY_NONE) {
      Serial.print(F("IR UNKNOWN ADDR=0x"));
      Serial.print(frame.address, HEX);
      Serial.print(F(" CMD=0x"));
      Serial.println(frame.command, HEX);
      continue;
    }

    queueRemoteKey(remoteKey);
  }
}

static void resetTtyState() {
  ttyState = TTY_NORMAL;
  csiParam = 0;
  csiHaveParam = false;
}

static void processCsiFinal(uint8_t c) {
  KscSpecialKey key = KSC_KEY_NONE;

  switch (c) {
    case 'A':
      key = KSC_KEY_UP;
      break;
    case 'B':
      key = KSC_KEY_DOWN;
      break;
    case 'C':
      key = KSC_KEY_RIGHT;
      break;
    case 'D':
      key = KSC_KEY_LEFT;
      break;
    case 'H':
      key = KSC_KEY_HOME;
      break;
    case 'F':
      key = KSC_KEY_END;
      break;
    case '~':
      if (csiHaveParam) {
        switch (csiParam) {
          case 1:
          case 7:
            key = KSC_KEY_HOME;
            break;
          case 3:
            key = KSC_KEY_DELETE;
            break;
          case 4:
          case 8:
            key = KSC_KEY_END;
            break;
          case 20:
            key = KSC_KEY_MENU;
            break;
          case 21:
            key = KSC_KEY_POWER;
            break;
          default:
            break;
        }
      }
      break;
    default:
      break;
  }

  resetTtyState();

  if (key != KSC_KEY_NONE) {
    queueKey(KSC_SRC_TTY, key);
  }
}

static void handleNormalTtyByte(uint8_t c) {
  if (c == 0x1B) {
    ttyState = TTY_ESC;
    ttyStateMs = millis();
    return;
  }

  if (c == '\r') {
    queueKey(KSC_SRC_TTY, KSC_KEY_ENTER);
    lastWasCR = true;
    return;
  }

  if (c == '\n') {
    if (lastWasCR) {
      lastWasCR = false;
      return;
    }

    queueKey(KSC_SRC_TTY, KSC_KEY_ENTER);
    return;
  }

  lastWasCR = false;

  if (c == 0x08 || c == 0x7F) {
    queueKey(KSC_SRC_TTY, KSC_KEY_BACK);
    return;
  }

  if (c >= 0x20 && c <= 0x7E) {
    queueChar(KSC_SRC_TTY, c);
  }
}

static void serviceTtyInput() {
  while (Serial.available()) {
    const uint8_t c = (uint8_t)Serial.read();

    if (ttyState == TTY_NORMAL) {
      handleNormalTtyByte(c);
      continue;
    }

    if (ttyState == TTY_ESC) {
      if (c == '[') {
        ttyState = TTY_CSI;
        ttyStateMs = millis();
        csiParam = 0;
        csiHaveParam = false;
        continue;
      }

      if (c == 'O') {
        ttyState = TTY_SS3;
        ttyStateMs = millis();
        continue;
      }

      queueKey(KSC_SRC_TTY, KSC_KEY_BACK);
      resetTtyState();
      handleNormalTtyByte(c);
      continue;
    }

    if (ttyState == TTY_CSI) {
      ttyStateMs = millis();

      if (c >= '0' && c <= '9') {
        csiHaveParam = true;
        csiParam = (uint16_t)(csiParam * 10U + (c - '0'));
        continue;
      }

      if (c == ';') continue;

      processCsiFinal(c);
      continue;
    }

    if (ttyState == TTY_SS3) {
      resetTtyState();

      switch (c) {
        case 'A':
          queueKey(KSC_SRC_TTY, KSC_KEY_UP);
          break;
        case 'B':
          queueKey(KSC_SRC_TTY, KSC_KEY_DOWN);
          break;
        case 'C':
          queueKey(KSC_SRC_TTY, KSC_KEY_RIGHT);
          break;
        case 'D':
          queueKey(KSC_SRC_TTY, KSC_KEY_LEFT);
          break;
        case 'H':
          queueKey(KSC_SRC_TTY, KSC_KEY_HOME);
          break;
        default:
          break;
      }
    }
  }

  if (ttyState != TTY_NORMAL && millis() - ttyStateMs >= 80UL) {
    if (ttyState == TTY_ESC) {
      queueKey(KSC_SRC_TTY, KSC_KEY_BACK);
    }

    resetTtyState();
  }
}

static void printModel() {
  Serial.println();
  Serial.println(F("KSC-01C INPUT MODEL"));
  Serial.println();
  Serial.println(F("CHAR:"));
  Serial.println(F("  PC printable ASCII -> CHAR"));
  Serial.println(F("  IR 0..9            -> CHAR"));
  Serial.println();
  Serial.println(F("KEY:"));
  Serial.println(F("  arrows             -> KEY"));
  Serial.println(F("  Enter / IR OK      -> ENTER"));
  Serial.println(F("  Back/Esc/RETURN    -> BACK"));
  Serial.println(F("  Home / IR HOME     -> HOME"));
  Serial.println(F("  PC F9 / IR MENU    -> MENU"));
  Serial.println(F("  PC F10 / IR POWER  -> POWER"));
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  shield.begin();
  shield.ledRed(false);
  shield.ledBlue(false);
  shield.rgbOff();
  shield.buzzerOff();

  delay(100);

  Serial.println();
  Serial.println(F("KSC 0.1C"));
  Serial.println(F("Unified Character & Special-Key Input Model"));
  Serial.println(F("Arduino UNO / ATmega328P + HY-M302"));

  if (!shield.beginIrNecAsync()) {
    Serial.println(F("IR INIT: FAIL"));
  } else {
    Serial.println(F("IR INIT: OK"));
  }

  Serial.print(F("FREE RAM: "));
  Serial.print(freeRam());
  Serial.println(F(" B"));

  Serial.print(F("EVENT QUEUE: "));
  Serial.println(EVENT_QUEUE_SIZE);

  printModel();
  Serial.println(F("READY"));
}

void loop() {
  shield.service();
  serviceTtyInput();
  serviceIrInput();
  serviceEventConsumer();

  if (eventDrops != 0) {
    Serial.print(F("EVENT DROPS="));
    Serial.println(eventDrops);
    eventDrops = 0;
  }
}
