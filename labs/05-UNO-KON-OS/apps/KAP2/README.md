# KonSol KAP2 applications

KAP2 adds small persistent state and control flow to the external application
model while keeping applications on microSD.

## COUNTER.KAP

First KAP2 interactive program.

It counts five touches and displays:

- number of touches;
- last Touch X;
- last Touch Y.

After the fifth touch it displays `DONE`, waits for one final touch, and exits
back to resident KonSol.

This is intended as the first proof that an external KonSol program can make
its own repeated state transition and branch decision rather than merely
executing a fixed linear command sequence.

See:

- [KAP2 specification](../../docs/KAP2_SPEC.md)
- [TEST-06](../../tests/TEST-06-KAP2-INTERACTIVE.md)
