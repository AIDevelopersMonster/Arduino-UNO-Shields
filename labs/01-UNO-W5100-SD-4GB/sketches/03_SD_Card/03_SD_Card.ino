/*
  LAB-01 / 03_SD_Card
  Arduino UNO Clone + W5100 + SD 4 GB

  Purpose:
  Test the SD card separately.
  W5100 is explicitly deselected on D10.
*/

#include <SPI.h>
#include <SD.h>

const uint8_t SD_CS = 4;
const uint8_t ETH_CS = 10;

void listRoot() {
  File root = SD.open("/");
  if (!root) {
    Serial.println(F("Open root: FAIL"));
    return;
  }

  Serial.println(F("Root directory:"));
  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      break;
    }

    Serial.print(F("  "));
    Serial.print(entry.name());

    if (entry.isDirectory()) {
      Serial.println(F("/"));
    } else {
      Serial.print(F("  "));
      Serial.print(entry.size());
      Serial.println(F(" bytes"));
    }

    entry.close();
  }

  root.close();
}

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println(F(""));
  Serial.println(F("LAB-01 / 03_SD_Card"));

  // W5100 and SD share SPI. Keep Ethernet inactive.
  pinMode(ETH_CS, OUTPUT);
  digitalWrite(ETH_CS, HIGH);

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  Serial.println(F("Initializing SD..."));

  if (!SD.begin(SD_CS)) {
    Serial.println(F("SD init: FAIL"));
    Serial.println(F("Check card format, insertion, D4 CS and shield."));
    while (true) {
      delay(1000);
    }
  }

  Serial.println(F("SD init: PASS"));

  listRoot();

  File f = SD.open("LAB01.TXT", FILE_WRITE);
  if (!f) {
    Serial.println(F("Open LAB01.TXT for write: FAIL"));
    while (true) {
      delay(1000);
    }
  }

  f.println(F("LAB-01 SD write test: PASS"));
  f.close();
  Serial.println(F("Write LAB01.TXT: PASS"));

  f = SD.open("LAB01.TXT");
  if (!f) {
    Serial.println(F("Open LAB01.TXT for read: FAIL"));
    while (true) {
      delay(1000);
    }
  }

  Serial.println(F("LAB01.TXT contents:"));
  while (f.available()) {
    Serial.write(f.read());
  }
  f.close();

  Serial.println(F(""));
  Serial.println(F("SD readback: PASS"));
}

void loop() {
}
