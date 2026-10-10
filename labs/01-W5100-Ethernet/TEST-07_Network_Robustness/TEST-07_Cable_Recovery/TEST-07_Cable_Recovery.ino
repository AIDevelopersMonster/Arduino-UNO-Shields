/**
 * LAB-01B TEST-07: RJ45 cable outage and recovery
 * Arduino UNO + WIZnet W5100 Ethernet Shield; no SD.
 * UDP binary echo on port 5002, maximum 32-byte payload.
 * W5100 CS D10, SD CS D4 HIGH, Serial 115200.
 * USB must remain powered throughout unplug/replug.
 * DHCP settings of the router must NOT be changed.
 * Link-state detection is not available on the classic W5100.
 * Acceptance relies on actual UDP Echo response from the PC.
 */
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
const byte ETH_CS=10, SD_CS=4;
const uint16_t PORT=5002;
byte mac[]={0x02,0x4B,0x4F,0x4E,0x51,0x07};
EthernetUDP udp;
byte data[32];
bool active=false;
uint32_t rx=0,tx=0,errors=0,dhcpErrors=0;
unsigned long lastStatus=0,lastDhcp=0;

void statusLine(){
  Serial.print(F("STATUS uptime_s="));Serial.print(millis()/1000UL);
  Serial.print(F(" ip="));Serial.print(Ethernet.localIP());
  Serial.print(F(" rx="));Serial.print(rx);
  Serial.print(F(" tx="));Serial.print(tx);
  Serial.print(F(" errors="));Serial.print(errors);
  Serial.print(F(" dhcp_errors="));Serial.println(dhcpErrors);
}
void setup(){
  Serial.begin(115200);
  pinMode(ETH_CS,OUTPUT);digitalWrite(ETH_CS,HIGH);
  pinMode(SD_CS,OUTPUT);digitalWrite(SD_CS,HIGH);
  delay(350);
  Serial.println(F("KON W5100 TEST-07 / RJ45 CABLE RECOVERY / SD OFF"));
  Ethernet.init(ETH_CS);
  Serial.println(F("DHCP request..."));
  if(Ethernet.begin(mac,10000,2000)==0){
    Serial.println(F("DHCP FAIL / TEST NOT STARTED"));return;
  }
  if(!udp.begin(PORT)){
    Serial.println(F("UDP BIND FAIL / TEST NOT STARTED"));return;
  }
  active=true;
  Serial.print(F("Controller: "));
  Serial.println(Ethernet.hardwareStatus()==EthernetW5100?F("W5100"):F("OTHER"));
  Serial.print(F("IP: "));Serial.println(Ethernet.localIP());
  Serial.print(F("UDP_PORT: "));Serial.println(PORT);
  Serial.println(F("READY / TEST PENDING"));
  statusLine();
}
void loop(){
  if(!active)return;
  if(millis()-lastDhcp>=10000UL){
    lastDhcp=millis();
    int r=Ethernet.maintain();
    if(r==1||r==3){dhcpErrors++;Serial.println(F("DHCP MAINTENANCE ERROR"));}
    if(r==2||r==4){Serial.print(F("DHCP LEASE UPDATED ip="));Serial.println(Ethernet.localIP());}
  }
  if(millis()-lastStatus>=10000UL){lastStatus=millis();statusLine();}
  int length=udp.parsePacket();
  if(length<=0)return;
  rx++;
  if(length>32){
    errors++;
    while(udp.available())udp.read();
    Serial.println(F("DROP OVERSIZE"));return;
  }
  int n=udp.read(data,sizeof(data));
  if(n!=length){errors++;Serial.println(F("DROP SHORT READ"));return;}
  IPAddress peer=udp.remoteIP();
  uint16_t port=udp.remotePort();
  if(udp.beginPacket(peer,port)!=1){errors++;return;}
  size_t wrote=udp.write(data,(size_t)n);
  int ok=udp.endPacket();
  if(wrote!=(size_t)n||ok!=1){errors++;return;}
  tx++;
  if(tx<=5||tx%10==0){Serial.print(F("UDP ECHO #"));Serial.println(tx);}
}
