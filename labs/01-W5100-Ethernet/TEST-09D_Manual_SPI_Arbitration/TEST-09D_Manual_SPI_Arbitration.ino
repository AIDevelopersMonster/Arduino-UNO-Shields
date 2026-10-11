/*
 * LAB-01B / TEST-09D: MANUAL SPI ARBITRATION (FINAL CONTROL EXPERIMENT)
 * UNO + W5100 Shield: W5100 CS D10, SD CS D4, Serial 115200.
 * Startup: D10 HIGH, D4 HIGH; wait for operator command.
 *
 * Commands (Serial Monitor):
 *   H    - help
 *   ?    - status
 *   S1   - initialize SD, overwrite K09D.TXT, verify readback
 *   S0   - close SD operations and deassert SD CS
 *   L1   - initialize W5100 DHCP, open UDP Echo port 5003
 *   L0   - stop UDP and deassert W5100 CS
 *
 * CS HIGH means LOGICALLY DESELECTED, not powered off.
 * No background SD read/write. UDP Echo only in LAN mode.
 * Only send S1 with LAN mode OFF, and L1 with SD mode OFF.
 * All operations and results are announced via COM4.
 *
 * WARNING: K09D.TXT is overwritten (deleted if present) by S1.
 * No other SD files intentionally changed.
 */
#include <SPI.h>
#include <SD.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <string.h>

const byte PIN_LAN=10, PIN_SD=4;
const uint16_t UDP_PORT=5003;
const char NAME[]="K09D.TXT";
const char MESSAGE[]="TEST09D SD READ WRITE VERIFIED";
byte mac[]={0x02,0x4B,0x4F,0x4E,0x59,0x0D};
EthernetUDP udp;
byte udpBuf[32];
char cmd[12];
byte used=0;
bool sdMode=false, lanMode=false;
unsigned int udpCount=0;

void inactive(){
  digitalWrite(PIN_LAN,HIGH);
  digitalWrite(PIN_SD,HIGH);
}
void showState(){
  Serial.print(F("STATE SD="));
  Serial.print(sdMode?F("ON"):F("OFF"));
  Serial.print(F(" LAN="));
  Serial.print(lanMode?F("ON"):F("OFF"));
  Serial.print(F(" D4="));
  Serial.print(digitalRead(PIN_SD)==HIGH?F("HIGH"):F("LOW"));
  Serial.print(F(" D10="));
  Serial.println(digitalRead(PIN_LAN)==HIGH?F("HIGH"):F("LOW"));
}
void help(){
  Serial.println(F("COMMANDS: H / ? / S1 / S0 / L1 / L0"));
  Serial.println(F("1: S1 -> SD INIT, WRITE, READ, VERIFY"));
  Serial.println(F("2: S0 -> SD DESELECT (D4 HIGH)"));
  Serial.println(F("3: L1 -> DHCP, UDP ECHO ON PORT 5003"));
  Serial.println(F("4: L0 -> STOP UDP, LAN DESELECT (D10 HIGH)"));
  Serial.println(F("NOTE: OFF = SPI CS HIGH, NOT POWER OFF"));
}
void sdStart(){
  if(lanMode){Serial.println(F("BLOCK: LAN ON. SEND L0 FIRST"));return;}
  if(sdMode){Serial.println(F("SD ALREADY ON; SEND S0 FIRST"));return;}
  inactive();
  Serial.println(F("S1 STEP 1: LAN CS D10 HIGH / SD INIT..."));
  if(!SD.begin(PIN_SD)){
    digitalWrite(PIN_SD,HIGH);
    Serial.println(F("S1 FAIL: SD.begin(4)"));
    showState();return;
  }
  sdMode=true;
  Serial.println(F("S1 STEP 1: SD CONNECTED PASS"));
  if(SD.exists(NAME)){
    if(!SD.remove(NAME)){
      Serial.println(F("S1 FAIL: REMOVE TEST FILE"));
      return;
    }
  }
  Serial.println(F("S1 STEP 2: OPEN/WRITE K09D.TXT..."));
  File f=SD.open(NAME,FILE_WRITE);
  if(!f){Serial.println(F("S1 FAIL: OPEN WRITE"));return;}
  size_t wrote=f.println(MESSAGE);
  f.close();
  if(wrote != sizeof(MESSAGE)-1+2){
    Serial.println(F("S1 FAIL: WRITE LENGTH"));return;
  }
  Serial.println(F("S1 STEP 2: WRITE PASS / FILE CLOSED"));
  Serial.println(F("S1 STEP 3: REOPEN/READ/VERIFY..."));
  File r=SD.open(NAME,FILE_READ);
  if(!r){Serial.println(F("S1 FAIL: OPEN READ"));return;}
  bool good=true;
  for(size_t i=0;i<sizeof(MESSAGE)-1;i++){
    if(r.read() != (int)(byte)MESSAGE[i])good=false;
  }
  if(r.read()!=13 || r.read()!=10 || r.available()!=0)good=false;
  r.close();
  Serial.println(good?F("S1 STEP 3: READ VERIFY PASS / FILE CLOSED"):F("S1 FAIL: READ VERIFY"));
  if(good)Serial.println(F("S1 COMPLETE: SD PASS. NEXT COMMAND S0"));
  showState();
}
void sdStop(){
  if(lanMode){Serial.println(F("BLOCK: LAN ON"));return;}
  // There are no open File objects: each S1 opens and closes synchronously.
  digitalWrite(PIN_SD,HIGH);
  sdMode=false;
  Serial.println(F("S0: SD DESELECTED / D4 HIGH / NO SD OPERATIONS"));
  Serial.println(F("NEXT COMMAND L1"));
  showState();
}
void lanStart(){
  if(sdMode){Serial.println(F("BLOCK: SD ON. SEND S0 FIRST"));return;}
  if(lanMode){Serial.println(F("LAN ALREADY ON"));return;}
  inactive();
  Serial.println(F("L1 STEP 1: SD CS D4 HIGH / ETHERNET INIT..."));
  Ethernet.init(PIN_LAN);
  Serial.println(F("L1 STEP 2: DHCP REQUEST (UP TO ~10S)..."));
  if(Ethernet.begin(mac,10000,2000)==0){
    digitalWrite(PIN_LAN,HIGH);
    Serial.println(F("L1 FAIL: DHCP / LAN MODE OFF"));
    showState();return;
  }
  if(udp.begin(UDP_PORT)==0){
    digitalWrite(PIN_LAN,HIGH);
    Serial.println(F("L1 FAIL: UDP BEGIN / LAN MODE OFF"));
    showState();return;
  }
  lanMode=true;
  udpCount=0;
  Serial.println(F("L1 PASS: LAN CONNECTED / UDP ECHO READY"));
  Serial.print(F("IP: "));Serial.println(Ethernet.localIP());
  Serial.print(F("UDP PORT: "));Serial.println(UDP_PORT);
  Serial.println(F("SEND UDP DATAGRAM, OR SEND L0 TO STOP"));
  showState();
}
void lanStop(){
  if(lanMode)udp.stop();
  digitalWrite(PIN_LAN,HIGH);
  lanMode=false;
  Serial.println(F("L0: UDP STOPPED / LAN DESELECTED / D10 HIGH"));
  Serial.println(F("NEXT COMMAND S1 OR ?"));
  showState();
}
void onCommand(){
  cmd[used]=0;
  used=0;
  if(strcmp(cmd,"H")==0){help();return;}
  if(strcmp(cmd,"?")==0){showState();return;}
  if(strcmp(cmd,"S1")==0){sdStart();return;}
  if(strcmp(cmd,"S0")==0){sdStop();return;}
  if(strcmp(cmd,"L1")==0){lanStart();return;}
  if(strcmp(cmd,"L0")==0){lanStop();return;}
  if(cmd[0])Serial.println(F("UNKNOWN COMMAND. SEND H"));
}
void setup(){
  Serial.begin(115200);
  pinMode(PIN_LAN,OUTPUT);
  pinMode(PIN_SD,OUTPUT);
  inactive();
  delay(200);
  Serial.println(F("KON TEST-09D MANUAL SPI / WAIT FOR COMMAND"));
  showState();
  help();
}
void loop(){
  while(Serial.available()>0){
    char c=(char)Serial.read();
    if(c=='\r'||c=='\n'){
      if(used)onCommand();
    } else if(used<sizeof(cmd)-1 && c>=32 && c<=126){
      cmd[used++]=c;
    }
  }
  if(!lanMode)return;
  int length=udp.parsePacket();
  if(length<=0)return;
  IPAddress remote=udp.remoteIP();
  uint16_t port=udp.remotePort();
  if(length>(int)sizeof(udpBuf)){
    while(udp.available()>0)udp.read();
    Serial.println(F("UDP DROP: >32 BYTES"));return;
  }
  int n=udp.read(udpBuf,sizeof(udpBuf));
  if(n<=0)return;
  if(udp.beginPacket(remote,port)==1){
    size_t w=udp.write(udpBuf,(size_t)n);
    int ok=udp.endPacket();
    if(ok==1 && w==(size_t)n){
      udpCount++;
      Serial.print(F("UDP ECHO PASS #"));Serial.println(udpCount);
    }else Serial.println(F("UDP ECHO FAIL"));
  }else Serial.println(F("UDP BEGIN PACKET FAIL"));
}
