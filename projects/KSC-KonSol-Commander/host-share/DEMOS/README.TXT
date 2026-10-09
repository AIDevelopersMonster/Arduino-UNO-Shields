KSC-04B / KSC-04C DEMO PACK

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
SW1_0.KSC     load resident SW1 profile: press -> RED
SW1_1.KSC     load resident SW1 profile: press -> BLUE
RESET.KSC     all writable outputs OFF

KSC-04C adds one small RAM-resident configuration node:

/sys/sw1

0 = the next stable SW1 press selects RED and clears BLUE
1 = the next stable SW1 press selects BLUE and clears RED

SW1_0.KSC and SW1_1.KSC therefore change the later behavior of the same
physical SW1 button without firmware recompilation, upload, or reset.

The profile is RAM-resident and is lost on MCU reset. This is one fixed
certified profile slot, not a general event-binding language.

The host-side demo files consume no additional Arduino Flash/SRAM merely by
being added or changed. New interpreter semantics would require firmware growth.

Final KSC-04C representative status:

RUN OK
RAM 380 B   IN drop 0   IR drop 0/0   HOST M/0 R0

Stage preprint:
https://doi.org/10.5281/zenodo.23251546

KSC-04 demonstration video:
https://youtu.be/pqV5DG1o-WA
