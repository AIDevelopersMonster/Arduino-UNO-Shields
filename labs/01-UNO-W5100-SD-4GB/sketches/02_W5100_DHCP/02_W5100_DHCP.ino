/*
  LAB-01 / 02_W5100_DHCP
  Arduino UNO Clone + W5100 + SD 4 GB

  Purpose:
  Test W5100 Ethernet separately.
  SD is explicitly deselected on D4.
*/

#include <SPI.h>
#include <Ethernet.h>

const uint8_t SD_CS = 4;
const uint8_t ETH_CS = 10;

byte mac[] = { 0x02, 0x41, 0x52, 0x44, 0x01, 0x01 };

void printAddress(const __FlashStringHelper *label, IPAddress value) {
  Serial.print(label);
  Serial.println(value);
}

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println(F(""));
  Serial.println(F("LAB-01 / 02_W5100_DHCP"));

  // W5100 and SD share SPI. Keep SD inactive.
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  pinMode(ETH_CS, OUTPUT);
  digitalWrite(ETH_CS, HIGH);

  Ethernet.init(ETH_CS);

  Serial.println(F("Requesting DHCP address..."));

  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("DHCP: FAIL"));
    Serial.println(F("Check RJ45 cable, router/DHCP, shield seating and W5100."));
    while (true) {
      delay(1000);
    }
  }

  delay(1000);

  Serial.println(F("DHCP: PASS"));
  printAddress(F("IP:      "), Ethernet.localIP());
  printAddress(F("Gateway: "), Ethernet.gatewayIP());
  printAddress(F("Subnet:  "), Ethernet.subnetMask());
  printAddress(F("DNS:     "), Ethernet.dnsServerIP());

  EthernetHardwareStatus hw = Ethernet.hardwareStatus();
  Serial.print(F("Hardware status: "));
  if (hw == EthernetW5100) {
    Serial.println(F("W5100"));
  } else if (hw == EthernetW5200) {
    Serial.println(F("W5200"));
  } else if (hw == EthernetW5500) {
    Serial.println(F("W5500"));
  } else {
    Serial.println(F("unknown / not detected"));
  }

  EthernetLinkStatus link = Ethernet.linkStatus();
  Serial.print(F("Link: "));
  if (link == LinkON) {
    Serial.println(F("ON"));
  } else if (link == LinkOFF) {
    Serial.println(F("OFF"));
  } else {
    Serial.println(F("UNKNOWN"));
  }
}

void loop() {
  Ethernet.maintain();
}
