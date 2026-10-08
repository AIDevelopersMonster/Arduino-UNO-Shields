#include <Arduino.h>
#include <avr/pgmspace.h>
#include <HY_M302.h>
#include <HY_M302_Remote.h>
#include <HY_M302_Remote_iDroid_OrangePi.h>
#include "KSC_02_Types.h"

using namespace HY_M302_Remote;
using HY_M302_Remote::IDroidOrangePi::decode;

HY_M302 shield;

extern unsigned int __heap_start;
extern void *__brkval;

// -----------------------------------------------------------------------------
// KSC 0.2 - first real one-panel ANSI KonSol Commander
// -----------------------------------------------------------------------------

static const uint8_t PATH_SIZE = 24;
static const uint8_t VALUE_SIZE = 24;
static const uint8_t SHELL_LINE_SIZE = 64;
static const uint8_t EVENT_QUEUE_SIZE = 8;

static bool ledRedState = false;
static bool ledBlueState = false;
static bool buzzerState = false;

static uint8_t rgbR = 0;
static uint8_t rgbG = 0;
static uint8_t rgbB = 0;

static HY_M302::DhtReading dhtCache;
static unsigned long dhtLastMs = 0;
static bool dhtHaveValue = false;

// -----------------------------------------------------------------------------
// Memory
// -----------------------------------------------------------------------------

static int freeRam() {
  int v;

  return (int)&v -
         (__brkval == 0
            ? (int)&__heap_start
            : (int)__brkval);
}

// -----------------------------------------------------------------------------
// Small text helpers
// -----------------------------------------------------------------------------

static void copyFlashText(
  char *out,
  uint8_t outSize,
  const __FlashStringHelper *text
) {
  if (!out || outSize == 0) return;

  strncpy_P(
    out,
    (PGM_P)text,
    outSize - 1
  );

  out[outSize - 1] = 0;
}

static char *skipSpaces(char *p) {
  while (*p == ' ' || *p == '\t') ++p;
  return p;
}

static void upperCommand(char *p) {
  while (*p && *p != ' ' && *p != '\t') {
    if (*p >= 'a' && *p <= 'z') {
      *p = (char)(*p - 'a' + 'A');
    }

    ++p;
  }
}

static char *splitCommand(char *line) {
  char *p = line;

  while (*p && *p != ' ' && *p != '\t') ++p;

  if (*p) {
    *p++ = 0;
    p = skipSpaces(p);
  }

  return p;
}

static bool parseLong(const char *text, long &value) {
  if (!text || !*text) return false;

  char *endp = nullptr;
  value = strtol(text, &endp, 10);

  return endp && *endp == 0;
}

// -----------------------------------------------------------------------------
// VFS
// -----------------------------------------------------------------------------

static const __FlashStringHelper *vfsResultName(VfsResult result) {
  switch (result) {
    case VFS_OK:
      return F("OK");

    case VFS_ERR_NOT_FOUND:
      return F("ERR NOT_FOUND");

    case VFS_ERR_IS_DIR:
      return F("ERR IS_DIR");

    case VFS_ERR_READ_ONLY:
      return F("ERR READ_ONLY");

    case VFS_ERR_RANGE:
      return F("ERR RANGE");

    case VFS_ERR_VALUE:
      return F("ERR VALUE");

    case VFS_ERR_DHT:
      return F("ERR DHT");

    default:
      return F("ERR");
  }
}

static bool isDirectoryPath(const char *path) {
  return strcmp(path, "/") == 0 ||
         strcmp(path, "/dev") == 0 ||
         strcmp(path, "/dev/dht") == 0 ||
         strcmp(path, "/dev/led") == 0 ||
         strcmp(path, "/dev/rgb") == 0 ||
         strcmp(path, "/proc") == 0 ||
         strcmp(path, "/sys") == 0;
}

static VfsNodeType vfsPathType(const char *path) {
  if (isDirectoryPath(path)) {
    return VFS_DIR;
  }

  if (strcmp(path, "/dev/sw1") == 0 ||
      strcmp(path, "/dev/sw2") == 0 ||
      strcmp(path, "/dev/pot") == 0 ||
      strcmp(path, "/dev/light") == 0 ||
      strcmp(path, "/dev/dht/temp") == 0 ||
      strcmp(path, "/dev/dht/humidity") == 0 ||
      strcmp(path, "/dev/dht/status") == 0 ||
      strcmp(path, "/dev/dht/age") == 0 ||
      strcmp(path, "/proc/mem") == 0 ||
      strcmp(path, "/proc/uptime") == 0 ||
      strcmp(path, "/sys/version") == 0 ||
      strcmp(path, "/sys/target") == 0 ||
      strcmp(path, "/sys/storage") == 0) {
    return VFS_RO;
  }

  if (strcmp(path, "/dev/led/red") == 0 ||
      strcmp(path, "/dev/led/blue") == 0 ||
      strcmp(path, "/dev/rgb/red") == 0 ||
      strcmp(path, "/dev/rgb/green") == 0 ||
      strcmp(path, "/dev/rgb/blue") == 0 ||
      strcmp(path, "/dev/buzzer") == 0) {
    return VFS_RW;
  }

  return VFS_NONE;
}

static uint8_t vfsDirCount(const char *path) {
  if (strcmp(path, "/") == 0) {
    return 3;
  }

  if (strcmp(path, "/dev") == 0) {
    return 8;
  }

  if (strcmp(path, "/dev/dht") == 0) {
    return 4;
  }

  if (strcmp(path, "/dev/led") == 0) {
    return 2;
  }

  if (strcmp(path, "/dev/rgb") == 0) {
    return 3;
  }

  if (strcmp(path, "/proc") == 0) {
    return 2;
  }

  if (strcmp(path, "/sys") == 0) {
    return 3;
  }

  return 0;
}

static const __FlashStringHelper *vfsDirEntryName(
  const char *path,
  uint8_t index
) {
  if (strcmp(path, "/") == 0) {
    switch (index) {
      case 0: return F("dev");
      case 1: return F("proc");
      case 2: return F("sys");
      default: return F("?");
    }
  }

  if (strcmp(path, "/dev") == 0) {
    switch (index) {
      case 0: return F("sw1");
      case 1: return F("sw2");
      case 2: return F("pot");
      case 3: return F("light");
      case 4: return F("dht");
      case 5: return F("led");
      case 6: return F("rgb");
      case 7: return F("buzzer");
      default: return F("?");
    }
  }

  if (strcmp(path, "/dev/dht") == 0) {
    switch (index) {
      case 0: return F("temp");
      case 1: return F("humidity");
      case 2: return F("status");
      case 3: return F("age");
      default: return F("?");
    }
  }

  if (strcmp(path, "/dev/led") == 0) {
    switch (index) {
      case 0: return F("red");
      case 1: return F("blue");
      default: return F("?");
    }
  }

  if (strcmp(path, "/dev/rgb") == 0) {
    switch (index) {
      case 0: return F("red");
      case 1: return F("green");
      case 2: return F("blue");
      default: return F("?");
    }
  }

  if (strcmp(path, "/proc") == 0) {
    switch (index) {
      case 0: return F("mem");
      case 1: return F("uptime");
      default: return F("?");
    }
  }

  if (strcmp(path, "/sys") == 0) {
    switch (index) {
      case 0: return F("version");
      case 1: return F("target");
      case 2: return F("storage");
      default: return F("?");
    }
  }

  return F("?");
}

static VfsNodeType vfsDirEntryType(
  const char *path,
  uint8_t index
) {
  if (strcmp(path, "/") == 0) {
    return index < 3 ? VFS_DIR : VFS_NONE;
  }

  if (strcmp(path, "/dev") == 0) {
    switch (index) {
      case 0:
      case 1:
      case 2:
      case 3:
        return VFS_RO;

      case 4:
      case 5:
      case 6:
        return VFS_DIR;

      case 7:
        return VFS_RW;

      default:
        return VFS_NONE;
    }
  }

  if (strcmp(path, "/dev/dht") == 0) {
    return index < 4 ? VFS_RO : VFS_NONE;
  }

  if (strcmp(path, "/dev/led") == 0) {
    return index < 2 ? VFS_RW : VFS_NONE;
  }

  if (strcmp(path, "/dev/rgb") == 0) {
    return index < 3 ? VFS_RW : VFS_NONE;
  }

  if (strcmp(path, "/proc") == 0) {
    return index < 2 ? VFS_RO : VFS_NONE;
  }

  if (strcmp(path, "/sys") == 0) {
    return index < 3 ? VFS_RO : VFS_NONE;
  }

  return VFS_NONE;
}

static void parentPath(
  const char *path,
  char *out,
  uint8_t outSize
) {
  if (strcmp(path, "/") == 0) {
    strncpy(out, "/", outSize);
    out[outSize - 1] = 0;
    return;
  }

  strncpy(out, path, outSize);
  out[outSize - 1] = 0;

  char *last = strrchr(out, '/');

  if (!last || last == out) {
    strcpy(out, "/");
    return;
  }

  *last = 0;
}

static void joinPath(
  const char *base,
  const char *name,
  char *out,
  uint8_t outSize
) {
  if (strcmp(base, "/") == 0) {
    out[0] = '/';
    out[1] = 0;

    strncat(
      out,
      name,
      outSize - 2
    );

    return;
  }

  strncpy(out, base, outSize);
  out[outSize - 1] = 0;

  const uint8_t n = strlen(out);

  if (n + 1 < outSize) {
    out[n] = '/';
    out[n + 1] = 0;
  }

  strncat(
    out,
    name,
    outSize - strlen(out) - 1
  );
}

static void makePath(
  const char *cwd,
  const char *arg,
  char *out,
  uint8_t outSize
) {
  if (!arg || !*arg) {
    strncpy(out, cwd, outSize);
    out[outSize - 1] = 0;
    return;
  }

  if (strcmp(arg, "..") == 0) {
    parentPath(cwd, out, outSize);
    return;
  }

  if (arg[0] == '/') {
    strncpy(out, arg, outSize);
    out[outSize - 1] = 0;
    return;
  }

  joinPath(cwd, arg, out, outSize);
}

static void refreshDht() {
  const unsigned long now = millis();

  if (dhtHaveValue && (now - dhtLastMs) < 2000UL) {
    return;
  }

  const HY_M302::DhtReading reading =
    shield.readDht11();

  dhtCache = reading;
  dhtLastMs = millis();
  dhtHaveValue = reading.ok;
}

static VfsResult vfsRead(
  const char *path,
  char *out,
  uint8_t outSize
) {
  if (outSize == 0) {
    return VFS_ERR_VALUE;
  }

  out[0] = 0;

  const VfsNodeType type =
    vfsPathType(path);

  if (type == VFS_DIR) {
    return VFS_ERR_IS_DIR;
  }

  if (type == VFS_NONE) {
    return VFS_ERR_NOT_FOUND;
  }

  if (strcmp(path, "/dev/sw1") == 0) {
    strcpy(out, shield.button1Pressed() ? "1" : "0");
    return VFS_OK;
  }

  if (strcmp(path, "/dev/sw2") == 0) {
    strcpy(out, shield.button2Pressed() ? "1" : "0");
    return VFS_OK;
  }

  if (strcmp(path, "/dev/pot") == 0) {
    itoa(shield.readPotRaw(), out, 10);
    return VFS_OK;
  }

  if (strcmp(path, "/dev/light") == 0) {
    itoa(shield.readLightRaw(), out, 10);
    return VFS_OK;
  }

  if (strcmp(path, "/dev/dht/temp") == 0) {
    refreshDht();

    if (!dhtHaveValue) {
      return VFS_ERR_DHT;
    }

    dtostrf(
      dhtCache.temperatureC,
      0,
      1,
      out
    );

    return VFS_OK;
  }

  if (strcmp(path, "/dev/dht/humidity") == 0) {
    refreshDht();

    if (!dhtHaveValue) {
      return VFS_ERR_DHT;
    }

    dtostrf(
      dhtCache.humidity,
      0,
      1,
      out
    );

    return VFS_OK;
  }

  if (strcmp(path, "/dev/dht/status") == 0) {
    refreshDht();

    copyFlashText(
      out,
      outSize,
      dhtHaveValue ? F("OK") : F("ERROR")
    );

    return VFS_OK;
  }

  if (strcmp(path, "/dev/dht/age") == 0) {
    if (!dhtLastMs) {
      copyFlashText(out, outSize, F("NEVER"));
    } else {
      ultoa(
        millis() - dhtLastMs,
        out,
        10
      );
    }

    return VFS_OK;
  }

  if (strcmp(path, "/dev/led/red") == 0) {
    strcpy(out, ledRedState ? "1" : "0");
    return VFS_OK;
  }

  if (strcmp(path, "/dev/led/blue") == 0) {
    strcpy(out, ledBlueState ? "1" : "0");
    return VFS_OK;
  }

  if (strcmp(path, "/dev/rgb/red") == 0) {
    utoa(rgbR, out, 10);
    return VFS_OK;
  }

  if (strcmp(path, "/dev/rgb/green") == 0) {
    utoa(rgbG, out, 10);
    return VFS_OK;
  }

  if (strcmp(path, "/dev/rgb/blue") == 0) {
    utoa(rgbB, out, 10);
    return VFS_OK;
  }

  if (strcmp(path, "/dev/buzzer") == 0) {
    strcpy(out, buzzerState ? "1" : "0");
    return VFS_OK;
  }

  if (strcmp(path, "/proc/mem") == 0) {
    itoa(freeRam(), out, 10);
    return VFS_OK;
  }

  if (strcmp(path, "/proc/uptime") == 0) {
    ultoa(millis(), out, 10);
    return VFS_OK;
  }

  if (strcmp(path, "/sys/version") == 0) {
    copyFlashText(out, outSize, F("KSC 0.2"));
    return VFS_OK;
  }

  if (strcmp(path, "/sys/target") == 0) {
    copyFlashText(
      out,
      outSize,
      F("Arduino UNO + HY-M302")
    );

    return VFS_OK;
  }

  if (strcmp(path, "/sys/storage") == 0) {
    copyFlashText(
      out,
      outSize,
      F("VIRTUAL ONLY")
    );

    return VFS_OK;
  }

  return VFS_ERR_NOT_FOUND;
}

static bool vfsWritableRange(
  const char *path,
  uint16_t &minValue,
  uint16_t &maxValue
) {
  if (strcmp(path, "/dev/led/red") == 0 ||
      strcmp(path, "/dev/led/blue") == 0 ||
      strcmp(path, "/dev/buzzer") == 0) {
    minValue = 0;
    maxValue = 1;
    return true;
  }

  if (strcmp(path, "/dev/rgb/red") == 0 ||
      strcmp(path, "/dev/rgb/green") == 0 ||
      strcmp(path, "/dev/rgb/blue") == 0) {
    minValue = 0;
    maxValue = 255;
    return true;
  }

  return false;
}

static bool vfsReadWritableLong(
  const char *path,
  long &value
) {
  if (strcmp(path, "/dev/led/red") == 0) {
    value = ledRedState ? 1 : 0;
    return true;
  }

  if (strcmp(path, "/dev/led/blue") == 0) {
    value = ledBlueState ? 1 : 0;
    return true;
  }

  if (strcmp(path, "/dev/buzzer") == 0) {
    value = buzzerState ? 1 : 0;
    return true;
  }

  if (strcmp(path, "/dev/rgb/red") == 0) {
    value = rgbR;
    return true;
  }

  if (strcmp(path, "/dev/rgb/green") == 0) {
    value = rgbG;
    return true;
  }

  if (strcmp(path, "/dev/rgb/blue") == 0) {
    value = rgbB;
    return true;
  }

  return false;
}

static VfsResult vfsWrite(
  const char *path,
  long value
) {
  const VfsNodeType type =
    vfsPathType(path);

  if (type == VFS_NONE) {
    return VFS_ERR_NOT_FOUND;
  }

  if (type == VFS_DIR) {
    return VFS_ERR_IS_DIR;
  }

  if (type != VFS_RW) {
    return VFS_ERR_READ_ONLY;
  }

  uint16_t minValue = 0;
  uint16_t maxValue = 0;

  if (!vfsWritableRange(
        path,
        minValue,
        maxValue
      )) {
    return VFS_ERR_READ_ONLY;
  }

  if (value < minValue ||
      value > maxValue) {
    return VFS_ERR_RANGE;
  }

  if (strcmp(path, "/dev/led/red") == 0) {
    ledRedState = value != 0;
    shield.ledRed(ledRedState);
    return VFS_OK;
  }

  if (strcmp(path, "/dev/led/blue") == 0) {
    ledBlueState = value != 0;
    shield.ledBlue(ledBlueState);
    return VFS_OK;
  }

  if (strcmp(path, "/dev/buzzer") == 0) {
    buzzerState = value != 0;

    if (buzzerState) {
      shield.buzzerOn();
    } else {
      shield.buzzerOff();
    }

    return VFS_OK;
  }

  if (strcmp(path, "/dev/rgb/red") == 0) {
    rgbR = (uint8_t)value;
  } else if (strcmp(path, "/dev/rgb/green") == 0) {
    rgbG = (uint8_t)value;
  } else if (strcmp(path, "/dev/rgb/blue") == 0) {
    rgbB = (uint8_t)value;
  } else {
    return VFS_ERR_NOT_FOUND;
  }

  shield.setRGB(
    rgbR,
    rgbG,
    rgbB
  );

  return VFS_OK;
}

static bool vfsPathIsDynamic(
  const char *path
) {
  return strcmp(path, "/dev/sw1") == 0 ||
         strcmp(path, "/dev/sw2") == 0 ||
         strcmp(path, "/dev/pot") == 0 ||
         strcmp(path, "/dev/light") == 0 ||
         strcmp(path, "/dev/dht/temp") == 0 ||
         strcmp(path, "/dev/dht/humidity") == 0 ||
         strcmp(path, "/dev/dht/status") == 0 ||
         strcmp(path, "/dev/dht/age") == 0 ||
         strcmp(path, "/proc/mem") == 0 ||
         strcmp(path, "/proc/uptime") == 0;
}

static void vfsListShell(
  const char *path
) {
  if (!isDirectoryPath(path)) {
    Serial.println(F("ERR NOT_DIR"));
    return;
  }

  const uint8_t count =
    vfsDirCount(path);

  for (uint8_t i = 0; i < count; ++i) {
    const VfsNodeType type =
      vfsDirEntryType(path, i);

    Serial.print(
      type == VFS_DIR
        ? F("D ")
        : F("F ")
    );

    Serial.print(
      vfsDirEntryName(path, i)
    );

    if (type == VFS_RO) {
      Serial.print(F("  RO"));
    } else if (type == VFS_RW) {
      Serial.print(F("  RW"));
    }

    Serial.println();
  }
}

// -----------------------------------------------------------------------------
// Unified input model
// -----------------------------------------------------------------------------

static KscInputEvent eventQueue[
  EVENT_QUEUE_SIZE
];

static uint8_t eventHead = 0;
static uint8_t eventTail = 0;
static uint8_t eventCount = 0;

static uint16_t eventDrops = 0;

static bool queueEvent(
  uint8_t type,
  uint8_t source,
  uint8_t code
) {
  if (eventCount >= EVENT_QUEUE_SIZE) {
    ++eventDrops;
    return false;
  }

  eventQueue[eventHead].type = type;
  eventQueue[eventHead].source = source;
  eventQueue[eventHead].code = code;

  eventHead =
    (uint8_t)((eventHead + 1) %
              EVENT_QUEUE_SIZE);

  ++eventCount;

  return true;
}

static bool queueChar(
  KscInputSource source,
  uint8_t value
) {
  return queueEvent(
    KSC_INPUT_CHAR,
    source,
    value
  );
}

static bool queueKey(
  KscInputSource source,
  KscSpecialKey key
) {
  if (key == KSC_KEY_NONE) {
    return false;
  }

  return queueEvent(
    KSC_INPUT_KEY,
    source,
    (uint8_t)key
  );
}

// -----------------------------------------------------------------------------
// IR input
// -----------------------------------------------------------------------------

static void queueRemoteKey(
  Key remoteKey
) {
  const int8_t digit =
    HY_M302_Remote::digit(remoteKey);

  if (digit >= 0 &&
      digit <= 9) {
    queueChar(
      KSC_SRC_IR,
      (uint8_t)('0' + digit)
    );

    return;
  }

  switch (remoteKey) {
    case KEY_UP:
      queueKey(
        KSC_SRC_IR,
        KSC_KEY_UP
      );
      break;

    case KEY_DOWN:
      queueKey(
        KSC_SRC_IR,
        KSC_KEY_DOWN
      );
      break;

    case KEY_LEFT:
      queueKey(
        KSC_SRC_IR,
        KSC_KEY_LEFT
      );
      break;

    case KEY_RIGHT:
      queueKey(
        KSC_SRC_IR,
        KSC_KEY_RIGHT
      );
      break;

    case KEY_OK:
      queueKey(
        KSC_SRC_IR,
        KSC_KEY_ENTER
      );
      break;

    case KEY_RETURN:
      queueKey(
        KSC_SRC_IR,
        KSC_KEY_BACK
      );
      break;

    case KEY_HOME:
      queueKey(
        KSC_SRC_IR,
        KSC_KEY_HOME
      );
      break;

    case KEY_MENU:
      queueKey(
        KSC_SRC_IR,
        KSC_KEY_MENU
      );
      break;

    case KEY_POWER:
      queueKey(
        KSC_SRC_IR,
        KSC_KEY_POWER
      );
      break;

    default:
      break;
  }
}

static void serviceIrInput() {
  HY_M302::IrNecFrame frame;

  while (shield.readIrNecAsync(frame)) {
    if (frame.repeat) {
      continue;
    }

    const Key remoteKey =
      decode(
        frame.address,
        frame.command
      );

    if (remoteKey == KEY_NONE) {
      continue;
    }

    queueRemoteKey(remoteKey);
  }
}

// -----------------------------------------------------------------------------
// TTY / ANSI keyboard decoder
// -----------------------------------------------------------------------------

static TtyState ttyState =
  TTY_NORMAL;

static unsigned long ttyStateMs = 0;
static uint16_t csiParam = 0;
static bool csiHaveParam = false;
static bool lastWasCR = false;

static void resetTtyState() {
  ttyState = TTY_NORMAL;
  csiParam = 0;
  csiHaveParam = false;
}

static void processCsiFinal(
  uint8_t c
) {
  KscSpecialKey key =
    KSC_KEY_NONE;

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
    queueKey(
      KSC_SRC_TTY,
      key
    );
  }
}

static void handleNormalTtyByte(
  uint8_t c
) {
  if (c == 0x1B) {
    ttyState = TTY_ESC;
    ttyStateMs = millis();
    return;
  }

  if (c == '\r') {
    queueKey(
      KSC_SRC_TTY,
      KSC_KEY_ENTER
    );

    lastWasCR = true;
    return;
  }

  if (c == '\n') {
    if (lastWasCR) {
      lastWasCR = false;
      return;
    }

    queueKey(
      KSC_SRC_TTY,
      KSC_KEY_ENTER
    );

    return;
  }

  lastWasCR = false;

  if (c == 0x08 ||
      c == 0x7F) {
    queueKey(
      KSC_SRC_TTY,
      KSC_KEY_BACK
    );

    return;
  }

  if (c >= 0x20 &&
      c <= 0x7E) {
    queueChar(
      KSC_SRC_TTY,
      c
    );
  }
}

static void serviceTtyInput() {
  while (Serial.available()) {
    const uint8_t c =
      (uint8_t)Serial.read();

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

      queueKey(
        KSC_SRC_TTY,
        KSC_KEY_BACK
      );

      resetTtyState();
      handleNormalTtyByte(c);
      continue;
    }

    if (ttyState == TTY_CSI) {
      ttyStateMs = millis();

      if (c >= '0' &&
          c <= '9') {
        csiHaveParam = true;

        csiParam =
          (uint16_t)(
            csiParam * 10U +
            (c - '0')
          );

        continue;
      }

      if (c == ';') {
        continue;
      }

      processCsiFinal(c);
      continue;
    }

    if (ttyState == TTY_SS3) {
      resetTtyState();

      switch (c) {
        case 'A':
          queueKey(
            KSC_SRC_TTY,
            KSC_KEY_UP
          );
          break;

        case 'B':
          queueKey(
            KSC_SRC_TTY,
            KSC_KEY_DOWN
          );
          break;

        case 'C':
          queueKey(
            KSC_SRC_TTY,
            KSC_KEY_RIGHT
          );
          break;

        case 'D':
          queueKey(
            KSC_SRC_TTY,
            KSC_KEY_LEFT
          );
          break;

        case 'H':
          queueKey(
            KSC_SRC_TTY,
            KSC_KEY_HOME
          );
          break;

        default:
          break;
      }
    }
  }

  if (ttyState != TTY_NORMAL &&
      millis() - ttyStateMs >= 80UL) {
    if (ttyState == TTY_ESC) {
      queueKey(
        KSC_SRC_TTY,
        KSC_KEY_BACK
      );
    }

    resetTtyState();
  }
}

// -----------------------------------------------------------------------------
// Shell
// -----------------------------------------------------------------------------

static char shellCwd[PATH_SIZE] = "/";
static char shellLine[SHELL_LINE_SIZE];
static uint8_t shellLineLen = 0;

static bool commanderActive = false;

static void shellPrompt() {
  Serial.print(F("KSC:"));
  Serial.print(shellCwd);
  Serial.print(F("> "));
}

static void shellHelp() {
  Serial.println(F("KSC                   open ANSI Commander"));
  Serial.println(F("HELP                  command list"));
  Serial.println(F("PWD                   current virtual path"));
  Serial.println(F("LS [path]             list virtual directory"));
  Serial.println(F("CD <path>             change virtual directory"));
  Serial.println(F("CAT <path>            read virtual node"));
  Serial.println(F("WRITE <path> <value>  write virtual node"));
  Serial.println(F("MEM                   free SRAM"));
}

static void shellWriteCommand(
  char *args
) {
  args = skipSpaces(args);

  if (!*args) {
    Serial.println(F("ERR PATH"));
    return;
  }

  char *valueText = args;

  while (*valueText &&
         *valueText != ' ' &&
         *valueText != '\t') {
    ++valueText;
  }

  if (!*valueText) {
    Serial.println(F("ERR VALUE"));
    return;
  }

  *valueText++ = 0;
  valueText = skipSpaces(valueText);

  char path[PATH_SIZE];

  makePath(
    shellCwd,
    args,
    path,
    sizeof(path)
  );

  long value = 0;

  if (!parseLong(
        valueText,
        value
      )) {
    Serial.println(F("ERR VALUE"));
    return;
  }

  Serial.println(
    vfsResultName(
      vfsWrite(
        path,
        value
      )
    )
  );
}

// Commander declarations used by shell.
static void enterCommander();

static void executeShellLine(
  char *line
) {
  line = skipSpaces(line);

  if (!*line) {
    return;
  }

  upperCommand(line);

  char *args =
    splitCommand(line);

  if (strcmp(line, "HELP") == 0) {
    shellHelp();
    return;
  }

  if (strcmp(line, "KSC") == 0) {
    enterCommander();
    return;
  }

  if (strcmp(line, "PWD") == 0) {
    Serial.println(shellCwd);
    return;
  }

  if (strcmp(line, "MEM") == 0) {
    Serial.println(freeRam());
    return;
  }

  if (strcmp(line, "LS") == 0) {
    char path[PATH_SIZE];

    makePath(
      shellCwd,
      args,
      path,
      sizeof(path)
    );

    vfsListShell(path);
    return;
  }

  if (strcmp(line, "CD") == 0) {
    char path[PATH_SIZE];

    makePath(
      shellCwd,
      args,
      path,
      sizeof(path)
    );

    if (!isDirectoryPath(path)) {
      Serial.println(F("ERR NOT_DIR"));
      return;
    }

    strncpy(
      shellCwd,
      path,
      sizeof(shellCwd)
    );

    shellCwd[
      sizeof(shellCwd) - 1
    ] = 0;

    Serial.println(F("OK"));
    return;
  }

  if (strcmp(line, "CAT") == 0) {
    if (!*args) {
      Serial.println(F("ERR PATH"));
      return;
    }

    char path[PATH_SIZE];
    char value[VALUE_SIZE];

    makePath(
      shellCwd,
      args,
      path,
      sizeof(path)
    );

    const VfsResult result =
      vfsRead(
        path,
        value,
        sizeof(value)
      );

    if (result == VFS_OK) {
      Serial.println(value);
    } else {
      Serial.println(
        vfsResultName(result)
      );
    }

    return;
  }

  if (strcmp(line, "WRITE") == 0) {
    shellWriteCommand(args);
    return;
  }

  Serial.println(F("ERR UNKNOWN_COMMAND"));
}

// -----------------------------------------------------------------------------
// ANSI renderer
// -----------------------------------------------------------------------------

static void ansiClear() {
  Serial.print(
    F("\x1B[0m\x1B[2J\x1B[H")
  );
}

static void ansiHideCursor() {
  Serial.print(F("\x1B[?25l"));
}

static void ansiShowCursor() {
  Serial.print(F("\x1B[?25h"));
}

static void ansiInverseOn() {
  Serial.print(F("\x1B[7m"));
}

static void ansiNormal() {
  Serial.print(F("\x1B[0m"));
}

// -----------------------------------------------------------------------------
// Commander state
// -----------------------------------------------------------------------------

static CommanderView commanderView =
  CMD_VIEW_DIR;

static CommanderView helpReturnView =
  CMD_VIEW_DIR;

static char commanderPath[PATH_SIZE] = "/";
static char commanderNodePath[PATH_SIZE] = "";

static uint8_t commanderSelected = 0;

static bool nodeEditActive = false;
static uint16_t nodeEditValue = 0;
static NodeMessage nodeMessage =
  NODE_MSG_NONE;

static unsigned long commanderLastRenderMs = 0;

static void commanderRender();

static void commanderSetRoot() {
  strcpy(commanderPath, "/");
  commanderSelected = 0;
  commanderView = CMD_VIEW_DIR;
  nodeEditActive = false;
  nodeMessage = NODE_MSG_NONE;
}

static void commanderOpenSelected() {
  const uint8_t count =
    vfsDirCount(commanderPath);

  if (count == 0 ||
      commanderSelected >= count) {
    return;
  }

  char name[12];

  copyFlashText(
    name,
    sizeof(name),
    vfsDirEntryName(
      commanderPath,
      commanderSelected
    )
  );

  char child[PATH_SIZE];

  joinPath(
    commanderPath,
    name,
    child,
    sizeof(child)
  );

  const VfsNodeType type =
    vfsPathType(child);

  if (type == VFS_DIR) {
    strncpy(
      commanderPath,
      child,
      sizeof(commanderPath)
    );

    commanderPath[
      sizeof(commanderPath) - 1
    ] = 0;

    commanderSelected = 0;
    commanderView = CMD_VIEW_DIR;
    nodeMessage = NODE_MSG_NONE;
    commanderRender();
    return;
  }

  if (type == VFS_RO ||
      type == VFS_RW) {
    strncpy(
      commanderNodePath,
      child,
      sizeof(commanderNodePath)
    );

    commanderNodePath[
      sizeof(commanderNodePath) - 1
    ] = 0;

    commanderView = CMD_VIEW_NODE;
    nodeEditActive = false;
    nodeMessage = NODE_MSG_NONE;
    commanderRender();
  }
}

static void commanderParent() {
  if (strcmp(
        commanderPath,
        "/"
      ) == 0) {
    return;
  }

  char parent[PATH_SIZE];

  parentPath(
    commanderPath,
    parent,
    sizeof(parent)
  );

  strncpy(
    commanderPath,
    parent,
    sizeof(commanderPath)
  );

  commanderPath[
    sizeof(commanderPath) - 1
  ] = 0;

  commanderSelected = 0;
  commanderView = CMD_VIEW_DIR;
  nodeMessage = NODE_MSG_NONE;
  commanderRender();
}

static void commanderShowHelp() {
  helpReturnView =
    commanderView;

  commanderView =
    CMD_VIEW_HELP;

  commanderRender();
}

static void commanderCloseHelp() {
  commanderView =
    helpReturnView;

  commanderRender();
}

static void exitCommander() {
  commanderActive = false;

  if (isDirectoryPath(
        commanderPath
      )) {
    strncpy(
      shellCwd,
      commanderPath,
      sizeof(shellCwd)
    );

    shellCwd[
      sizeof(shellCwd) - 1
    ] = 0;
  }

  ansiNormal();
  ansiShowCursor();
  ansiClear();

  Serial.println(
    F("KSC Commander closed.")
  );

  Serial.print(F("FREE RAM: "));
  Serial.print(freeRam());
  Serial.println(F(" B"));

  shellPrompt();
}

static void enterCommander() {
  commanderActive = true;

  if (isDirectoryPath(shellCwd)) {
    strncpy(
      commanderPath,
      shellCwd,
      sizeof(commanderPath)
    );

    commanderPath[
      sizeof(commanderPath) - 1
    ] = 0;
  } else {
    strcpy(commanderPath, "/");
  }

  commanderSelected = 0;
  commanderView = CMD_VIEW_DIR;
  nodeEditActive = false;
  nodeMessage = NODE_MSG_NONE;

  ansiHideCursor();
  commanderRender();
}

// -----------------------------------------------------------------------------
// Commander rendering
// -----------------------------------------------------------------------------

static void renderHeader() {
  Serial.println(
    F("KSC 0.2 - KonSol Commander")
  );

  Serial.println(
    F("Arduino UNO + HY-M302")
  );

  Serial.print(F("Path: "));
}

static void renderDirectory() {
  ansiClear();
  renderHeader();

  Serial.println(commanderPath);
  Serial.println(
    F("----------------------------------------")
  );

  const uint8_t count =
    vfsDirCount(commanderPath);

  for (uint8_t i = 0; i < count; ++i) {
    const VfsNodeType type =
      vfsDirEntryType(
        commanderPath,
        i
      );

    if (i == commanderSelected) {
      ansiInverseOn();
    }

    Serial.print(
      i == commanderSelected
        ? F("> ")
        : F("  ")
    );

    if (type == VFS_DIR) {
      Serial.print(F("["));
      Serial.print(
        vfsDirEntryName(
          commanderPath,
          i
        )
      );

      Serial.print(F("/]"));
    } else {
      Serial.print(
        vfsDirEntryName(
          commanderPath,
          i
        )
      );

      if (type == VFS_RO) {
        Serial.print(F("  [RO]"));
      } else if (type == VFS_RW) {
        Serial.print(F("  [RW]"));
      }
    }

    if (i == commanderSelected) {
      ansiNormal();
    }

    Serial.println();
  }

  Serial.println(
    F("----------------------------------------")
  );

  Serial.println(
    F("UP/DOWN Select   ENTER/RIGHT Open")
  );

  Serial.println(
    F("LEFT/BACK Parent HOME Root")
  );

  Serial.println(
    F("F9/MENU Help     F10/POWER/Q Shell")
  );

  Serial.print(F("RAM "));
  Serial.print(freeRam());
  Serial.print(F(" B"));

  Serial.print(F("   IR drop "));
  Serial.print(
    shield.irNecDroppedEdges()
  );

  Serial.print('/');
  Serial.println(
    shield.irNecDroppedFrames()
  );
}

static void renderNode() {
  ansiClear();
  renderHeader();

  Serial.println(commanderNodePath);
  Serial.println(
    F("----------------------------------------")
  );

  const VfsNodeType type =
    vfsPathType(commanderNodePath);

  Serial.print(F("Type: "));

  Serial.println(
    type == VFS_RW
      ? F("RW")
      : F("RO")
  );

  char value[VALUE_SIZE];

  const VfsResult result =
    vfsRead(
      commanderNodePath,
      value,
      sizeof(value)
    );

  Serial.print(F("Value: "));

  if (result == VFS_OK) {
    Serial.println(value);
  } else {
    Serial.println(
      vfsResultName(result)
    );
  }

  if (type == VFS_RW) {
    uint16_t minValue = 0;
    uint16_t maxValue = 0;

    if (vfsWritableRange(
          commanderNodePath,
          minValue,
          maxValue
        )) {
      Serial.print(F("Range: "));
      Serial.print(minValue);
      Serial.print(F(".."));
      Serial.println(maxValue);
    }

    Serial.print(F("Edit: "));

    if (nodeEditActive) {
      Serial.print(nodeEditValue);
      Serial.println(F("_"));
    } else {
      Serial.println(F("-"));
    }

    switch (nodeMessage) {
      case NODE_MSG_APPLIED:
        Serial.println(F("Status: APPLIED"));
        break;

      case NODE_MSG_RANGE:
        Serial.println(F("Status: RANGE ERROR"));
        break;

      case NODE_MSG_CANCELED:
        Serial.println(F("Status: EDIT CANCELED"));
        break;

      default:
        Serial.println(F("Status: READY"));
        break;
    }

    Serial.println();
    Serial.println(
      F("0..9 Exact value   ENTER Apply/Refresh")
    );

    Serial.println(
      F("LEFT/RIGHT -/+     BACK Cancel/Return")
    );
  } else {
    Serial.println();
    Serial.println(
      F("ENTER Refresh      LEFT/BACK Return")
    );
  }

  Serial.println(
    F("HOME Root          F9/MENU Help")
  );

  Serial.println(
    F("F10/POWER/Q Shell")
  );

  Serial.println(
    F("----------------------------------------")
  );

  Serial.print(F("RAM "));
  Serial.print(freeRam());
  Serial.println(F(" B"));
}

static void renderHelp() {
  ansiClear();

  Serial.println(
    F("KSC 0.2 - KonSol Commander HELP")
  );

  Serial.println(
    F("----------------------------------------")
  );

  Serial.println(
    F("DIR:")
  );

  Serial.println(
    F("  UP/DOWN       select entry")
  );

  Serial.println(
    F("  ENTER/RIGHT   open")
  );

  Serial.println(
    F("  LEFT/BACK     parent")
  );

  Serial.println(
    F("  HOME          root")
  );

  Serial.println();

  Serial.println(
    F("RW NODE:")
  );

  Serial.println(
    F("  0..9          enter exact value")
  );

  Serial.println(
    F("  ENTER         apply typed value")
  );

  Serial.println(
    F("  LEFT/RIGHT    decrement/increment")
  );

  Serial.println(
    F("  BACK          cancel edit / return")
  );

  Serial.println();

  Serial.println(
    F("GLOBAL:")
  );

  Serial.println(
    F("  F9 / IR MENU       help")
  );

  Serial.println(
    F("  F10 / IR POWER / Q shell")
  );

  Serial.println(
    F("----------------------------------------")
  );

  Serial.println(
    F("F9/MENU, ENTER or BACK -> return")
  );
}

static void commanderRender() {
  commanderLastRenderMs =
    millis();

  switch (commanderView) {
    case CMD_VIEW_DIR:
      renderDirectory();
      break;

    case CMD_VIEW_NODE:
      renderNode();
      break;

    case CMD_VIEW_HELP:
      renderHelp();
      break;
  }
}

// -----------------------------------------------------------------------------
// Commander input handling
// -----------------------------------------------------------------------------

static void commanderMoveSelection(
  int8_t delta
) {
  const uint8_t count =
    vfsDirCount(commanderPath);

  if (count == 0) {
    commanderSelected = 0;
    return;
  }

  if (delta < 0) {
    if (commanderSelected == 0) {
      commanderSelected =
        count - 1;
    } else {
      --commanderSelected;
    }
  } else {
    ++commanderSelected;

    if (commanderSelected >= count) {
      commanderSelected = 0;
    }
  }

  commanderRender();
}

static void commanderAdjustWritable(
  int8_t delta
) {
  if (vfsPathType(
        commanderNodePath
      ) != VFS_RW) {
    return;
  }

  long current = 0;

  if (!vfsReadWritableLong(
        commanderNodePath,
        current
      )) {
    return;
  }

  uint16_t minValue = 0;
  uint16_t maxValue = 0;

  if (!vfsWritableRange(
        commanderNodePath,
        minValue,
        maxValue
      )) {
    return;
  }

  long next =
    current + delta;

  if (next < minValue) {
    next = minValue;
  }

  if (next > maxValue) {
    next = maxValue;
  }

  const VfsResult result =
    vfsWrite(
      commanderNodePath,
      next
    );

  nodeEditActive = false;

  nodeMessage =
    result == VFS_OK
      ? NODE_MSG_APPLIED
      : NODE_MSG_RANGE;

  commanderRender();
}

static void commanderAppendDigit(
  uint8_t digit
) {
  if (vfsPathType(
        commanderNodePath
      ) != VFS_RW) {
    return;
  }

  uint16_t minValue = 0;
  uint16_t maxValue = 0;

  if (!vfsWritableRange(
        commanderNodePath,
        minValue,
        maxValue
      )) {
    return;
  }

  const uint16_t next =
    nodeEditActive
      ? (uint16_t)(
          nodeEditValue * 10U +
          digit
        )
      : digit;

  if (next > maxValue) {
    nodeMessage =
      NODE_MSG_RANGE;

    commanderRender();
    return;
  }

  nodeEditValue = next;
  nodeEditActive = true;
  nodeMessage = NODE_MSG_NONE;

  commanderRender();
}

static void commanderApplyEdit() {
  if (!nodeEditActive) {
    commanderRender();
    return;
  }

  const VfsResult result =
    vfsWrite(
      commanderNodePath,
      nodeEditValue
    );

  nodeEditActive = false;

  nodeMessage =
    result == VFS_OK
      ? NODE_MSG_APPLIED
      : NODE_MSG_RANGE;

  commanderRender();
}

static void commanderBackFromNode() {
  if (nodeEditActive) {
    nodeEditActive = false;
    nodeMessage = NODE_MSG_CANCELED;
    commanderRender();
    return;
  }

  commanderView = CMD_VIEW_DIR;
  nodeMessage = NODE_MSG_NONE;
  commanderRender();
}

static void handleCommanderChar(
  uint8_t c
) {
  if (c == 'q' ||
      c == 'Q') {
    exitCommander();
    return;
  }

  if (commanderView ==
        CMD_VIEW_HELP) {
    return;
  }

  if (commanderView ==
        CMD_VIEW_NODE &&
      c >= '0' &&
      c <= '9') {
    commanderAppendDigit(
      (uint8_t)(c - '0')
    );
  }
}

static void handleCommanderKey(
  KscSpecialKey key
) {
  if (key == KSC_KEY_POWER) {
    exitCommander();
    return;
  }

  if (commanderView ==
        CMD_VIEW_HELP) {
    if (key == KSC_KEY_MENU ||
        key == KSC_KEY_ENTER ||
        key == KSC_KEY_BACK) {
      commanderCloseHelp();
    } else if (key == KSC_KEY_HOME) {
      commanderSetRoot();
      commanderRender();
    }

    return;
  }

  if (key == KSC_KEY_MENU) {
    commanderShowHelp();
    return;
  }

  if (key == KSC_KEY_HOME) {
    commanderSetRoot();
    commanderRender();
    return;
  }

  if (commanderView ==
        CMD_VIEW_DIR) {
    switch (key) {
      case KSC_KEY_UP:
        commanderMoveSelection(-1);
        break;

      case KSC_KEY_DOWN:
        commanderMoveSelection(1);
        break;

      case KSC_KEY_ENTER:
      case KSC_KEY_RIGHT:
        commanderOpenSelected();
        break;

      case KSC_KEY_LEFT:
      case KSC_KEY_BACK:
        commanderParent();
        break;

      case KSC_KEY_END: {
        const uint8_t count =
          vfsDirCount(
            commanderPath
          );

        if (count) {
          commanderSelected =
            count - 1;

          commanderRender();
        }

        break;
      }

      default:
        break;
    }

    return;
  }

  if (commanderView ==
        CMD_VIEW_NODE) {
    const VfsNodeType type =
      vfsPathType(
        commanderNodePath
      );

    switch (key) {
      case KSC_KEY_ENTER:
        if (type == VFS_RW) {
          commanderApplyEdit();
        } else {
          commanderRender();
        }
        break;

      case KSC_KEY_BACK:
        commanderBackFromNode();
        break;

      case KSC_KEY_LEFT:
        if (type == VFS_RW) {
          commanderAdjustWritable(-1);
        } else {
          commanderBackFromNode();
        }
        break;

      case KSC_KEY_RIGHT:
        if (type == VFS_RW) {
          commanderAdjustWritable(1);
        } else {
          commanderRender();
        }
        break;

      case KSC_KEY_DELETE:
        if (nodeEditActive) {
          nodeEditActive = false;
          nodeMessage =
            NODE_MSG_CANCELED;

          commanderRender();
        }
        break;

      default:
        break;
    }
  }
}

// -----------------------------------------------------------------------------
// Unified event consumer
// -----------------------------------------------------------------------------

static void handleShellEvent(
  const KscInputEvent &event
) {
  if (event.type ==
        KSC_INPUT_CHAR) {
    const uint8_t c =
      event.code;

    if (c >= 0x20 &&
        c <= 0x7E &&
        shellLineLen <
          SHELL_LINE_SIZE - 1) {
      shellLine[
        shellLineLen++
      ] = (char)c;

      Serial.write(c);
    }

    return;
  }

  const KscSpecialKey key =
    (KscSpecialKey)event.code;

  if (key == KSC_KEY_ENTER) {
    Serial.println();

    shellLine[
      shellLineLen
    ] = 0;

    executeShellLine(shellLine);

    shellLineLen = 0;

    if (!commanderActive) {
      shellPrompt();
    }

    return;
  }

  if (key == KSC_KEY_BACK) {
    if (shellLineLen) {
      --shellLineLen;
      Serial.print(F("\b \b"));
    }

    return;
  }
}

static void serviceInputEvents() {
  while (eventCount != 0) {
    KscInputEvent inputEvent =
      eventQueue[eventTail];

    eventTail =
      (uint8_t)((eventTail + 1) %
                EVENT_QUEUE_SIZE);

    --eventCount;

    if (commanderActive) {
      if (inputEvent.type ==
            KSC_INPUT_CHAR) {
        handleCommanderChar(
          inputEvent.code
        );
      } else {
        handleCommanderKey(
          (KscSpecialKey)inputEvent.code
        );
      }
    } else {
      handleShellEvent(inputEvent);
    }
  }
}

// -----------------------------------------------------------------------------
// Periodic Commander refresh
// -----------------------------------------------------------------------------

static void serviceCommanderRefresh() {
  if (!commanderActive ||
      commanderView !=
        CMD_VIEW_NODE ||
      nodeEditActive) {
    return;
  }

  if (!vfsPathIsDynamic(
        commanderNodePath
      )) {
    return;
  }

  if (millis() -
        commanderLastRenderMs >=
      1000UL) {
    commanderRender();
  }
}

// -----------------------------------------------------------------------------
// Setup / loop
// -----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  shield.begin();

  shield.ledRed(false);
  shield.ledBlue(false);
  shield.rgbOff();
  shield.buzzerOff();

  delay(100);

  Serial.println();
  Serial.println(F("KSC 0.2"));
  Serial.println(
    F("KonSol Commander - ANSI VFS Navigator")
  );

  Serial.println(
    F("Arduino UNO / ATmega328P + HY-M302")
  );

  if (!shield.beginIrNecAsync()) {
    Serial.println(F("IR INIT: FAIL"));
  } else {
    Serial.println(F("IR INIT: OK"));
  }

  Serial.print(F("FREE RAM: "));
  Serial.print(freeRam());
  Serial.println(F(" B"));

  Serial.println(
    F("Storage: virtual namespace only")
  );

  Serial.println(
    F("Type KSC to open Commander, HELP for shell commands.")
  );

  Serial.println();

  shellPrompt();
}

void loop() {
  shield.service();

  serviceTtyInput();
  serviceIrInput();
  serviceInputEvents();
  serviceCommanderRefresh();

  if (eventDrops != 0 &&
      !commanderActive) {
    Serial.print(F("\r\nINPUT DROPS: "));
    Serial.println(eventDrops);

    eventDrops = 0;
    shellPrompt();
  }
}
