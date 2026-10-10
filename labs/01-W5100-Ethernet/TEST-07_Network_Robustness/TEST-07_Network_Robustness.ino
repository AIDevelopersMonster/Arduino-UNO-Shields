/* LAB-01B TEST-07 v0.2 — UNO ATmega328P + W5100, SD disabled.
 * UDP binary echo 1..128 B :5001; TCP TEST-05 line echo :5000.
 * DHCP retry after startup/maintenance failure; no static fallback.
 * Ethernet 2.0.2, CS D10, SD CS D4 HIGH, UART 115200.
 * W5100 cannot report physical link state: Unknown is not LinkOFF.
 * Events indicate software observations, never hardware PASS.
 */
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

const uint8_t ETH_CS = 10, SD_CS = 4;
const uint16_t UDP_PORT = 5001, TCP_PORT = 5000, UDP_CAP = 128;
const uint32_t DHCP_TIMEOUT = 6000UL, DHCP_RESPONSE_TIMEOUT = 1000UL;
const uint32_t RETRY_MS = 5000UL, MAINTAIN_MS = 1000UL;
const uint32_t STAT_MS = 5000UL, TCP_LINE_MS = 1500UL;
byte mac[] = {0x02, 0x4B, 0x4F, 0x4E, 0x51, 0x07};
EthernetUDP udp;
EthernetServer server(TCP_PORT);
EthernetClient client;
uint8_t packet[UDP_CAP];
char line[64];
uint8_t lineLength = 0;
bool ready = false, retryNow = true, ethernetInitialized = false;
IPAddress boundIP;
uint32_t retryTick = 0, maintainTick = 0, statTick = 0, clientTick = 0;
uint32_t attempts = 0, dhcpOK = 0, dhcpFail = 0, renewOK = 0, renewFail = 0;
uint32_t rebindOK = 0, rebindFail = 0, rx = 0, tx = 0, drop = 0, sendFail = 0;
uint32_t tcpOK = 0, tcpReject = 0, serviceRestarts = 0, maxDhcpMs = 0;
int minFree = 32767;
EthernetLinkStatus lastLink = Unknown;
extern int __heap_start;
extern void *__brkval;

int freeRam() {
  int top;
  return (int)&top - (__brkval ? (int)__brkval : (int)&__heap_start);
}
void sampleRam() {
  int free = freeRam();
  if (free < minFree) minFree = free;
}
void event(const __FlashStringHelper *name) {
  Serial.print(F("EVT ms=")); Serial.print(millis());
  Serial.print(F(" name=")); Serial.print(name);
}
void simpleEvent(const __FlashStringHelper *name) {
  event(name); Serial.println();
}
void closeClient() {
  if (ethernetInitialized && client) client.stop();
  lineLength = 0;
}
void closeAllSockets() {
  // Indexed clients access hardware even when no application socket is open.
  if (!ethernetInitialized) return;
  // Public indexed-client API. Ethernet.socketClose() itself is private.
  for (uint8_t i = 0; i < 4; ++i) {
    EthernetClient socket(i);
    socket.setConnectionTimeout(20);
    socket.stop();
  }
}
void stopServices() {
  ready = false;
  if (!ethernetInitialized) { lineLength = 0; return; }
  closeClient();
  udp.stop();
  // EthernetServer has no end(). Close all four W5100 sockets explicitly.
  closeAllSockets();
}
void scheduleDhcp(const __FlashStringHelper *reason) {
  stopServices();
  if (ethernetInitialized) Ethernet.setLocalIP(IPAddress(0,0,0,0));
  retryTick = millis();
  retryNow = false;
  event(F("RETRY_SCHEDULED")); Serial.print(F(" reason="));
  Serial.print(reason); Serial.println(F(" delay_ms=5000"));
}
bool startServices() {
  if (!ethernetInitialized) return false;
  udp.stop();
  closeClient();
  closeAllSockets();
  if (!udp.begin(UDP_PORT)) return false;
  server.begin();
  if (!server) { udp.stop(); return false; }
  boundIP = Ethernet.localIP();
  ready = true;
  serviceRestarts++;
  event(F("SERVICES_READY")); Serial.print(F(" ip=")); Serial.print(boundIP);
  Serial.println(F(" udp=5001 tcp=5000"));
  return true;
}
void networkConfig() {
  event(F("NET")); Serial.print(F(" ip=")); Serial.print(Ethernet.localIP());
  Serial.print(F(" mask=")); Serial.print(Ethernet.subnetMask());
  Serial.print(F(" gateway=")); Serial.print(Ethernet.gatewayIP());
  Serial.print(F(" dns=")); Serial.println(Ethernet.dnsServerIP());
}
void acquireDhcp() {
  stopServices();
  attempts++;
  event(F("DHCP_BEGIN")); Serial.print(F(" attempt=")); Serial.println(attempts);
  uint32_t started = millis();
  int result = Ethernet.begin(mac, DHCP_TIMEOUT, DHCP_RESPONSE_TIMEOUT);
  uint32_t elapsed = millis() - started;
  if (elapsed > maxDhcpMs) maxDhcpMs = elapsed;
  sampleRam();
  retryNow = false;
  retryTick = millis();
  // begin() initializes the driver even when DHCP fails. hardwareStatus()
  // reads its cached chip identity; it does not issue a register transaction.
  ethernetInitialized = Ethernet.hardwareStatus() != EthernetNoHardware;
  event(F("HARDWARE")); Serial.print(F(" chip="));
  Serial.println(Ethernet.hardwareStatus() == EthernetW5100 ? F("W5100") : F("OTHER_OR_NONE"));
  if (!ethernetInitialized) {
    dhcpFail++;
    event(F("DHCP_FAIL")); Serial.print(F(" elapsed_ms=")); Serial.print(elapsed);
    Serial.println(F(" reason=NO_HARDWARE"));
    return;
  }
  // Bounds ARP/TCP retries. These are chip settings, not total call deadlines.
  Ethernet.setRetransmissionTimeout(200);
  Ethernet.setRetransmissionCount(2);
  if (result != 1 || Ethernet.localIP() == IPAddress(0,0,0,0)) {
    dhcpFail++;
    Ethernet.setLocalIP(IPAddress(0,0,0,0));
    event(F("DHCP_FAIL")); Serial.print(F(" elapsed_ms=")); Serial.println(elapsed);
    return;
  }
  dhcpOK++;
  event(F("DHCP_OK")); Serial.print(F(" elapsed_ms=")); Serial.println(elapsed);
  networkConfig();
  if (!startServices()) scheduleDhcp(F("BIND_FAIL"));
  maintainTick = millis();
}
void maintainDhcp() {
  uint32_t started = millis();
  int rc = Ethernet.maintain();
  uint32_t elapsed = millis() - started;
  if (elapsed > maxDhcpMs) maxDhcpMs = elapsed;
  sampleRam();
  maintainTick = millis();
  if (!rc) return; // 0 is NOT evidence of renewal.
  if (rc == 1) renewFail++;
  if (rc == 2) renewOK++;
  if (rc == 3) rebindFail++;
  if (rc == 4) rebindOK++;
  event(F("DHCP_MAINTAIN")); Serial.print(F(" rc=")); Serial.print(rc);
  Serial.print(F(" elapsed_ms=")); Serial.println(elapsed);
  if (rc == 1 || rc == 3) {
    // Conservative: cease serving the old address and reacquire a valid lease.
    scheduleDhcp(F("LEASE_FAIL"));
  } else if (rc == 2 || rc == 4) {
    networkConfig();
    if (Ethernet.localIP() != boundIP) {
      simpleEvent(F("IP_CHANGED"));
      if (!startServices()) scheduleDhcp(F("BIND_FAIL"));
    }
  }
}
void serviceUdp() {
  int size = udp.parsePacket();
  if (size <= 0) return;
  rx++;
  if (size > (int)UDP_CAP) {
    drop++;
    // Drain with a bounded number of reads; avoid a stuck remaining counter.
    int remaining = size;
    while (remaining > 0) {
      int got = udp.read(packet, min((int)UDP_CAP, remaining));
      if (got <= 0) { scheduleDhcp(F("UDP_READ_FAIL")); break; }
      remaining -= got;
    }
    simpleEvent(F("UDP_OVERSIZE"));
    return;
  }
  int n = udp.read(packet, size);
  if (n != size) { drop++; scheduleDhcp(F("UDP_SHORT_READ")); return; }
  sampleRam(); // Includes the application UDP stack frame, not library internals.
  IPAddress peer = udp.remoteIP();
  uint16_t port = udp.remotePort();
  bool sent = udp.beginPacket(peer, port) == 1;
  if (sent) {
    size_t wrote = udp.write(packet, n);
    int rc = udp.endPacket();
    sent = wrote == (size_t)n && rc == 1;
  }
  if (sent) tx++;
  else { sendFail++; simpleEvent(F("UDP_SEND_FAIL")); }
}
void serviceTcp() {
  if (!client) {
    // accept() observes silent connections too; available() would leak idle sockets.
    client = server.accept();
    if (!client) return;
    client.setConnectionTimeout(200);
    clientTick = millis();
    lineLength = 0;
  }
  sampleRam();
  uint8_t budget = 65; // Fixed per-loop work; DHCP/UDP remain serviced.
  while (client.available() && budget--) {
    int c = client.read();
    if (c < 0) break;
    if (c == '\r') continue;
    if (c == '\n') {
      bool sent = client.print(F("ECHO ")) == 5;
      sent = (client.write((uint8_t*)line, lineLength) == lineLength) && sent;
      sent = (client.write((uint8_t)'\n') == 1) && sent;
      if (sent) tcpOK++;
      else { tcpReject++; simpleEvent(F("TCP_SEND_FAIL")); }
      closeClient();
      return;
    }
    if (lineLength >= sizeof(line)) {
      tcpReject++; simpleEvent(F("TCP_TOO_LONG")); closeClient(); return;
    }
    line[lineLength++] = (char)c;
  }
  if (!client.connected() || millis() - clientTick >= TCP_LINE_MS) {
    tcpReject++; simpleEvent(F("TCP_ABORT_OR_TIMEOUT")); closeClient();
  }
}
void stats() {
  sampleRam();
  Serial.print(F("STAT ms=")); Serial.print(millis());
  Serial.print(F(" ready=")); Serial.print(ready);
  Serial.print(F(" ip="));
  Serial.print(ethernetInitialized ? Ethernet.localIP() : IPAddress(0,0,0,0));
  Serial.print(F(" free=")); Serial.print(freeRam());
  Serial.print(F(" min_free=")); Serial.print(minFree);
  Serial.print(F(" attempts=")); Serial.print(attempts);
  Serial.print(F(" dhcp_ok=")); Serial.print(dhcpOK);
  Serial.print(F(" dhcp_fail=")); Serial.print(dhcpFail);
  Serial.print(F(" renew_ok=")); Serial.print(renewOK);
  Serial.print(F(" renew_fail=")); Serial.print(renewFail);
  Serial.print(F(" rebind_ok=")); Serial.print(rebindOK);
  Serial.print(F(" rebind_fail=")); Serial.print(rebindFail);
  Serial.print(F(" rx=")); Serial.print(rx);
  Serial.print(F(" tx=")); Serial.print(tx);
  Serial.print(F(" drop=")); Serial.print(drop);
  Serial.print(F(" send_fail=")); Serial.print(sendFail);
  Serial.print(F(" tcp_ok=")); Serial.print(tcpOK);
  Serial.print(F(" tcp_reject=")); Serial.print(tcpReject);
  Serial.print(F(" restarts=")); Serial.print(serviceRestarts);
  Serial.print(F(" max_dhcp_ms=")); Serial.println(maxDhcpMs);
}
void setup() {
  Serial.begin(115200);
  digitalWrite(ETH_CS, HIGH); digitalWrite(SD_CS, HIGH);
  pinMode(ETH_CS, OUTPUT); pinMode(SD_CS, OUTPUT);
  delay(350);
  Serial.println(); // Separate the BOOT line from bytes received during reset.
  simpleEvent(F("BOOT"));
  Serial.println(F("INFO test=07 version=0.2 mac=02:4B:4F:4E:51:07 link=Unknown_expected"));
  Ethernet.init(ETH_CS);
}
void loop() {
  sampleRam();
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '?') stats();
    if (c == 'D') { simpleEvent(F("MANUAL_DHCP")); scheduleDhcp(F("UART")); }
  }
  if (!ready) {
    if (retryNow || millis() - retryTick >= RETRY_MS) acquireDhcp();
  } else {
    if (millis() - maintainTick >= MAINTAIN_MS) maintainDhcp();
    if (ready) {
      EthernetLinkStatus link = Ethernet.linkStatus();
      if (link != lastLink) {
        lastLink = link;
        event(F("LINK")); Serial.print(F(" value=")); Serial.println((int)link);
      }
      serviceUdp();
      if (ready) serviceTcp();
    }
  }
  if (millis() - statTick >= STAT_MS) { statTick = millis(); stats(); }
}
