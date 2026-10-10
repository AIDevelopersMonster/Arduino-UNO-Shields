// Buffered observation of existing driver reads; no SPI or GPIO writes.
#include "LibraryStartupTrace.h"
#if !defined(__AVR_ATmega328P__)
#error This diagnostic is restricted to the ATmega328P UNO.
#endif

namespace {
struct Attempt {
  uint8_t chip, stage, expected, observed, resetPolls, resetLast, detected;
};
struct Snapshot {
  uint8_t phase, ddrb, ddrd, portb, portd, pinb, pind, spcr, spsr;
};
Attempt attempts[3];
Snapshot snapshots[4];
uint8_t attemptCount = 0, snapshotCount = 0, current = 255;
bool overflow = false;
static_assert(sizeof(attempts) + sizeof(snapshots) + 4 <= 80, "Trace RAM budget");

void hexByte(uint8_t value) {
  if (value < 16) Serial.print('0');
  Serial.print(value, HEX);
}
const __FlashStringHelper *stageName(uint8_t stage) {
  switch (stage) {
    case LS_MR08: return F("MR08");
    case LS_MR10: return F("MR10");
    case LS_MR12: return F("MR12");
    case LS_MR00: return F("MR00");
    case LS_VERSION: return F("VERSION");
    default: return F("RESET");
  }
}
const __FlashStringHelper *phaseName(uint8_t phase) {
  switch (phase) {
    case LS_AFTER_SD: return F("AFTER_SD");
    case LS_BEFORE_ETH: return F("BEFORE_ETH");
    case LS_AFTER_ETH: return F("AFTER_ETH");
    default: return F("BEFORE_SD");
  }
}
}

void libraryStartupTraceBegin(uint8_t chip) {
  if (attemptCount >= 3) { current = 255; overflow = true; return; }
  current = attemptCount++;
  attempts[current] = {chip, LS_RESET, 0, 0, 0, 0, 0};
}
void libraryStartupTraceObserve(uint8_t stage, uint8_t expected, uint8_t observed) {
  if (current >= 3) return;
  attempts[current].stage = stage;
  attempts[current].expected = expected;
  attempts[current].observed = observed;
}
void libraryStartupTraceReset(uint8_t observed) {
  if (current >= 3) return;
  ++attempts[current].resetPolls;
  attempts[current].resetLast = observed;
  libraryStartupTraceObserve(LS_RESET, 0, observed);
}
void libraryStartupTraceDetected() {
  if (current < 3) attempts[current].detected = 1;
}
void libraryStartupTraceSnapshot(uint8_t phase) {
  if (snapshotCount >= 4) { overflow = true; return; }
  // Read-only MCU snapshots, captured outside library SPI transactions.
  snapshots[snapshotCount++] = {phase, DDRB, DDRD, PORTB, PORTD,
                               PINB, PIND, SPCR, SPSR};
}
void libraryStartupTracePrint() {
  // Called only after Ethernet.begin returns; no UART inside detection.
  Serial.print(F("TRACE_SUMMARY attempts=")); Serial.print(attemptCount);
  Serial.print(F(" snapshots=")); Serial.print(snapshotCount);
  Serial.print(F(" overflow=")); Serial.print(overflow ? 1 : 0);
  Serial.print(F(" buffer_bytes="));
  Serial.println(sizeof(attempts) + sizeof(snapshots) + 4);
  for (uint8_t i = 0; i < snapshotCount; ++i) {
    const Snapshot &s = snapshots[i];
    Serial.print(F("SPI_STATE phase=")); Serial.print(phaseName(s.phase));
    Serial.print(F(" eth_out=")); Serial.print((s.ddrb >> 2) & 1);
    Serial.print(F(" sd_out=")); Serial.print((s.ddrd >> 4) & 1);
    Serial.print(F(" eth_latch=")); Serial.print((s.portb >> 2) & 1);
    Serial.print(F(" sd_latch=")); Serial.print((s.portd >> 4) & 1);
    Serial.print(F(" eth_pin=")); Serial.print((s.pinb >> 2) & 1);
    Serial.print(F(" sd_pin=")); Serial.print((s.pind >> 4) & 1);
    Serial.print(F(" spcr=")); hexByte(s.spcr);
    Serial.print(F(" spsr=")); hexByte(s.spsr);
    Serial.println();
  }
  for (uint8_t i = 0; i < attemptCount; ++i) {
    const Attempt &a = attempts[i];
    Serial.print(F("DETECT_TRACE attempt=")); Serial.print(i + 1);
    Serial.print(F(" candidate=W")); Serial.print(a.chip); Serial.print(F("00"));
    Serial.print(F(" stage=")); Serial.print(stageName(a.stage));
    Serial.print(F(" expected=")); hexByte(a.expected);
    Serial.print(F(" observed=")); hexByte(a.observed);
    Serial.print(F(" reset_polls=")); Serial.print(a.resetPolls);
    Serial.print(F(" reset_last=")); hexByte(a.resetLast);
    Serial.print(F(" matched=")); Serial.print(a.expected == a.observed ? 1 : 0);
    Serial.print(F(" detected=")); Serial.println(a.detected);
  }
}
