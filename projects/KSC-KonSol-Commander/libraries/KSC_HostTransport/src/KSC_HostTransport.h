#pragma once

#include <Arduino.h>

class KscHostMuxStream : public Stream {
public:
  static const uint8_t SOF1 = 0x1B;
  static const uint8_t SOF2 = 0x5D;

  static const uint8_t TYPE_PING_REQ = 0x01;
  static const uint8_t TYPE_MOUNT_REQ = 0x02;
  static const uint8_t TYPE_LS_REQ = 0x03;
  static const uint8_t TYPE_STAT_REQ = 0x04;

  static const uint8_t TYPE_PING_RESP = 0x81;
  static const uint8_t TYPE_MOUNT_RESP = 0x82;
  static const uint8_t TYPE_LS_RESP = 0x83;
  static const uint8_t TYPE_STAT_RESP = 0x84;

  static const uint8_t TYPE_ERROR_RESP = 0x7F;

  static const uint8_t ERR_BAD_LENGTH = 0x01;
  static const uint8_t ERR_BAD_CRC = 0x02;
  static const uint8_t ERR_TIMEOUT = 0x03;
  static const uint8_t ERR_UNSUPPORTED = 0x04;

  static const uint8_t MAX_PAYLOAD = 48;
  static const uint8_t TTY_QUEUE_SIZE = 32;

  explicit KscHostMuxStream(Stream &wire);

  void service();

  int available() override;
  int read() override;
  int peek() override;
  void flush() override;

  size_t write(uint8_t value) override;
  size_t write(
    const uint8_t *buffer,
    size_t size
  ) override;

  using Print::write;

  bool exchange(
    uint8_t requestType,
    const uint8_t *requestPayload,
    uint8_t requestLen,
    uint8_t expectedResponseType,
    uint8_t *responsePayload,
    uint8_t responseCapacity,
    uint8_t &responseLen,
    uint8_t &errorCode,
    unsigned long timeoutMs = 400UL
  );

  uint16_t framesRx() const {
    return _framesRx;
  }

  uint16_t framesTx() const {
    return _framesTx;
  }

  uint16_t crcErrors() const {
    return _crcErrors;
  }

  uint16_t timeouts() const {
    return _timeouts;
  }

  uint16_t badLengths() const {
    return _badLengths;
  }

  uint16_t unsupportedFrames() const {
    return _unsupportedFrames;
  }

  uint16_t ttyDrops() const {
    return _ttyDrops;
  }

  static uint8_t crc8Update(
    uint8_t crc,
    uint8_t value
  );

private:
  enum ParserState : uint8_t {
    PARSER_IDLE = 0,
    PARSER_SOF1,
    PARSER_TYPE,
    PARSER_SEQ,
    PARSER_LEN,
    PARSER_PAYLOAD,
    PARSER_CRC,
    PARSER_DISCARD
  };

  Stream &_wire;

  uint8_t _ttyQueue[TTY_QUEUE_SIZE];
  uint8_t _ttyHead;
  uint8_t _ttyTail;
  uint8_t _ttyCount;

  ParserState _state;
  uint8_t _type;
  uint8_t _seq;
  uint8_t _len;
  uint8_t _payloadIndex;
  uint8_t _payload[MAX_PAYLOAD];
  uint8_t _crc;
  uint16_t _discardRemaining;
  unsigned long _lastParserByteMs;

  bool _waitingResponse;
  bool _responseReady;
  uint8_t _responseType;
  uint8_t _responseSeq;
  uint8_t _responseLen;
  uint8_t _requestSeq;

  uint16_t _framesRx;
  uint16_t _framesTx;
  uint16_t _crcErrors;
  uint16_t _timeouts;
  uint16_t _badLengths;
  uint16_t _unsupportedFrames;
  uint16_t _ttyDrops;

  bool queueTty(uint8_t value);
  void processWireByte(uint8_t value);
  void serviceTimeout();

  void resetParser();
  void startFrame();

  bool isKnownResponse(
    uint8_t type
  ) const;

  void handleFrame();

  void sendFrame(
    uint8_t type,
    uint8_t seq,
    const uint8_t *payload,
    uint8_t len
  );

  void sendError(
    uint8_t seq,
    uint8_t errorCode
  );
};
