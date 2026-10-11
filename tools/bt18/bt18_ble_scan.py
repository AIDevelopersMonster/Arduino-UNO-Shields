#!/usr/bin/env python3
"""Discover BLE advertisements and optionally list a device's GATT layout.

No GATT characteristics are written. Classic Bluetooth SPP is NOT scanned.
"""
from __future__ import annotations

import argparse
import asyncio
import sys

try:
    from bleak import BleakClient, BleakScanner
except ImportError:
    print("Missing bleak. Install: py -m pip install -r tools/bt18/requirements.txt", file=sys.stderr)
    sys.exit(2)


async def run(args: argparse.Namespace) -> int:
    print(f"BLE scan: {args.duration:g}s (NOT Classic SPP)")
    discovered = await BleakScanner.discover(timeout=args.duration, return_adv=True)
    matches = 0
    for device, adv in sorted(discovered.values(), key=lambda item: item[0].address):
        name = adv.local_name or device.name or "(no name)"
        if args.name and args.name.casefold() not in name.casefold():
            continue
        matches += 1
        print(f"\nName: {name}\nBLE address/OS identifier: {device.address}\nRSSI: {adv.rssi} dBm")
        print("Advertised service UUIDs:", ", ".join(adv.service_uuids) or "(none)")
        if adv.manufacturer_data:
            print("Manufacturer data IDs:",
                  ", ".join(f"0x{code:04X}" for code in sorted(adv.manufacturer_data)))
    print(f"\nDevices matching filter: {matches}")
    print("Name, UUIDs and Bluetooth addresses do not prove original DX-BT18 hardware.")

    if args.address:
        print(f"\nConnecting to BLE device {args.address!r} for GATT service enumeration...")
        async with BleakClient(args.address, timeout=15.0) as client:
            print("Connected:", client.is_connected)
            for service in client.services:
                print(f"SERVICE {service.uuid}  {service.description}")
                for characteristic in service.characteristics:
                    print("  CHAR", characteristic.uuid, "properties:", ",".join(characteristic.properties))
                    for descriptor in characteristic.descriptors:
                        print("    DESC", descriptor.uuid)
        print("Disconnected. No GATT characteristic value was read or written.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--duration", type=float, default=10.0, help="scan duration, seconds")
    parser.add_argument("--name", help="optional name substring filter, e.g. BT18")
    parser.add_argument("--address", help="optional BLE identifier for GATT enumeration (may connect)")
    args = parser.parse_args()
    if not 1.0 <= args.duration <= 120:
        parser.error("--duration must be 1..120 seconds")
    try:
        return asyncio.run(run(args))
    except (OSError, RuntimeError, TimeoutError, ValueError) as error:
        print(f"Bluetooth error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
