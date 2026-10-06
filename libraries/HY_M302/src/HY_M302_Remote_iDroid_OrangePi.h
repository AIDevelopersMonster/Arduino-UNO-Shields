#pragma once

#include <Arduino.h>
#include "HY_M302_Remote.h"

namespace HY_M302_Remote {
namespace IDroidOrangePi {

static const uint16_t ADDRESS = 0x0004;

Key decode(uint16_t address, uint8_t command);
bool encode(Key key, uint16_t& address, uint8_t& command);

}  // namespace IDroidOrangePi
}  // namespace HY_M302_Remote
