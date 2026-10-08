#include "KSC_Core.h"

#include <stdlib.h>
#include <string.h>

extern unsigned int __heap_start;
extern void *__brkval;

int kscFreeRam() {
  int v;

  return (int)&v -
         (__brkval == 0
            ? (int)&__heap_start
            : (int)__brkval);
}

KscCore::KscCore(
  Stream &io,
  KscTarget &target
)
  : _io(io),
    _target(target),
    _eventHead(0),
    _eventTail(0),
    _eventCount(0),
    _eventDrops(0),
    _ttyState(TTY_NORMAL),
    _ttyStateMs(0),
    _csiParam(0),
    _csiHaveParam(false),
    _lastWasCR(false),
    _shellLineLen(0),
    _commanderActive(false),
    _view(VIEW_DIR),
    _helpReturnView(VIEW_DIR),
    _selected(0),
    _editActive(false),
    _editValue(0),
    _nodeMessage(NODE_MSG_NONE),
    _lastRenderMs(0),
    _fileOffset(0),
    _fileSize(0),
    _fileBytesRead(0),
    _actionSelected(0),
    _launchResult(KSC_OK) {
  strcpy(_shellCwd, "/");
  _shellLine[0] = 0;
  strcpy(_commanderPath, "/");
  _nodePath[0] = 0;
}

void KscCore::begin() {
  _target.begin();

  _io.println();
  _io.println(F("KSC Core 0.1"));
  _io.println(F("Hardware-independent ANSI VFS core"));

  char targetName[VALUE_SIZE];
  targetName[0] = 0;
  _target.name(
    targetName,
    sizeof(targetName)
  );

  _io.print(F("Target: "));
  _io.println(targetName);

  _io.print(F("FREE RAM: "));
  _io.print(kscFreeRam());
  _io.println(F(" B"));

  _target.bootReport(_io);

  _io.println(F("Type KSC to open Commander, HELP for shell commands."));
  _io.println();

  shellPrompt();
}

void KscCore::service() {
  serviceTarget();
  serviceTty();
  serviceEvents();
  serviceRefresh();

  if (_eventDrops != 0 &&
      !_commanderActive) {
    _io.print(F("\r\nINPUT DROPS: "));
    _io.println(_eventDrops);

    _eventDrops = 0;
    shellPrompt();
  }
}

bool KscCore::injectChar(
  KscInputSource source,
  uint8_t value
) {
  return queueEvent(
    KSC_INPUT_CHAR,
    source,
    value
  );
}

bool KscCore::injectKey(
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

bool KscCore::queueEvent(
  uint8_t type,
  uint8_t source,
  uint8_t code
) {
  if (_eventCount >= EVENT_QUEUE_SIZE) {
    ++_eventDrops;
    return false;
  }

  _eventQueue[_eventHead].type = type;
  _eventQueue[_eventHead].source = source;
  _eventQueue[_eventHead].code = code;

  _eventHead =
    (uint8_t)((_eventHead + 1) %
              EVENT_QUEUE_SIZE);

  ++_eventCount;

  return true;
}

bool KscCore::popEvent(
  KscInputEvent &event
) {
  if (_eventCount == 0) {
    return false;
  }

  event = _eventQueue[_eventTail];

  _eventTail =
    (uint8_t)((_eventTail + 1) %
              EVENT_QUEUE_SIZE);

  --_eventCount;

  return true;
}

void KscCore::serviceTarget() {
  _target.service();
  _target.pollInput(*this);
}

void KscCore::resetTtyState() {
  _ttyState = TTY_NORMAL;
  _csiParam = 0;
  _csiHaveParam = false;
}

void KscCore::processCsiFinal(
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
      if (_csiHaveParam) {
        switch (_csiParam) {
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
    injectKey(
      KSC_SRC_TTY,
      key
    );
  }
}

void KscCore::handleNormalTtyByte(
  uint8_t c
) {
  if (c == 0x1B) {
    _ttyState = TTY_ESC;
    _ttyStateMs = millis();
    return;
  }

  if (c == '\r') {
    injectKey(
      KSC_SRC_TTY,
      KSC_KEY_ENTER
    );

    _lastWasCR = true;
    return;
  }

  if (c == '\n') {
    if (_lastWasCR) {
      _lastWasCR = false;
      return;
    }

    injectKey(
      KSC_SRC_TTY,
      KSC_KEY_ENTER
    );

    return;
  }

  _lastWasCR = false;

  if (c == 0x08 ||
      c == 0x7F) {
    injectKey(
      KSC_SRC_TTY,
      KSC_KEY_BACK
    );

    return;
  }

  if (c >= 0x20 &&
      c <= 0x7E) {
    injectChar(
      KSC_SRC_TTY,
      c
    );
  }
}

void KscCore::serviceTty() {
  while (_io.available()) {
    const uint8_t c =
      (uint8_t)_io.read();

    if (_ttyState == TTY_NORMAL) {
      handleNormalTtyByte(c);
      continue;
    }

    if (_ttyState == TTY_ESC) {
      if (c == '[') {
        _ttyState = TTY_CSI;
        _ttyStateMs = millis();
        _csiParam = 0;
        _csiHaveParam = false;
        continue;
      }

      if (c == 'O') {
        _ttyState = TTY_SS3;
        _ttyStateMs = millis();
        continue;
      }

      injectKey(
        KSC_SRC_TTY,
        KSC_KEY_BACK
      );

      resetTtyState();
      handleNormalTtyByte(c);
      continue;
    }

    if (_ttyState == TTY_CSI) {
      _ttyStateMs = millis();

      if (c >= '0' &&
          c <= '9') {
        _csiHaveParam = true;

        _csiParam =
          (uint16_t)(
            _csiParam * 10U +
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

    if (_ttyState == TTY_SS3) {
      resetTtyState();

      switch (c) {
        case 'A':
          injectKey(
            KSC_SRC_TTY,
            KSC_KEY_UP
          );
          break;

        case 'B':
          injectKey(
            KSC_SRC_TTY,
            KSC_KEY_DOWN
          );
          break;

        case 'C':
          injectKey(
            KSC_SRC_TTY,
            KSC_KEY_RIGHT
          );
          break;

        case 'D':
          injectKey(
            KSC_SRC_TTY,
            KSC_KEY_LEFT
          );
          break;

        case 'H':
          injectKey(
            KSC_SRC_TTY,
            KSC_KEY_HOME
          );
          break;

        default:
          break;
      }
    }
  }

  if (_ttyState != TTY_NORMAL &&
      millis() - _ttyStateMs >= 80UL) {
    if (_ttyState == TTY_ESC) {
      injectKey(
        KSC_SRC_TTY,
        KSC_KEY_BACK
      );
    }

    resetTtyState();
  }
}

char *KscCore::skipSpaces(
  char *p
) {
  while (*p == ' ' ||
         *p == '\t') {
    ++p;
  }

  return p;
}

void KscCore::upperCommand(
  char *p
) {
  while (*p &&
         *p != ' ' &&
         *p != '\t') {
    if (*p >= 'a' &&
        *p <= 'z') {
      *p =
        (char)(*p - 'a' + 'A');
    }

    ++p;
  }
}

char *KscCore::splitCommand(
  char *line
) {
  char *p = line;

  while (*p &&
         *p != ' ' &&
         *p != '\t') {
    ++p;
  }

  if (*p) {
    *p++ = 0;
    p = skipSpaces(p);
  }

  return p;
}

bool KscCore::parseLong(
  const char *text,
  long &value
) {
  if (!text ||
      !*text) {
    return false;
  }

  char *endp = nullptr;

  value =
    strtol(
      text,
      &endp,
      10
    );

  return endp &&
         *endp == 0;
}

void KscCore::parentPath(
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

  char *last =
    strrchr(out, '/');

  if (!last ||
      last == out) {
    strcpy(out, "/");
    return;
  }

  *last = 0;
}

void KscCore::joinPath(
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

  strncpy(
    out,
    base,
    outSize
  );

  out[outSize - 1] = 0;

  const uint8_t n =
    strlen(out);

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

void KscCore::makePath(
  const char *cwd,
  const char *arg,
  char *out,
  uint8_t outSize
) {
  if (!arg ||
      !*arg) {
    strncpy(out, cwd, outSize);
    out[outSize - 1] = 0;
    return;
  }

  if (strcmp(arg, "..") == 0) {
    parentPath(
      cwd,
      out,
      outSize
    );

    return;
  }

  if (arg[0] == '/') {
    strncpy(out, arg, outSize);
    out[outSize - 1] = 0;
    return;
  }

  joinPath(
    cwd,
    arg,
    out,
    outSize
  );
}

const __FlashStringHelper *KscCore::resultName(
  KscResult result
) {
  switch (result) {
    case KSC_OK:
      return F("OK");

    case KSC_ERR_NOT_FOUND:
      return F("ERR NOT_FOUND");

    case KSC_ERR_IS_DIR:
      return F("ERR IS_DIR");

    case KSC_ERR_READ_ONLY:
      return F("ERR READ_ONLY");

    case KSC_ERR_RANGE:
      return F("ERR RANGE");

    case KSC_ERR_VALUE:
      return F("ERR VALUE");

    case KSC_ERR_TARGET:
      return F("ERR TARGET");

    default:
      return F("ERR");
  }
}

bool KscCore::isKscScript(
  const char *path
) {
  if (!path) {
    return false;
  }

  const size_t len =
    strlen(path);

  if (len < 4) {
    return false;
  }

  const char *ext =
    path + len - 4;

  return
    (ext[0] == '.') &&
    (ext[1] == 'K' || ext[1] == 'k') &&
    (ext[2] == 'S' || ext[2] == 's') &&
    (ext[3] == 'C' || ext[3] == 'c');
}

void KscCore::shellPrompt() {
  _io.print(F("KSC:"));
  _io.print(_shellCwd);
  _io.print(F("> "));
}

void KscCore::shellHelp() {
  _io.println(F("KSC                   open ANSI Commander"));
  _io.println(F("HELP                  command list"));
  _io.println(F("PWD                   current virtual path"));
  _io.println(F("LS [path]             list virtual directory"));
  _io.println(F("CD <path>             change virtual directory"));
  _io.println(F("CAT <path>            read virtual node"));
  _io.println(F("WRITE <path> <value>  write virtual node"));
  _io.println(F("MEM                   free SRAM"));
}

void KscCore::listShell(
  const char *path
) {
  if (_target.pathType(path) !=
      KSC_NODE_DIR) {
    _io.println(F("ERR NOT_DIR"));
    return;
  }

  const uint8_t count =
    _target.dirCount(path);

  for (uint8_t i = 0;
       i < count;
       ++i) {
    char name[16];
    KscNodeType type =
      KSC_NODE_NONE;

    if (!_target.dirEntry(
          path,
          i,
          name,
          sizeof(name),
          type
        )) {
      continue;
    }

    _io.print(
      type == KSC_NODE_DIR
        ? F("D ")
        : F("F ")
    );

    _io.print(name);

    if (type == KSC_NODE_RO) {
      _io.print(F("  RO"));
    } else if (type ==
               KSC_NODE_RW) {
      _io.print(F("  RW"));
    }

    _io.println();
  }
}

void KscCore::shellWriteCommand(
  char *args
) {
  args = skipSpaces(args);

  if (!*args) {
    _io.println(F("ERR PATH"));
    return;
  }

  char *valueText = args;

  while (*valueText &&
         *valueText != ' ' &&
         *valueText != '\t') {
    ++valueText;
  }

  if (!*valueText) {
    _io.println(F("ERR VALUE"));
    return;
  }

  *valueText++ = 0;
  valueText =
    skipSpaces(valueText);

  char path[PATH_SIZE];

  makePath(
    _shellCwd,
    args,
    path,
    sizeof(path)
  );

  long value = 0;

  if (!parseLong(
        valueText,
        value
      )) {
    _io.println(F("ERR VALUE"));
    return;
  }

  _io.println(
    resultName(
      _target.write(
        path,
        value
      )
    )
  );
}

void KscCore::executeShellLine(
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
    _io.println(_shellCwd);
    return;
  }

  if (strcmp(line, "MEM") == 0) {
    _io.println(kscFreeRam());
    return;
  }

  if (strcmp(line, "LS") == 0) {
    char path[PATH_SIZE];

    makePath(
      _shellCwd,
      args,
      path,
      sizeof(path)
    );

    listShell(path);
    return;
  }

  if (strcmp(line, "CD") == 0) {
    char path[PATH_SIZE];

    makePath(
      _shellCwd,
      args,
      path,
      sizeof(path)
    );

    if (_target.pathType(path) !=
        KSC_NODE_DIR) {
      _io.println(F("ERR NOT_DIR"));
      return;
    }

    strncpy(
      _shellCwd,
      path,
      sizeof(_shellCwd)
    );

    _shellCwd[
      sizeof(_shellCwd) - 1
    ] = 0;

    _io.println(F("OK"));
    return;
  }

  if (strcmp(line, "CAT") == 0) {
    if (!*args) {
      _io.println(F("ERR PATH"));
      return;
    }

    char path[PATH_SIZE];
    char value[VALUE_SIZE];

    makePath(
      _shellCwd,
      args,
      path,
      sizeof(path)
    );

    bool streamed = false;

    const KscResult streamResult =
      _target.streamRead(
        path,
        _io,
        streamed
      );

    if (streamed) {
      if (streamResult != KSC_OK) {
        _io.println(
          resultName(streamResult)
        );
      } else {
        _io.println();
      }

      return;
    }

    const KscResult result =
      _target.read(
        path,
        value,
        sizeof(value)
      );

    if (result == KSC_OK) {
      _io.println(value);
    } else {
      _io.println(
        resultName(result)
      );
    }

    return;
  }

  if (strcmp(line, "WRITE") == 0) {
    shellWriteCommand(args);
    return;
  }

  _io.println(
    F("ERR UNKNOWN_COMMAND")
  );
}

void KscCore::handleShellEvent(
  const KscInputEvent &event
) {
  if (event.type ==
      KSC_INPUT_CHAR) {
    const uint8_t c =
      event.code;

    if (c >= 0x20 &&
        c <= 0x7E &&
        _shellLineLen <
          SHELL_LINE_SIZE - 1) {
      _shellLine[
        _shellLineLen++
      ] = (char)c;

      _io.write(c);
    }

    return;
  }

  const KscSpecialKey key =
    (KscSpecialKey)event.code;

  if (key == KSC_KEY_ENTER) {
    _io.println();

    _shellLine[
      _shellLineLen
    ] = 0;

    executeShellLine(
      _shellLine
    );

    _shellLineLen = 0;

    if (!_commanderActive) {
      shellPrompt();
    }

    return;
  }

  if (key == KSC_KEY_BACK) {
    if (_shellLineLen) {
      --_shellLineLen;
      _io.print(F("\b \b"));
    }

    return;
  }
}

void KscCore::ansiClear() {
  _io.print(
    F("\x1B[0m\x1B[2J\x1B[H")
  );
}

void KscCore::ansiHideCursor() {
  _io.print(F("\x1B[?25l"));
}

void KscCore::ansiShowCursor() {
  _io.print(F("\x1B[?25h"));
}

void KscCore::ansiInverseOn() {
  _io.print(F("\x1B[7m"));
}

void KscCore::ansiNormal() {
  _io.print(F("\x1B[0m"));
}

void KscCore::setCommanderRoot() {
  strcpy(
    _commanderPath,
    "/"
  );

  _selected = 0;
  _view = VIEW_DIR;
  _editActive = false;
  _nodeMessage =
    NODE_MSG_NONE;
}

void KscCore::enterCommander() {
  _commanderActive = true;

  if (_target.pathType(
        _shellCwd
      ) == KSC_NODE_DIR) {
    strncpy(
      _commanderPath,
      _shellCwd,
      sizeof(_commanderPath)
    );

    _commanderPath[
      sizeof(_commanderPath) - 1
    ] = 0;
  } else {
    strcpy(
      _commanderPath,
      "/"
    );
  }

  _selected = 0;
  _view = VIEW_DIR;
  _editActive = false;
  _nodeMessage =
    NODE_MSG_NONE;

  ansiHideCursor();
  renderCommander();
}

void KscCore::exitCommander() {
  _commanderActive = false;

  if (_target.pathType(
        _commanderPath
      ) == KSC_NODE_DIR) {
    strncpy(
      _shellCwd,
      _commanderPath,
      sizeof(_shellCwd)
    );

    _shellCwd[
      sizeof(_shellCwd) - 1
    ] = 0;
  }

  ansiNormal();
  ansiShowCursor();
  ansiClear();

  _io.println(
    F("KSC Commander closed.")
  );

  _io.print(F("FREE RAM: "));
  _io.print(kscFreeRam());
  _io.println(F(" B"));

  shellPrompt();
}

void KscCore::openSelected() {
  const uint8_t count =
    _target.dirCount(
      _commanderPath
    );

  if (count == 0 ||
      _selected >= count) {
    return;
  }

  char name[16];
  KscNodeType type =
    KSC_NODE_NONE;

  if (!_target.dirEntry(
        _commanderPath,
        _selected,
        name,
        sizeof(name),
        type
      )) {
    return;
  }

  char child[PATH_SIZE];

  joinPath(
    _commanderPath,
    name,
    child,
    sizeof(child)
  );

  type =
    _target.pathType(child);

  if (type == KSC_NODE_DIR) {
    strncpy(
      _commanderPath,
      child,
      sizeof(_commanderPath)
    );

    _commanderPath[
      sizeof(_commanderPath) - 1
    ] = 0;

    _selected = 0;
    _view = VIEW_DIR;
    _nodeMessage =
      NODE_MSG_NONE;

    renderCommander();
    return;
  }

  if (type == KSC_NODE_RO ||
      type == KSC_NODE_RW) {
    strncpy(
      _nodePath,
      child,
      sizeof(_nodePath)
    );

    _nodePath[
      sizeof(_nodePath) - 1
    ] = 0;

    _editActive = false;
    _nodeMessage =
      NODE_MSG_NONE;

    if (type == KSC_NODE_RO) {
      if (isKscScript(
            _nodePath
          )) {
        _actionSelected = 0;
        _view = VIEW_ACTIONS;
        renderCommander();
        return;
      }

      bool handled = false;
      uint16_t bytesRead = 0;
      uint32_t totalSize = 0;

      const KscResult probe =
        _target.streamWindow(
          _nodePath,
          0,
          0,
          _io,
          bytesRead,
          totalSize,
          handled
        );

      if (handled &&
          probe == KSC_OK) {
        _view = VIEW_FILE;
        _fileOffset = 0;
        _fileSize = totalSize;
        _fileBytesRead = 0;
        renderCommander();
        return;
      }
    }

    _view = VIEW_NODE;
    renderCommander();
  }
}

void KscCore::parentCommander() {
  if (strcmp(
        _commanderPath,
        "/"
      ) == 0) {
    return;
  }

  char parent[PATH_SIZE];

  parentPath(
    _commanderPath,
    parent,
    sizeof(parent)
  );

  strncpy(
    _commanderPath,
    parent,
    sizeof(_commanderPath)
  );

  _commanderPath[
    sizeof(_commanderPath) - 1
  ] = 0;

  _selected = 0;
  _view = VIEW_DIR;
  _nodeMessage =
    NODE_MSG_NONE;

  renderCommander();
}

void KscCore::showHelp() {
  _helpReturnView = _view;
  _view = VIEW_HELP;
  renderCommander();
}

void KscCore::closeHelp() {
  _view = _helpReturnView;
  renderCommander();
}

void KscCore::renderHeader() {
  _io.println(
    F("KSC Core 0.1 - KonSol Commander")
  );

  char targetName[VALUE_SIZE];
  targetName[0] = 0;

  _target.name(
    targetName,
    sizeof(targetName)
  );

  _io.print(F("Target: "));
  _io.println(targetName);
  _io.print(F("Path: "));
}

void KscCore::renderStatusLine() {
  _io.print(F("RAM "));
  _io.print(kscFreeRam());
  _io.print(F(" B   IN drop "));
  _io.print(_eventDrops);

  _target.printStatus(_io);
  _io.println();
}

void KscCore::renderDirectory() {
  ansiClear();
  renderHeader();

  _io.println(_commanderPath);
  _io.println(
    F("----------------------------------------")
  );

  const uint8_t count =
    _target.dirCount(
      _commanderPath
    );

  for (uint8_t i = 0;
       i < count;
       ++i) {
    char name[16];
    KscNodeType type =
      KSC_NODE_NONE;

    if (!_target.dirEntry(
          _commanderPath,
          i,
          name,
          sizeof(name),
          type
        )) {
      continue;
    }

    if (i == _selected) {
      ansiInverseOn();
    }

    _io.print(
      i == _selected
        ? F("> ")
        : F("  ")
    );

    if (type == KSC_NODE_DIR) {
      _io.print('[');
      _io.print(name);
      _io.print(F("/]"));
    } else {
      _io.print(name);

      if (type == KSC_NODE_RO) {
        _io.print(F("  [RO]"));
      } else if (type ==
                 KSC_NODE_RW) {
        _io.print(F("  [RW]"));
      }
    }

    if (i == _selected) {
      ansiNormal();
    }

    _io.println();
  }

  _io.println(
    F("----------------------------------------")
  );

  _io.println(
    F("UP/DOWN Select   ENTER/RIGHT Open")
  );

  _io.println(
    F("LEFT/BACK Parent HOME Root")
  );

  _io.println(
    F("F9/MENU Help     F10/POWER/Q Shell")
  );

  renderStatusLine();
}

void KscCore::renderNode() {
  ansiClear();
  renderHeader();

  _io.println(_nodePath);
  _io.println(
    F("----------------------------------------")
  );

  const KscNodeType type =
    _target.pathType(
      _nodePath
    );

  _io.print(F("Type: "));

  _io.println(
    type == KSC_NODE_RW
      ? F("RW")
      : F("RO")
  );

  char value[VALUE_SIZE];

  const KscResult result =
    _target.read(
      _nodePath,
      value,
      sizeof(value)
    );

  _io.print(F("Value: "));

  if (result == KSC_OK) {
    _io.println(value);
  } else {
    _io.println(
      resultName(result)
    );
  }

  if (type == KSC_NODE_RW) {
    uint16_t minValue = 0;
    uint16_t maxValue = 0;

    if (_target.writableRange(
          _nodePath,
          minValue,
          maxValue
        )) {
      _io.print(F("Range: "));
      _io.print(minValue);
      _io.print(F(".."));
      _io.println(maxValue);
    }

    _io.print(F("Edit: "));

    if (_editActive) {
      _io.print(_editValue);
      _io.println('_');
    } else {
      _io.println('-');
    }

    switch (_nodeMessage) {
      case NODE_MSG_APPLIED:
        _io.println(
          F("Status: APPLIED")
        );
        break;

      case NODE_MSG_RANGE:
        _io.println(
          F("Status: RANGE ERROR")
        );
        break;

      case NODE_MSG_CANCELED:
        _io.println(
          F("Status: EDIT CANCELED")
        );
        break;

      default:
        _io.println(
          F("Status: READY")
        );
        break;
    }

    _io.println();
    _io.println(
      F("0..9 Exact value   ENTER Apply/Refresh")
    );

    _io.println(
      F("LEFT/RIGHT -/+     BACK Cancel/Return")
    );
  } else {
    _io.println();
    _io.println(
      F("ENTER Refresh      LEFT/BACK Return")
    );
  }

  _io.println(
    F("HOME Root          F9/MENU Help")
  );

  _io.println(
    F("F10/POWER/Q Shell")
  );

  _io.println(
    F("----------------------------------------")
  );

  renderStatusLine();
}

void KscCore::renderFile() {
  static const uint16_t PAGE_BYTES = 192;

  ansiClear();
  renderHeader();

  _io.println(_nodePath);
  _io.println(
    F("----------------------------------------")
  );

  _io.print(F("FILE "));
  _io.print(_fileOffset);
  _io.print('/');
  _io.println(_fileSize);

  _io.println(
    F("----------------------------------------")
  );

  bool handled = false;
  uint16_t bytesRead = 0;
  uint32_t totalSize = 0;

  const KscResult result =
    _target.streamWindow(
      _nodePath,
      _fileOffset,
      PAGE_BYTES,
      _io,
      bytesRead,
      totalSize,
      handled
    );

  _fileBytesRead = bytesRead;

  if (handled &&
      result == KSC_OK) {
    _fileSize = totalSize;
  } else {
    _io.println();
    _io.println(
      resultName(result)
    );
  }

  _io.println();
  _io.println(
    F("----------------------------------------")
  );

  _io.println(
    F("UP/LEFT Prev     DOWN/RIGHT Next")
  );

  _io.println(
    F("HOME Start       END Last")
  );

  _io.println(
    F("BACK Return      F10/POWER/Q Shell")
  );

  renderStatusLine();
}

void KscCore::renderActions() {
  ansiClear();
  renderHeader();

  _io.println(_nodePath);
  _io.println(
    F("----------------------------------------")
  );

  _io.println(F("ACTIONS"));

  if (_actionSelected == 0) {
    ansiInverseOn();
  }

  _io.println(
    _actionSelected == 0
      ? F("> VIEW")
      : F("  VIEW")
  );

  if (_actionSelected == 0) {
    ansiNormal();
  }

  if (_actionSelected == 1) {
    ansiInverseOn();
  }

  _io.println(
    _actionSelected == 1
      ? F("> RUN")
      : F("  RUN")
  );

  if (_actionSelected == 1) {
    ansiNormal();
  }

  _io.println(
    F("----------------------------------------")
  );

  _io.println(F("UP/DOWN Select  ENTER Action"));
  _io.println(F("BACK Return"));

  renderStatusLine();
}

void KscCore::renderRunResult() {
  ansiClear();
  renderHeader();

  _io.println(_nodePath);
  _io.println(
    F("----------------------------------------")
  );

  _io.print(F("RUN "));
  _io.println(
    resultName(
      _launchResult
    )
  );

  _io.println(F("ENTER/BACK Return"));

  renderStatusLine();
}

void KscCore::renderHelp() {
  ansiClear();

  _io.println(
    F("KSC Core 0.1 - HELP")
  );

  _io.println(
    F("----------------------------------------")
  );

  _io.println(F("DIR:"));
  _io.println(
    F("  UP/DOWN       select entry")
  );

  _io.println(
    F("  ENTER/RIGHT   open")
  );

  _io.println(
    F("  LEFT/BACK     parent")
  );

  _io.println(
    F("  HOME          root")
  );

  _io.println();
  _io.println(F("RW NODE:"));

  _io.println(
    F("  0..9          enter exact value")
  );

  _io.println(
    F("  ENTER         apply typed value")
  );

  _io.println(
    F("  LEFT/RIGHT    decrement/increment")
  );

  _io.println(
    F("  BACK          cancel edit / return")
  );

  _io.println();
  _io.println(F("GLOBAL:"));

  _io.println(
    F("  F9 / MENU     help")
  );

  _io.println(
    F("  F10 / POWER / Q shell")
  );

  _io.println(
    F("----------------------------------------")
  );

  _io.println(
    F("F9/MENU, ENTER or BACK -> return")
  );

  renderStatusLine();
}

void KscCore::renderCommander() {
  _lastRenderMs = millis();

  switch (_view) {
    case VIEW_DIR:
      renderDirectory();
      break;

    case VIEW_NODE:
      renderNode();
      break;

    case VIEW_FILE:
      renderFile();
      break;

    case VIEW_ACTIONS:
      renderActions();
      break;

    case VIEW_RUN_RESULT:
      renderRunResult();
      break;

    case VIEW_HELP:
      renderHelp();
      break;
  }
}

void KscCore::moveSelection(
  int8_t delta
) {
  const uint8_t count =
    _target.dirCount(
      _commanderPath
    );

  if (count == 0) {
    _selected = 0;
    return;
  }

  if (delta < 0) {
    if (_selected == 0) {
      _selected = count - 1;
    } else {
      --_selected;
    }
  } else {
    ++_selected;

    if (_selected >= count) {
      _selected = 0;
    }
  }

  renderCommander();
}

void KscCore::adjustWritable(
  int8_t delta
) {
  if (_target.pathType(
        _nodePath
      ) != KSC_NODE_RW) {
    return;
  }

  long current = 0;

  if (!_target.readWritableLong(
        _nodePath,
        current
      )) {
    return;
  }

  uint16_t minValue = 0;
  uint16_t maxValue = 0;

  if (!_target.writableRange(
        _nodePath,
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

  const KscResult result =
    _target.write(
      _nodePath,
      next
    );

  _editActive = false;

  _nodeMessage =
    result == KSC_OK
      ? NODE_MSG_APPLIED
      : NODE_MSG_RANGE;

  renderCommander();
}

void KscCore::appendDigit(
  uint8_t digit
) {
  if (_target.pathType(
        _nodePath
      ) != KSC_NODE_RW) {
    return;
  }

  uint16_t minValue = 0;
  uint16_t maxValue = 0;

  if (!_target.writableRange(
        _nodePath,
        minValue,
        maxValue
      )) {
    return;
  }

  const uint16_t next =
    _editActive
      ? (uint16_t)(
          _editValue * 10U +
          digit
        )
      : digit;

  if (next > maxValue) {
    _nodeMessage =
      NODE_MSG_RANGE;

    renderCommander();
    return;
  }

  _editValue = next;
  _editActive = true;
  _nodeMessage =
    NODE_MSG_NONE;

  renderCommander();
}

void KscCore::applyEdit() {
  if (!_editActive) {
    renderCommander();
    return;
  }

  const KscResult result =
    _target.write(
      _nodePath,
      _editValue
    );

  _editActive = false;

  _nodeMessage =
    result == KSC_OK
      ? NODE_MSG_APPLIED
      : NODE_MSG_RANGE;

  renderCommander();
}

void KscCore::backFromNode() {
  if (_editActive) {
    _editActive = false;
    _nodeMessage =
      NODE_MSG_CANCELED;

    renderCommander();
    return;
  }

  _view = VIEW_DIR;
  _nodeMessage =
    NODE_MSG_NONE;

  renderCommander();
}

void KscCore::runSelectedFile() {
  bool handled = false;

  _launchResult =
    _target.launch(
      _nodePath,
      _io,
      handled
    );

  if (!handled) {
    _launchResult =
      KSC_ERR_TARGET;
  }

  _view = VIEW_RUN_RESULT;
  renderCommander();
}

void KscCore::handleCommanderChar(
  uint8_t c
) {
  if (c == 'q' ||
      c == 'Q') {
    exitCommander();
    return;
  }

  if (_view == VIEW_HELP ||
      _view == VIEW_FILE ||
      _view == VIEW_ACTIONS ||
      _view == VIEW_RUN_RESULT) {
    return;
  }

  if (_view == VIEW_NODE &&
      c >= '0' &&
      c <= '9') {
    appendDigit(
      (uint8_t)(c - '0')
    );
  }
}

void KscCore::handleCommanderKey(
  KscSpecialKey key
) {
  if (key == KSC_KEY_POWER) {
    exitCommander();
    return;
  }

  if (_view == VIEW_HELP) {
    if (key == KSC_KEY_MENU ||
        key == KSC_KEY_ENTER ||
        key == KSC_KEY_BACK) {
      closeHelp();
    } else if (key ==
               KSC_KEY_HOME) {
      setCommanderRoot();
      renderCommander();
    }

    return;
  }

  if (key == KSC_KEY_MENU) {
    showHelp();
    return;
  }

  if (key == KSC_KEY_HOME &&
      _view != VIEW_FILE) {
    setCommanderRoot();
    renderCommander();
    return;
  }

  if (_view == VIEW_DIR) {
    switch (key) {
      case KSC_KEY_UP:
        moveSelection(-1);
        break;

      case KSC_KEY_DOWN:
        moveSelection(1);
        break;

      case KSC_KEY_ENTER:
      case KSC_KEY_RIGHT:
        openSelected();
        break;

      case KSC_KEY_LEFT:
      case KSC_KEY_BACK:
        parentCommander();
        break;

      case KSC_KEY_END: {
        const uint8_t count =
          _target.dirCount(
            _commanderPath
          );

        if (count) {
          _selected = count - 1;
          renderCommander();
        }

        break;
      }

      default:
        break;
    }

    return;
  }

  if (_view == VIEW_ACTIONS) {
    switch (key) {
      case KSC_KEY_UP:
      case KSC_KEY_DOWN:
        _actionSelected =
          _actionSelected == 0
            ? 1
            : 0;
        renderCommander();
        break;

      case KSC_KEY_ENTER:
      case KSC_KEY_RIGHT:
        if (_actionSelected == 0) {
          _view = VIEW_FILE;
          _fileOffset = 0;
          _fileSize = 0;
          _fileBytesRead = 0;
          renderCommander();
        } else {
          runSelectedFile();
        }
        break;

      case KSC_KEY_LEFT:
      case KSC_KEY_BACK:
        _view = VIEW_DIR;
        renderCommander();
        break;

      default:
        break;
    }

    return;
  }

  if (_view == VIEW_RUN_RESULT) {
    if (key == KSC_KEY_ENTER ||
        key == KSC_KEY_BACK ||
        key == KSC_KEY_LEFT) {
      _view = VIEW_ACTIONS;
      renderCommander();
    }

    return;
  }

  if (_view == VIEW_FILE) {
    static const uint16_t PAGE_BYTES = 192;

    switch (key) {
      case KSC_KEY_UP:
      case KSC_KEY_LEFT:
        if (_fileOffset >= PAGE_BYTES) {
          _fileOffset -= PAGE_BYTES;
        } else {
          _fileOffset = 0;
        }
        renderCommander();
        break;

      case KSC_KEY_DOWN:
      case KSC_KEY_RIGHT:
      case KSC_KEY_ENTER:
        if (_fileOffset +
              _fileBytesRead <
            _fileSize) {
          _fileOffset += PAGE_BYTES;
          renderCommander();
        }
        break;

      case KSC_KEY_HOME:
        _fileOffset = 0;
        renderCommander();
        break;

      case KSC_KEY_END:
        if (_fileSize > PAGE_BYTES) {
          _fileOffset =
            ((_fileSize - 1) /
             PAGE_BYTES) *
            PAGE_BYTES;
        } else {
          _fileOffset = 0;
        }
        renderCommander();
        break;

      case KSC_KEY_BACK:
        _view = VIEW_DIR;
        renderCommander();
        break;

      default:
        break;
    }

    return;
  }

  if (_view == VIEW_NODE) {
    const KscNodeType type =
      _target.pathType(
        _nodePath
      );

    switch (key) {
      case KSC_KEY_ENTER:
        if (type == KSC_NODE_RW) {
          applyEdit();
        } else {
          renderCommander();
        }
        break;

      case KSC_KEY_BACK:
        backFromNode();
        break;

      case KSC_KEY_LEFT:
        if (type == KSC_NODE_RW) {
          adjustWritable(-1);
        } else {
          backFromNode();
        }
        break;

      case KSC_KEY_RIGHT:
        if (type == KSC_NODE_RW) {
          adjustWritable(1);
        } else {
          renderCommander();
        }
        break;

      case KSC_KEY_DELETE:
        if (_editActive) {
          _editActive = false;
          _nodeMessage =
            NODE_MSG_CANCELED;

          renderCommander();
        }
        break;

      default:
        break;
    }
  }
}

void KscCore::serviceEvents() {
  KscInputEvent event;

  while (popEvent(event)) {
    if (_commanderActive) {
      if (event.type ==
          KSC_INPUT_CHAR) {
        handleCommanderChar(
          event.code
        );
      } else {
        handleCommanderKey(
          (KscSpecialKey)event.code
        );
      }
    } else {
      handleShellEvent(event);
    }
  }
}

void KscCore::serviceRefresh() {
  if (!_commanderActive ||
      _view != VIEW_NODE ||
      _editActive) {
    return;
  }

  if (!_target.pathIsDynamic(
        _nodePath
      )) {
    return;
  }

  if (millis() -
        _lastRenderMs >=
      1000UL) {
    renderCommander();
  }
}
