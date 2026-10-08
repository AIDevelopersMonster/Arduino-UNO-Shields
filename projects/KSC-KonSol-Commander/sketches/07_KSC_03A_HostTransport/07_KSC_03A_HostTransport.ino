#include <Arduino.h>
#include <KSC_Core.h>
#include <KSC_HostTransport.h>
#include <KSC_HYM302Target.h>

KscHyM302Target target;
KscHostMuxStream hostMux(Serial);
KscCore ksc(hostMux, target);

void setup() {
  Serial.begin(115200);
  delay(100);

  hostMux.println();
  hostMux.println(F("KSC-03A Host Transport"));
  hostMux.println(F("TTY + framed HOSTFS on one serial link"));
  hostMux.println(F("HOSTFS framing: ESC ] TYPE SEQ LEN PAYLOAD CRC8"));

  ksc.begin();

  hostMux.print(F("TRANSPORT RX payload: "));
  hostMux.print(KscHostMuxStream::MAX_PAYLOAD);
  hostMux.println(F(" B"));

  hostMux.print(F("TRANSPORT TTY queue: "));
  hostMux.print(KscHostMuxStream::TTY_QUEUE_SIZE);
  hostMux.println(F(" B"));

  hostMux.print(F("FREE RAM AFTER TRANSPORT INIT: "));
  hostMux.print(kscFreeRam());
  hostMux.println(F(" B"));
}

void loop() {
  hostMux.service();
  ksc.service();
}
