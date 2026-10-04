/*
  KON-OS 0.1
  LAB-05 / TEST-01
  Arduino UNO R3 + MAR2406 microSD

  A deliberately small cooperative operating environment for ATmega328P.

  Design goals:
    - no FreeRTOS;
    - no dynamic allocation by KON-OS;
    - no Arduino String;
    - one shared stack;
    - fixed command buffers;
    - cooperative task dispatcher;
    - Serial shell;
    - SD filesystem commands.

  This is not POSIX and not a general-purpose OS. It is an embedded operating
  environment designed specifically for 32 KB Flash / 2 KB SRAM hardware.

  MAR2406 microSD:
    CS   = D10
    MOSI = D11
    MISO = D12
    SCK  = D13

  D13 is SPI SCK. Do not use LED_BUILTIN as an activity indicator.
*/

#include <SPI.h>
#include <SD.h>
#include <avr/wdt.h>

#define KONOS_VERSION "0.1"

const uint8_t SD_CS = 10;
const uint32_t SERIAL_BAUD = 115200UL;

const uint8_t CMD_SIZE = 88;

static char commandLine[CMD_SIZE];
static uint8_t commandLen = 0;
static bool sdReady = false;

// -----------------------------------------------------------------------------
// Cooperative kernel
// -----------------------------------------------------------------------------

typedef void (*TaskFunction)();

struct KernelTask {
  TaskFunction fn;
  uint16_t periodMs;
  uint32_t nextRun;
  uint32_t runs;
  bool enabled;
};

static void taskSerial();
static void taskClock();

static KernelTask tasks[] = {
  { taskSerial, 1,   0, 0, true },
  { taskClock,  100, 0, 0, true }
};

const uint8_t TASK_COUNT = sizeof(tasks) / sizeof(tasks[0]);

static uint32_t kernelTicks = 0;

static void kernelInit() {
  uint32_t now = millis();

  for (uint8_t i = 0; i < TASK_COUNT; ++i) {
    tasks[i].nextRun = now + tasks[i].periodMs;
  }
}

static void kernelDispatch() {
  uint32_t now = millis();

  for (uint8_t i = 0; i < TASK_COUNT; ++i) {
    KernelTask &t = tasks[i];

    if (!t.enabled) continue;

    if ((int32_t)(now - t.nextRun) >= 0) {
      // Advance from the previous deadline, not from "now".
      // This limits long-term drift in the cooperative scheduler.
      t.nextRun += t.periodMs;
      ++t.runs;
      t.fn();
    }
  }
}

static void taskClock() {
  ++kernelTicks;
}

// -----------------------------------------------------------------------------
// Memory diagnostics
// -----------------------------------------------------------------------------

extern int __heap_start;
extern void *__brkval;

static int freeRam() {
  int stackTop;
  int heapTop = (__brkval == 0) ? (int)&__heap_start : (int)__brkval;
  return (int)&stackTop - heapTop;
}

// -----------------------------------------------------------------------------
// Shell helpers
// -----------------------------------------------------------------------------

static char *skipSpaces(char *p) {
  while (*p == ' ' || *p == '\t') ++p;
  return p;
}

static void toUpperCommand(char *p) {
  while (*p && *p != ' ' && *p != '\t') {
    if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
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

static void printPrompt() {
  Serial.print(F("A:/> "));
}

static void printBanner() {
  Serial.println();
  Serial.println(F("KON-OS 0.1"));
  Serial.println(F("Arduino UNO / ATmega328P / 16 MHz"));
  Serial.println(F("32 KB FLASH / 2 KB SRAM"));
  Serial.println(F("Cooperative kernel + Serial shell + microSD"));
  Serial.println();
}

static bool mountSD() {
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  return SD.begin(SD_CS);
}

static void printSdStatus() {
  Serial.print(F("SD: "));
  Serial.println(sdReady ? F("READY") : F("NOT READY"));
}

static void cmdHelp() {
  Serial.println(F("HELP              command list"));
  Serial.println(F("INFO              system information"));
  Serial.println(F("MEM               free SRAM"));
  Serial.println(F("UPTIME            milliseconds since boot"));
  Serial.println(F("PS                cooperative task table"));
  Serial.println(F("MOUNT             initialize microSD"));
  Serial.println(F("DIR [path]        list directory"));
  Serial.println(F("TYPE <file>       print text file"));
  Serial.println(F("WRITE <file> txt  replace file with text"));
  Serial.println(F("APPEND <file> txt append text"));
  Serial.println(F("DEL <file>        delete file"));
  Serial.println(F("MKDIR <path>      create directory"));
  Serial.println(F("RMDIR <path>      remove empty directory"));
  Serial.println(F("CLS               clear ANSI terminal"));
  Serial.println(F("REBOOT            watchdog reset"));
}

static void cmdInfo() {
  Serial.println(F("KON-OS 0.1"));
  Serial.println(F("CPU: ATmega328P @ 16 MHz"));
  Serial.println(F("FLASH: 32 KB"));
  Serial.println(F("SRAM: 2 KB"));
  Serial.println(F("SCHED: cooperative"));
  Serial.print(F("TASKS: "));
  Serial.println(TASK_COUNT);
  printSdStatus();
  Serial.print(F("FREE RAM: "));
  Serial.print(freeRam());
  Serial.println(F(" B"));
}

static void cmdMem() {
  Serial.print(F("FREE RAM: "));
  Serial.print(freeRam());
  Serial.println(F(" B"));
}

static void cmdUptime() {
  Serial.print(F("UPTIME: "));
  Serial.print(millis());
  Serial.println(F(" ms"));
  Serial.print(F("KERNEL TICKS: "));
  Serial.println(kernelTicks);
}

static void cmdPs() {
  Serial.println(F("ID  TASK      PERIOD  RUNS"));

  Serial.print(F("0   SERIAL    "));
  Serial.print(tasks[0].periodMs);
  Serial.print(F(" ms    "));
  Serial.println(tasks[0].runs);

  Serial.print(F("1   CLOCK     "));
  Serial.print(tasks[1].periodMs);
  Serial.print(F(" ms  "));
  Serial.println(tasks[1].runs);
}

static void cmdMount() {
  Serial.println(F("MOUNT BEGIN"));
  sdReady = mountSD();
  Serial.println(sdReady ? F("MOUNT PASS") : F("MOUNT FAIL"));
}

static void cmdDir(char *args) {
  if (!sdReady) {
    Serial.println(F("ERR SD_NOT_READY"));
    return;
  }

  char *path = skipSpaces(args);
  if (!*path) path = (char *)"/";

  Serial.print(F("DIR "));
  Serial.println(path);

  File dir = SD.open(path);

  if (!dir) {
    Serial.println(F("ERR OPEN"));
    return;
  }

  if (!dir.isDirectory()) {
    dir.close();
    Serial.println(F("ERR NOT_DIR"));
    return;
  }

  uint16_t count = 0;

  for (;;) {
    File entry = dir.openNextFile();
    if (!entry) break;

    Serial.write(entry.isDirectory() ? 'D' : 'F');
    Serial.write(' ');
    Serial.print((uint32_t)entry.size());
    Serial.write(' ');
    Serial.println(entry.name());

    entry.close();
    ++count;
  }

  dir.close();

  Serial.print(F("FILES: "));
  Serial.println(count);
}

static void cmdType(char *args) {
  if (!sdReady) {
    Serial.println(F("ERR SD_NOT_READY"));
    return;
  }

  char *path = skipSpaces(args);

  if (!*path) {
    Serial.println(F("ERR PATH"));
    return;
  }

  File f = SD.open(path, FILE_READ);

  if (!f) {
    Serial.println(F("ERR OPEN"));
    return;
  }

  if (f.isDirectory()) {
    f.close();
    Serial.println(F("ERR IS_DIR"));
    return;
  }

  Serial.println(F("-----"));

  while (f.available()) {
    Serial.write((uint8_t)f.read());
  }

  f.close();

  if (Serial.peek() != '\n') Serial.println();
  Serial.println(F("-----"));
}

static void writeText(char *args, bool appendMode) {
  if (!sdReady) {
    Serial.println(F("ERR SD_NOT_READY"));
    return;
  }

  args = skipSpaces(args);

  if (!*args) {
    Serial.println(F("ERR PATH"));
    return;
  }

  char *p = args;
  while (*p && *p != ' ' && *p != '\t') ++p;

  if (!*p) {
    Serial.println(F("ERR TEXT"));
    return;
  }

  *p++ = 0;
  char *text = skipSpaces(p);

  if (!appendMode && SD.exists(args)) {
    if (!SD.remove(args)) {
      Serial.println(F("ERR REMOVE_OLD"));
      return;
    }
  }

  File f = SD.open(args, FILE_WRITE);

  if (!f) {
    Serial.println(F("ERR CREATE"));
    return;
  }

  size_t n = f.print(text);
  f.write('\n');
  f.close();

  Serial.print(F("OK "));
  Serial.print((uint16_t)n);
  Serial.println(F(" B"));
}

static void cmdDelete(char *args) {
  if (!sdReady) {
    Serial.println(F("ERR SD_NOT_READY"));
    return;
  }

  char *path = skipSpaces(args);

  if (!*path) {
    Serial.println(F("ERR PATH"));
    return;
  }

  Serial.println(SD.remove(path) ? F("OK") : F("ERR DELETE"));
}

static void cmdMkdir(char *args) {
  if (!sdReady) {
    Serial.println(F("ERR SD_NOT_READY"));
    return;
  }

  char *path = skipSpaces(args);

  if (!*path) {
    Serial.println(F("ERR PATH"));
    return;
  }

  Serial.println(SD.mkdir(path) ? F("OK") : F("ERR MKDIR"));
}

static void cmdRmdir(char *args) {
  if (!sdReady) {
    Serial.println(F("ERR SD_NOT_READY"));
    return;
  }

  char *path = skipSpaces(args);

  if (!*path) {
    Serial.println(F("ERR PATH"));
    return;
  }

  Serial.println(SD.rmdir(path) ? F("OK") : F("ERR RMDIR"));
}

static void cmdCls() {
  Serial.write(27);
  Serial.print(F("[2J"));
  Serial.write(27);
  Serial.print(F("[H"));
}

static void cmdReboot() {
  Serial.println(F("REBOOT"));
  Serial.flush();
  wdt_enable(WDTO_15MS);
  for (;;) {}
}

static void executeCommand(char *line) {
  line = skipSpaces(line);
  if (!*line) return;

  toUpperCommand(line);
  char *args = splitCommand(line);

  if (strcmp(line, "HELP") == 0) {
    cmdHelp();
  } else if (strcmp(line, "INFO") == 0) {
    cmdInfo();
  } else if (strcmp(line, "MEM") == 0) {
    cmdMem();
  } else if (strcmp(line, "UPTIME") == 0) {
    cmdUptime();
  } else if (strcmp(line, "PS") == 0) {
    cmdPs();
  } else if (strcmp(line, "MOUNT") == 0) {
    cmdMount();
  } else if (strcmp(line, "DIR") == 0 || strcmp(line, "LS") == 0) {
    cmdDir(args);
  } else if (strcmp(line, "TYPE") == 0 || strcmp(line, "CAT") == 0) {
    cmdType(args);
  } else if (strcmp(line, "WRITE") == 0) {
    writeText(args, false);
  } else if (strcmp(line, "APPEND") == 0) {
    writeText(args, true);
  } else if (strcmp(line, "DEL") == 0 || strcmp(line, "RM") == 0) {
    cmdDelete(args);
  } else if (strcmp(line, "MKDIR") == 0) {
    cmdMkdir(args);
  } else if (strcmp(line, "RMDIR") == 0) {
    cmdRmdir(args);
  } else if (strcmp(line, "CLS") == 0) {
    cmdCls();
  } else if (strcmp(line, "REBOOT") == 0) {
    cmdReboot();
  } else {
    Serial.println(F("ERR UNKNOWN COMMAND"));
  }
}

// -----------------------------------------------------------------------------
// Serial task
// -----------------------------------------------------------------------------

static void taskSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      commandLine[commandLen] = 0;

      if (commandLen) {
        executeCommand(commandLine);
      }

      commandLen = 0;
      printPrompt();
      continue;
    }

    if (c == 8 || c == 127) {
      if (commandLen) --commandLen;
      continue;
    }

    if (commandLen < CMD_SIZE - 1) {
      commandLine[commandLen++] = c;
    }
  }
}

// -----------------------------------------------------------------------------
// Arduino entry points
// -----------------------------------------------------------------------------

void setup() {
  MCUSR = 0;
  wdt_disable();

  Serial.begin(SERIAL_BAUD);

  printBanner();

  Serial.println(F("BOOT: kernel init"));
  kernelInit();

  Serial.println(F("BOOT: SD mount"));
  sdReady = mountSD();
  printSdStatus();

  Serial.print(F("FREE RAM: "));
  Serial.print(freeRam());
  Serial.println(F(" B"));

  Serial.println(F("Type HELP"));
  printPrompt();
}

void loop() {
  kernelDispatch();
}
