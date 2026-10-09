/**
 * LAB-01B TEST-06: UDP Binary Echo, Arduino UNO + W5100.
 * DHCP, UDP port 5001, Serial 115200. No microSD card.
 * Ethernet CS=D10, SD CS=D4 HIGH, SPI via ICSP.
 * Binary datagrams up to 128 bytes. Echo exact payload to sender.
 * Oversized messages discarded. Trusted LAN only.
 * UDP is connectionless; delivery/order are not guaranteed.
 * Hardware verification PENDING.
 */
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
const uint8_t ETH_CS=10, SD_CS=4;
const uint16_t PORT=5001, CAP=128;
byte mac[]={0x02,0x4B,0x4F,0x4E,0x51,0x06};
EthernetUDP udp;
uint8_t payload[CAP];
bool active=false;
uint32_t received=0, echoed=0, discarded=0;
unsigned long dhcpTick=0;

void setup() {
  Serial.begin(115200);
  pinMode(ETH_CS,OUTPUT); digitalWrite(ETH_CS,HIGH);
  pinMode(SD_CS,OUTPUT); digitalWrite(SD_CS,HIGH);
  delay(350);
  Serial.println(F("KON W5100 TEST-06 / UDP BINARY ECHO / SD OFF"));
  Ethernet.init(ETH_CS);
  Serial.println(F("DHCP request..."));
  if(Ethernet.begin(mac,10000,2000)==0) {
    Serial.println(F("DHCP FAIL")); return;
  }
  if(!udp.begin(PORT)) { Serial.println(F("UDP BIND FAIL")); return; }
  active=true;
  Serial.print(F("Controller: "));
  Serial.println(Ethernet.hardwareStatus()==EthernetW5100?F("W5100"):F("OTHER"));
  Serial.print(F("IP: ")); Serial.println(Ethernet.localIP());
  Serial.print(F("UDP_PORT: ")); Serial.println(PORT);
  Serial.println(F("UDP READY / TEST PENDING"));
}
void loop() {
  if(!active)return;
  if(millis()-dhcpTick>=30000UL) {
    dhcpTick=millis();
    int result=Ethernet.maintain();
    if(result==1||result==3)Serial.println(F("DHCP MAINTENANCE ERROR"));
    if(result==2||result==4) {
      Serial.print(F("DHCP IP: "));Serial.println(Ethernet.localIP());
    }
  }
  int size=udp.parsePacket();
  if(size<=0)return;
  received++;
  // Reject full oversized datagrams rather than echoing truncated bytes.
  if(size>CAP) {
    discarded++;
    while(udp.available()>0)udp.read();
    Serial.print(F("UDP OVERSIZE size="));Serial.println(size);
    return;
  }
  int n=udp.read(payload,CAP);
  if(n!=size) {
    discarded++;Serial.println(F("UDP SHORT READ"));return;
  }
  IPAddress peer=udp.remoteIP();
  uint16_t peerPort=udp.remotePort();
  if(udp.beginPacket(peer,peerPort)!=1) {
    discarded++;Serial.println(F("UDP SEND INIT FAIL"));return;
  }
  size_t wrote=udp.write(payload,(size_t)n);
  int sent=udp.endPacket();
  if(wrote!=(size_t)n||sent!=1) {
    discarded++;Serial.println(F("UDP SEND FAIL"));return;
  }
  echoed++;
  Serial.print(F("UDP ECHO #"));Serial.print(echoed);
  Serial.print(F(" bytes="));Serial.print(n);
  Serial.print(F(" peer="));Serial.print(peer);
  Serial.print(':');Serial.println(peerPort);
}
