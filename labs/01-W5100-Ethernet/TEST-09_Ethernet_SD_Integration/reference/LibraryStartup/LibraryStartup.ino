/*
 * Reference startup using the public SD and Ethernet APIs only.
 * UNO + W5100: D10 Ethernet CS, D4 SD CS, UART 115200.
 * This is a startup check, not the TEST-09 SD/UDP integration test.
 */
#include <SPI.h>
#include <SD.h>
#include <Ethernet.h>

const uint8_t ETHERNET_CS = 10;
const uint8_t SD_CS = 4;
const unsigned long DHCP_TIMEOUT_MS = 6000;
const unsigned long DHCP_RESPONSE_TIMEOUT_MS = 1000;
// Locally administered address for this one reference board.
byte mac[] = {0x02, 0x4C, 0x49, 0x42, 0x09, 0x01};

bool finished = false;
unsigned long lastHeartbeat = 0;

int freeRam() {
  extern int __heap_start;
  extern int *__brkval;
  int stack;
  return reinterpret_cast<char *>(&stack) -
         (__brkval ? reinterpret_cast<char *>(__brkval)
                   : reinterpret_cast<char *>(&__heap_start));
}

void result(bool pass, const __FlashStringHelper *stage) {
  Serial.print(F("RESULT test=LIBRARY_STARTUP status="));
  Serial.print(pass ? F("PASS") : F("FAIL"));
  Serial.print(F(" stage="));
  Serial.print(stage);
  Serial.print(F(" free="));
  Serial.println(freeRam());
  finished = true;
}

void setup() {
  // Set the output latches before switching to OUTPUT; keep both CS inactive.
  digitalWrite(ETHERNET_CS, HIGH);
  pinMode(ETHERNET_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  pinMode(SD_CS, OUTPUT);

  Serial.begin(115200);
  delay(300);  // UART/reset settling, not an SPI handoff delay.
  Serial.println();
  Serial.println(F("BOOT test=LIBRARY_STARTUP fw=0.1 eth_cs=10 sd_cs=4 uart=115200"));
  Serial.print(F("START free="));
  Serial.println(freeRam());

  Serial.println(F("STEP name=SD_BEGIN"));
  Serial.flush();
  unsigned long started = millis();
  bool sdOK = SD.begin(SD_CS);
  Serial.print(F("CHECK name=SD_BEGIN status="));
  Serial.print(sdOK ? F("PASS") : F("FAIL"));
  Serial.print(F(" elapsed_ms="));
  Serial.println(millis() - started);
  if (!sdOK) {
    result(false, F("SD_BEGIN"));
    return;
  }

  // Ethernet.init selects CS; Ethernet.begin initializes the driver and DHCP.
  // Do not open/close sockets or read W5100 registers before this call.
  Ethernet.init(ETHERNET_CS);
  Serial.println(F("STEP name=ETHERNET_BEGIN timeout_ms=6000 response_timeout_ms=1000"));
  Serial.flush();
  started = millis();
  int dhcp = Ethernet.begin(mac, DHCP_TIMEOUT_MS, DHCP_RESPONSE_TIMEOUT_MS);
  unsigned long elapsed = millis() - started;

  // hardwareStatus() uses the chip type cached by the library initialization.
  EthernetHardwareStatus hardware = Ethernet.hardwareStatus();
  Serial.print(F("CHECK name=HARDWARE chip="));
  switch (hardware) {
    case EthernetW5100: Serial.print(F("W5100")); break;
    case EthernetW5200: Serial.print(F("W5200")); break;
    case EthernetW5500: Serial.print(F("W5500")); break;
    default: Serial.print(F("NONE")); break;
  }
  Serial.print(F(" status="));
  Serial.println(hardware == EthernetW5100 ? F("PASS") : F("FAIL"));
  Serial.print(F("CHECK name=DHCP rc="));
  Serial.print(dhcp);
  Serial.print(F(" status="));
  if (hardware == EthernetNoHardware) {
    // begin returned before DHCP; rc=0 is not evidence of a DHCP server fault.
    Serial.print(F("NOT_ATTEMPTED"));
  } else {
    Serial.print(dhcp == 1 ? F("PASS") : F("FAIL"));
  }
  Serial.print(F(" begin_elapsed_ms="));
  Serial.println(elapsed);

  if (hardware != EthernetW5100) {
    result(false, F("HARDWARE"));
    return;
  }
  if (dhcp != 1) {
    result(false, F("DHCP"));
    return;
  }

  IPAddress ip = Ethernet.localIP();
  bool ipOK = ip != IPAddress(0, 0, 0, 0);
  Serial.print(F("CHECK name=IP ip="));
  Serial.print(ip);
  Serial.print(F(" status="));
  Serial.println(ipOK ? F("PASS") : F("FAIL"));
  result(ipOK, ipOK ? F("COMPLETE") : F("IP"));
}

void loop() {
  if (millis() - lastHeartbeat >= 5000) {
    lastHeartbeat = millis();
    Serial.print(F("ALIVE ms="));
    Serial.print(millis());
    Serial.print(F(" finished="));
    Serial.print(finished ? 1 : 0);
    Serial.print(F(" free="));
    Serial.println(freeRam());
  }
}
