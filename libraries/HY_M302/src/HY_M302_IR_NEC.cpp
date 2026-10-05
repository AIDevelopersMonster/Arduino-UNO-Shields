#include "HY_M302.h"

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
#include <avr/interrupt.h>

namespace {

enum IrAsyncState : uint8_t {
  IR_WAIT_LEADER_LOW = 0,
  IR_WAIT_LEADER_HIGH,
  IR_WAIT_REPEAT_LOW,
  IR_WAIT_BIT_LOW,
  IR_WAIT_BIT_HIGH
};

static const uint8_t IR_EDGE_BUFFER_SIZE = 96;
static const uint8_t IR_FRAME_QUEUE_SIZE = 4;

volatile uint16_t g_edgeDurationUs[IR_EDGE_BUFFER_SIZE];
volatile uint8_t g_edgeLevel[IR_EDGE_BUFFER_SIZE];
volatile uint8_t g_edgeHead = 0;
volatile uint8_t g_edgeTail = 0;
volatile uint16_t g_droppedEdges = 0;
volatile uint32_t g_lastEdgeUs = 0;
volatile bool g_captureEnabled = false;

HY_M302::IrNecFrame g_frames[IR_FRAME_QUEUE_SIZE];
uint8_t g_frameHead = 0;
uint8_t g_frameTail = 0;
uint16_t g_droppedFrames = 0;

IrAsyncState g_state = IR_WAIT_LEADER_LOW;
uint8_t g_bitIndex = 0;
uint32_t g_raw = 0;
HY_M302::IrNecFrame g_lastFull = {0, 0, 0, false, false};

static bool inRangeLocal(unsigned long value,
                         unsigned long minUs,
                         unsigned long maxUs) {
  return value >= minUs && value <= maxUs;
}

static void resetDecoder() {
  g_state = IR_WAIT_LEADER_LOW;
  g_bitIndex = 0;
  g_raw = 0;
}

static void queueFrame(const HY_M302::IrNecFrame& frame) {
  uint8_t next = uint8_t(g_frameHead + 1);
  if (next >= IR_FRAME_QUEUE_SIZE) next = 0;

  if (next == g_frameTail) {
    ++g_droppedFrames;
    return;
  }

  g_frames[g_frameHead] = frame;
  g_frameHead = next;
}

static void processPulse(uint8_t level, uint16_t durationUs) {
  if (level == LOW && inRangeLocal(durationUs, 8000UL, 10000UL)) {
    g_state = IR_WAIT_LEADER_HIGH;
    g_bitIndex = 0;
    g_raw = 0;
    return;
  }

  switch (g_state) {
    case IR_WAIT_LEADER_LOW:
      return;

    case IR_WAIT_LEADER_HIGH:
      if (level != HIGH) {
        resetDecoder();
        return;
      }

      if (inRangeLocal(durationUs, 3800UL, 5200UL)) {
        g_bitIndex = 0;
        g_raw = 0;
        g_state = IR_WAIT_BIT_LOW;
        return;
      }

      if (inRangeLocal(durationUs, 1800UL, 2800UL)) {
        g_state = IR_WAIT_REPEAT_LOW;
        return;
      }

      resetDecoder();
      return;

    case IR_WAIT_REPEAT_LOW:
      if (level == LOW && inRangeLocal(durationUs, 300UL, 900UL)) {
        HY_M302::IrNecFrame repeat = g_lastFull;
        repeat.repeat = true;
        repeat.ok = true;
        queueFrame(repeat);
      }
      resetDecoder();
      return;

    case IR_WAIT_BIT_LOW:
      if (level == LOW && inRangeLocal(durationUs, 300UL, 900UL)) {
        g_state = IR_WAIT_BIT_HIGH;
        return;
      }
      resetDecoder();
      return;

    case IR_WAIT_BIT_HIGH:
      if (level != HIGH) {
        resetDecoder();
        return;
      }

      if (inRangeLocal(durationUs, 300UL, 900UL)) {
        // logical zero
      } else if (inRangeLocal(durationUs, 1200UL, 2100UL)) {
        g_raw |= (uint32_t(1) << g_bitIndex);
      } else {
        resetDecoder();
        return;
      }

      ++g_bitIndex;

      if (g_bitIndex < 32) {
        g_state = IR_WAIT_BIT_LOW;
        return;
      }

      {
        const uint8_t b0 = uint8_t(g_raw & 0xFFUL);
        const uint8_t b1 = uint8_t((g_raw >> 8) & 0xFFUL);
        const uint8_t b2 = uint8_t((g_raw >> 16) & 0xFFUL);
        const uint8_t b3 = uint8_t((g_raw >> 24) & 0xFFUL);

        if (uint8_t(b2 ^ b3) == 0xFFU) {
          HY_M302::IrNecFrame frame;
          frame.raw = g_raw;
          frame.address =
              (uint8_t(b0 ^ b1) == 0xFFU)
                  ? uint16_t(b0)
                  : uint16_t(b0) | (uint16_t(b1) << 8);
          frame.command = b2;
          frame.repeat = false;
          frame.ok = true;

          g_lastFull = frame;
          queueFrame(frame);
        }
      }

      resetDecoder();
      return;
  }
}

ISR(PCINT2_vect) {
  if (!g_captureEnabled) return;

  const uint32_t now = micros();
  const uint8_t newLevel = (PIND & _BV(PD6)) ? HIGH : LOW;
  const uint8_t previousLevel = (newLevel == HIGH) ? LOW : HIGH;
  const uint32_t elapsed = now - g_lastEdgeUs;
  g_lastEdgeUs = now;

  uint8_t next = uint8_t(g_edgeHead + 1);
  if (next >= IR_EDGE_BUFFER_SIZE) next = 0;

  if (next == g_edgeTail) {
    ++g_droppedEdges;
    return;
  }

  g_edgeDurationUs[g_edgeHead] =
      (elapsed > 65535UL) ? 65535U : uint16_t(elapsed);
  g_edgeLevel[g_edgeHead] = previousLevel;
  g_edgeHead = next;
}

}  // namespace
#endif

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

bool HY_M302::inRange(unsigned long value,
                       unsigned long minUs,
                       unsigned long maxUs) {
  return value >= minUs && value <= maxUs;
}

bool HY_M302::readIrNec(IrNecFrame& frame, unsigned long startTimeoutUs) {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  if (g_captureEnabled) return false;
#endif

  frame.address = 0;
  frame.command = 0;
  frame.raw = 0;
  frame.repeat = false;
  frame.ok = false;

  const unsigned long leadLow = pulseIn(_pins.ir, LOW, startTimeoutUs);
  if (!inRange(leadLow, 8000UL, 10000UL)) return false;

  const uint8_t bitMask = digitalPinToBitMask(_pins.ir);
  const uint8_t port = digitalPinToPort(_pins.ir);
  if (port == NOT_A_PIN) return false;
  volatile uint8_t* inputReg = portInputRegister(port);

  const unsigned long leadHigh =
      measureCurrentPulseUs(inputReg, bitMask, HIGH, 6000UL);

  if (inRange(leadHigh, 1800UL, 2800UL)) {
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

  frame.address =
      (uint8_t(b0 ^ b1) == 0xFFU)
          ? uint16_t(b0)
          : uint16_t(b0) | (uint16_t(b1) << 8);
  frame.command = b2;
  frame.ok = true;
  return true;
}

bool HY_M302::beginIrNecAsync() {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  if (_pins.ir != 6) return false;

  endIrNecAsync();

  pinMode(_pins.ir, INPUT);
  resetDecoder();
  g_frameHead = 0;
  g_frameTail = 0;
  g_droppedFrames = 0;
  g_lastFull = {0, 0, 0, false, false};

  const uint32_t now = micros();
  const uint8_t savedSreg = SREG;
  cli();

  g_edgeHead = 0;
  g_edgeTail = 0;
  g_droppedEdges = 0;
  g_lastEdgeUs = now;
  g_captureEnabled = true;

  PCIFR |= _BV(PCIF2);
  PCMSK2 |= _BV(PCINT22);
  PCICR |= _BV(PCIE2);

  SREG = savedSreg;
  return true;
#else
  return false;
#endif
}

void HY_M302::endIrNecAsync() {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  const uint8_t savedSreg = SREG;
  cli();

  g_captureEnabled = false;
  PCMSK2 &= uint8_t(~_BV(PCINT22));
  if (PCMSK2 == 0) {
    PCICR &= uint8_t(~_BV(PCIE2));
  }

  SREG = savedSreg;
  resetDecoder();
#endif
}

void HY_M302::serviceIrNec() {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  if (!g_captureEnabled) return;

  while (g_edgeTail != g_edgeHead) {
    const uint8_t index = g_edgeTail;
    const uint16_t durationUs = g_edgeDurationUs[index];
    const uint8_t level = g_edgeLevel[index];

    uint8_t next = uint8_t(index + 1);
    if (next >= IR_EDGE_BUFFER_SIZE) next = 0;
    g_edgeTail = next;

    processPulse(level, durationUs);
  }
#endif
}

bool HY_M302::irNecAvailable() const {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  return g_frameHead != g_frameTail;
#else
  return false;
#endif
}

bool HY_M302::readIrNecAsync(IrNecFrame& frame) {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  if (g_frameHead == g_frameTail) return false;

  frame = g_frames[g_frameTail];

  uint8_t next = uint8_t(g_frameTail + 1);
  if (next >= IR_FRAME_QUEUE_SIZE) next = 0;
  g_frameTail = next;
  return true;
#else
  (void)frame;
  return false;
#endif
}

uint16_t HY_M302::irNecDroppedEdges() const {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  const uint8_t savedSreg = SREG;
  cli();
  const uint16_t value = g_droppedEdges;
  SREG = savedSreg;
  return value;
#else
  return 0;
#endif
}

uint16_t HY_M302::irNecDroppedFrames() const {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  return g_droppedFrames;
#else
  return 0;
#endif
}

void HY_M302::resetIrNecStats() {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  const uint8_t savedSreg = SREG;
  cli();
  g_droppedEdges = 0;
  SREG = savedSreg;
  g_droppedFrames = 0;
#endif
}
