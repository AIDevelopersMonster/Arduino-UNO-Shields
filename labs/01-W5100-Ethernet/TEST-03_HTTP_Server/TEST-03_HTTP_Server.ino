/**
 * Arduino UNO & Shields | LAB-01B (blue W5100, no microSD)
 * TEST-03: Minimal HTTP Server
 *
 * Purpose: Serve diagnostic HTML at / and plain text at /health.
 * Target: Arduino UNO / ATmega328P (16 MHz, 32 KB flash, 2 KB SRAM).
 * Hardware: Blue WIZnet W5100 Ethernet Shield (HanRun RJ45 sample).
 * Setup: Ethernet library; DHCP; TCP port 80; SD disabled; 115200 baud.
 * Serial monitor: 115200 baud. Build: arduino-cli --fqbn arduino:avr:uno.
 * Expected: PASS: HTTP 200 for / and /health; request count and uptime progress.
 * Limitations: No authentication/TLS; basic parser, no endurance or security certification.
 * Source: https://github.com/AIDevelopersMonster/Arduino-UNO-Shields
 * Evidence: see sibling RESULT_2026-10-09.md when the test is certified.
 * Documentation-only revision: operational logic preserved.
 */

#include <SPI.h>
#include <Ethernet.h>
const uint8_t ETH_CS=10, SD_CS=4;
byte mac[]={0x02,0x4B,0x4F,0x4E,0x51,0x03};
EthernetServer server(80);
uint32_t requests=0;
bool started=false;
unsigned long lastMaintenance=0;

// Stream the HTML page from flash-resident string literals to save AVR SRAM.
void printPage(EthernetClient &c) {
  c.println(F("HTTP/1.1 200 OK"));
  c.println(F("Content-Type: text/html; charset=utf-8"));
  c.println(F("Connection: close"));
  c.println(F("Cache-Control: no-store"));
  c.println();
  c.println(F("<!doctype html><html><head><meta charset='utf-8'><title>KON W5100 TEST-03</title>"));
  c.println(F("<meta name='viewport' content='width=device-width,initial-scale=1'></head>"));
  c.println(F("<body style='font:18px Arial;background:#101e2d;color:white;padding:24px'>"));
  c.println(F("<h1>Arduino UNO + W5100</h1><h2>TEST-03 HTTP PASS (server responded)</h2>"));
  c.print(F("<p>IP: "));c.print(Ethernet.localIP());c.println(F("</p>"));
  c.print(F("<p>Uptime: "));c.print(millis()/1000UL);c.println(F(" s</p>"));
  c.print(F("<p>Requests: "));c.print(requests);c.println(F("</p>"));
  c.println(F("<p>SPI and DHCP tested earlier. SD not used.</p></body></html>"));
}
// Provide a small, machine-readable text health endpoint.
void printHealth(EthernetClient &c) {
  c.println(F("HTTP/1.1 200 OK"));
  c.println(F("Content-Type: text/plain; charset=utf-8"));
  c.println(F("Connection: close"));
  c.println(F("Cache-Control: no-store"));
  c.println();
  c.println(F("KON W5100 TEST-03 HTTP OK"));
  c.print(F("uptime_s="));c.println(millis()/1000UL);
  c.print(F("requests="));c.println(requests);
}
// Keep both SPI peripherals deselected until Ethernet initialization.
void setup() {
  Serial.begin(115200);
  pinMode(ETH_CS,OUTPUT);digitalWrite(ETH_CS,HIGH);
  pinMode(SD_CS,OUTPUT);digitalWrite(SD_CS,HIGH);
  delay(400);
  Serial.println(F("KON LAB W5100 TEST-03 / HTTP Server / SD OFF"));
  Ethernet.init(ETH_CS);
  Serial.println(F("DHCP request..."));
  if(Ethernet.begin(mac,10000,2000)==0) {
    Serial.println(F("RESULT: DHCP FAIL. HTTP NOT STARTED"));
    return;
  }
  Serial.print(F("Controller: "));
  Serial.println(Ethernet.hardwareStatus()==EthernetW5100?F("W5100"):F("OTHER"));
  Serial.print(F("DHCP IP: "));Serial.println(Ethernet.localIP());
  server.begin();
  started=true;
  Serial.print(F("OPEN: http://"));Serial.print(Ethernet.localIP());Serial.println(F("/"));
  Serial.println(F("HEALTH: /health"));
  Serial.println(F("RESULT: SERVER STARTED / HTTP REQUEST PENDING"));
}
// Handle one short HTTP request at a time using fixed-size stack buffers.
void loop() {
  if(!started)return;
  if(millis()-lastMaintenance>=30000UL) {
    lastMaintenance=millis();
    int lease=Ethernet.maintain();
    if(lease==1 || lease==3)Serial.println(F("WARNING: DHCP renewal error"));
    if(lease==2 || lease==4){
      Serial.print(F("DHCP updated IP: "));Serial.println(Ethernet.localIP());
    }
  }
  EthernetClient c=server.available();
  if(!c)return;
  char path[12]={0};
  char method[5]={0};
  byte mi=0,pi=0;
  uint8_t stage=0;
  unsigned long deadline=millis()+1500UL;
  bool lineDone=false;
  // Limit request-line waiting time to prevent an indefinitely stalled client.
  while(c.connected() && (long)(deadline-millis())>0 && !lineDone) {
    if(!c.available())continue;
    char ch=c.read();
    if(ch=='\r'||ch=='\n'){lineDone=true;break;}
    if(stage==0) {
      if(ch==' '){stage=1;continue;}
      if(mi<sizeof(method)-1)method[mi++]=ch;
    } else if(stage==1) {
      if(ch==' '){stage=2;continue;}
      if(pi<sizeof(path)-1)path[pi++]=ch;
    }
  }
  requests++;
  Serial.print(F("REQ #"));Serial.print(requests);
  Serial.print(F(" "));Serial.print(method);
  Serial.print(F(" "));Serial.println(path);
  if(!lineDone) {
    c.println(F("HTTP/1.1 408 Request Timeout\r\nConnection: close\r\n\r\n"));
  } else if(strcmp(method,"GET")!=0) {
    c.println(F("HTTP/1.1 405 Method Not Allowed\r\nAllow: GET\r\nConnection: close\r\n\r\n"));
  } else if(strcmp(path,"/health")==0) {
    printHealth(c);
  } else if(strcmp(path,"/")==0) {
    printPage(c);
  } else {
    c.println(F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\nNot found"));
  }
  delay(2);
  c.stop();
}
