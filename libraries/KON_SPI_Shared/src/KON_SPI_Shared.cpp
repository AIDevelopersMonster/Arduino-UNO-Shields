#include "KON_SPI_Shared.h"

KON_SPI_Shared::KON_SPI_Shared(uint8_t sdCs, uint8_t lanCs)
  : _sdCs(sdCs), _lanCs(lanCs), _owner(OWNER_NONE) {}

void KON_SPI_Shared::begin() {
  pinMode(_sdCs, OUTPUT);
  digitalWrite(_sdCs, HIGH);
  pinMode(_lanCs, OUTPUT);
  digitalWrite(_lanCs, HIGH);

  // On UNO, D10 is the hardware SS pin. Keeping it OUTPUT is mandatory
  // for stable SPI master mode; pinMode above already guarantees this
  // for the standard W5100 shield mapping.
  SPI.begin();
  _owner = OWNER_NONE;
}

void KON_SPI_Shared::park() {
  digitalWrite(_sdCs, HIGH);
  digitalWrite(_lanCs, HIGH);
  _owner = OWNER_NONE;
}

void KON_SPI_Shared::idleClocks(uint8_t bytes, uint32_t clockHz) {
  // Neither peripheral is selected while clocks are emitted.
  digitalWrite(_sdCs, HIGH);
  digitalWrite(_lanCs, HIGH);

  SPI.beginTransaction(SPISettings(clockHz, MSBFIRST, SPI_MODE0));
  while (bytes--) {
    SPI.transfer(0xFF);
  }
  SPI.endTransaction();
}

void KON_SPI_Shared::prepareSD() {
  park();

  // SD SPI initialization requires at least 74 clocks while CS is HIGH
  // before CMD0. The stock SD library also provides such clocks, but
  // repeating them here deliberately establishes a known bus boundary
  // after another SPI peripheral has been active.
  idleClocks(10, 250000UL); // 80 clocks
  _owner = OWNER_SD;
}

void KON_SPI_Shared::releaseSD(uint8_t releaseBytes) {
  digitalWrite(_sdCs, HIGH);
  digitalWrite(_lanCs, HIGH);

  // Some SD/MMC devices need trailing clocks after the final SPI
  // transaction before DO/MISO is guaranteed released. One byte is the
  // minimum experiment; callers may request more.
  if (releaseBytes) {
    idleClocks(releaseBytes, 250000UL);
  }
  _owner = OWNER_NONE;
}

void KON_SPI_Shared::prepareLAN() {
  // Do not assert W5100 CS here. The Ethernet library owns its CS timing.
  digitalWrite(_sdCs, HIGH);
  digitalWrite(_lanCs, HIGH);
  _owner = OWNER_LAN;
}

void KON_SPI_Shared::releaseLAN() {
  digitalWrite(_lanCs, HIGH);
  digitalWrite(_sdCs, HIGH);
  _owner = OWNER_NONE;
}

void KON_SPI_Shared::w5100Write8(uint16_t address, uint8_t value) {
  // W5100 VDM byte-write frame: 0xF0, address high, address low, data.
  digitalWrite(_sdCs, HIGH);
  digitalWrite(_lanCs, LOW);
  SPI.transfer(0xF0);
  SPI.transfer((uint8_t)(address >> 8));
  SPI.transfer((uint8_t)(address & 0xFF));
  SPI.transfer(value);
  digitalWrite(_lanCs, HIGH);
}

uint8_t KON_SPI_Shared::w5100Read8(uint16_t address) {
  // W5100 VDM byte-read frame: 0x0F, address high, address low, dummy/data.
  digitalWrite(_sdCs, HIGH);
  digitalWrite(_lanCs, LOW);
  SPI.transfer(0x0F);
  SPI.transfer((uint8_t)(address >> 8));
  SPI.transfer((uint8_t)(address & 0xFF));
  uint8_t value = SPI.transfer(0x00);
  digitalWrite(_lanCs, HIGH);
  return value;
}

bool KON_SPI_Shared::resetW5100(bool restoreBuffers) {
  park();
  SPI.beginTransaction(SPISettings(4000000UL, MSBFIRST, SPI_MODE0));

  // MR (0x0000), RST bit = 1.
  w5100Write8(0x0000, 0x80);

  bool ok = false;
  for (uint8_t i = 0; i < 20; ++i) {
    delay(1);
    if (w5100Read8(0x0000) == 0x00) {
      ok = true;
      break;
    }
  }

  if (ok && restoreBuffers) {
    // Standard W5100 / Arduino Ethernet Shield allocation:
    // 2 KiB RX and 2 KiB TX per socket for four sockets.
    w5100Write8(0x001A, 0x55); // RMSR
    w5100Write8(0x001B, 0x55); // TMSR
  }

  SPI.endTransaction();
  park();
  return ok;
}
