/*
  HY-M302 IR receiver reference test

  Purpose:
    Identify the real protocol produced by a remote control using the
    established Arduino-IRremote library, independently of the HY_M302
    NEC-only decoder.

  Hardware:
    Arduino UNO + HY-M302
    onboard IR receiver output = D6

  Required library:
    IRremote by Armin Joachimsmeyer

  Serial:
    115200 baud
*/

#include <IRremote.hpp>

static const uint8_t IR_RECEIVE_PIN = 6;

void setup() {
  Serial.begin(115200);

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  Serial.println(F("HY-M302 IRREMOTE REFERENCE TEST"));
  Serial.println(F("IR receiver = D6"));
  Serial.println(F("Press remote buttons one at a time."));
  Serial.println();
}

void loop() {
  if (!IrReceiver.decode()) {
    return;
  }

  Serial.println(F("--- IR FRAME ---"));

  Serial.print(F("Protocol: "));
  Serial.println(getProtocolString(IrReceiver.decodedIRData.protocol));

  Serial.print(F("Address: 0x"));
  Serial.println(IrReceiver.decodedIRData.address, HEX);

  Serial.print(F("Command: 0x"));
  Serial.println(IrReceiver.decodedIRData.command, HEX);

  Serial.print(F("Raw-Data: 0x"));
  Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);

  Serial.print(F("Flags: 0x"));
  Serial.println(IrReceiver.decodedIRData.flags, HEX);

  // Library-native compact representation, useful if protocol is unknown
  // or carries extra information.
  IrReceiver.printIRResultShort(&Serial);

  Serial.println();

  IrReceiver.resume();
}
