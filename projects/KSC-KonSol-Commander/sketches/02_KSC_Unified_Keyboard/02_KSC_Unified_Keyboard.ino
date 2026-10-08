#include <Arduino.h>
#include <HY_M302.h>
#include <HY_M302_Remote.h>
#include <HY_M302_Remote_iDroid_OrangePi.h>

using namespace HY_M302_Remote;
using HY_M302_Remote::IDroidOrangePi::decode;

HY_M302 shield;

extern unsigned int __heap_start;
extern void *__brkval;

enum KscKey : uint8_t {
  KSC_KEY_NONE = 0,

  KSC_KEY_UP,
  KSC_KEY_DOWN,
  KSC_KEY_LEFT,
  KSC_KEY_RIGHT,

  KSC_KEY_ENTER,
  KSC_KEY_BACK,
  KSC_KEY_HOME,
  KSC_KEY_MENU,
  KSC_KEY_POWER,

  KSC_KEY_0,
  KSC_KEY_1,
  KSC_KEY_2,
  KSC_KEY_3,
  KSC_KEY_4,
  KSC_KEY_5,
  KSC_KEY_6,
  KSC_KEY_7,
  KSC_KEY_8,
  KSC_KEY_9
};

enum InputSource : uint8_t {
  SRC_TTY = 0,
  SRC_IR
};

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

static uint32_t keySequence = 0;

static KscKey lastKey = KSC_KEY_NONE;
static InputSource lastSource = SRC_TTY;

static int freeRam() {
  int v;

  return (int)&v -
         (__brkval == 0
            ? (int)&__heap_start
            : (int)__brkval);
}

static const __FlashStringHelper *kscKeyName(KscKey key) {
  switch (key) {
    case KSC_KEY_UP:    return F("UP");
    case KSC_KEY_DOWN:  return F("DOWN");
    case KSC_KEY_LEFT:  return F("LEFT");
    case KSC_KEY_RIGHT: return F("RIGHT");

    case KSC_KEY_ENTER: return F("ENTER");
    case KSC_KEY_BACK:  return F("BACK");
    case KSC_KEY_HOME:  return F("HOME");
    case KSC_KEY_MENU:  return F("MENU");
    case KSC_KEY_POWER: return F("POWER");

    case KSC_KEY_0: return F("0");
    case KSC_KEY_1: return F("1");
    case KSC_KEY_2: return F("2");
    case KSC_KEY_3: return F("3");
    case KSC_KEY_4: return F("4");
    case KSC_KEY_5: return F("5");
    case KSC_KEY_6: return F("6");
    case KSC_KEY_7: return F("7");
    case KSC_KEY_8: return F("8");
    case KSC_KEY_9: return F("9");

    default:
      return F("NONE");
  }
}

static const __FlashStringHelper *sourceName(InputSource src) {
  return src == SRC_IR ? F("IR") : F("TTY");
}

static void emitKey(InputSource src, KscKey key) {
  if (key == KSC_KEY_NONE) return;

  ++keySequence;

  lastKey = key;
  lastSource = src;

  Serial.print(F("KEY #"));
  Serial.print(keySequence);

  Serial.print(F(" SRC="));
  Serial.print(sourceName(src));

  Serial.print(F(" CODE="));
  Serial.println(kscKeyName(key));
}

static KscKey digitKey(uint8_t digit) {
  if (digit > 9) return KSC_KEY_NONE;

  return static_cast<KscKey>(
    static_cast<uint8_t>(KSC_KEY_0) + digit
  );
}

static KscKey remoteToKscKey(Key key) {
  const int8_t d = HY_M302_Remote::digit(key);

  if (d >= 0 && d <= 9) {
    return digitKey((uint8_t)d);
  }

  switch (key) {
    case KEY_UP:
      return KSC_KEY_UP;

    case KEY_DOWN:
      return KSC_KEY_DOWN;

    case KEY_LEFT:
      return KSC_KEY_LEFT;

    case KEY_RIGHT:
      return KSC_KEY_RIGHT;

    case KEY_OK:
      return KSC_KEY_ENTER;

    case KEY_RETURN:
      return KSC_KEY_BACK;

    case KEY_HOME:
      return KSC_KEY_HOME;

    case KEY_MENU:
      return KSC_KEY_MENU;

    case KEY_POWER:
      return KSC_KEY_POWER;

    default:
      return KSC_KEY_NONE;
  }
}

static void printHelp() {
  Serial.println();
  Serial.println(F("KSC-01B UNIFIED KEYBOARD TEST"));
  Serial.println();
  Serial.println(F("TTY:"));
  Serial.println(F("  Arrow keys       -> UP/DOWN/LEFT/RIGHT"));
  Serial.println(F("  Enter            -> ENTER"));
  Serial.println(F("  Backspace        -> BACK"));
  Serial.println(F("  Esc              -> BACK"));
  Serial.println(F("  Home             -> HOME"));
  Serial.println(F("  M                -> MENU"));
  Serial.println(F("  P                -> POWER"));
  Serial.println(F("  0..9             -> digit key"));
  Serial.println();
  Serial.println(F("IR:"));
  Serial.println(F("  UP/DOWN/LEFT/RIGHT"));
  Serial.println(F("  OK               -> ENTER"));
  Serial.println(F("  RETURN           -> BACK"));
  Serial.println(F("  HOME"));
  Serial.println(F("  MENU"));
  Serial.println(F("  POWER"));
  Serial.println(F("  0..9"));
  Serial.println();
  Serial.println(F("Diagnostics:"));
  Serial.println(F("  S = IR stats"));
  Serial.println(F("  Z = reset IR stats"));
  Serial.println(F("  ? = help"));
  Serial.println();
}

static void printIrStats() {
  Serial.print(F("IR STATS dropped_edges="));
  Serial.print(shield.irNecDroppedEdges());

  Serial.print(F(" dropped_frames="));
  Serial.println(shield.irNecDroppedFrames());
}

static void resetIrStats() {
  shield.resetIrNecStats();
  Serial.println(F("IR STATS RESET"));
}

static void ttyStateReset() {
  ttyState = TTY_NORMAL;
  csiParam = 0;
  csiHaveParam = false;
}

static void handleNormalTtyByte(uint8_t c);

static void processCsiFinal(uint8_t c) {
  KscKey key = KSC_KEY_NONE;

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

    case '~':
      if (csiHaveParam) {
        if (csiParam == 1 || csiParam == 7) {
          key = KSC_KEY_HOME;
        } else if (csiParam == 3) {
          key = KSC_KEY_BACK;
        }
      }
      break;

    default:
      break;
  }

  ttyStateReset();

  if (key != KSC_KEY_NONE) {
    emitKey(SRC_TTY, key);
  }
}

static void handleNormalTtyByte(uint8_t c) {
  if (c == 0x1B) {
    ttyState = TTY_ESC;
    ttyStateMs = millis();
    return;
  }

  if (c == '\r') {
    emitKey(SRC_TTY, KSC_KEY_ENTER);
    lastWasCR = true;
    return;
  }

  if (c == '\n') {
    if (lastWasCR) {
      lastWasCR = false;
      return;
    }

    emitKey(SRC_TTY, KSC_KEY_ENTER);
    return;
  }

  lastWasCR = false;

  if (c == 0x08 || c == 0x7F) {
    emitKey(SRC_TTY, KSC_KEY_BACK);
    return;
  }

  if (c >= '0' && c <= '9') {
    emitKey(SRC_TTY, digitKey(c - '0'));
    return;
  }

  if (c == 'm' || c == 'M') {
    emitKey(SRC_TTY, KSC_KEY_MENU);
    return;
  }

  if (c == 'p' || c == 'P') {
    emitKey(SRC_TTY, KSC_KEY_POWER);
    return;
  }

  if (c == 'h' || c == 'H') {
    emitKey(SRC_TTY, KSC_KEY_HOME);
    return;
  }

  if (c == 's' || c == 'S') {
    printIrStats();
    return;
  }

  if (c == 'z' || c == 'Z') {
    resetIrStats();
    return;
  }

  if (c == '?') {
    printHelp();
    return;
  }
}

static void serviceTty() {
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

      emitKey(SRC_TTY, KSC_KEY_BACK);
      ttyStateReset();

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

      if (c == ';') {
        continue;
      }

      processCsiFinal(c);
      continue;
    }

    if (ttyState == TTY_SS3) {
      ttyStateReset();

      switch (c) {
        case 'A':
          emitKey(SRC_TTY, KSC_KEY_UP);
          break;

        case 'B':
          emitKey(SRC_TTY, KSC_KEY_DOWN);
          break;

        case 'C':
          emitKey(SRC_TTY, KSC_KEY_RIGHT);
          break;

        case 'D':
          emitKey(SRC_TTY, KSC_KEY_LEFT);
          break;

        case 'H':
          emitKey(SRC_TTY, KSC_KEY_HOME);
          break;

        default:
          break;
      }
    }
  }

  if (ttyState != TTY_NORMAL) {
    if (millis() - ttyStateMs >= 80UL) {
      if (ttyState == TTY_ESC) {
        emitKey(SRC_TTY, KSC_KEY_BACK);
      }

      ttyStateReset();
    }
  }
}

static void serviceIr() {
  HY_M302::IrNecFrame frame;

  while (shield.readIrNecAsync(frame)) {
    if (frame.repeat) {
      continue;
    }

    const Key remoteKey = decode(frame.address, frame.command);
    const KscKey key = remoteToKscKey(remoteKey);

    if (key == KSC_KEY_NONE) {
      Serial.print(F("IR UNKNOWN ADDR=0x"));
      Serial.print(frame.address, HEX);

      Serial.print(F(" CMD=0x"));
      Serial.println(frame.command, HEX);

      continue;
    }

    emitKey(SRC_IR, key);
  }
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
  Serial.println(F("KSC 0.1B"));
  Serial.println(F("KonSol Commander - Unified Keyboard Layer"));
  Serial.println(F("Arduino UNO / ATmega328P + HY-M302"));

  if (!shield.beginIrNecAsync()) {
    Serial.println(F("IR INIT: FAIL"));
  } else {
    Serial.println(F("IR INIT: OK"));
  }

  Serial.print(F("FREE RAM: "));
  Serial.print(freeRam());
  Serial.println(F(" B"));

  printHelp();

  Serial.println(F("READY"));
}

void loop() {
  shield.service();

  serviceTty();
  serviceIr();
}
