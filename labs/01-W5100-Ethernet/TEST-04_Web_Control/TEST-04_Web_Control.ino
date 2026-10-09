/**
 * Arduino UNO & Shields | LAB-01 (W5100 Ethernet, no microSD)
 * TEST-04: Local Ethernet GPIO Web Control
 *
 * Purpose: Serve a browser control panel and JSON state for digital outputs D6 and D7.
 * Target: Arduino UNO / ATmega328P (16 MHz, 32 KB flash, 2 KB SRAM).
 * Hardware: Blue WIZnet W5100 Ethernet Shield (HanRun RJ45 sample).
 * Setup: Ethernet library; DHCP; W5100 D10; SD D4 disabled; outputs LOW at boot.
 * Serial monitor: 115200 baud. Build: arduino-cli --fqbn arduino:avr:uno.
 * Expected: PASS target: HTTP controls, JSON state, and separately measured GPIO electrical levels.
 * Limitations: Trusted LAN only. No auth/TLS/CSRF protection; do not attach mains loads.
 * Source: https://github.com/AIDevelopersMonster/Arduino-UNO-Shields
 * Evidence: see sibling RESULT_2026-10-09.md when the test is certified.
 * Documentation-only revision: operational logic preserved.
 */

#include <SPI.h>
#include <Ethernet.h>
#include <string.h>
const byte CS_ETH=10, CS_SD=4, P6=6, P7=7;
byte mac[]={0x02,0x4B,0x4F,0x4E,0x51,0x04};
EthernetServer server(80);
bool ready=false, s6=false, s7=false;
unsigned long tick=0, count=0;

// Send common no-cache HTTP headers; each response closes its connection.
void header(EthernetClient &c, const __FlashStringHelper *type) {
  c.println(F("HTTP/1.1 200 OK"));
  c.print(F("Content-Type: ")); c.println(type);
  c.println(F("Connection: close"));
  c.println(F("Cache-Control: no-store"));
  c.println();
}
// Browser commands redirect to the dashboard after changing an output.
void redirect(EthernetClient &c) {
  c.println(F("HTTP/1.1 303 See Other"));
  c.println(F("Location: /"));
  c.println(F("Connection: close"));
  c.println(F("Cache-Control: no-store"));
  c.println();
}
void error(EthernetClient &c, bool method) {
  if (method) c.println(F("HTTP/1.1 405 Method Not Allowed"));
  else c.println(F("HTTP/1.1 404 Not Found"));
  c.println(F("Content-Type: text/plain"));
  c.println(F("Connection: close"));
  c.println();
  c.println(method?F("Method not allowed"):F("Not found"));
}
void control(EthernetClient &c, byte pin, bool val) {
  c.print(F("<section><h2>D"));c.print(pin);
  c.print(F(": "));c.print(val?F("ON"):F("OFF"));
  c.println(F("</h2>"));
  c.print(F("<a href='/d"));c.print(pin);c.println(F("/on'>ON</a>"));
  c.print(F("<a href='/d"));c.print(pin);c.println(F("/off'>OFF</a></section>"));
}
// Generate the dashboard directly from PROGMEM-backed F() strings.
void page(EthernetClient &c) {
  header(c,F("text/html; charset=utf-8"));
  c.println(F("<!doctype html><html><head><meta charset='utf-8'>"));
  c.println(F("<meta name='viewport' content='width=device-width,initial-scale=1'>"));
  c.println(F("<title>KON W5100 TEST-04</title>"));
  c.println(F("<style>body{background:#102030;color:white;font:18px Arial;max-width:650px;margin:20px auto;padding:12px}"));
  c.println(F("section{background:#253e55;border-radius:12px;padding:12px;margin:12px 0}"));
  c.println(F("a{display:inline-block;background:#347bd1;color:white;text-decoration:none;padding:14px 24px;border-radius:8px;margin:6px}</style></head><body>"));
  c.println(F("<h1>Arduino UNO + W5100</h1><h2>TEST-04 Web Control</h2>"));
  control(c,P6,s6);control(c,P7,s7);
  c.print(F("<p>IP: "));c.print(Ethernet.localIP());c.println(F("</p>"));
  c.print(F("<p>Uptime: "));c.print(millis()/1000UL);c.println(F(" s</p>"));
  c.print(F("<p>Requests: "));c.print(count);c.println(F("</p>"));
  c.println(F("<p><a href='/'>Refresh</a><a href='/api'>JSON API</a></p>"));
  c.println(F("<p>No SD. Trusted local network only.</p></body></html>"));
}
// Report application state; physical GPIO level needs an independent check.
void api(EthernetClient &c) {
  header(c,F("application/json; charset=utf-8"));
  c.print(F("{\"d6\":"));c.print(s6?F("true"):F("false"));
  c.print(F(",\"d7\":"));c.print(s7?F("true"):F("false"));
  c.print(F(",\"uptime_s\":"));c.print(millis()/1000UL);
  c.print(F(",\"requests\":"));c.print(count);
  c.println(F("}"));
}
// Apply the output level and mirror it into the page/API state.
void setPin(byte pin, bool value) {
  digitalWrite(pin,value?HIGH:LOW);
  if(pin==P6)s6=value;
  else if(pin==P7)s7=value;
}
// Force outputs LOW before starting any network operation.
void setup() {
  Serial.begin(115200);
  pinMode(P6,OUTPUT);digitalWrite(P6,LOW);
  pinMode(P7,OUTPUT);digitalWrite(P7,LOW);
  pinMode(CS_ETH,OUTPUT);digitalWrite(CS_ETH,HIGH);
  pinMode(CS_SD,OUTPUT);digitalWrite(CS_SD,HIGH);
  delay(350);
  Serial.println(F("KON W5100 TEST-04 / D6 D7 WEB CONTROL / SD OFF"));
  Ethernet.init(CS_ETH);
  Serial.println(F("DHCP request..."));
  if(Ethernet.begin(mac,10000,2000)==0) {
    Serial.println(F("DHCP FAIL / SERVER DISABLED"));return;
  }
  ready=true;server.begin();
  Serial.print(F("Controller: "));
  Serial.println(Ethernet.hardwareStatus()==EthernetW5100?F("W5100"):F("OTHER"));
  Serial.print(F("IP: "));Serial.println(Ethernet.localIP());
  Serial.print(F("OPEN: http://"));Serial.print(Ethernet.localIP());Serial.println('/');
  Serial.println(F("SERVER STARTED / TEST PENDING"));
}
// Parse only the bounded HTTP request line; handle one client at a time.
void loop() {
  if(!ready)return;
  if(millis()-tick>=30000UL) {
    tick=millis();
    int r=Ethernet.maintain();
    if(r==1||r==3)Serial.println(F("WARNING: DHCP maintenance error"));
    if(r==2||r==4){Serial.print(F("NEW IP: "));Serial.println(Ethernet.localIP());}
  }
  EthernetClient c=server.available();
  if(!c)return;
  char method[5]={0}, path[16]={0};
  byte mi=0,pi=0,stage=0;
  bool complete=false;
  unsigned long start=millis();
  while(c.connected() && millis()-start<1500UL && !complete) {
    if(!c.available())continue;
    char ch=c.read();
    if(ch=='\n'){complete=true;break;}
    if(ch=='\r')continue;
    if(stage==0) {
      if(ch==' '){stage=1;continue;}
      if(mi<sizeof(method)-1)method[mi++]=ch;
    } else if(stage==1) {
      if(ch==' '){stage=2;continue;}
      if(pi<sizeof(path)-1)path[pi++]=ch;
    }
  }
  count++;
  Serial.print(F("REQ #"));Serial.print(count);
  Serial.print(' ');Serial.print(method);Serial.print(' ');Serial.println(path);
  if(!complete||strcmp(method,"GET")!=0)error(c,true);
  else if(strcmp(path,"/")==0)page(c);
  else if(strcmp(path,"/api")==0)api(c);
  else if(strcmp(path,"/d6/on")==0){setPin(P6,true);redirect(c);}
  else if(strcmp(path,"/d6/off")==0){setPin(P6,false);redirect(c);}
  else if(strcmp(path,"/d7/on")==0){setPin(P7,true);redirect(c);}
  else if(strcmp(path,"/d7/off")==0){setPin(P7,false);redirect(c);}
  else error(c,false);
  delay(2);c.stop();
}
