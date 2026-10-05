#include "HY_M302.h"

uint32_t HY_M302::expectPulse(volatile uint8_t* inputReg,
                              uint8_t bitMask,
                              uint8_t level,
                              uint32_t maxLoops) {
  uint32_t count = 0;

  if (level == HIGH) {
    while ((*inputReg & bitMask) != 0) {
      if (++count >= maxLoops) return 0;
    }
  } else {
    while ((*inputReg & bitMask) == 0) {
      if (++count >= maxLoops) return 0;
    }
  }

  return count;
}

HY_M302::DhtReading HY_M302::readDht11() {
  DhtReading out = {0.0f, 0.0f, false};
  uint8_t data[5] = {0, 0, 0, 0, 0};

  pinMode(_pins.dht, OUTPUT);
  digitalWrite(_pins.dht, LOW);
  delay(20);
  digitalWrite(_pins.dht, HIGH);
  delayMicroseconds(35);
  pinMode(_pins.dht, INPUT_PULLUP);

  const uint8_t bitMask = digitalPinToBitMask(_pins.dht);
  const uint8_t port = digitalPinToPort(_pins.dht);
  if (port == NOT_A_PIN) return out;

  volatile uint8_t* inputReg = portInputRegister(port);
  const uint32_t maxLoops = microsecondsToClockCycles(120UL) / 4UL + 20UL;

  // The timing-critical DHT11 transaction occupies only a few milliseconds.
  // Interrupts are disabled so pulse classification is deterministic.
  // TEST-05 explicitly checks interaction with the asynchronous IR driver.
  noInterrupts();

  if (expectPulse(inputReg, bitMask, LOW, maxLoops) == 0 ||
      expectPulse(inputReg, bitMask, HIGH, maxLoops) == 0) {
    interrupts();
    return out;
  }

  for (uint8_t i = 0; i < 40; ++i) {
    const uint32_t lowCycles =
        expectPulse(inputReg, bitMask, LOW, maxLoops);
    const uint32_t highCycles =
        expectPulse(inputReg, bitMask, HIGH, maxLoops);

    if (lowCycles == 0 || highCycles == 0) {
      interrupts();
      return out;
    }

    data[i / 8] <<= 1;
    if (highCycles > lowCycles) data[i / 8] |= 1;
  }

  interrupts();

  const uint8_t checksum =
      uint8_t(data[0] + data[1] + data[2] + data[3]);
  if (checksum != data[4]) return out;

  out.humidity = data[0] + data[1] * 0.1f;
  out.temperatureC = data[2] + data[3] * 0.1f;
  out.ok = true;
  return out;
}
