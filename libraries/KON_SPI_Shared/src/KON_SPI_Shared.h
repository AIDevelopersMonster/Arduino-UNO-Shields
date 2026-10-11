#ifndef KON_SPI_SHARED_H
#define KON_SPI_SHARED_H

#include <Arduino.h>
#include <SPI.h>

class KON_SPI_Shared {
public:
  enum Owner : uint8_t {
    OWNER_NONE = 0,
    OWNER_SD   = 1,
    OWNER_LAN  = 2
  };

  KON_SPI_Shared(uint8_t sdCs = 4, uint8_t lanCs = 10);

  // Configure CS pins, start AVR SPI master, park both slaves deselected.
  void begin();

  // Both chip-selects HIGH. Does not remove power from either device.
  void park();

  // Prepare for SD.begin(): both devices deselected, then >=80 clocks
  // at 250 kHz with CS HIGH as required by the SD SPI initialization path.
  void prepareSD();

  // End an SD ownership interval. Caller must close all File objects first.
  // Generates releaseBytes dummy bytes (8 clocks each) while both CS are HIGH.
  void releaseSD(uint8_t releaseBytes = 1);

  // Prepare for Ethernet/W5100 access. Both CS lines are parked HIGH.
  void prepareLAN();

  // End LAN ownership. Caller should stop sockets first.
  void releaseLAN();

  // Extra clocks with both slaves deselected. Useful for bus recovery tests.
  void idleClocks(uint8_t bytes, uint32_t clockHz = 250000UL);

  // W5100-only raw software reset via MR bit 7.
  // This bypasses Ethernet library lifecycle state. It does NOT re-run DHCP.
  // restoreBuffers=true writes RMSR/TMSR=0x55 for the normal UNO/W5100 layout.
  bool resetW5100(bool restoreBuffers = true);

  Owner owner() const { return _owner; }
  uint8_t sdCS() const { return _sdCs; }
  uint8_t lanCS() const { return _lanCs; }

private:
  uint8_t _sdCs;
  uint8_t _lanCs;
  Owner _owner;

  uint8_t w5100Read8(uint16_t address);
  void w5100Write8(uint16_t address, uint8_t value);
};

#endif
