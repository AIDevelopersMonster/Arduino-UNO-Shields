#ifndef LIBRARY_STARTUP_TRACE_H
#define LIBRARY_STARTUP_TRACE_H
#include <Arduino.h>

enum LibraryStartupTraceStage : uint8_t {
  LS_RESET, LS_MR08, LS_MR10, LS_MR12, LS_MR00, LS_VERSION
};
enum LibraryStartupSnapshot : uint8_t {
  LS_BEFORE_SD, LS_AFTER_SD, LS_BEFORE_ETH, LS_AFTER_ETH
};
void libraryStartupTraceBegin(uint8_t chip);
void libraryStartupTraceReset(uint8_t observed);
void libraryStartupTraceObserve(uint8_t stage, uint8_t expected, uint8_t observed);
void libraryStartupTraceDetected();
void libraryStartupTraceSnapshot(uint8_t phase);
void libraryStartupTracePrint();
#endif
