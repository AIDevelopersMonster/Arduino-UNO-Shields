/*
  Arduino UNO + W5100 Ethernet Shield
  LAB-01 / TEST-02: Ethernet DHCP + IP + PC ping
  microSD absent. W5100 CS=D10, SD CS=D4 (disabled).
  W5100 linkStatus() may report Unknown: inspect LINK LED.
  Serial 115200.
*/
#include <SPI.h>
#include <Ethernet.h>

const uint8_t ETH_CS = 10;
const uint8_t SD_CS  = 4;
byte mac[] = {0x02,0x4B,0x4F,0x4E,0x51,0x02}; // locally administered test MAC
bool dhcpOK = false;
unsigned long lastCheck = 0;

void printAddress(const __FlashStringHelper *label, IPAddress address) {
  Serial.print(label);
  Serial.println(address);
}

void printLink() {
  EthernetLinkStatus status = Ethernet.linkStatus();
  Serial.print(F("Library link status: "));
  if (status == LinkON) Serial.println(F("ON"));
  else if (status == LinkOFF) Serial.println(F("OFF"));
  else Serial.println(F("UNKNOWN (normal for W5100; inspect LINK LED)"));
}

void setup() {
  Serial.begin(115200);
  pinMode(ETH_CS, OUTPUT); digitalWrite(ETH_CS, HIGH);
  pinMode(SD_CS, OUTPUT); digitalWrite(SD_CS, HIGH);
  delay(500);
  Serial.println(F(""));
  Serial.println(F("KON LAB W5100 TEST-02 / Ethernet DHCP + IP + PING"));
  Serial.println(F("Board: UNO / W5100 CS=D10 / SD disabled"));
  Serial.println(F("Connect RJ45 to router/switch with DHCP"));
  Ethernet.init(ETH_CS);

  int hw = Ethernet.hardwareStatus();
  Serial.print(F("Controller before begin: "));
  if (hw == EthernetW5100) Serial.println(F("W5100"));
  else if (hw == EthernetNoHardware) Serial.println(F("NOT DETECTED (may require begin)"));
  else Serial.println(F("OTHER / UNKNOWN"));

  Serial.println(F("DHCP request..."));
  // Timeout 10 seconds, response timeout 2 seconds
  dhcpOK = (Ethernet.begin(mac, 10000, 2000) != 0);

  hw = Ethernet.hardwareStatus();
  Serial.print(F("Controller: "));
  if (hw == EthernetW5100) Serial.println(F("W5100"));
  else if (hw == EthernetNoHardware) Serial.println(F("NO HARDWARE"));
  else Serial.println(F("OTHER"));

  printLink();
  if (!dhcpOK) {
    Serial.println(F("DHCP: FAIL (no lease). Check cable, LINK LED, DHCP router."));
    Serial.println(F("RESULT: NOT PASSED (no IP; ping cannot be tested)"));
    return;
  }
  Serial.println(F("DHCP: PASS"));
  printAddress(F("IP      : "), Ethernet.localIP());
  printAddress(F("MASK    : "), Ethernet.subnetMask());
  printAddress(F("GATEWAY : "), Ethernet.gatewayIP());
  printAddress(F("DNS     : "), Ethernet.dnsServerIP());
  Serial.println(F("RESULT: DHCP PASS / PING PENDING"));
  Serial.println(F("On PC in the same LAN: ping <IP printed above>"));
  Serial.println(F("Record LINK LED state and ping result."));
}

void loop() {
  if (!dhcpOK) return;
  if (millis() - lastCheck >= 30000UL) {
    lastCheck = millis();
    int lease = Ethernet.maintain();
    if (lease == 1 || lease == 3) Serial.println(F("DHCP lease maintenance: FAIL"));
    else if (lease == 2 || lease == 4) {
      Serial.println(F("DHCP lease renewed/rebound"));
      printAddress(F("IP: "), Ethernet.localIP());
    }
    printLink();
  }
}
