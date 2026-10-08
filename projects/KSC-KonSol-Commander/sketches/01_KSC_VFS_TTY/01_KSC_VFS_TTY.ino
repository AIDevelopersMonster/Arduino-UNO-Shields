#include <Arduino.h>
#include <HY_M302.h>

extern unsigned int __heap_start;
extern void *__brkval;

HY_M302 shield;

static const uint8_t CMD_SIZE = 88;
static char cmdLine[CMD_SIZE];
static uint8_t cmdLen = 0;

static char cwd[16] = "/";

static bool ledRedState = false;
static bool ledBlueState = false;
static bool buzzerState = false;

static uint8_t rgbR = 0;
static uint8_t rgbG = 0;
static uint8_t rgbB = 0;

static HY_M302::DhtReading dhtCache;
static unsigned long dhtLastMs = 0;
static bool dhtHaveValue = false;

static int freeRam() {
  int v;
  return (int)&v -
         (__brkval == 0
            ? (int)&__heap_start
            : (int)__brkval);
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

static void prompt() {
  Serial.print(F("KSC:"));
  Serial.print(cwd);
  Serial.print(F("> "));
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

static void parentPath(const char *path, char *out, uint8_t outSize) {
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

static void makePath(const char *arg, char *out, uint8_t outSize) {
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

  if (strcmp(cwd, "/") == 0) {
    out[0] = '/';
    strncpy(out + 1, arg, outSize - 1);
    out[outSize - 1] = 0;
    return;
  }

  strncpy(out, cwd, outSize);
  out[outSize - 1] = 0;

  size_t n = strlen(out);

  if (n + 1 < outSize) {
    out[n++] = '/';
    out[n] = 0;
  }

  if (n < outSize - 1) {
    strncat(out, arg, outSize - n - 1);
  }
}

static void refreshDht() {
  unsigned long now = millis();

  if (dhtHaveValue && (now - dhtLastMs) < 2000UL) {
    return;
  }

  HY_M302::DhtReading r = shield.readDht11();

  dhtCache = r;
  dhtLastMs = millis();
  dhtHaveValue = r.ok;
}

static void listPath(const char *path) {
  if (strcmp(path, "/") == 0) {
    Serial.println(F("D dev"));
    Serial.println(F("D proc"));
    Serial.println(F("D sys"));
    return;
  }

  if (strcmp(path, "/dev") == 0) {
    Serial.println(F("F sw1          RO"));
    Serial.println(F("F sw2          RO"));
    Serial.println(F("F pot          RO"));
    Serial.println(F("F light        RO"));
    Serial.println(F("D dht"));
    Serial.println(F("D led"));
    Serial.println(F("D rgb"));
    Serial.println(F("F buzzer       RW"));
    return;
  }

  if (strcmp(path, "/dev/dht") == 0) {
    Serial.println(F("F temp         RO"));
    Serial.println(F("F humidity     RO"));
    Serial.println(F("F status       RO"));
    Serial.println(F("F age          RO"));
    return;
  }

  if (strcmp(path, "/dev/led") == 0) {
    Serial.println(F("F red          RW"));
    Serial.println(F("F blue         RW"));
    return;
  }

  if (strcmp(path, "/dev/rgb") == 0) {
    Serial.println(F("F red          RW"));
    Serial.println(F("F green        RW"));
    Serial.println(F("F blue         RW"));
    return;
  }

  if (strcmp(path, "/proc") == 0) {
    Serial.println(F("F mem          RO"));
    Serial.println(F("F uptime       RO"));
    return;
  }

  if (strcmp(path, "/sys") == 0) {
    Serial.println(F("F version      RO"));
    Serial.println(F("F target       RO"));
    Serial.println(F("F storage      RO"));
    return;
  }

  Serial.println(F("ERR NOT_DIR"));
}

static void catPath(const char *path) {
  if (isDirectoryPath(path)) {
    Serial.println(F("ERR IS_DIR"));
    return;
  }

  if (strcmp(path, "/dev/sw1") == 0) {
    Serial.println(shield.button1Pressed() ? 1 : 0);
    return;
  }

  if (strcmp(path, "/dev/sw2") == 0) {
    Serial.println(shield.button2Pressed() ? 1 : 0);
    return;
  }

  if (strcmp(path, "/dev/pot") == 0) {
    Serial.println(shield.readPotRaw());
    return;
  }

  if (strcmp(path, "/dev/light") == 0) {
    Serial.println(shield.readLightRaw());
    return;
  }

  if (strcmp(path, "/dev/dht/temp") == 0) {
    refreshDht();

    if (!dhtHaveValue) {
      Serial.println(F("ERR DHT"));
      return;
    }

    Serial.println(dhtCache.temperatureC, 1);
    return;
  }

  if (strcmp(path, "/dev/dht/humidity") == 0) {
    refreshDht();

    if (!dhtHaveValue) {
      Serial.println(F("ERR DHT"));
      return;
    }

    Serial.println(dhtCache.humidity, 1);
    return;
  }

  if (strcmp(path, "/dev/dht/status") == 0) {
    refreshDht();
    Serial.println(dhtHaveValue ? F("OK") : F("ERROR"));
    return;
  }

  if (strcmp(path, "/dev/dht/age") == 0) {
    if (!dhtLastMs) {
      Serial.println(F("NEVER"));
    } else {
      Serial.println(millis() - dhtLastMs);
    }
    return;
  }

  if (strcmp(path, "/dev/led/red") == 0) {
    Serial.println(ledRedState ? 1 : 0);
    return;
  }

  if (strcmp(path, "/dev/led/blue") == 0) {
    Serial.println(ledBlueState ? 1 : 0);
    return;
  }

  if (strcmp(path, "/dev/rgb/red") == 0) {
    Serial.println(rgbR);
    return;
  }

  if (strcmp(path, "/dev/rgb/green") == 0) {
    Serial.println(rgbG);
    return;
  }

  if (strcmp(path, "/dev/rgb/blue") == 0) {
    Serial.println(rgbB);
    return;
  }

  if (strcmp(path, "/dev/buzzer") == 0) {
    Serial.println(buzzerState ? 1 : 0);
    return;
  }

  if (strcmp(path, "/proc/mem") == 0) {
    Serial.println(freeRam());
    return;
  }

  if (strcmp(path, "/proc/uptime") == 0) {
    Serial.println(millis());
    return;
  }

  if (strcmp(path, "/sys/version") == 0) {
    Serial.println(F("KSC 0.1"));
    return;
  }

  if (strcmp(path, "/sys/target") == 0) {
    Serial.println(F("Arduino UNO + HY-M302"));
    return;
  }

  if (strcmp(path, "/sys/storage") == 0) {
    Serial.println(F("VIRTUAL ONLY"));
    return;
  }

  Serial.println(F("ERR NOT_FOUND"));
}

static bool parseLong(const char *s, long &value) {
  if (!s || !*s) return false;

  char *endp = nullptr;
  value = strtol(s, &endp, 10);

  return endp && *endp == 0;
}

static void writePath(char *args) {
  args = skipSpaces(args);

  if (!*args) {
    Serial.println(F("ERR PATH"));
    return;
  }

  char *valueText = args;

  while (*valueText && *valueText != ' ' && *valueText != '\t') {
    ++valueText;
  }

  if (!*valueText) {
    Serial.println(F("ERR VALUE"));
    return;
  }

  *valueText++ = 0;
  valueText = skipSpaces(valueText);

  char path[32];
  makePath(args, path, sizeof(path));

  long value;

  if (!parseLong(valueText, value)) {
    Serial.println(F("ERR VALUE"));
    return;
  }

  if (strcmp(path, "/dev/led/red") == 0) {
    if (value < 0 || value > 1) {
      Serial.println(F("ERR RANGE"));
      return;
    }

    ledRedState = value != 0;
    shield.ledRed(ledRedState);
    Serial.println(F("OK"));
    return;
  }

  if (strcmp(path, "/dev/led/blue") == 0) {
    if (value < 0 || value > 1) {
      Serial.println(F("ERR RANGE"));
      return;
    }

    ledBlueState = value != 0;
    shield.ledBlue(ledBlueState);
    Serial.println(F("OK"));
    return;
  }

  if (strcmp(path, "/dev/buzzer") == 0) {
    if (value < 0 || value > 1) {
      Serial.println(F("ERR RANGE"));
      return;
    }

    buzzerState = value != 0;

    if (buzzerState) {
      shield.buzzerOn();
    } else {
      shield.buzzerOff();
    }

    Serial.println(F("OK"));
    return;
  }

  if (strcmp(path, "/dev/rgb/red") == 0 ||
      strcmp(path, "/dev/rgb/green") == 0 ||
      strcmp(path, "/dev/rgb/blue") == 0) {

    if (value < 0 || value > 255) {
      Serial.println(F("ERR RANGE"));
      return;
    }

    if (strcmp(path, "/dev/rgb/red") == 0) {
      rgbR = (uint8_t)value;
    } else if (strcmp(path, "/dev/rgb/green") == 0) {
      rgbG = (uint8_t)value;
    } else {
      rgbB = (uint8_t)value;
    }

    shield.setRGB(rgbR, rgbG, rgbB);

    Serial.println(F("OK"));
    return;
  }

  Serial.println(F("ERR READ_ONLY_OR_NOT_FOUND"));
}

static void cmdHelp() {
  Serial.println(F("HELP                  command list"));
  Serial.println(F("PWD                   current virtual path"));
  Serial.println(F("LS [path]             list virtual directory"));
  Serial.println(F("CD <path>             change virtual directory"));
  Serial.println(F("CAT <path>            read virtual node"));
  Serial.println(F("WRITE <path> <value>  write virtual node"));
  Serial.println(F("MEM                   free SRAM"));
}

static void executeCommand(char *line) {
  line = skipSpaces(line);

  if (!*line) return;

  upperCommand(line);

  char *args = splitCommand(line);

  if (strcmp(line, "HELP") == 0) {
    cmdHelp();
    return;
  }

  if (strcmp(line, "PWD") == 0) {
    Serial.println(cwd);
    return;
  }

  if (strcmp(line, "MEM") == 0) {
    Serial.println(freeRam());
    return;
  }

  if (strcmp(line, "LS") == 0) {
    char path[32];
    makePath(args, path, sizeof(path));
    listPath(path);
    return;
  }

  if (strcmp(line, "CD") == 0) {
    char path[32];
    makePath(args, path, sizeof(path));

    if (!isDirectoryPath(path)) {
      Serial.println(F("ERR NOT_DIR"));
      return;
    }

    strncpy(cwd, path, sizeof(cwd));
    cwd[sizeof(cwd) - 1] = 0;

    Serial.println(F("OK"));
    return;
  }

  if (strcmp(line, "CAT") == 0) {
    if (!*args) {
      Serial.println(F("ERR PATH"));
      return;
    }

    char path[32];
    makePath(args, path, sizeof(path));
    catPath(path);
    return;
  }

  if (strcmp(line, "WRITE") == 0) {
    writePath(args);
    return;
  }

  Serial.println(F("ERR UNKNOWN_COMMAND"));
}

static void serviceSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      cmdLine[cmdLen] = 0;

      executeCommand(cmdLine);

      cmdLen = 0;
      prompt();
      continue;
    }

    if (cmdLen < CMD_SIZE - 1) {
      cmdLine[cmdLen++] = c;
    }
  }
}

void setup() {
  Serial.begin(115200);

  shield.begin();

  shield.ledRed(false);
  shield.ledBlue(false);
  shield.rgbOff();
  shield.buzzerOff();

  delay(200);

  Serial.println();
  Serial.println(F("KSC 0.1"));
  Serial.println(F("KonSol Commander - Virtual Device Filesystem"));
  Serial.println(F("Arduino UNO / ATmega328P + HY-M302"));
  Serial.println(F("Storage: virtual namespace only"));
  Serial.print(F("FREE RAM: "));
  Serial.print(freeRam());
  Serial.println(F(" B"));
  Serial.println(F("Type HELP"));
  Serial.println();

  prompt();
}

void loop() {
  shield.service();
  serviceSerial();
}
