#pragma once

#include <Arduino.h>

namespace HY_M302_RemoteMap {

enum Key : uint8_t {
  KEY_NONE = 0,
  KEY_0 = 1,
  KEY_1 = 2,
  KEY_2 = 3,
  KEY_3 = 4,
  KEY_4 = 5,
  KEY_5 = 6,
  KEY_6 = 7,
  KEY_7 = 8,
  KEY_8 = 9,
  KEY_9 = 10,
  KEY_OK = 11,
  KEY_HOME = 12,
  KEY_RETURN = 13,
  KEY_MENU = 14,
  KEY_UP = 15,
  KEY_DOWN = 16,
  KEY_LEFT = 17,
  KEY_RIGHT = 18,
  KEY_POWER = 19,
};

struct Entry {
  Key key;
  uint16_t address;
  uint8_t command;
  uint32_t raw;
};

extern const Entry kEntries[];
extern const uint8_t kEntryCount;

Key decode(uint16_t address, uint8_t command);

}  // namespace HY_M302_RemoteMap
