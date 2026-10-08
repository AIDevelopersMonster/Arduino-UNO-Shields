#pragma once

#include <Arduino.h>

enum KscNodeType : uint8_t {
  KSC_NODE_NONE = 0,
  KSC_NODE_DIR,
  KSC_NODE_RO,
  KSC_NODE_RW
};

enum KscResult : uint8_t {
  KSC_OK = 0,
  KSC_ERR_NOT_FOUND,
  KSC_ERR_IS_DIR,
  KSC_ERR_READ_ONLY,
  KSC_ERR_RANGE,
  KSC_ERR_VALUE,
  KSC_ERR_TARGET
};

enum KscInputType : uint8_t {
  KSC_INPUT_CHAR = 1,
  KSC_INPUT_KEY = 2
};

enum KscInputSource : uint8_t {
  KSC_SRC_TTY = 1,
  KSC_SRC_TARGET = 2
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

class KscCore;

class KscTarget {
public:
  virtual ~KscTarget() {}

  virtual void begin() {}
  virtual void service() {}
  virtual void pollInput(KscCore &core) {}

  virtual void name(char *out, uint8_t outSize) = 0;
  virtual void bootReport(Print &out) {}

  virtual KscNodeType pathType(const char *path) = 0;
  virtual uint8_t dirCount(const char *path) = 0;
  virtual bool dirEntry(
    const char *path,
    uint8_t index,
    char *nameOut,
    uint8_t nameOutSize,
    KscNodeType &typeOut
  ) = 0;

  virtual KscResult read(
    const char *path,
    char *out,
    uint8_t outSize
  ) = 0;

  // Optional streaming read hook. Targets that do not implement streaming
  // leave handled=false and the core falls back to the legacy buffered read().
  virtual KscResult streamRead(
    const char *path,
    Print &out,
    bool &handled
  ) {
    (void)path;
    (void)out;
    handled = false;
    return KSC_ERR_TARGET;
  }

  virtual KscResult streamWindow(
    const char *path,
    uint32_t offset,
    uint16_t maxBytes,
    Print &out,
    uint16_t &bytesRead,
    uint32_t &totalSize,
    bool &handled
  ) {
    (void)path;
    (void)offset;
    (void)maxBytes;
    (void)out;
    bytesRead = 0;
    totalSize = 0;
    handled = false;
    return KSC_ERR_TARGET;
  }

  virtual KscResult launch(
    const char *path,
    Print &out,
    bool &handled
  ) {
    (void)path;
    (void)out;
    handled = false;
    return KSC_ERR_TARGET;
  }

  virtual KscResult write(
    const char *path,
    long value
  ) = 0;

  virtual bool writableRange(
    const char *path,
    uint16_t &minValue,
    uint16_t &maxValue
  ) = 0;

  virtual bool readWritableLong(
    const char *path,
    long &value
  ) = 0;

  virtual bool pathIsDynamic(
    const char *path
  ) = 0;

  virtual void printStatus(Print &out) {}
};

int kscFreeRam();

class KscCore {
public:
  static const uint8_t PATH_SIZE = 24;
  static const uint8_t VALUE_SIZE = 24;
  static const uint8_t SHELL_LINE_SIZE = 64;
  static const uint8_t EVENT_QUEUE_SIZE = 8;

  KscCore(Stream &io, KscTarget &target);

  void begin();
  void service();

  bool injectChar(
    KscInputSource source,
    uint8_t value
  );

  bool injectKey(
    KscInputSource source,
    KscSpecialKey key
  );

  bool commanderActive() const {
    return _commanderActive;
  }

  uint16_t inputDrops() const {
    return _eventDrops;
  }

private:
  enum TtyState : uint8_t {
    TTY_NORMAL = 0,
    TTY_ESC,
    TTY_CSI,
    TTY_SS3
  };

  enum CommanderView : uint8_t {
    VIEW_DIR = 0,
    VIEW_NODE,
    VIEW_FILE,
    VIEW_ACTIONS,
    VIEW_RUN_RESULT,
    VIEW_HELP
  };

  enum NodeMessage : uint8_t {
    NODE_MSG_NONE = 0,
    NODE_MSG_APPLIED,
    NODE_MSG_RANGE,
    NODE_MSG_CANCELED
  };

  Stream &_io;
  KscTarget &_target;

  KscInputEvent _eventQueue[EVENT_QUEUE_SIZE];
  uint8_t _eventHead;
  uint8_t _eventTail;
  uint8_t _eventCount;
  uint16_t _eventDrops;

  TtyState _ttyState;
  unsigned long _ttyStateMs;
  uint16_t _csiParam;
  bool _csiHaveParam;
  bool _lastWasCR;

  char _shellCwd[PATH_SIZE];
  char _shellLine[SHELL_LINE_SIZE];
  uint8_t _shellLineLen;

  bool _commanderActive;
  CommanderView _view;
  CommanderView _helpReturnView;
  char _commanderPath[PATH_SIZE];
  char _nodePath[PATH_SIZE];
  uint8_t _selected;
  bool _editActive;
  uint16_t _editValue;
  NodeMessage _nodeMessage;
  unsigned long _lastRenderMs;
  uint32_t _fileOffset;
  uint32_t _fileSize;
  uint16_t _fileBytesRead;
  uint8_t _actionSelected;
  KscResult _launchResult;

  bool queueEvent(
    uint8_t type,
    uint8_t source,
    uint8_t code
  );

  bool popEvent(KscInputEvent &event);

  void serviceTarget();
  void serviceTty();
  void serviceEvents();
  void serviceRefresh();

  void resetTtyState();
  void processCsiFinal(uint8_t c);
  void handleNormalTtyByte(uint8_t c);

  static char *skipSpaces(char *p);
  static void upperCommand(char *p);
  static char *splitCommand(char *line);
  static bool parseLong(
    const char *text,
    long &value
  );

  static void parentPath(
    const char *path,
    char *out,
    uint8_t outSize
  );

  static void joinPath(
    const char *base,
    const char *name,
    char *out,
    uint8_t outSize
  );

  static void makePath(
    const char *cwd,
    const char *arg,
    char *out,
    uint8_t outSize
  );

  static const __FlashStringHelper *resultName(
    KscResult result
  );

  static bool isKscScript(
    const char *path
  );

  void shellPrompt();
  void shellHelp();
  void executeShellLine(char *line);
  void shellWriteCommand(char *args);
  void handleShellEvent(
    const KscInputEvent &event
  );
  void listShell(const char *path);

  void ansiClear();
  void ansiHideCursor();
  void ansiShowCursor();
  void ansiInverseOn();
  void ansiNormal();

  void enterCommander();
  void exitCommander();
  void setCommanderRoot();
  void openSelected();
  void parentCommander();
  void showHelp();
  void closeHelp();

  void renderCommander();
  void renderHeader();
  void renderDirectory();
  void renderNode();
  void renderFile();
  void renderActions();
  void renderRunResult();
  void renderHelp();
  void renderStatusLine();

  void moveSelection(int8_t delta);
  void adjustWritable(int8_t delta);
  void appendDigit(uint8_t digit);
  void applyEdit();
  void backFromNode();
  void runSelectedFile();

  void handleCommanderChar(uint8_t c);
  void handleCommanderKey(KscSpecialKey key);
};
