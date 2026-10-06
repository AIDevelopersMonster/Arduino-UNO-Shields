#include "HY_M302_Remote.h"

namespace HY_M302_Remote {

bool isDigit(Key key) {
  return key >= KEY_0 && key <= KEY_9;
}

int8_t digit(Key key) {
  if (!isDigit(key)) return -1;
  return static_cast<int8_t>(key) - static_cast<int8_t>(KEY_0);
}

const __FlashStringHelper* keyName(Key key) {
  switch (key) {
    case KEY_0:      return F("0");
    case KEY_1:      return F("1");
    case KEY_2:      return F("2");
    case KEY_3:      return F("3");
    case KEY_4:      return F("4");
    case KEY_5:      return F("5");
    case KEY_6:      return F("6");
    case KEY_7:      return F("7");
    case KEY_8:      return F("8");
    case KEY_9:      return F("9");
    case KEY_OK:     return F("OK");
    case KEY_HOME:   return F("HOME");
    case KEY_RETURN: return F("RETURN");
    case KEY_MENU:   return F("MENU");
    case KEY_UP:     return F("UP");
    case KEY_DOWN:   return F("DOWN");
    case KEY_LEFT:   return F("LEFT");
    case KEY_RIGHT:  return F("RIGHT");
    case KEY_POWER:  return F("POWER");
    default:         return F("NONE");
  }
}

}  // namespace HY_M302_Remote
