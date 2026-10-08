#include <Arduino.h>
#include <KSC_Core.h>
#include <KSC_HostTransport.h>
#include <KSC_HYM302Target.h>
#include <KSC_HostMountTarget.h>

KscHyM302Target physicalTarget;
KscHostMuxStream hostMux(Serial);
KscHostMountTarget mountedTarget(
  physicalTarget,
  hostMux
);
KscCore ksc(
  hostMux,
  mountedTarget
);

void setup() {
  Serial.begin(115200);
  delay(100);

  hostMux.println();
  hostMux.println(F("KSC-04A Commander File Viewer"));
  hostMux.println(F("Remote /host byte-window viewer"));
  hostMux.println(F("Viewer page: 192 B, transport chunk: 32 B"));

  ksc.begin();

  hostMux.print(F("FREE RAM AFTER VIEWER INIT: "));
  hostMux.print(kscFreeRam());
  hostMux.println(F(" B"));
}

void loop() {
  hostMux.service();
  ksc.service();
}
