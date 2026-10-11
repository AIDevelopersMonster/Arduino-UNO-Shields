/**
 * TEST-09B — Shared SPI / Ethernet W5100 + microSD, silent UART.
 * Uno ATmega328P, CS: W5100 D10 / SD D4. UDP port 5003.
 * 24-byte datagrams: "K9D:" + unique binary data. Each accepted
 * request is appended to K9BTEST.BIN and read back byte-for-byte,
 * then replied to via UDP. SD transaction finishes before Ethernet TX.
 * "K9END" returns test counters via UDP, prints ONE serial report.
 * Boot diagnostics once, then no COM read/print during measurement.
 * Test file is APPENDED to; existing data is not removed.
 * Bench result PENDING.
 */
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SD.h>
#include <string.h>
#include <stdio.h>

const uint8_t ETH_CS=10, SD_CS=4;
const uint16_t UDP_PORT=5003;
const uint8_t PACKET_BYTES=24;
const char TEST_FILE[]="K9BTEST.BIN";
byte mac[]={0x02,0x4B,0x4F,0x4E,0x59,0x0B};
EthernetUDP udp;
uint8_t packet[32], check[PACKET_BYTES];
bool sdReady=false, ethReady=false, done=false;
uint16_t received=0, echoes=0, verifiedWrites=0, verifiedReads=0, sdErrors=0;
uint32_t fileBytes=0;
unsigned long begunAt=0;

bool appendAndCheck(const uint8_t* payload) {
  if(!sdReady) { ++sdErrors; return false; }
  File f=SD.open(TEST_FILE, FILE_WRITE);
  if(!f) { ++sdErrors; return false; }
  uint32_t offset=f.position();
  size_t wrote=f.write(payload, PACKET_BYTES);
  f.close();
  if(wrote!=PACKET_BYTES) { ++sdErrors; return false; }
  ++verifiedWrites;
  File r=SD.open(TEST_FILE, FILE_READ);
  if(!r) { ++sdErrors; return false; }
  bool positioned=r.seek(offset);
  int count=positioned ? r.read(check, PACKET_BYTES) : -1;
  r.close();
  if(count!=PACKET_BYTES || memcmp(check,payload,PACKET_BYTES)!=0) {
    ++sdErrors; return false;
  }
  ++verifiedReads;
  fileBytes+=PACKET_BYTES;
  return true;
}

void sendResult(IPAddress ip,uint16_t port) {
  char summary[128];
  snprintf(summary,sizeof(summary),
    "K9RESULT eth=%u sd=%u rx=%u echo=%u wr=%u rd=%u err=%u bytes=%lu ms=%lu",
    ethReady?1:0,sdReady?1:0,received,echoes,verifiedWrites,
    verifiedReads,sdErrors,(unsigned long)fileBytes,
    (unsigned long)(millis()-begunAt));
  if(udp.beginPacket(ip,port)==1) {
    udp.write((const uint8_t*)summary,strlen(summary));
    udp.endPacket();
  }
  Serial.println(summary);  // ONLY UART output, after END.
}

void setup() {
  Serial.begin(115200);
  pinMode(ETH_CS,OUTPUT); digitalWrite(ETH_CS,HIGH);
  pinMode(SD_CS,OUTPUT); digitalWrite(SD_CS,HIGH);
  // Initialize card while W5100 is deselected.
  sdReady=SD.begin(SD_CS);
  digitalWrite(SD_CS,HIGH);
  Ethernet.init(ETH_CS);
  ethReady=Ethernet.begin(mac,10000,2000)!=0;
  if(ethReady) ethReady=udp.begin(UDP_PORT)!=0;
  begunAt=millis();
  // One-time start-up diagnostics ONLY. No UART during packet processing.
  Serial.print(F("TEST-09B DIAG sd=")); Serial.print(sdReady?F("PASS"):F("FAIL"));
  Serial.print(F(" eth=")); Serial.print(ethReady?F("PASS"):F("FAIL"));
  Serial.print(F(" hw=")); Serial.print((int)Ethernet.hardwareStatus());
  Serial.print(F(" ip=")); Serial.print(Ethernet.localIP());
  Serial.print(F(" udp=")); Serial.println(ethReady?UDP_PORT:0);
  Serial.println(F("UART QUIET / READY FOR HOST TEST"));
}

void loop() {
  if(!ethReady || done) return;
  int length=udp.parsePacket();
  if(length<=0) return;
  IPAddress ip=udp.remoteIP();
  uint16_t port=udp.remotePort();
  if(length>(int)sizeof(packet)) {
    while(udp.available()>0) udp.read();
    return;
  }
  int count=udp.read(packet,sizeof(packet));
  if(count==5 && memcmp(packet,"K9END",5)==0) {
    done=true; sendResult(ip,port); return;
  }
  if(count!=PACKET_BYTES || memcmp(packet,"K9D:",4)!=0) return;
  ++received;
  appendAndCheck(packet); // SD File is closed before Ethernet TX.
  if(udp.beginPacket(ip,port)==1) {
    size_t wrote=udp.write(packet,PACKET_BYTES);
    int ok=udp.endPacket();
    if(wrote==PACKET_BYTES && ok==1) ++echoes;
  }
}
