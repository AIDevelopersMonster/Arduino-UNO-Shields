/*
  LAB-01 / 02A_W5100_Diagnostics
  Arduino UNO Clone + W5100 + SD 4 GB

  Purpose:
  First initialize the WIZnet controller without DHCP, then report the
  detected hardware. Only after hardware detection passes, try DHCP.

  Note:
  Ethernet.hardwareStatus() is meaningful after Ethernet.begin().
  On W5100, Ethernet.linkStatus() is not a reliable link indicator and
  normally reports UNKNOWN, so inspect the RJ45 LEDs for physical link.
*/

#include <SPI.h>
#include <Ethernet.h>

const uint8_t SD_CS = 4;
const uint8_t ETH_CS = 10;

byte mac[] = { 0x02, 0x41, 0x52, 0x44, 0x01, 0x0A };
IPAddress probeIp(192, 168, 254, 250);

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

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println();
  Serial.println(F("LAB-01 / 02A_W5100_Diagnostics v2"));

  // Ethernet and SD share SPI. Keep SD inactive.
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  pinMode(ETH_CS, OUTPUT);
  digitalWrite(ETH_CS, HIGH);

  Ethernet.init(ETH_CS);

  Serial.println(F("Step 1: initialize WIZnet controller without DHCP..."));
  Ethernet.begin(mac, probeIp);
  delay(100);

  printHardwareStatus();

  EthernetHardwareStatus hw = Ethernet.hardwareStatus();
  if (hw == EthernetNoHardware) {
    Serial.println(F("RESULT: Ethernet controller not detected after initialization."));
    Serial.println(F("Check shield seating, D10 CS, SPI D11/D12/D13, power and shield hardware."));
    while (true) {
      delay(1000);
    }
  }

  if (hw == EthernetW5100) {
    Serial.println(F("W5100 detected: SPI communication PASS."));
    Serial.println(F("Link status API is not reliable for W5100; inspect RJ45 LEDs."));
  }

  Serial.println(F("Step 2: trying DHCP..."));

  if (Ethernet.begin(mac, 10000, 4000) == 0) {
    Serial.println(F("DHCP: FAIL"));
    printHardwareStatus();

    if (Ethernet.hardwareStatus() != EthernetNoHardware) {
      Serial.println(F("Controller is detected; investigate cable, RJ45 LEDs, router/switch and DHCP."));
    }

    while (true) {
      delay(1000);
    }
  }

  Serial.println(F("DHCP: PASS"));
  printHardwareStatus();

  Serial.print(F("IP:      "));
  Serial.println(Ethernet.localIP());
  Serial.print(F("Gateway: "));
  Serial.println(Ethernet.gatewayIP());
  Serial.print(F("Subnet:  "));
  Serial.println(Ethernet.subnetMask());
  Serial.print(F("DNS:     "));
  Serial.println(Ethernet.dnsServerIP());
}

void loop() {
  Ethernet.maintain();
}
