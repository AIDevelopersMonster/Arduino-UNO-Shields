#include "HY_M302.h"

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
#include <avr/interrupt.h>

namespace {
static const uint8_t IR_EDGE_BUFFER_SIZE = 96;

volatile uint16_t g_irEdgeDurationUs[IR_EDGE_BUFFER_SIZE];
volatile uint8_t g_irEdgeLevel[IR_EDGE_BUFFER_SIZE];
volatile uint8_t g_irEdgeHead = 0;
volatile uint8_t g_irEdgeTail = 0;
volatile uint16_t g_irDroppedEdges = 0;
volatile uint32_t g_irLastEdgeUs = 0;
volatile bool g_irCaptureEnabled = false;

ISR(PCINT2_vect) {
  if (!g_irCaptureEnabled) return;

  const uint32_t now = micros();
  const uint8_t newLevel = (PIND & _BV(PD6)) ? HIGH : LOW;
  const uint8_t previousLevel = (newLevel == HIGH) ? LOW : HIGH;
  const uint32_t elapsed = now - g_irLastEdgeUs;
  g_irLastEdgeUs = now;

  uint8_t next = uint8_t(g_irEdgeHead + 1);
  if (next >= IR_EDGE_BUFFER_SIZE) next = 0;

  if (next == g_irEdgeTail) {
    ++g_irDroppedEdges;
    return;
  }

  g_irEdgeDurationUs[g_irEdgeHead] =
      (elapsed > 65535UL) ? 65535U : uint16_t(elapsed);
  g_irEdgeLevel[g_irEdgeHead] = previousLevel;
  g_irEdgeHead = next;
}
}  // namespace
#endif

HY_M302::HY_M302() : _pins() {}
HY_M302::HY_M302(const PinMap& pins) : _pins(pins) {}

void HY_M302::begin() {
  pinMode(_pins.sw1, INPUT_PULLUP);
  pinMode(_pins.sw2, INPUT_PULLUP);

  pinMode(_pins.buzzer, OUTPUT);
  pinMode(_pins.ir, INPUT);
  pinMode(_pins.gpio7, INPUT);
  pinMode(_pins.gpio8, INPUT);

  pinMode(_pins.rgb1, OUTPUT);
  pinMode(_pins.rgb2, OUTPUT);
  pinMode(_pins.rgb3, OUTPUT);
  pinMode(_pins.led1, OUTPUT);
  pinMode(_pins.led2, OUTPUT);

  digitalWrite(_pins.led1, LOW);
  digitalWrite(_pins.led2, LOW);
  rgbOff();
  buzzerOff();
}

bool HY_M302::button1Pressed() const {
  return digitalRead(_pins.sw1) == LOW;
}

bool HY_M302::button2Pressed() const {
  return digitalRead(_pins.sw2) == LOW;
}

void HY_M302::led1(bool on) {
  digitalWrite(_pins.led1, on ? HIGH : LOW);
}

void HY_M302::led2(bool on) {
  digitalWrite(_pins.led2, on ? HIGH : LOW);
}

void HY_M302::ledBlue(bool on) {
  led1(on);
}

void HY_M302::ledRed(bool on) {
  led2(on);
}

void HY_M302::setRGB(uint8_t red, uint8_t green, uint8_t blue) {
  analogWrite(_pins.rgb1, red);
  analogWrite(_pins.rgb2, green);
  analogWrite(_pins.rgb3, blue);
}

void HY_M302::setRgbRaw(uint8_t ch1, uint8_t ch2, uint8_t ch3) {
  analogWrite(_pins.rgb1, ch1);
  analogWrite(_pins.rgb2, ch2);
  analogWrite(_pins.rgb3, ch3);
}

void HY_M302::rgbOff() {
  setRGB(0, 0, 0);
}

void HY_M302::buzzerOn() {
  noTone(_pins.buzzer);
  pinMode(_pins.buzzer, OUTPUT);
  digitalWrite(_pins.buzzer, HIGH);
}

void HY_M302::buzzerTone(unsigned int frequency, unsigned long durationMs) {
  if (durationMs == 0) tone(_pins.buzzer, frequency);
  else tone(_pins.buzzer, frequency, durationMs);
}

void HY_M302::buzzerOff() {
  noTone(_pins.buzzer);
  digitalWrite(_pins.buzzer, LOW);
}

int HY_M302::readPotRaw() const {
  return analogRead(_pins.pot);
}

int HY_M302::readLightRaw() const {
  return analogRead(_pins.light);
}

int HY_M302::readLm35Raw() const {
  return analogRead(_pins.lm35);
}

int HY_M302::readAnalog3Raw() const {
  return analogRead(_pins.analog3);
}

float HY_M302::readLm35C(float aref) const {
  const int raw = readLm35Raw();
  const float volts = (raw * aref) / 1023.0f;
  return volts * 100.0f;
}

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

  // DHT11 host start signal.
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

  // Loop-count timeout only. Do not use micros() while interrupts are disabled:
  // on AVR, micros() depends on Timer0 overflow bookkeeping.
  const uint32_t maxLoops = microsecondsToClockCycles(120UL) / 4UL + 20UL;

  noInterrupts();

  // Sensor response: ~80 us LOW, then ~80 us HIGH.
  if (expectPulse(inputReg, bitMask, LOW, maxLoops) == 0 ||
      expectPulse(inputReg, bitMask, HIGH, maxLoops) == 0) {
    interrupts();
    return out;
  }

  // Each data bit: ~50 us LOW followed by either ~26-28 us HIGH (0)
  // or ~70 us HIGH (1). Compare HIGH against the preceding LOW duration;
  // this avoids relying on an absolute microsecond threshold.
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

unsigned long HY_M302::measureCurrentPulseUs(volatile uint8_t* inputReg,
                                               uint8_t bitMask,
                                               uint8_t level,
                                               unsigned long timeoutUs) {
  const unsigned long started = micros();

  if (level == HIGH) {
    if ((*inputReg & bitMask) == 0) return 0;
    while ((*inputReg & bitMask) != 0) {
      if ((micros() - started) >= timeoutUs) return 0;
    }
  } else {
    if ((*inputReg & bitMask) != 0) return 0;
    while ((*inputReg & bitMask) == 0) {
      if ((micros() - started) >= timeoutUs) return 0;
    }
  }

  return micros() - started;
}

bool HY_M302::inRange(unsigned long value, unsigned long minUs, unsigned long maxUs) {
  return value >= minUs && value <= maxUs;
}

bool HY_M302::readIrNec(IrNecFrame& frame, unsigned long startTimeoutUs) {
  if (_irAsyncEnabled) return false;

  frame.address = 0;
  frame.command = 0;
  frame.raw = 0;
  frame.repeat = false;
  frame.ok = false;

  // pulseIn() is used only to wait for and measure the initial LOW leader.
  // When it returns, the receiver has just transitioned to HIGH.
  const unsigned long leadLow = pulseIn(_pins.ir, LOW, startTimeoutUs);
  if (!inRange(leadLow, 8000UL, 10000UL)) return false;

  const uint8_t bitMask = digitalPinToBitMask(_pins.ir);
  const uint8_t port = digitalPinToPort(_pins.ir);
  if (port == NOT_A_PIN) return false;
  volatile uint8_t* inputReg = portInputRegister(port);

  // IMPORTANT: do not call pulseIn(HIGH) here. The leader HIGH is already in
  // progress, and pulseIn() would first wait for that pulse to end and skip it.
  const unsigned long leadHigh =
      measureCurrentPulseUs(inputReg, bitMask, HIGH, 6000UL);

  if (inRange(leadHigh, 1800UL, 2800UL)) {
    // NEC repeat: 9 ms LOW + 2.25 ms HIGH + ~560 us LOW.
    const unsigned long repeatLow =
        measureCurrentPulseUs(inputReg, bitMask, LOW, 1200UL);
    if (inRange(repeatLow, 300UL, 900UL)) {
      frame.repeat = true;
      frame.ok = true;
      return true;
    }
    return false;
  }

  if (!inRange(leadHigh, 3800UL, 5200UL)) return false;

  uint32_t raw = 0;

  for (uint8_t i = 0; i < 32; ++i) {
    // At this point each next pulse is already active. Measure the current
    // level directly so no transition is skipped.
    const unsigned long bitLow =
        measureCurrentPulseUs(inputReg, bitMask, LOW, 1200UL);
    if (!inRange(bitLow, 300UL, 900UL)) return false;

    const unsigned long bitHigh =
        measureCurrentPulseUs(inputReg, bitMask, HIGH, 2300UL);

    if (inRange(bitHigh, 300UL, 900UL)) {
      // logical zero
    } else if (inRange(bitHigh, 1200UL, 2100UL)) {
      raw |= (uint32_t(1) << i);
    } else {
      return false;
    }
  }

  frame.raw = raw;

  const uint8_t b0 = uint8_t(raw & 0xFFUL);
  const uint8_t b1 = uint8_t((raw >> 8) & 0xFFUL);
  const uint8_t b2 = uint8_t((raw >> 16) & 0xFFUL);
  const uint8_t b3 = uint8_t((raw >> 24) & 0xFFUL);

  if (uint8_t(b2 ^ b3) != 0xFFU) return false;

  if (uint8_t(b0 ^ b1) == 0xFFU) {
    frame.address = b0;
  } else {
    frame.address = uint16_t(b0) | (uint16_t(b1) << 8);
  }

  frame.command = b2;
  frame.ok = true;
  return true;
}


void HY_M302::resetIrAsyncDecoder() {
  _irAsyncState = IR_WAIT_LEADER_LOW;
  _irAsyncBitIndex = 0;
  _irAsyncRaw = 0;
}

void HY_M302::queueIrAsyncFrame(const IrNecFrame& frame) {
  uint8_t next = uint8_t(_irAsyncFrameHead + 1);
  if (next >= IR_ASYNC_FRAME_QUEUE_SIZE) next = 0;

  if (next == _irAsyncFrameTail) {
    ++_irAsyncDroppedFrames;
    return;
  }

  _irAsyncFrames[_irAsyncFrameHead] = frame;
  _irAsyncFrameHead = next;
}

void HY_M302::processIrAsyncPulse(uint8_t level, uint16_t durationUs) {
  // A valid NEC leader LOW is also a natural re-synchronization point.
  if (level == LOW && inRange(durationUs, 8000UL, 10000UL)) {
    _irAsyncState = IR_WAIT_LEADER_HIGH;
    _irAsyncBitIndex = 0;
    _irAsyncRaw = 0;
    return;
  }

  switch (_irAsyncState) {
    case IR_WAIT_LEADER_LOW:
      return;

    case IR_WAIT_LEADER_HIGH:
      if (level != HIGH) {
        resetIrAsyncDecoder();
        return;
      }

      if (inRange(durationUs, 3800UL, 5200UL)) {
        _irAsyncBitIndex = 0;
        _irAsyncRaw = 0;
        _irAsyncState = IR_WAIT_BIT_LOW;
        return;
      }

      if (inRange(durationUs, 1800UL, 2800UL)) {
        _irAsyncState = IR_WAIT_REPEAT_LOW;
        return;
      }

      resetIrAsyncDecoder();
      return;

    case IR_WAIT_REPEAT_LOW:
      if (level == LOW && inRange(durationUs, 300UL, 900UL)) {
        IrNecFrame repeat = _irAsyncLastFull;
        repeat.repeat = true;
        repeat.ok = true;
        queueIrAsyncFrame(repeat);
      }
      resetIrAsyncDecoder();
      return;

    case IR_WAIT_BIT_LOW:
      if (level == LOW && inRange(durationUs, 300UL, 900UL)) {
        _irAsyncState = IR_WAIT_BIT_HIGH;
        return;
      }
      resetIrAsyncDecoder();
      return;

    case IR_WAIT_BIT_HIGH:
      if (level != HIGH) {
        resetIrAsyncDecoder();
        return;
      }

      if (inRange(durationUs, 300UL, 900UL)) {
        // logical zero
      } else if (inRange(durationUs, 1200UL, 2100UL)) {
        _irAsyncRaw |= (uint32_t(1) << _irAsyncBitIndex);
      } else {
        resetIrAsyncDecoder();
        return;
      }

      ++_irAsyncBitIndex;

      if (_irAsyncBitIndex < 32) {
        _irAsyncState = IR_WAIT_BIT_LOW;
        return;
      }

      {
        const uint8_t b0 = uint8_t(_irAsyncRaw & 0xFFUL);
        const uint8_t b1 = uint8_t((_irAsyncRaw >> 8) & 0xFFUL);
        const uint8_t b2 = uint8_t((_irAsyncRaw >> 16) & 0xFFUL);
        const uint8_t b3 = uint8_t((_irAsyncRaw >> 24) & 0xFFUL);

        if (uint8_t(b2 ^ b3) == 0xFFU) {
          IrNecFrame frame;
          frame.raw = _irAsyncRaw;
          frame.address =
              (uint8_t(b0 ^ b1) == 0xFFU)
                  ? uint16_t(b0)
                  : uint16_t(b0) | (uint16_t(b1) << 8);
          frame.command = b2;
          frame.repeat = false;
          frame.ok = true;

          _irAsyncLastFull = frame;
          queueIrAsyncFrame(frame);
        }
      }

      resetIrAsyncDecoder();
      return;
  }
}

bool HY_M302::beginIrNecAsync() {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  // The optimized interrupt path is intentionally tied to the standard
  // HY-M302 wiring: D6 = PD6 = PCINT22 on Arduino UNO-class ATmega328P/168.
  if (_pins.ir != 6) return false;

  endIrNecAsync();

  pinMode(_pins.ir, INPUT);
  resetIrAsyncDecoder();
  _irAsyncFrameHead = 0;
  _irAsyncFrameTail = 0;
  _irAsyncDroppedFrames = 0;
  _irAsyncLastFull = {0, 0, 0, false, false};

  const uint32_t now = micros();
  const uint8_t savedSreg = SREG;
  cli();

  g_irEdgeHead = 0;
  g_irEdgeTail = 0;
  g_irDroppedEdges = 0;
  g_irLastEdgeUs = now;
  g_irCaptureEnabled = true;

  PCIFR |= _BV(PCIF2);       // clear any pending Port D pin-change flag
  PCMSK2 |= _BV(PCINT22);    // D6 / PD6
  PCICR |= _BV(PCIE2);       // enable Port D pin-change group

  SREG = savedSreg;

  _irAsyncEnabled = true;
  return true;
#else
  return false;
#endif
}

void HY_M302::endIrNecAsync() {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  const uint8_t savedSreg = SREG;
  cli();

  g_irCaptureEnabled = false;
  PCMSK2 &= uint8_t(~_BV(PCINT22));
  if (PCMSK2 == 0) {
    PCICR &= uint8_t(~_BV(PCIE2));
  }

  SREG = savedSreg;
#endif

  _irAsyncEnabled = false;
  resetIrAsyncDecoder();
}

void HY_M302::serviceIrNec() {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  if (!_irAsyncEnabled) return;

  while (g_irEdgeTail != g_irEdgeHead) {
    const uint8_t index = g_irEdgeTail;
    const uint16_t durationUs = g_irEdgeDurationUs[index];
    const uint8_t level = g_irEdgeLevel[index];

    uint8_t next = uint8_t(index + 1);
    if (next >= IR_EDGE_BUFFER_SIZE) next = 0;
    g_irEdgeTail = next;

    processIrAsyncPulse(level, durationUs);
  }
#endif
}

bool HY_M302::irNecAvailable() const {
  return _irAsyncFrameHead != _irAsyncFrameTail;
}

bool HY_M302::readIrNecAsync(IrNecFrame& frame) {
  if (!irNecAvailable()) return false;

  frame = _irAsyncFrames[_irAsyncFrameTail];

  uint8_t next = uint8_t(_irAsyncFrameTail + 1);
  if (next >= IR_ASYNC_FRAME_QUEUE_SIZE) next = 0;
  _irAsyncFrameTail = next;

  return true;
}

uint16_t HY_M302::irNecDroppedEdges() const {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  const uint8_t savedSreg = SREG;
  cli();
  const uint16_t value = g_irDroppedEdges;
  SREG = savedSreg;
  return value;
#else
  return 0;
#endif
}

uint16_t HY_M302::irNecDroppedFrames() const {
  return _irAsyncDroppedFrames;
}

void HY_M302::resetIrNecStats() {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  const uint8_t savedSreg = SREG;
  cli();
  g_irDroppedEdges = 0;
  SREG = savedSreg;
#endif
  _irAsyncDroppedFrames = 0;
}

void HY_M302::gpio7Mode(uint8_t mode) {
  pinMode(_pins.gpio7, mode);
}

void HY_M302::gpio8Mode(uint8_t mode) {
  pinMode(_pins.gpio8, mode);
}

int HY_M302::gpio7Read() const {
  return digitalRead(_pins.gpio7);
}

int HY_M302::gpio8Read() const {
  return digitalRead(_pins.gpio8);
}

void HY_M302::gpio7Write(uint8_t value) {
  digitalWrite(_pins.gpio7, value);
}

void HY_M302::gpio8Write(uint8_t value) {
  digitalWrite(_pins.gpio8, value);
}
