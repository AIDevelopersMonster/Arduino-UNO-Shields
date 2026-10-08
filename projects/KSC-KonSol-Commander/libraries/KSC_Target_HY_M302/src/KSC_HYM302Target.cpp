#include "KSC_HYM302Target.h"

#include <HY_M302_Remote.h>
#include <HY_M302_Remote_iDroid_OrangePi.h>

#include <stdlib.h>
#include <string.h>

using namespace HY_M302_Remote;
using HY_M302_Remote::IDroidOrangePi::decode;

KscHyM302Target::KscHyM302Target()
  : _irOk(false),
    _ledRed(false),
    _ledBlue(false),
    _buzzer(false),
    _rgbR(0),
    _rgbG(0),
    _rgbB(0),
    _sw1Mode(0),
    _sw1Raw(false),
    _sw1Stable(false),
    _sw1RawChangedMs(0),
    _dhtLastMs(0),
    _dhtHaveValue(false) {
  _dht.temperatureC = 0.0f;
  _dht.humidity = 0.0f;
  _dht.ok = false;
}

void KscHyM302Target::copyText(
  char *out,
  uint8_t outSize,
  const char *text
) {
  if (!out || outSize == 0) {
    return;
  }

  strncpy(
    out,
    text,
    outSize
  );

  out[outSize - 1] = 0;
}

void KscHyM302Target::begin() {
  _shield.begin();

  _ledRed = false;
  _ledBlue = false;
  _buzzer = false;

  _rgbR = 0;
  _rgbG = 0;
  _rgbB = 0;

  _shield.ledRed(false);
  _shield.ledBlue(false);
  _shield.rgbOff();
  _shield.buzzerOff();

  _sw1Raw =
    _shield.button1Pressed();
  _sw1Stable = _sw1Raw;
  _sw1RawChangedMs = millis();

  _irOk =
    _shield.beginIrNecAsync();
}

void KscHyM302Target::service() {
  _shield.service();
  serviceSw1Profile();
}

void KscHyM302Target::serviceSw1Profile() {
  const bool raw =
    _shield.button1Pressed();

  if (raw != _sw1Raw) {
    _sw1Raw = raw;
    _sw1RawChangedMs = millis();
    return;
  }

  if (raw == _sw1Stable ||
      millis() - _sw1RawChangedMs <
        25UL) {
    return;
  }

  _sw1Stable = raw;

  if (!_sw1Stable) {
    return;
  }

  if (_sw1Mode == 0) {
    _ledBlue = false;
    _ledRed = true;
  } else {
    _ledRed = false;
    _ledBlue = true;
  }

  _shield.ledRed(_ledRed);
  _shield.ledBlue(_ledBlue);
}

void KscHyM302Target::pollInput(
  KscCore &core
) {
  HY_M302::IrNecFrame frame;

  while (_shield.readIrNecAsync(
           frame
         )) {
    if (frame.repeat) {
      continue;
    }

    const Key remoteKey =
      decode(
        frame.address,
        frame.command
      );

    if (remoteKey == KEY_NONE) {
      continue;
    }

    const int8_t digit =
      HY_M302_Remote::digit(
        remoteKey
      );

    if (digit >= 0 &&
        digit <= 9) {
      core.injectChar(
        KSC_SRC_TARGET,
        (uint8_t)('0' + digit)
      );

      continue;
    }

    KscSpecialKey key =
      KSC_KEY_NONE;

    switch (remoteKey) {
      case KEY_UP:
        key = KSC_KEY_UP;
        break;

      case KEY_DOWN:
        key = KSC_KEY_DOWN;
        break;

      case KEY_LEFT:
        key = KSC_KEY_LEFT;
        break;

      case KEY_RIGHT:
        key = KSC_KEY_RIGHT;
        break;

      case KEY_OK:
        key = KSC_KEY_ENTER;
        break;

      case KEY_RETURN:
        key = KSC_KEY_BACK;
        break;

      case KEY_HOME:
        key = KSC_KEY_HOME;
        break;

      case KEY_MENU:
        key = KSC_KEY_MENU;
        break;

      case KEY_POWER:
        key = KSC_KEY_POWER;
        break;

      default:
        break;
    }

    if (key != KSC_KEY_NONE) {
      core.injectKey(
        KSC_SRC_TARGET,
        key
      );
    }
  }
}

void KscHyM302Target::name(
  char *out,
  uint8_t outSize
) {
  copyText(
    out,
    outSize,
    "Arduino UNO + HY-M302"
  );
}

void KscHyM302Target::bootReport(
  Print &out
) {
  out.print(F("IR INIT: "));
  out.println(
    _irOk
      ? F("OK")
      : F("FAIL")
  );

  out.println(
    F("Target backend: physical HY-M302")
  );

  out.println(
    F("Storage: virtual namespace only")
  );
}

KscNodeType KscHyM302Target::pathType(
  const char *path
) {
  if (strcmp(path, "/") == 0 ||
      strcmp(path, "/dev") == 0 ||
      strcmp(path, "/dev/dht") == 0 ||
      strcmp(path, "/dev/led") == 0 ||
      strcmp(path, "/dev/rgb") == 0 ||
      strcmp(path, "/proc") == 0 ||
      strcmp(path, "/sys") == 0) {
    return KSC_NODE_DIR;
  }

  if (strcmp(path, "/dev/sw1") == 0 ||
      strcmp(path, "/dev/sw2") == 0 ||
      strcmp(path, "/dev/pot") == 0 ||
      strcmp(path, "/dev/light") == 0 ||
      strcmp(path, "/dev/dht/temp") == 0 ||
      strcmp(path, "/dev/dht/humidity") == 0 ||
      strcmp(path, "/dev/dht/status") == 0 ||
      strcmp(path, "/dev/dht/age") == 0 ||
      strcmp(path, "/proc/mem") == 0 ||
      strcmp(path, "/proc/uptime") == 0 ||
      strcmp(path, "/sys/version") == 0 ||
      strcmp(path, "/sys/target") == 0 ||
      strcmp(path, "/sys/storage") == 0) {
    return KSC_NODE_RO;
  }

  if (strcmp(path, "/sys/sw1") == 0 ||
      strcmp(path, "/dev/led/red") == 0 ||
      strcmp(path, "/dev/led/blue") == 0 ||
      strcmp(path, "/dev/rgb/red") == 0 ||
      strcmp(path, "/dev/rgb/green") == 0 ||
      strcmp(path, "/dev/rgb/blue") == 0 ||
      strcmp(path, "/dev/buzzer") == 0) {
    return KSC_NODE_RW;
  }

  return KSC_NODE_NONE;
}

uint8_t KscHyM302Target::dirCount(
  const char *path
) {
  if (strcmp(path, "/") == 0) {
    return 3;
  }

  if (strcmp(path, "/dev") == 0) {
    return 8;
  }

  if (strcmp(path, "/dev/dht") == 0) {
    return 4;
  }

  if (strcmp(path, "/dev/led") == 0) {
    return 2;
  }

  if (strcmp(path, "/dev/rgb") == 0) {
    return 3;
  }

  if (strcmp(path, "/proc") == 0) {
    return 2;
  }

  if (strcmp(path, "/sys") == 0) {
    return 4;
  }

  return 0;
}

bool KscHyM302Target::dirEntry(
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
        name = "dev";
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
  } else if (strcmp(path, "/dev") == 0) {
    switch (index) {
      case 0:
        name = "sw1";
        type = KSC_NODE_RO;
        break;

      case 1:
        name = "sw2";
        type = KSC_NODE_RO;
        break;

      case 2:
        name = "pot";
        type = KSC_NODE_RO;
        break;

      case 3:
        name = "light";
        type = KSC_NODE_RO;
        break;

      case 4:
        name = "dht";
        type = KSC_NODE_DIR;
        break;

      case 5:
        name = "led";
        type = KSC_NODE_DIR;
        break;

      case 6:
        name = "rgb";
        type = KSC_NODE_DIR;
        break;

      case 7:
        name = "buzzer";
        type = KSC_NODE_RW;
        break;

      default:
        return false;
    }
  } else if (strcmp(path, "/dev/dht") == 0) {
    switch (index) {
      case 0:
        name = "temp";
        type = KSC_NODE_RO;
        break;

      case 1:
        name = "humidity";
        type = KSC_NODE_RO;
        break;

      case 2:
        name = "status";
        type = KSC_NODE_RO;
        break;

      case 3:
        name = "age";
        type = KSC_NODE_RO;
        break;

      default:
        return false;
    }
  } else if (strcmp(path, "/dev/led") == 0) {
    switch (index) {
      case 0:
        name = "red";
        type = KSC_NODE_RW;
        break;

      case 1:
        name = "blue";
        type = KSC_NODE_RW;
        break;

      default:
        return false;
    }
  } else if (strcmp(path, "/dev/rgb") == 0) {
    switch (index) {
      case 0:
        name = "red";
        type = KSC_NODE_RW;
        break;

      case 1:
        name = "green";
        type = KSC_NODE_RW;
        break;

      case 2:
        name = "blue";
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

      case 3:
        name = "sw1";
        type = KSC_NODE_RW;
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

  copyText(
    nameOut,
    nameOutSize,
    name
  );

  typeOut = type;
  return true;
}

void KscHyM302Target::refreshDht() {
  const unsigned long now =
    millis();

  if (_dhtHaveValue &&
      now - _dhtLastMs <
        2000UL) {
    return;
  }

  _dht =
    _shield.readDht11();

  _dhtLastMs = millis();
  _dhtHaveValue = _dht.ok;
}

KscResult KscHyM302Target::read(
  const char *path,
  char *out,
  uint8_t outSize
) {
  if (!out ||
      outSize == 0) {
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

  if (strcmp(path, "/dev/sw1") == 0) {
    copyText(
      out,
      outSize,
      _shield.button1Pressed()
        ? "1"
        : "0"
    );

    return KSC_OK;
  }

  if (strcmp(path, "/dev/sw2") == 0) {
    copyText(
      out,
      outSize,
      _shield.button2Pressed()
        ? "1"
        : "0"
    );

    return KSC_OK;
  }

  if (strcmp(path, "/dev/pot") == 0) {
    itoa(
      _shield.readPotRaw(),
      out,
      10
    );

    return KSC_OK;
  }

  if (strcmp(path, "/dev/light") == 0) {
    itoa(
      _shield.readLightRaw(),
      out,
      10
    );

    return KSC_OK;
  }

  if (strcmp(path, "/dev/dht/temp") == 0) {
    refreshDht();

    if (!_dhtHaveValue) {
      return KSC_ERR_TARGET;
    }

    dtostrf(
      _dht.temperatureC,
      0,
      1,
      out
    );

    return KSC_OK;
  }

  if (strcmp(path, "/dev/dht/humidity") == 0) {
    refreshDht();

    if (!_dhtHaveValue) {
      return KSC_ERR_TARGET;
    }

    dtostrf(
      _dht.humidity,
      0,
      1,
      out
    );

    return KSC_OK;
  }

  if (strcmp(path, "/dev/dht/status") == 0) {
    refreshDht();

    copyText(
      out,
      outSize,
      _dhtHaveValue
        ? "OK"
        : "ERROR"
    );

    return KSC_OK;
  }

  if (strcmp(path, "/dev/dht/age") == 0) {
    if (!_dhtLastMs) {
      copyText(
        out,
        outSize,
        "NEVER"
      );
    } else {
      ultoa(
        millis() - _dhtLastMs,
        out,
        10
      );
    }

    return KSC_OK;
  }

  if (strcmp(path, "/dev/led/red") == 0) {
    copyText(
      out,
      outSize,
      _ledRed ? "1" : "0"
    );

    return KSC_OK;
  }

  if (strcmp(path, "/dev/led/blue") == 0) {
    copyText(
      out,
      outSize,
      _ledBlue ? "1" : "0"
    );

    return KSC_OK;
  }

  if (strcmp(path, "/dev/rgb/red") == 0) {
    utoa(
      _rgbR,
      out,
      10
    );

    return KSC_OK;
  }

  if (strcmp(path, "/dev/rgb/green") == 0) {
    utoa(
      _rgbG,
      out,
      10
    );

    return KSC_OK;
  }

  if (strcmp(path, "/dev/rgb/blue") == 0) {
    utoa(
      _rgbB,
      out,
      10
    );

    return KSC_OK;
  }

  if (strcmp(path, "/dev/buzzer") == 0) {
    copyText(
      out,
      outSize,
      _buzzer ? "1" : "0"
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
    copyText(
      out,
      outSize,
      "KSC Core 0.1"
    );

    return KSC_OK;
  }

  if (strcmp(path, "/sys/target") == 0) {
    copyText(
      out,
      outSize,
      "UNO + HY-M302"
    );

    return KSC_OK;
  }

  if (strcmp(path, "/sys/storage") == 0) {
    copyText(
      out,
      outSize,
      "VIRTUAL ONLY"
    );

    return KSC_OK;
  }

  if (strcmp(path, "/sys/sw1") == 0) {
    utoa(
      _sw1Mode,
      out,
      10
    );

    return KSC_OK;
  }

  return KSC_ERR_NOT_FOUND;
}

bool KscHyM302Target::writableRange(
  const char *path,
  uint16_t &minValue,
  uint16_t &maxValue
) {
  if (strcmp(path, "/sys/sw1") == 0 ||
      strcmp(path, "/dev/led/red") == 0 ||
      strcmp(path, "/dev/led/blue") == 0 ||
      strcmp(path, "/dev/buzzer") == 0) {
    minValue = 0;
    maxValue = 1;
    return true;
  }

  if (strcmp(path, "/dev/rgb/red") == 0 ||
      strcmp(path, "/dev/rgb/green") == 0 ||
      strcmp(path, "/dev/rgb/blue") == 0) {
    minValue = 0;
    maxValue = 255;
    return true;
  }

  return false;
}

bool KscHyM302Target::readWritableLong(
  const char *path,
  long &value
) {
  if (strcmp(path, "/sys/sw1") == 0) {
    value = _sw1Mode;
    return true;
  }

  if (strcmp(path, "/dev/led/red") == 0) {
    value = _ledRed ? 1 : 0;
    return true;
  }

  if (strcmp(path, "/dev/led/blue") == 0) {
    value = _ledBlue ? 1 : 0;
    return true;
  }

  if (strcmp(path, "/dev/buzzer") == 0) {
    value = _buzzer ? 1 : 0;
    return true;
  }

  if (strcmp(path, "/dev/rgb/red") == 0) {
    value = _rgbR;
    return true;
  }

  if (strcmp(path, "/dev/rgb/green") == 0) {
    value = _rgbG;
    return true;
  }

  if (strcmp(path, "/dev/rgb/blue") == 0) {
    value = _rgbB;
    return true;
  }

  return false;
}

KscResult KscHyM302Target::write(
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

  uint16_t minValue = 0;
  uint16_t maxValue = 0;

  if (!writableRange(
        path,
        minValue,
        maxValue
      )) {
    return KSC_ERR_READ_ONLY;
  }

  if (value < minValue ||
      value > maxValue) {
    return KSC_ERR_RANGE;
  }

  if (strcmp(path, "/sys/sw1") == 0) {
    _sw1Mode = (uint8_t)value;

    _ledRed = false;
    _ledBlue = false;
    _shield.ledRed(false);
    _shield.ledBlue(false);

    return KSC_OK;
  }

  if (strcmp(path, "/dev/led/red") == 0) {
    _ledRed = value != 0;
    _shield.ledRed(_ledRed);
    return KSC_OK;
  }

  if (strcmp(path, "/dev/led/blue") == 0) {
    _ledBlue = value != 0;
    _shield.ledBlue(_ledBlue);
    return KSC_OK;
  }

  if (strcmp(path, "/dev/buzzer") == 0) {
    _buzzer = value != 0;

    if (_buzzer) {
      _shield.buzzerOn();
    } else {
      _shield.buzzerOff();
    }

    return KSC_OK;
  }

  if (strcmp(path, "/dev/rgb/red") == 0) {
    _rgbR = (uint8_t)value;
  } else if (strcmp(path, "/dev/rgb/green") == 0) {
    _rgbG = (uint8_t)value;
  } else if (strcmp(path, "/dev/rgb/blue") == 0) {
    _rgbB = (uint8_t)value;
  } else {
    return KSC_ERR_NOT_FOUND;
  }

  _shield.setRGB(
    _rgbR,
    _rgbG,
    _rgbB
  );

  return KSC_OK;
}

bool KscHyM302Target::pathIsDynamic(
  const char *path
) {
  return
    strcmp(path, "/dev/sw1") == 0 ||
    strcmp(path, "/dev/sw2") == 0 ||
    strcmp(path, "/dev/pot") == 0 ||
    strcmp(path, "/dev/light") == 0 ||
    strcmp(path, "/dev/dht/temp") == 0 ||
    strcmp(path, "/dev/dht/humidity") == 0 ||
    strcmp(path, "/dev/dht/status") == 0 ||
    strcmp(path, "/dev/dht/age") == 0 ||
    strcmp(path, "/proc/mem") == 0 ||
    strcmp(path, "/proc/uptime") == 0;
}

void KscHyM302Target::printStatus(
  Print &out
) {
  out.print(F("   IR drop "));
  out.print(
    _shield.irNecDroppedEdges()
  );

  out.print('/');
  out.print(
    _shield.irNecDroppedFrames()
  );
}
