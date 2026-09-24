/*
  LAB-01 / 01_UNO_Smoke
  Arduino UNO Clone + W5100 + SD 4 GB

  Purpose:
  Verify upload, CPU execution, built-in LED and Serial Monitor
  before testing the shield.
*/

const unsigned long BAUD = 115200;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(BAUD);
  delay(500);

  Serial.println(F(""));
  Serial.println(F("LAB-01 / 01_UNO_Smoke"));
  Serial.println(F("Arduino UNO execution: PASS"));
  Serial.println(F("Built-in LED should toggle every second."));
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
