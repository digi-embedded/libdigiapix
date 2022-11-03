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
from digi.apix.network import IPMode
from digi.apix.wifi import WifiInterface, WifiProfile, SecurityMode

CONFIG_IFACE = "wlan0"
SSID = "SSID"
PASSWORD = "password"


def print_iface(iface_name):
    """
    Prints the information of the given Wi-Fi interface.

    Args:
        iface_name (String): Interface to print its information.

    Return:
        Boolean: `True` on success, `False` otherwise.
    """
    try:
        iface = WifiInterface.get(iface_name)
    except DigiAPIXException as exc:
        print(f"[ERROR] Unable to get interface '{iface_name}': {str(exc)}")
        return False

    print(f"  '{iface.name}' details")
    print("  ------------------------------")
    print(f"    - MAC: {iface.mac}")
    print(f"    - Status: {iface.status.description}")
    print(f"    - SSID: {iface.ssid}")
    print(f"    - Security: {iface.sec_mode.description}")
    print(f"    - Frequency: {iface.frequency}")
    print(f"    - Channel: {iface.channel}")
    print(f"    - IP mode: {iface.ip_mode.description}")
    print(f"    - IP address: {iface.ipv4}")
    print(f"    - Subnet mask: {iface.netmask}")
    print(f"    - Default gateway: {iface.gateway}")
    print(f"    - Broadcast: {iface.broadcast}")
    print(f"    - DNS 1: {iface.dns1}")
    print(f"    - DNS 2: {iface.dns2}")
    print(f"    - MTU: {iface.mtu}")
    print("    - Statistics:")
    stats = iface.get_statistics()
    print(f"      * rx_packets: {stats.rx_packets}")
    print(f"      * tx_packets: {stats.tx_packets}")
    print(f"      * rx_bytes: {stats.rx_bytes}")
    print(f"      * tx_bytes: {stats.tx_bytes}")
    print(f"      * rx_errors: {stats.rx_errors}")
    print(f"      * tx_errors: {stats.tx_errors}")
    print(f"      * rx_dropped_packets: {stats.rx_dropped_packets}")
    print(f"      * tx_dropped_packets: {stats.tx_dropped_packets}")
    print(f"      * rx_multicast_packets: {stats.rx_multicast_packets}")
    print(f"      * tx_collisions: {stats.tx_collisions}")
    print(f"      * rx_length_errors: {stats.rx_length_errors}")
    print(f"      * rx_over_errors: {stats.rx_over_errors}")
    print(f"      * rx_crc_errors: {stats.rx_crc_errors}")
    print(f"      * rx_frame_errors: {stats.rx_frame_errors}")
    print(f"      * rx_fifo_errors: {stats.rx_fifo_errors}")
    print(f"      * rx_missed_errors: {stats.rx_missed_errors}")
    print(f"      * tx_aborted_errors: {stats.tx_aborted_errors}")
    print(f"      * tx_carrier_errors: {stats.tx_carrier_errors}")
    print(f"      * tx_fifo_errors: {stats.tx_fifo_errors}")
    print(f"      * tx_heartbeat_errors: {stats.tx_heartbeat_errors}")
    print(f"      * tx_window_errors: {stats.tx_window_errors}")
    print(f"      * rx_compressed_packets: {stats.rx_compressed_packets}")
    print(f"      * tx_compressed_packets: {stats.tx_compressed_packets}")
    print(f"      * rx_nohandler_packets: {stats.rx_nohandler_packets}")
    print()

    return True


def configure_iface(iface_name):
    """
    Configures the given Wi-Fi interface to connect in DHCP mode.

    Args:
        iface_name (String): Name of the interface to configure.

    Return:
        Boolean: `True` on success, `False` otherwise.
    """
    print(f"\nConfigure interface '{iface_name}' (DHCP)... ", end='')

    try:
        iface = WifiInterface.get(iface_name)
    except DigiAPIXException as exc:
        print(f"[ERROR] Unable to get interface '{iface_name}': {str(exc)}")
        return False

    profile = WifiProfile()
    profile.connect = True
    profile.ssid = SSID
    profile.sec_mode = SecurityMode.WPA2
    profile.psk = PASSWORD
    profile.mode = IPMode.DHCP

    try:
        iface.configure(profile)
        print("OK\n")
    except DigiAPIXException as exc:
        print(f"[ERROR] '{iface.name}': {str(exc)}\n")
        return False

    return True


def main():
    """
    Main execution function.
    """
    print(" +------------------------+")
    print(" | Digi APIX Wi-Fi Sample |")
    print(" +------------------------+\n")

    print("Available Wi-Fi interfaces:")
    try:
        iface_list = WifiInterface.list_interfaces()
    except DigiAPIXException as exc:
        print(f"[ERROR] {str(exc)}")
        sys.exit(1)

    if not iface_list:
        print("[ERROR] No Wi-Fi interfaces available")
        sys.exit(1)
    print('\n'.join([f"    {i + 1}. {val}" for i, val in (enumerate(iface_list))]))
    print()

    for iface in iface_list:
        if not print_iface(iface):
            sys.exit(1)

    if CONFIG_IFACE not in iface_list:
        print(f"[ERROR] '{CONFIG_IFACE}' is not available")
        sys.exit(1)

    sys.exit(0 if configure_iface(CONFIG_IFACE) else 1)


if __name__ == "__main__":
    main()
