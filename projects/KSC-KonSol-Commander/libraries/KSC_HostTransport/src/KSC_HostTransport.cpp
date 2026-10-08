#include "KSC_HostTransport.h"

KscHostMuxStream::KscHostMuxStream(
  Stream &wire
)
  : _wire(wire),
    _ttyHead(0),
    _ttyTail(0),
    _ttyCount(0),
    _state(PARSER_IDLE),
    _type(0),
    _seq(0),
    _len(0),
    _payloadIndex(0),
    _crc(0),
    _discardRemaining(0),
    _lastParserByteMs(0),
    _framesRx(0),
    _framesTx(0),
    _crcErrors(0),
    _timeouts(0),
    _badLengths(0),
    _unsupportedFrames(0),
    _ttyDrops(0) {
}

uint8_t KscHostMuxStream::crc8Update(
  uint8_t crc,
  uint8_t value
) {
  crc ^= value;

  for (uint8_t i = 0; i < 8; ++i) {
    if (crc & 0x80) {
      crc =
        (uint8_t)((crc << 1) ^ 0x07);
    } else {
      crc =
        (uint8_t)(crc << 1);
    }
  }

  return crc;
}

bool KscHostMuxStream::queueTty(
  uint8_t value
) {
  if (_ttyCount >= TTY_QUEUE_SIZE) {
    ++_ttyDrops;
    return false;
  }

  _ttyQueue[_ttyHead] = value;

  _ttyHead =
    (uint8_t)((_ttyHead + 1) %
              TTY_QUEUE_SIZE);

  ++_ttyCount;
  return true;
}

void KscHostMuxStream::resetParser() {
  _state = PARSER_IDLE;
  _type = 0;
  _seq = 0;
  _len = 0;
  _payloadIndex = 0;
  _crc = 0;
  _discardRemaining = 0;
  _lastParserByteMs = 0;
}

void KscHostMuxStream::startFrame() {
  _state = PARSER_TYPE;
  _type = 0;
  _seq = 0;
  _len = 0;
  _payloadIndex = 0;
  _crc = 0;
  _discardRemaining = 0;
  _lastParserByteMs = millis();
}

void KscHostMuxStream::sendFrame(
  uint8_t type,
  uint8_t seq,
  const uint8_t *payload,
  uint8_t len
) {
  if (len > MAX_PAYLOAD) {
    return;
  }

  uint8_t crc = 0;

  _wire.write(SOF1);
  _wire.write(SOF2);

  _wire.write(type);
  crc = crc8Update(crc, type);

  _wire.write(seq);
  crc = crc8Update(crc, seq);

  _wire.write(len);
  crc = crc8Update(crc, len);

  for (uint8_t i = 0; i < len; ++i) {
    const uint8_t value =
      payload ? payload[i] : 0;

    _wire.write(value);
    crc = crc8Update(crc, value);
  }

  _wire.write(crc);
  ++_framesTx;
}

void KscHostMuxStream::sendError(
  uint8_t seq,
  uint8_t errorCode
) {
  const uint8_t payload[1] = {
    errorCode
  };

  sendFrame(
    TYPE_ERROR_RESP,
    seq,
    payload,
    sizeof(payload)
  );
}

void KscHostMuxStream::handleFrame() {
  ++_framesRx;

  if (_type == TYPE_PING_REQ) {
    if (_len != 0) {
      ++_badLengths;
      sendError(
        _seq,
        ERR_BAD_LENGTH
      );
      return;
    }

    sendFrame(
      TYPE_PING_RESP,
      _seq,
      nullptr,
      0
    );

    return;
  }

  ++_unsupportedFrames;

  sendError(
    _seq,
    ERR_UNSUPPORTED
  );
}

void KscHostMuxStream::processWireByte(
  uint8_t value
) {
  const unsigned long now =
    millis();

  if (_state == PARSER_IDLE) {
    if (value == SOF1) {
      _state = PARSER_SOF1;
      _lastParserByteMs = now;
      return;
    }

    queueTty(value);
    return;
  }

  if (_state == PARSER_SOF1) {
    if (value == SOF2) {
      startFrame();
      return;
    }

    queueTty(SOF1);
    resetParser();

    if (value == SOF1) {
      _state = PARSER_SOF1;
      _lastParserByteMs = now;
    } else {
      queueTty(value);
    }

    return;
  }

  _lastParserByteMs = now;

  switch (_state) {
    case PARSER_TYPE:
      _type = value;
      _crc = crc8Update(0, value);
      _state = PARSER_SEQ;
      break;

    case PARSER_SEQ:
      _seq = value;
      _crc = crc8Update(_crc, value);
      _state = PARSER_LEN;
      break;

    case PARSER_LEN:
      _len = value;
      _crc = crc8Update(_crc, value);
      _payloadIndex = 0;

      if (_len > MAX_PAYLOAD) {
        ++_badLengths;

        _discardRemaining =
          (uint16_t)_len + 1U;

        _state = PARSER_DISCARD;
      } else if (_len == 0) {
        _state = PARSER_CRC;
      } else {
        _state = PARSER_PAYLOAD;
      }
      break;

    case PARSER_PAYLOAD:
      _payload[_payloadIndex++] =
        value;

      _crc =
        crc8Update(
          _crc,
          value
        );

      if (_payloadIndex >= _len) {
        _state = PARSER_CRC;
      }
      break;

    case PARSER_CRC:
      if (value == _crc) {
        handleFrame();
      } else {
        ++_crcErrors;

        sendError(
          _seq,
          ERR_BAD_CRC
        );
      }

      resetParser();
      break;

    case PARSER_DISCARD:
      if (_discardRemaining) {
        --_discardRemaining;
      }

      if (_discardRemaining == 0) {
        sendError(
          _seq,
          ERR_BAD_LENGTH
        );

        resetParser();
      }
      break;

    default:
      resetParser();
      break;
  }
}

void KscHostMuxStream::serviceTimeout() {
  if (_state == PARSER_IDLE ||
      _lastParserByteMs == 0) {
    return;
  }

  const unsigned long elapsed =
    millis() - _lastParserByteMs;

  if (_state == PARSER_SOF1) {
    if (elapsed >= 25UL) {
      queueTty(SOF1);
      resetParser();
    }

    return;
  }

  if (elapsed >= 120UL) {
    ++_timeouts;

    sendError(
      _seq,
      ERR_TIMEOUT
    );

    resetParser();
  }
}

void KscHostMuxStream::service() {
  while (_wire.available()) {
    const int value =
      _wire.read();

    if (value < 0) {
      break;
    }

    processWireByte(
      (uint8_t)value
    );
  }

  serviceTimeout();
}

int KscHostMuxStream::available() {
  service();
  return _ttyCount;
}

int KscHostMuxStream::read() {
  service();

  if (_ttyCount == 0) {
    return -1;
  }

  const uint8_t value =
    _ttyQueue[_ttyTail];

  _ttyTail =
    (uint8_t)((_ttyTail + 1) %
              TTY_QUEUE_SIZE);

  --_ttyCount;

  return value;
}

int KscHostMuxStream::peek() {
  service();

  if (_ttyCount == 0) {
    return -1;
  }

  return _ttyQueue[_ttyTail];
}

void KscHostMuxStream::flush() {
  _wire.flush();
}

size_t KscHostMuxStream::write(
  uint8_t value
) {
  return _wire.write(value);
}

size_t KscHostMuxStream::write(
  const uint8_t *buffer,
  size_t size
) {
  return _wire.write(
    buffer,
    size
  );
}
