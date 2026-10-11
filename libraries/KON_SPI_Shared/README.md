# KON_SPI_Shared

**Status: EXPERIMENTAL / ARCHIVED (2026-10-11).**

This small AVR helper library was created only during the bounded investigation of shared SPI behavior between a W5100 Ethernet controller and the onboard microSD interface on tested clone shields.

It provides explicit CS parking, idle clocks, SD handoff helpers and a raw W5100 software-reset probe. These mechanisms were useful diagnostically, but they did **not** establish reliable combined W5100 + onboard microSD operation on the tested hardware.

The final stock-library acceptance test failed and the SD/shared-SPI path was closed. Therefore:

- this library is preserved for reproducibility of TEST-10/11 diagnostics;
- it is not a recommended production abstraction;
- it is not an active library line;
- no further compatibility work is planned for the legacy W5100 clone hardware.

For ordinary Ethernet-only work, use the standard Arduino Ethernet library and leave the microSD physically absent or deselected as appropriate.
