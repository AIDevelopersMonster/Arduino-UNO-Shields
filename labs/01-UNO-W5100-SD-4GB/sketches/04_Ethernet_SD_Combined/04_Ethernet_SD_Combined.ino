/*
  LAB-01 / 04_Ethernet_SD_Combined
  Arduino UNO Clone + W5100 + SD 4 GB

  Purpose:
  Prove that W5100 Ethernet and SD work in one Arduino UNO program.
*/

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

const uint8_t SD_CS = 4;
const uint8_t ETH_CS = 10;

byte mac[] = { 0x02, 0x41, 0x52, 0x44, 0x01, 0x02 };

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println(F(""));
  Serial.println(F("LAB-01 / 04_Ethernet_SD_Combined"));

  // Put both SPI devices into the inactive state before initialization.
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  pinMode(ETH_CS, OUTPUT);
  digitalWrite(ETH_CS, HIGH);

  Ethernet.init(ETH_CS);

  Serial.println(F("Initializing Ethernet via DHCP..."));
  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("Ethernet DHCP: FAIL"));
    while (true) {
      delay(1000);
    }
  }

  Serial.print(F("Ethernet DHCP: PASS, IP="));
  Serial.println(Ethernet.localIP());

  Serial.println(F("Initializing SD..."));
  if (!SD.begin(SD_CS)) {
    Serial.println(F("SD init: FAIL"));
    while (true) {
      delay(1000);
    }
  }

  Serial.println(F("SD init: PASS"));

  File f = SD.open("LAB01.TXT");
  if (!f) {
    Serial.println(F("Read LAB01.TXT: FAIL"));
    Serial.println(F("Run 03_SD_Card first to create the file."));
    while (true) {
      delay(1000);
    }
  }

  Serial.println(F("LAB01.TXT:"));
  while (f.available()) {
    Serial.write(f.read());
  }
  f.close();

  Serial.println(F(""));
  Serial.println(F("Ethernet + SD combined test: PASS"));
}

void loop() {
  Ethernet.maintain();
}
