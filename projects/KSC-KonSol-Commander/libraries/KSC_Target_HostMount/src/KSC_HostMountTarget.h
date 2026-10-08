#pragma once

#include <Arduino.h>
#include <KSC_Core.h>
#include <KSC_HostTransport.h>

class KscHostMountTarget : public KscTarget {
public:
  KscHostMountTarget(
    KscTarget &localTarget,
    KscHostMuxStream &transport
  );

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

  KscResult streamRead(
    const char *path,
    Print &out,
    bool &handled
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
  static const uint8_t REMOTE_NAME_MAX = 15;

  static const uint8_t HOST_TYPE_NONE = 0;
  static const uint8_t HOST_TYPE_DIR = 1;
  static const uint8_t HOST_TYPE_FILE = 2;

  KscTarget &_local;
  KscHostMuxStream &_transport;

  bool _mounted;
  uint16_t _hostErrors;
  uint16_t _streamRecoveries;

  bool isHostPath(
    const char *path
  ) const;

  bool ensureMounted();

  bool remoteStat(
    const char *path,
    uint8_t &hostType
  );

  bool remoteList(
    const char *path,
    uint8_t index,
    uint8_t &count,
    uint8_t &hostType,
    char *nameOut,
    uint8_t nameOutSize
  );

  bool remoteOpen(
    const char *path,
    uint8_t &handle
  );

  bool remoteRead(
    uint8_t handle,
    uint32_t offset,
    uint8_t *dataOut,
    uint8_t dataCapacity,
    uint8_t &dataLen,
    bool &eof,
    uint8_t &errorCode
  );

  void remoteClose(
    uint8_t handle
  );

  static KscNodeType toNodeType(
    uint8_t hostType
  );
};
