#pragma once

#include <Arduino.h>

enum VfsNodeType : uint8_t {
  VFS_NONE = 0,
  VFS_DIR,
  VFS_RO,
  VFS_RW
};

enum VfsResult : uint8_t {
  VFS_OK = 0,
  VFS_ERR_NOT_FOUND,
  VFS_ERR_IS_DIR,
  VFS_ERR_READ_ONLY,
  VFS_ERR_RANGE,
  VFS_ERR_VALUE,
  VFS_ERR_DHT
};

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

enum TtyState : uint8_t {
  TTY_NORMAL = 0,
  TTY_ESC,
  TTY_CSI,
  TTY_SS3
};

enum CommanderView : uint8_t {
  CMD_VIEW_DIR = 0,
  CMD_VIEW_NODE,
  CMD_VIEW_HELP
};

enum NodeMessage : uint8_t {
  NODE_MSG_NONE = 0,
  NODE_MSG_APPLIED,
  NODE_MSG_RANGE,
  NODE_MSG_CANCELED
};
