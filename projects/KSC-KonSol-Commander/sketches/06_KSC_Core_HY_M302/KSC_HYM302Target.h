#pragma once

#include <Arduino.h>
#include <KSC_Core.h>
#include <HY_M302.h>

class KscHyM302Target : public KscTarget {
public:
  KscHyM302Target();

  void begin() override;
  void service() override;
  void pollInput(KscCore &core) override;

  void name(char *out, uint8_t outSize) override;
  void bootReport(Print &out) override;

  KscNodeType pathType(const char *path) override;
  uint8_t dirCount(const char *path) override;

  bool dirEntry(
    const char *path,
    uint8_t index,
    char *nameOut,
    uint8_t nameOutSize,
    KscNodeType &typeOut
  ) override;

  KscResult read(
    const char *path,
    char *out,
    uint8_t outSize
  ) override;

  KscResult write(
    const char *path,
    long value
  ) override;

  bool writableRange(
    const char *path,
    uint16_t &minValue,
    uint16_t &maxValue
  ) override;

  bool readWritableLong(
    const char *path,
    long &value
  ) override;

  bool pathIsDynamic(
    const char *path
  ) override;

  void printStatus(Print &out) override;

private:
  HY_M302 _shield;

  bool _irOk;

  bool _ledRed;
  bool _ledBlue;
  bool _buzzer;

  uint8_t _rgbR;
  uint8_t _rgbG;
  uint8_t _rgbB;

  HY_M302::DhtReading _dht;
  unsigned long _dhtLastMs;
  bool _dhtHaveValue;

  void refreshDht();

  static void copyText(
    char *out,
    uint8_t outSize,
    const char *text
  );
};
