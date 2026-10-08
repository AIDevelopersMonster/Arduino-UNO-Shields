#pragma once

#include <Arduino.h>
#include <KSC_Core.h>

class KscReferenceTarget : public KscTarget {
public:
  KscReferenceTarget();

  void begin() override;
  void service() override;
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
  uint8_t _value;
  bool _flag;
};
