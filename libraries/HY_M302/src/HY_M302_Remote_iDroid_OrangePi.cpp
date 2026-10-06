#include "HY_M302_Remote_iDroid_OrangePi.h"

#include <avr/pgmspace.h>

namespace HY_M302_Remote {
namespace IDroidOrangePi {

namespace {

// Index is HY_M302_Remote::Key. Entry zero is KEY_NONE.
// Stored in program memory so the learned profile does not consume UNO SRAM.
const uint8_t kCommands[] PROGMEM = {
  0xFF,  // KEY_NONE
  0x47,  // KEY_0
  0x13,  // KEY_1
  0x10,  // KEY_2
  0x11,  // KEY_3
  0x0F,  // KEY_4
  0x0C,  // KEY_5
  0x0D,  // KEY_6
  0x0B,  // KEY_7
  0x08,  // KEY_8
  0x09,  // KEY_9
  0x5C,  // KEY_OK
  0x1F,  // KEY_HOME
  0x0A,  // KEY_RETURN
  0x5D,  // KEY_MENU
  0x44,  // KEY_UP
  0x1D,  // KEY_DOWN
  0x1C,  // KEY_LEFT
  0x48,  // KEY_RIGHT
  0x1A   // KEY_POWER
};

static const uint8_t KEY_COUNT =
    static_cast<uint8_t>(KEY_POWER) + 1U;

}  // namespace

Key decode(uint16_t address, uint8_t command) {
  if (address != ADDRESS) return KEY_NONE;

  for (uint8_t i = 1; i < KEY_COUNT; ++i) {
    if (pgm_read_byte(&kCommands[i]) == command) {
      return static_cast<Key>(i);
    }
  }

  return KEY_NONE;
}

bool encode(Key key, uint16_t& address, uint8_t& command) {
  const uint8_t index = static_cast<uint8_t>(key);

  if (index == 0 || index >= KEY_COUNT) return false;

  address = ADDRESS;
  command = pgm_read_byte(&kCommands[index]);
  return true;
}

}  // namespace IDroidOrangePi
}  // namespace HY_M302_Remote
