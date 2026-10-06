#pragma once

#include <Arduino.h>

namespace HY_M302_Remote {

enum Key : uint8_t {
  KEY_NONE = 0,
  KEY_0,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_OK,
  KEY_HOME,
  KEY_RETURN,
  KEY_MENU,
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_POWER
};

bool isDigit(Key key);
int8_t digit(Key key);
const __FlashStringHelper* keyName(Key key);

}  // namespace HY_M302_Remote
