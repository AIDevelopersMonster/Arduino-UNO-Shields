/*
 * TEST-10: KON_SPI_Shared transition test.
 * Goal: prove or disprove controlled SD <-> W5100 switching without AVR reset.
 * Requires local library: libraries/KON_SPI_Shared
 */
#include <SPI.h>
#include <SD.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <KON_SPI_Shared.h>

const byte SD_CS=4, ETH_CS=10;
const uint16_t UDP_PORT=5004;
const char FILE_NAME[]="K10.TXT";
const char PAYLOAD[]="KON SPI TEST10";
byte mac[]={0x02,0x4B,0x4F,0x4E,0x5A,0x10};

KON_SPI_Shared bus(SD_CS,ETH_CS);
EthernetUDP udp;
char cmd[8];
byte used=0;
bool sdOn=false, lanOn=false;

void showHelp(){
  Serial.println(F("COMMANDS: ? / S1 / S0 / L1 / L0 / R"));
  Serial.println(F("S1: prepare SD, SD.begin, write/read verify"));
  Serial.println(F("S0: SD.end + release clocks"));
  Serial.println(F("L1: prepare LAN, optional W5100 reset, DHCP, UDP 5004"));
  Serial.println(F("L0: UDP stop + LAN release"));
  Serial.println(F("R : raw W5100 soft reset only"));
}
void status(){
  Serial.print(F("STATE sd="));Serial.print(sdOn?F("ON"):F("OFF"));
  Serial.print(F(" lan="));Serial.print(lanOn?F("ON"):F("OFF"));
  Serial.print(F(" owner="));Serial.println((int)bus.owner());
}
bool sdVerify(){
  if(SD.exists(FILE_NAME))SD.remove(FILE_NAME);
  File f=SD.open(FILE_NAME,FILE_WRITE);
  if(!f)return false;
  size_t n=f.println(PAYLOAD);
  f.close();
  if(n!=sizeof(PAYLOAD)-1+2)return false;
  f=SD.open(FILE_NAME,FILE_READ);
  if(!f)return false;
  bool ok=true;
  for(size_t i=0;i<sizeof(PAYLOAD)-1;i++)if(f.read()!=PAYLOAD[i])ok=false;
  if(f.read()!=13||f.read()!=10||f.available()!=0)ok=false;
  f.close();
  return ok;
}
void doS1(){
  if(lanOn){Serial.println(F("BLOCK L0 FIRST"));return;}
  bus.prepareSD();
  Serial.println(F("S1 prepareSD: OK"));
  if(!SD.begin(SD_CS)){Serial.println(F("S1 SD.begin: FAIL"));bus.releaseSD(2);return;}
  Serial.println(F("S1 SD.begin: PASS"));
  if(!sdVerify()){Serial.println(F("S1 verify: FAIL"));return;}
  sdOn=true;
  Serial.println(F("S1 write/read verify: PASS"));
}
void doS0(){
  if(!sdOn){Serial.println(F("S0 SD already OFF"));bus.releaseSD(2);return;}
  SD.end();
  bus.releaseSD(2);
  sdOn=false;
  Serial.println(F("S0 SD.end + 16 release clocks: DONE"));
}
void doL1(){
  if(sdOn){Serial.println(F("BLOCK S0 FIRST"));return;}
  bus.prepareLAN();
  Serial.println(F("L1 prepareLAN: OK"));
  bool rst=bus.resetW5100(true);
  Serial.print(F("L1 W5100 soft reset: "));Serial.println(rst?F("PASS"):F("FAIL"));
  bus.prepareLAN();
  Ethernet.init(ETH_CS);
  if(Ethernet.begin(mac,10000,2000)==0){Serial.println(F("L1 DHCP: FAIL"));bus.releaseLAN();return;}
  if(!udp.begin(UDP_PORT)){Serial.println(F("L1 UDP begin: FAIL"));bus.releaseLAN();return;}
  lanOn=true;
  Serial.print(F("L1 LAN PASS IP="));Serial.println(Ethernet.localIP());
  Serial.print(F("UDP="));Serial.println(UDP_PORT);
}
void doL0(){
  if(lanOn)udp.stop();
  bus.releaseLAN();
  lanOn=false;
  Serial.println(F("L0 UDP stop + LAN release: DONE"));
}
void doR(){
  if(sdOn||lanOn){Serial.println(F("BLOCK: S0/L0 FIRST"));return;}
  Serial.print(F("R W5100 soft reset: "));
  Serial.println(bus.resetW5100(true)?F("PASS"):F("FAIL"));
}
void command(){
  cmd[used]=0;used=0;
  if(!strcmp(cmd,"?"))status();
  else if(!strcmp(cmd,"S1"))doS1();
  else if(!strcmp(cmd,"S0"))doS0();
  else if(!strcmp(cmd,"L1"))doL1();
  else if(!strcmp(cmd,"L0"))doL0();
  else if(!strcmp(cmd,"R"))doR();
  else showHelp();
}
void setup(){
  Serial.begin(115200);
  bus.begin();
  Serial.println(F("KON TEST-10 / SHARED SPI RECOVERY"));
  status();
  showHelp();
}
void loop(){
  while(Serial.available()){
    char c=Serial.read();
    if(c=='\r'||c=='\n'){if(used)command();}
    else if(used<sizeof(cmd)-1&&c>=32&&c<=126)cmd[used++]=c;
  }
  if(lanOn){
    int n=udp.parsePacket();
    if(n>0){
      IPAddress ip=udp.remoteIP();uint16_t p=udp.remotePort();
      byte b[32];int r=udp.read(b,sizeof(b));
      if(r>0&&udp.beginPacket(ip,p)==1){udp.write(b,r);udp.endPacket();}
    }
  }
}
