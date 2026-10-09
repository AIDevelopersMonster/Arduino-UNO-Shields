/**
 * Arduino UNO & Shields | LAB-01B | TEST-05 TCP Echo Server
 *
 * Goal: certify TCP payload integrity, connection closure and reconnection
 *       after normal as well as deliberately interrupted client sessions.
 * Hardware: Arduino UNO ATmega328P + blue WIZnet W5100 Ethernet Shield.
 * Wiring: W5100 CS=D10, unused SD CS=D4 (held HIGH), SPI via ICSP.
 * Network: wired DHCP LAN, TCP port 5000, no microSD card.
 * Protocol: ASCII line up to 64 characters, terminated by LF (optional CR).
 *           Response: "ECHO " + exact payload + LF.
 * Limits: exactly one request per connection; 64-byte maximum payload;
 *         1500 ms line timeout; no TLS/authentication; trusted LAN only.
 * Serial: 115200 baud.
 *
 * Acceptance: host-side test repeats sessions, verifies exact echoed bytes,
 *             tests maximum payload and reconnects after abort.
 * Status: UNTESTED ON HARDWARE until user supplies observations.
 * Repository: https://github.com/AIDevelopersMonster/Arduino-UNO-Shields
 */
#include <SPI.h>
#include <Ethernet.h>

const uint8_t ETH_CS = 10;
const uint8_t SD_CS = 4;
const uint16_t TCP_PORT = 5000;
const uint8_t MAX_PAYLOAD = 64;
const unsigned long CLIENT_TIMEOUT_MS = 1500UL;
byte mac[] = {0x02,0x4B,0x4F,0x4E,0x51,0x05};
EthernetServer server(TCP_PORT);
bool ready = false;
unsigned long lastDhcpCheck = 0;
uint32_t connections = 0;
uint32_t echoed = 0;
uint32_t rejected = 0;

// The message buffer is deliberately fixed-size to avoid String heap usage.
void handleClient(EthernetClient &client) {
  char payload[MAX_PAYLOAD + 1];
  uint8_t len = 0;
  bool done = false;
  bool overflow = false;
  unsigned long started = millis();

  // Read only a single newline-delimited request with a finite deadline.
  while (client.connected() && millis() - started < CLIENT_TIMEOUT_MS && !done && !overflow) {
    if (!client.available()) continue;
    int incoming = client.read();
    if (incoming < 0) continue;
    char ch = (char)incoming;
    if (ch == '\n') {
      done = true;
    } else if (ch == '\r') {
      // Allow CRLF clients without changing the returned application data.
    } else if (len < MAX_PAYLOAD) {
      payload[len++] = ch;
    } else {
      overflow = true;
    }
  }
  payload[len] = '\0';
  if (overflow) {
    rejected++;
    client.println(F("ERR TOO_LONG"));
    Serial.println(F("TCP ERROR: payload too long"));
  } else if (!done) {
    rejected++;
    if (client.connected()) client.println(F("ERR INCOMPLETE"));
    Serial.println(F("TCP ERROR: incomplete or timed-out line"));
  } else {
    // Stream the reply without dynamic memory allocations.
    client.print(F("ECHO "));
    client.write((const uint8_t *)payload, len);
    client.write((uint8_t)'\n');
    echoed++;
    Serial.print(F("TCP ECHO #")); Serial.print(echoed);
    Serial.print(F(" bytes=")); Serial.println(len);
  }
  delay(3);
  client.stop();
}
void setup() {
  Serial.begin(115200);
  pinMode(ETH_CS, OUTPUT); digitalWrite(ETH_CS, HIGH);
  pinMode(SD_CS, OUTPUT); digitalWrite(SD_CS, HIGH);
  delay(350);
  Serial.println(F("KON W5100 TEST-05 / TCP ECHO / SD OFF"));
  Ethernet.init(ETH_CS);
  Serial.println(F("DHCP request..."));
  if (Ethernet.begin(mac, 10000, 2000) == 0) {
    Serial.println(F("DHCP FAIL / TCP SERVER DISABLED"));
    return;
  }
  ready = true;
  server.begin();
  Serial.print(F("Controller: "));
  Serial.println(Ethernet.hardwareStatus() == EthernetW5100 ? F("W5100") : F("OTHER"));
  Serial.print(F("IP: ")); Serial.println(Ethernet.localIP());
  Serial.print(F("TCP_PORT: ")); Serial.println(TCP_PORT);
  Serial.println(F("SERVER STARTED / TCP TEST PENDING"));
}
void loop() {
  if (!ready) return;
  if (millis() - lastDhcpCheck >= 30000UL) {
    lastDhcpCheck = millis();
    int result = Ethernet.maintain();
    if (result == 1 || result == 3) Serial.println(F("WARNING: DHCP maintenance error"));
    if (result == 2 || result == 4) {
      Serial.print(F("DHCP IP: ")); Serial.println(Ethernet.localIP());
    }
  }
  EthernetClient client = server.available();
  if (!client) return;
  connections++;
  Serial.print(F("TCP CONNECT #")); Serial.println(connections);
  handleClient(client);
  Serial.print(F("TCP CLOSE / echo=")); Serial.print(echoed);
  Serial.print(F(" rejected=")); Serial.println(rejected);
}
