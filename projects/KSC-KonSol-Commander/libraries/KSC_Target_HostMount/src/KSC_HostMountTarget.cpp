#include "KSC_HostMountTarget.h"

#include <string.h>

KscHostMountTarget::KscHostMountTarget(
  KscTarget &localTarget,
  KscHostMuxStream &transport
)
  : _local(localTarget),
    _transport(transport),
    _mounted(false),
    _hostErrors(0),
    _streamRecoveries(0) {
}

void KscHostMountTarget::begin() {
  _local.begin();
}

void KscHostMountTarget::service() {
  _local.service();
  _transport.service();
}

void KscHostMountTarget::pollInput(
  KscCore &core
) {
  _local.pollInput(core);
}

void KscHostMountTarget::name(
  char *out,
  uint8_t outSize
) {
  _local.name(
    out,
    outSize
  );
}

void KscHostMountTarget::bootReport(
  Print &out
) {
  _local.bootReport(out);
  out.println(
    F("Host mount: /host lazy remote")
  );
}

bool KscHostMountTarget::isHostPath(
  const char *path
) const {
  return
    strcmp(path, "/host") == 0 ||
    strncmp(path, "/host/", 6) == 0;
}

bool KscHostMountTarget::ensureMounted() {
  if (_mounted) {
    return true;
  }

  uint8_t response[2];
  uint8_t responseLen = 0;
  uint8_t errorCode = 0;

  const bool ok =
    _transport.exchange(
      KscHostMuxStream::TYPE_MOUNT_REQ,
      nullptr,
      0,
      KscHostMuxStream::TYPE_MOUNT_RESP,
      response,
      sizeof(response),
      responseLen,
      errorCode
    );

  if (!ok ||
      responseLen < 1 ||
      response[0] != 1) {
    ++_hostErrors;
    _mounted = false;
    return false;
  }

  _mounted = true;
  return true;
}

KscNodeType KscHostMountTarget::toNodeType(
  uint8_t hostType
) {
  if (hostType == HOST_TYPE_DIR) {
    return KSC_NODE_DIR;
  }

  if (hostType == HOST_TYPE_FILE) {
    return KSC_NODE_RO;
  }

  return KSC_NODE_NONE;
}

bool KscHostMountTarget::remoteStat(
  const char *path,
  uint8_t &hostType
) {
  hostType = HOST_TYPE_NONE;

  if (!ensureMounted()) {
    return false;
  }

  const uint8_t pathLen =
    (uint8_t)strlen(path);

  if (pathLen == 0 ||
      pathLen >= KscCore::PATH_SIZE) {
    ++_hostErrors;
    return false;
  }

  uint8_t response[2];
  uint8_t responseLen = 0;
  uint8_t errorCode = 0;

  const bool ok =
    _transport.exchange(
      KscHostMuxStream::TYPE_STAT_REQ,
      (const uint8_t *)path,
      pathLen,
      KscHostMuxStream::TYPE_STAT_RESP,
      response,
      sizeof(response),
      responseLen,
      errorCode
    );

  if (!ok ||
      responseLen != 1) {
    ++_hostErrors;
    return false;
  }

  hostType = response[0];
  return hostType != HOST_TYPE_NONE;
}

bool KscHostMountTarget::remoteList(
  const char *path,
  uint8_t index,
  uint8_t &count,
  uint8_t &hostType,
  char *nameOut,
  uint8_t nameOutSize
) {
  count = 0;
  hostType = HOST_TYPE_NONE;

  if (nameOut &&
      nameOutSize) {
    nameOut[0] = 0;
  }

  if (!ensureMounted()) {
    return false;
  }

  const uint8_t pathLen =
    (uint8_t)strlen(path);

  if (pathLen == 0 ||
      pathLen >= KscCore::PATH_SIZE ||
      pathLen + 1 > KscHostMuxStream::MAX_PAYLOAD) {
    ++_hostErrors;
    return false;
  }

  uint8_t request[
    KscCore::PATH_SIZE + 1
  ];

  request[0] = index;

  memcpy(
    request + 1,
    path,
    pathLen
  );

  uint8_t response[20];
  uint8_t responseLen = 0;
  uint8_t errorCode = 0;

  const bool ok =
    _transport.exchange(
      KscHostMuxStream::TYPE_LS_REQ,
      request,
      (uint8_t)(pathLen + 1),
      KscHostMuxStream::TYPE_LS_RESP,
      response,
      sizeof(response),
      responseLen,
      errorCode
    );

  if (!ok ||
      responseLen < 1) {
    ++_hostErrors;
    return false;
  }

  count = response[0];

  if (index == 0xFF) {
    return responseLen == 1;
  }

  if (responseLen < 3) {
    ++_hostErrors;
    return false;
  }

  hostType = response[1];

  const uint8_t nameLen =
    response[2];

  if (nameLen > REMOTE_NAME_MAX ||
      responseLen !=
        (uint8_t)(3 + nameLen) ||
      !nameOut ||
      nameOutSize == 0) {
    ++_hostErrors;
    return false;
  }

  const uint8_t copyLen =
    nameLen < nameOutSize - 1
      ? nameLen
      : (uint8_t)(nameOutSize - 1);

  memcpy(
    nameOut,
    response + 3,
    copyLen
  );

  nameOut[copyLen] = 0;

  return true;
}

bool KscHostMountTarget::remoteOpen(
  const char *path,
  uint8_t &handle
) {
  handle = 0;

  if (!ensureMounted()) {
    return false;
  }

  const uint8_t pathLen =
    (uint8_t)strlen(path);

  if (pathLen == 0 ||
      pathLen >= KscCore::PATH_SIZE ||
      pathLen > KscHostMuxStream::MAX_PAYLOAD) {
    ++_hostErrors;
    return false;
  }

  uint8_t response[5];
  uint8_t responseLen = 0;
  uint8_t errorCode = 0;

  const bool ok =
    _transport.exchange(
      KscHostMuxStream::TYPE_OPEN_REQ,
      (const uint8_t *)path,
      pathLen,
      KscHostMuxStream::TYPE_OPEN_RESP,
      response,
      sizeof(response),
      responseLen,
      errorCode
    );

  if (!ok ||
      responseLen != 5 ||
      response[0] == 0) {
    ++_hostErrors;
    return false;
  }

  handle = response[0];
  return true;
}

bool KscHostMountTarget::remoteRead(
  uint8_t handle,
  uint32_t offset,
  uint8_t *dataOut,
  uint8_t dataCapacity,
  uint8_t &dataLen,
  bool &eof,
  uint8_t &errorCode
) {
  dataLen = 0;
  eof = false;
  errorCode = 0;

  if (!dataOut ||
      dataCapacity == 0) {
    errorCode =
      KscHostMuxStream::ERR_BAD_LENGTH;
    return false;
  }

  const uint8_t requested =
    dataCapacity > 32
      ? 32
      : dataCapacity;

  uint8_t request[6];

  request[0] = handle;
  request[1] =
    (uint8_t)(offset & 0xFF);
  request[2] =
    (uint8_t)((offset >> 8) & 0xFF);
  request[3] =
    (uint8_t)((offset >> 16) & 0xFF);
  request[4] =
    (uint8_t)((offset >> 24) & 0xFF);
  request[5] = requested;

  uint8_t response[38];
  uint8_t responseLen = 0;

  const bool ok =
    _transport.exchange(
      KscHostMuxStream::TYPE_READ_REQ,
      request,
      sizeof(request),
      KscHostMuxStream::TYPE_READ_RESP,
      response,
      sizeof(response),
      responseLen,
      errorCode
    );

  if (!ok) {
    return false;
  }

  if (responseLen < 6 ||
      response[0] != handle) {
    errorCode =
      KscHostMuxStream::ERR_BAD_LENGTH;
    return false;
  }

  const uint32_t responseOffset =
    (uint32_t)response[1] |
    ((uint32_t)response[2] << 8) |
    ((uint32_t)response[3] << 16) |
    ((uint32_t)response[4] << 24);

  if (responseOffset != offset) {
    errorCode =
      KscHostMuxStream::ERR_BAD_LENGTH;
    return false;
  }

  eof = response[5] != 0;
  dataLen =
    (uint8_t)(responseLen - 6);

  if (dataLen > requested) {
    errorCode =
      KscHostMuxStream::ERR_BAD_LENGTH;
    return false;
  }

  if (dataLen) {
    memcpy(
      dataOut,
      response + 6,
      dataLen
    );
  }

  return true;
}

void KscHostMountTarget::remoteClose(
  uint8_t handle
) {
  if (handle == 0) {
    return;
  }

  const uint8_t request[1] = {
    handle
  };

  uint8_t response[1];
  uint8_t responseLen = 0;
  uint8_t errorCode = 0;

  const bool ok =
    _transport.exchange(
      KscHostMuxStream::TYPE_CLOSE_REQ,
      request,
      sizeof(request),
      KscHostMuxStream::TYPE_CLOSE_RESP,
      response,
      sizeof(response),
      responseLen,
      errorCode
    );

  if (!ok ||
      responseLen != 1 ||
      response[0] != handle) {
    ++_hostErrors;
  }
}

KscNodeType KscHostMountTarget::pathType(
  const char *path
) {
  if (strcmp(path, "/") == 0) {
    return KSC_NODE_DIR;
  }

  if (strcmp(path, "/host") == 0) {
    return KSC_NODE_DIR;
  }

  if (isHostPath(path)) {
    uint8_t hostType =
      HOST_TYPE_NONE;

    if (!remoteStat(
          path,
          hostType
        )) {
      return KSC_NODE_NONE;
    }

    return toNodeType(hostType);
  }

  return _local.pathType(path);
}

uint8_t KscHostMountTarget::dirCount(
  const char *path
) {
  if (strcmp(path, "/") == 0) {
    const uint8_t localCount =
      _local.dirCount(path);

    return
      localCount < 255
        ? (uint8_t)(localCount + 1)
        : localCount;
  }

  if (isHostPath(path)) {
    uint8_t count = 0;
    uint8_t hostType =
      HOST_TYPE_NONE;

    if (!remoteList(
          path,
          0xFF,
          count,
          hostType,
          nullptr,
          0
        )) {
      return 0;
    }

    return count;
  }

  return _local.dirCount(path);
}

bool KscHostMountTarget::dirEntry(
  const char *path,
  uint8_t index,
  char *nameOut,
  uint8_t nameOutSize,
  KscNodeType &typeOut
) {
  if (strcmp(path, "/") == 0) {
    const uint8_t localCount =
      _local.dirCount(path);

    if (index < localCount) {
      return _local.dirEntry(
        path,
        index,
        nameOut,
        nameOutSize,
        typeOut
      );
    }

    if (index == localCount) {
      if (!nameOut ||
          nameOutSize < 5) {
        return false;
      }

      strcpy(nameOut, "host");
      typeOut = KSC_NODE_DIR;
      return true;
    }

    return false;
  }

  if (isHostPath(path)) {
    uint8_t count = 0;
    uint8_t hostType =
      HOST_TYPE_NONE;

    if (!remoteList(
          path,
          index,
          count,
          hostType,
          nameOut,
          nameOutSize
        )) {
      return false;
    }

    typeOut =
      toNodeType(hostType);

    return
      typeOut != KSC_NODE_NONE;
  }

  return _local.dirEntry(
    path,
    index,
    nameOut,
    nameOutSize,
    typeOut
  );
}

KscResult KscHostMountTarget::read(
  const char *path,
  char *out,
  uint8_t outSize
) {
  if (isHostPath(path)) {
    const KscNodeType type =
      pathType(path);

    if (type == KSC_NODE_DIR) {
      return KSC_ERR_IS_DIR;
    }

    if (type == KSC_NODE_RO) {
      if (!out ||
          outSize == 0) {
        return KSC_ERR_VALUE;
      }

      strncpy(
        out,
        "CAT STREAM AVAILABLE",
        outSize
      );

      out[outSize - 1] = 0;
      return KSC_OK;
    }

    return KSC_ERR_NOT_FOUND;
  }

  return _local.read(
    path,
    out,
    outSize
  );
}

KscResult KscHostMountTarget::streamRead(
  const char *path,
  Print &out,
  bool &handled
) {
  handled = false;

  if (!isHostPath(path)) {
    return KSC_ERR_TARGET;
  }

  handled = true;

  const KscNodeType type =
    pathType(path);

  if (type == KSC_NODE_DIR) {
    return KSC_ERR_IS_DIR;
  }

  if (type != KSC_NODE_RO) {
    return KSC_ERR_NOT_FOUND;
  }

  uint8_t handle = 0;

  if (!remoteOpen(
        path,
        handle
      )) {
    return KSC_ERR_TARGET;
  }

  KscResult result =
    KSC_OK;

  uint32_t offset = 0;
  uint8_t retryCount = 0;

  for (;;) {
    uint8_t chunk[32];
    uint8_t chunkLen = 0;
    bool eof = false;
    uint8_t errorCode = 0;

    if (!remoteRead(
          handle,
          offset,
          chunk,
          sizeof(chunk),
          chunkLen,
          eof,
          errorCode
        )) {
      if (retryCount < 2 &&
          errorCode ==
            KscHostMuxStream::ERR_TIMEOUT) {
        ++retryCount;
        ++_streamRecoveries;
        continue;
      }

      if (retryCount < 2 &&
          errorCode ==
            KscHostMuxStream::ERR_BAD_HANDLE) {
        ++retryCount;
        ++_streamRecoveries;

        uint8_t reopened = 0;

        if (!remoteOpen(
              path,
              reopened
            )) {
          result = KSC_ERR_TARGET;
          break;
        }

        handle = reopened;
        continue;
      }

      ++_hostErrors;
      result = KSC_ERR_TARGET;
      break;
    }

    retryCount = 0;

    for (uint8_t i = 0;
         i < chunkLen;
         ++i) {
      out.write(chunk[i]);
    }

    offset += chunkLen;

    if (eof) {
      break;
    }

    if (chunkLen == 0) {
      ++_hostErrors;
      result = KSC_ERR_TARGET;
      break;
    }
  }

  remoteClose(handle);
  return result;
}

KscResult KscHostMountTarget::streamWindow(
  const char *path,
  uint32_t offset,
  uint16_t maxBytes,
  Print &out,
  uint16_t &bytesRead,
  uint32_t &totalSize,
  bool &handled
) {
  bytesRead = 0;
  totalSize = 0;
  handled = false;

  if (!isHostPath(path)) {
    return KSC_ERR_TARGET;
  }

  handled = true;

  const KscNodeType type =
    pathType(path);

  if (type == KSC_NODE_DIR) {
    return KSC_ERR_IS_DIR;
  }

  if (type != KSC_NODE_RO) {
    return KSC_ERR_NOT_FOUND;
  }

  uint8_t handle = 0;

  if (!remoteOpen(
        path,
        handle
      )) {
    return KSC_ERR_TARGET;
  }

  uint8_t response[5];
  uint8_t responseLen = 0;
  uint8_t errorCode = 0;

  const uint8_t pathLen =
    (uint8_t)strlen(path);

  const bool openAgain =
    _transport.exchange(
      KscHostMuxStream::TYPE_OPEN_REQ,
      (const uint8_t *)path,
      pathLen,
      KscHostMuxStream::TYPE_OPEN_RESP,
      response,
      sizeof(response),
      responseLen,
      errorCode
    );

  if (!openAgain ||
      responseLen != 5 ||
      response[0] == 0) {
    remoteClose(handle);
    ++_hostErrors;
    return KSC_ERR_TARGET;
  }

  remoteClose(handle);
  handle = response[0];

  totalSize =
    (uint32_t)response[1] |
    ((uint32_t)response[2] << 8) |
    ((uint32_t)response[3] << 16) |
    ((uint32_t)response[4] << 24);

  if (maxBytes == 0 ||
      offset >= totalSize) {
    remoteClose(handle);
    return KSC_OK;
  }

  uint32_t cursor = offset;
  uint16_t remaining = maxBytes;
  uint8_t retryCount = 0;

  while (remaining &&
         cursor < totalSize) {
    uint8_t chunk[32];
    uint8_t chunkLen = 0;
    bool eof = false;
    uint8_t readError = 0;

    uint8_t want =
      remaining > sizeof(chunk)
        ? sizeof(chunk)
        : (uint8_t)remaining;

    if (!remoteRead(
          handle,
          cursor,
          chunk,
          want,
          chunkLen,
          eof,
          readError
        )) {
      if (retryCount < 2 &&
          readError ==
            KscHostMuxStream::ERR_TIMEOUT) {
        ++retryCount;
        ++_streamRecoveries;
        continue;
      }

      if (retryCount < 2 &&
          readError ==
            KscHostMuxStream::ERR_BAD_HANDLE) {
        ++retryCount;
        ++_streamRecoveries;

        uint8_t reopened = 0;

        if (!remoteOpen(
              path,
              reopened
            )) {
          ++_hostErrors;
          remoteClose(handle);
          return KSC_ERR_TARGET;
        }

        handle = reopened;
        continue;
      }

      ++_hostErrors;
      remoteClose(handle);
      return KSC_ERR_TARGET;
    }

    retryCount = 0;

    for (uint8_t n = 0;
         n < chunkLen;
         ++n) {
      out.write(chunk[n]);
    }

    cursor += chunkLen;
    bytesRead += chunkLen;
    remaining -= chunkLen;

    if (eof ||
        chunkLen == 0) {
      break;
    }
  }

  remoteClose(handle);
  return KSC_OK;
}

KscResult KscHostMountTarget::write(
  const char *path,
  long value
) {
  if (isHostPath(path)) {
    return KSC_ERR_READ_ONLY;
  }

  return _local.write(
    path,
    value
  );
}

bool KscHostMountTarget::writableRange(
  const char *path,
  uint16_t &minValue,
  uint16_t &maxValue
) {
  if (isHostPath(path)) {
    return false;
  }

  return _local.writableRange(
    path,
    minValue,
    maxValue
  );
}

bool KscHostMountTarget::readWritableLong(
  const char *path,
  long &value
) {
  if (isHostPath(path)) {
    return false;
  }

  return _local.readWritableLong(
    path,
    value
  );
}

bool KscHostMountTarget::pathIsDynamic(
  const char *path
) {
  if (isHostPath(path)) {
    return false;
  }

  return _local.pathIsDynamic(path);
}

void KscHostMountTarget::printStatus(
  Print &out
) {
  _local.printStatus(out);

  out.print(F("   HOST "));
  out.print(
    _mounted ? F("M") : F("-")
  );

  out.print('/');
  out.print(_hostErrors);

  out.print(F(" R"));
  out.print(_streamRecoveries);
}
