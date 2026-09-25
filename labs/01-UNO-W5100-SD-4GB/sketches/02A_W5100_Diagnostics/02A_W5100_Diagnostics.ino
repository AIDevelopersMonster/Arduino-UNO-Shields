/*
  LAB-01 / 02A_W5100_Diagnostics
  Arduino UNO Clone + W5100 + SD 4 GB

  Purpose:
  Separate controller detection, physical link, and DHCP.
*/

#include <SPI.h>
#include <Ethernet.h>

const uint8_t SD_CS = 4;
const uint8_t ETH_CS = 10;

byte mac[] = { 0x02, 0x41, 0x52, 0x44, 0x01, 0x0A };

void printHardwareStatus() {
  EthernetHardwareStatus hw = Ethernet.hardwareStatus();

  Serial.print(F("Hardware: "));
  if (hw == EthernetNoHardware) {
    Serial.println(F("NO HARDWARE"));
  } else if (hw == EthernetW5100) {
    Serial.println(F("W5100"));
  } else if (hw == EthernetW5200) {
    Serial.println(F("W5200"));
  } else if (hw == EthernetW5500) {
    Serial.println(F("W5500"));
  } else {
    Serial.println(F("UNKNOWN"));
  }
}

void printLinkStatus() {
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

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println();
  Serial.println(F("LAB-01 / 02A_W5100_Diagnostics"));

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  pinMode(ETH_CS, OUTPUT);
  digitalWrite(ETH_CS, HIGH);

  Ethernet.init(ETH_CS);

  printHardwareStatus();
  printLinkStatus();

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println(F("STOP: Ethernet controller not detected."));
    while (true) delay(1000);
  }

  Serial.println(F("Trying DHCP..."));

  if (Ethernet.begin(mac, 10000, 4000) == 0) {
    Serial.println(F("DHCP: FAIL"));
    printHardwareStatus();
    printLinkStatus();
    Serial.println(F("Next: verify cable/router and try static IP test."));
    while (true) delay(1000);
  }

  Serial.println(F("DHCP: PASS"));
  Serial.print(F("IP: "));
  Serial.println(Ethernet.localIP());
  Serial.print(F("Gateway: "));
  Serial.println(Ethernet.gatewayIP());
  Serial.print(F("Subnet: "));
  Serial.println(Ethernet.subnetMask());
  Serial.print(F("DNS: "));
  Serial.println(Ethernet.dnsServerIP());
}

void loop() {
  Ethernet.maintain();
}
