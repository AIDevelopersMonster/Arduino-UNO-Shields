#include "KSC_ReferenceTarget.h"

#include <stdlib.h>
#include <string.h>

KscReferenceTarget::KscReferenceTarget()
  : _value(128),
    _flag(false) {
}

void KscReferenceTarget::begin() {
}

void KscReferenceTarget::service() {
}

void KscReferenceTarget::name(
  char *out,
  uint8_t outSize
) {
  if (!out || outSize == 0) {
    return;
  }

  strncpy(
    out,
    "UNO Reference Target",
    outSize
  );

  out[outSize - 1] = 0;
}

void KscReferenceTarget::bootReport(
  Print &out
) {
  out.println(
    F("Target backend: synthetic VFS only")
  );

  out.println(
    F("External hardware: none required")
  );
}

KscNodeType KscReferenceTarget::pathType(
  const char *path
) {
  if (strcmp(path, "/") == 0 ||
      strcmp(path, "/demo") == 0 ||
      strcmp(path, "/proc") == 0 ||
      strcmp(path, "/sys") == 0) {
    return KSC_NODE_DIR;
  }

  if (strcmp(path, "/demo/counter") == 0 ||
      strcmp(path, "/proc/mem") == 0 ||
      strcmp(path, "/proc/uptime") == 0 ||
      strcmp(path, "/sys/version") == 0 ||
      strcmp(path, "/sys/target") == 0 ||
      strcmp(path, "/sys/storage") == 0) {
    return KSC_NODE_RO;
  }

  if (strcmp(path, "/demo/value") == 0 ||
      strcmp(path, "/demo/flag") == 0) {
    return KSC_NODE_RW;
  }

  return KSC_NODE_NONE;
}

uint8_t KscReferenceTarget::dirCount(
  const char *path
) {
  if (strcmp(path, "/") == 0) {
    return 3;
  }

  if (strcmp(path, "/demo") == 0) {
    return 3;
  }

  if (strcmp(path, "/proc") == 0) {
    return 2;
  }

  if (strcmp(path, "/sys") == 0) {
    return 3;
  }

  return 0;
}

bool KscReferenceTarget::dirEntry(
  const char *path,
  uint8_t index,
  char *nameOut,
  uint8_t nameOutSize,
  KscNodeType &typeOut
) {
  const char *name = nullptr;
  KscNodeType type =
    KSC_NODE_NONE;

  if (strcmp(path, "/") == 0) {
    switch (index) {
      case 0:
        name = "demo";
        type = KSC_NODE_DIR;
        break;

      case 1:
        name = "proc";
        type = KSC_NODE_DIR;
        break;

      case 2:
        name = "sys";
        type = KSC_NODE_DIR;
        break;

      default:
        return false;
    }
  } else if (strcmp(path, "/demo") == 0) {
    switch (index) {
      case 0:
        name = "counter";
        type = KSC_NODE_RO;
        break;

      case 1:
        name = "value";
        type = KSC_NODE_RW;
        break;

      case 2:
        name = "flag";
        type = KSC_NODE_RW;
        break;

      default:
        return false;
    }
  } else if (strcmp(path, "/proc") == 0) {
    switch (index) {
      case 0:
        name = "mem";
        type = KSC_NODE_RO;
        break;

      case 1:
        name = "uptime";
        type = KSC_NODE_RO;
        break;

      default:
        return false;
    }
  } else if (strcmp(path, "/sys") == 0) {
    switch (index) {
      case 0:
        name = "version";
        type = KSC_NODE_RO;
        break;

      case 1:
        name = "target";
        type = KSC_NODE_RO;
        break;

      case 2:
        name = "storage";
        type = KSC_NODE_RO;
        break;

      default:
        return false;
    }
  } else {
    return false;
  }

  if (!nameOut ||
      nameOutSize == 0) {
    return false;
  }

  strncpy(
    nameOut,
    name,
    nameOutSize
  );

  nameOut[nameOutSize - 1] = 0;
  typeOut = type;

  return true;
}

KscResult KscReferenceTarget::read(
  const char *path,
  char *out,
  uint8_t outSize
) {
  if (!out || outSize == 0) {
    return KSC_ERR_VALUE;
  }

  out[0] = 0;

  const KscNodeType type =
    pathType(path);

  if (type == KSC_NODE_DIR) {
    return KSC_ERR_IS_DIR;
  }

  if (type == KSC_NODE_NONE) {
    return KSC_ERR_NOT_FOUND;
  }

  if (strcmp(path, "/demo/counter") == 0) {
    ultoa(
      millis() / 1000UL,
      out,
      10
    );

    return KSC_OK;
  }

  if (strcmp(path, "/demo/value") == 0) {
    utoa(
      _value,
      out,
      10
    );

    return KSC_OK;
  }

  if (strcmp(path, "/demo/flag") == 0) {
    strcpy(
      out,
      _flag ? "1" : "0"
    );

    return KSC_OK;
  }

  if (strcmp(path, "/proc/mem") == 0) {
    itoa(
      kscFreeRam(),
      out,
      10
    );

    return KSC_OK;
  }

  if (strcmp(path, "/proc/uptime") == 0) {
    ultoa(
      millis(),
      out,
      10
    );

    return KSC_OK;
  }

  if (strcmp(path, "/sys/version") == 0) {
    strncpy(
      out,
      "KSC Core 0.1",
      outSize
    );

    out[outSize - 1] = 0;
    return KSC_OK;
  }

  if (strcmp(path, "/sys/target") == 0) {
    strncpy(
      out,
      "UNO Reference Target",
      outSize
    );

    out[outSize - 1] = 0;
    return KSC_OK;
  }

  if (strcmp(path, "/sys/storage") == 0) {
    strncpy(
      out,
      "SYNTHETIC ONLY",
      outSize
    );

    out[outSize - 1] = 0;
    return KSC_OK;
  }

  return KSC_ERR_NOT_FOUND;
}

KscResult KscReferenceTarget::write(
  const char *path,
  long value
) {
  const KscNodeType type =
    pathType(path);

  if (type == KSC_NODE_NONE) {
    return KSC_ERR_NOT_FOUND;
  }

  if (type == KSC_NODE_DIR) {
    return KSC_ERR_IS_DIR;
  }

  if (type != KSC_NODE_RW) {
    return KSC_ERR_READ_ONLY;
  }

  if (strcmp(path, "/demo/value") == 0) {
    if (value < 0 ||
        value > 255) {
      return KSC_ERR_RANGE;
    }

    _value =
      (uint8_t)value;

    return KSC_OK;
  }

  if (strcmp(path, "/demo/flag") == 0) {
    if (value < 0 ||
        value > 1) {
      return KSC_ERR_RANGE;
    }

    _flag =
      value != 0;

    return KSC_OK;
  }

  return KSC_ERR_NOT_FOUND;
}

bool KscReferenceTarget::writableRange(
  const char *path,
  uint16_t &minValue,
  uint16_t &maxValue
) {
  if (strcmp(path, "/demo/value") == 0) {
    minValue = 0;
    maxValue = 255;
    return true;
  }

  if (strcmp(path, "/demo/flag") == 0) {
    minValue = 0;
    maxValue = 1;
    return true;
  }

  return false;
}

bool KscReferenceTarget::readWritableLong(
  const char *path,
  long &value
) {
  if (strcmp(path, "/demo/value") == 0) {
    value = _value;
    return true;
  }

  if (strcmp(path, "/demo/flag") == 0) {
    value =
      _flag ? 1 : 0;

    return true;
  }

  return false;
}

bool KscReferenceTarget::pathIsDynamic(
  const char *path
) {
  return
    strcmp(
      path,
      "/demo/counter"
    ) == 0 ||
    strcmp(
      path,
      "/proc/mem"
    ) == 0 ||
    strcmp(
      path,
      "/proc/uptime"
    ) == 0;
}

void KscReferenceTarget::printStatus(
  Print &out
) {
  out.print(
    F("   REF")
  );
}
