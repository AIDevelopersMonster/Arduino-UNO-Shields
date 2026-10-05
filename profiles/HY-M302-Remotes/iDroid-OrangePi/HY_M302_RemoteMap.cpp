#include "HY_M302_RemoteMap.h"

namespace HY_M302_RemoteMap {

const Entry kEntries[] = {
  {KEY_0,      0x0004, 0x47, 0xB847FB04UL},
  {KEY_1,      0x0004, 0x13, 0xEC13FB04UL},
  {KEY_2,      0x0004, 0x10, 0xEF10FB04UL},
  {KEY_3,      0x0004, 0x11, 0xEE11FB04UL},
  {KEY_4,      0x0004, 0x0F, 0xF00FFB04UL},
  {KEY_5,      0x0004, 0x0C, 0xF30CFB04UL},
  {KEY_6,      0x0004, 0x0D, 0xF20DFB04UL},
  {KEY_7,      0x0004, 0x0B, 0xF40BFB04UL},
  {KEY_8,      0x0004, 0x08, 0xF708FB04UL},
  {KEY_9,      0x0004, 0x09, 0xF609FB04UL},
  {KEY_OK,     0x0004, 0x5C, 0xA35CFB04UL},
  {KEY_HOME,   0x0004, 0x1F, 0xE01FFB04UL},
  {KEY_RETURN, 0x0004, 0x0A, 0xF50AFB04UL},
  {KEY_MENU,   0x0004, 0x5D, 0xA25DFB04UL},
  {KEY_UP,     0x0004, 0x44, 0xBB44FB04UL},
  {KEY_DOWN,   0x0004, 0x1D, 0xE21DFB04UL},
  {KEY_LEFT,   0x0004, 0x1C, 0xE31CFB04UL},
  {KEY_RIGHT,  0x0004, 0x48, 0xB748FB04UL},
  {KEY_POWER,  0x0004, 0x1A, 0xE51AFB04UL},
};

const uint8_t kEntryCount = sizeof(kEntries) / sizeof(kEntries[0]);

Key decode(uint16_t address, uint8_t command) {
  for (uint8_t i = 0; i < kEntryCount; ++i) {
    if (kEntries[i].address == address &&
        kEntries[i].command == command) {
      return kEntries[i].key;
    }
  }
  return KEY_NONE;
}

}  // namespace HY_M302_RemoteMap
