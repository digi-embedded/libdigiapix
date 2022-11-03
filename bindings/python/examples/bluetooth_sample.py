# Copyright 2022, Digi International Inc.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

import sys

from digi.apix.exceptions import DigiAPIXException
from digi.apix.bluetooth import BluetoothDevice, BluetoothProfile

CONFIG_DEVICE = "hci0"
ADVERT_NAME = "BLUETOOTH_SAMPLE"


def print_device(device):
    """
    Prints the information of the given Bluetooth device.

    Args:
        device (String): Bluetooth device name to print its information.

    Return:
        Boolean: `True` on success, `False` otherwise.
    """
    try:
        bt_device = BluetoothDevice.get(device)
    except DigiAPIXException as exc:
        print(f"[ERROR] Unable to get device '{device}': {str(exc)}")
        return False

    print(f"'{device}' details")
    print("------------------------------")
    print(f"  - Device ID: {bt_device.device_id}")
    print(f"  - Device name: {bt_device.device_name}")
    print(f"  - Advertising name: {bt_device.advert_name}")
    print(f"  - MAC: {bt_device.mac}")
    print(f"  - Is enabled?: {bt_device.is_enabled}")
    print(f"  - Is running?: {bt_device.is_running}")
    print("  - Statistics:")
    stats = bt_device.get_statistics()
    print(f"    * rx_bytes: {stats.rx_bytes}")
    print(f"    * tx_bytes: {stats.tx_bytes}")
    print(f"    * rx_errors: {stats.rx_errors}")
    print(f"    * tx_errors: {stats.tx_errors}")
    print(f"    * rx_acl: {stats.rx_acl}")
    print(f"    * tx_acl: {stats.tx_acl}")
    print(f"    * rx_sco: {stats.rx_sco}")
    print(f"    * tx_sco: {stats.tx_sco}")
    print(f"    * rx_events: {stats.rx_events}")
    print(f"    * tx_cmds: {stats.tx_commands}")
    print("")

    return True


def configure_device(device):
    """
    Configures the given Bluetooth device to be enabled.

    Args:
        device (String): Bluetooth device to configure.

    Return:
        Boolean: `True` on success, `False` otherwise.
    """
    print(f"\nConfigure device '{device}' (Enable)... ", end='')

    try:
        bt_device = BluetoothDevice.get(device)
    except DigiAPIXException as exc:
        print(f"[ERROR] Unable to get Bluetooth device '{device}': {str(exc)}")
        return False

    profile = BluetoothProfile()
    profile.enable = True
    profile.advert_name = ADVERT_NAME

    try:
        bt_device.configure(profile)
        print("OK\n")
    except DigiAPIXException as exc:
        print(f"[ERROR] Unable to configure Bluetooth device '{device}': "
              f"{str(exc)}\n")
        return False

    return True


def main():
    """
    Main execution function.
    """
    print(" +----------------------------+")
    print(" | Digi APIX Bluetooth Sample |")
    print(" +----------------------------+\n")

    print("Available Bluetooth devices:")
    try:
        devices_list = BluetoothDevice.list_devices()
    except DigiAPIXException as exc:
        print(f"[ERROR] {str(exc)}")
        sys.exit(1)

    if not devices_list:
        print("[ERROR] No Bluetooth devices available")
        sys.exit(1)
    print('\n'.join([f"    {i}. {val}" for i, val in (enumerate(devices_list))]))
    print()

    for device in devices_list:
        if not print_device(device):
            sys.exit(1)

    if CONFIG_DEVICE not in devices_list:
        print(f"[ERROR] '{CONFIG_DEVICE}' is not available")
        sys.exit(1)

    sys.exit(0 if configure_device(CONFIG_DEVICE) else 1)


if __name__ == "__main__":
    main()
