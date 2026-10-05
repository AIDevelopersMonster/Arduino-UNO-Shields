#include <HY_M302.h>
#include <string.h>

HY_M302 shield;

static char cmd[12];
static uint8_t cmdLen = 0;

static void printHelp() {
  Serial.println(F(""));
  Serial.println(F("HY-M302 TEST-02 DHT11 MANUAL"));
  Serial.println(F("One command = one measurement."));
  Serial.println(F(""));
  Serial.println(F("  D   read DHT11 once"));
  Serial.println(F("  D3  read DHT11 three times, 2 s apart"));
  Serial.println(F("  A   print analog snapshot"));
  Serial.println(F("  ?   help"));
  Serial.println(F(""));
}

static void readDhtOnce() {
  HY_M302::DhtReading dht = shield.readDht11();

  if (!dht.ok) {
    Serial.println(F("DHT11 ERR"));
    return;
  }

  Serial.print(F("DHT11 OK T="));
  Serial.print(dht.temperatureC, 1);
  Serial.print(F(" C RH="));
  Serial.print(dht.humidity, 1);
  Serial.println(F(" %"));
}

static void printAnalogSnapshot() {
  Serial.print(F("POT="));
  Serial.print(shield.readPotRaw());

  Serial.print(F(" LIGHT="));
  Serial.print(shield.readLightRaw());

  Serial.print(F(" LM35_RAW="));
  Serial.print(shield.readLm35Raw());

  Serial.print(F(" A3="));
  Serial.println(shield.readAnalog3Raw());
}

static void executeCommand(const char* s) {
  if (strcmp(s, "D") == 0) {
    readDhtOnce();
  } else if (strcmp(s, "D3") == 0) {
    for (uint8_t i = 0; i < 3; ++i) {
      Serial.print(F("READ "));
      Serial.print(i + 1);
      Serial.print(F(": "));
      HY_M302::DhtReading dht = shield.readDht11();

      if (!dht.ok) {
        Serial.println(F("DHT11 ERR"));
      } else {
        Serial.print(F("T="));
        Serial.print(dht.temperatureC, 1);
        Serial.print(F(" C RH="));
        Serial.print(dht.humidity, 1);
        Serial.println(F(" %"));
      }

      if (i != 2) delay(2000);
    }
  } else if (strcmp(s, "A") == 0) {
    printAnalogSnapshot();
  } else if (strcmp(s, "?") == 0 || strcmp(s, "HELP") == 0) {
    printHelp();
  } else {
    Serial.print(F("ERR UNKNOWN: "));
    Serial.println(s);
  }
}

void setup() {
  Serial.begin(115200);
  shield.begin();

  printHelp();
  Serial.println(F("READY"));
}

void loop() {
  while (Serial.available()) {
    const char c = (char)Serial.read();

    if (c == '\r' || c == '\n') {
      if (cmdLen != 0) {
        cmd[cmdLen] = '\0';
        executeCommand(cmd);
        cmdLen = 0;
      }
      continue;
    }

    if (cmdLen < sizeof(cmd) - 1) {
      if (c >= 'a' && c <= 'z') cmd[cmdLen++] = c - ('a' - 'A');
      else cmd[cmdLen++] = c;
    }
  }
}
