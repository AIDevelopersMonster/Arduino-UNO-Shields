KSC-04B DEMO PACK

All files use only the physically certified KSC Script v0.1 instructions:
PRINT, WRITE, WAIT, STOP.

DEMO.KSC      basic red then blue launcher proof (in /host root)
BEEP.KSC      active-buzzer rhythm
SOS.KSC       SOS timing pattern on the active buzzer
RGB.KSC       RGB color sequence
TRAFFIC.KSC   RGB traffic-light sequence
ALARM.KSC     LED + RGB + buzzer alarm
POLICE.KSC    red/blue alarm show
SHOW.KSC      combined LED + RGB + buzzer show
SW1_0.KSC     static output profile: red LED ON
SW1_1.KSC     static output profile: blue LED ON
RESET.KSC     all writable outputs OFF

IMPORTANT:
SW1_0.KSC and SW1_1.KSC are named to demonstrate two alternative loaded
profiles using the same firmware. KSC Script v0.1 does NOT yet provide
resident SW1 event binding, so these two files directly set outputs when RUN.

The demo pack intentionally changes no firmware logic and consumes no
additional Arduino Flash or SRAM.
