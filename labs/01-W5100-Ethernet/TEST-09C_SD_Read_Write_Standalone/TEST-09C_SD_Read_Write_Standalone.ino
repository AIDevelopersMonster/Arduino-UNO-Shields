/*
 * TEST-09C: standalone microSD write/read on Arduino UNO + W5100 Shield
 * Ethernet library is NOT used. RJ45 connection is irrelevant.
 * SPI CS: W5100 D10 HIGH (disabled); SD D4.
 * CAUTION: K09C.TXT is overwritten on every boot.
 */
#include <SPI.h>
#include <SD.h>
const byte ETH_CS=10;
const byte SD_CS=4;
const char FILE_NAME[]="K09C.TXT";
const char TEST_LINE[]="KON SD TEST 09C OK";
void setup() {
  Serial.begin(115200);
  pinMode(ETH_CS,OUTPUT);
  digitalWrite(ETH_CS,HIGH);
  pinMode(SD_CS,OUTPUT);
  digitalWrite(SD_CS,HIGH);
  delay(400);
  Serial.println(F("TEST-09C SD WRITE/READ / NO ETHERNET"));
  if (!SD.begin(SD_CS)) {
    Serial.println(F("SD INIT FAIL"));
    return;
  }
  Serial.println(F("SD INIT PASS"));
  if (SD.exists(FILE_NAME) && !SD.remove(FILE_NAME)) {
    Serial.println(F("SD REMOVE FAIL"));
    return;
  }
  File f=SD.open(FILE_NAME,FILE_WRITE);
  if (!f) {
    Serial.println(F("SD OPEN WRITE FAIL"));
    return;
  }
  size_t n=f.println(TEST_LINE);
  f.close();
  if (n!=sizeof(TEST_LINE)-1+2) {
    Serial.println(F("SD WRITE FAIL"));
    return;
  }
  Serial.println(F("SD WRITE PASS"));
  f=SD.open(FILE_NAME,FILE_READ);
  if (!f) {
    Serial.println(F("SD OPEN READ FAIL"));
    return;
  }
  bool ok=true;
  for (size_t i=0;i<sizeof(TEST_LINE)-1;i++) {
    if(f.read()!=TEST_LINE[i])ok=false;
  }
  if (f.read()!=13 || f.read()!=10 || f.available()!=0)ok=false;
  f.close();
  Serial.println(ok?F("SD READ VERIFY PASS"):F("SD READ VERIFY FAIL"));
  if(ok) Serial.println(F("TEST-09C FULL PASS"));
}
void loop() {}
