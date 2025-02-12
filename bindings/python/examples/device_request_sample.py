# Copyright 2025, Digi International Inc.
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

from digi.apix.device_request import register, unregister, DeviceRequestException

DEVICE_REQUEST_TARGET = "my_target"
DEVICE_REQUEST_RESPONSE = "OK"
DEVICE_REQUEST_STATUS_SUCCESS = 0


def device_request_handler(target, request):
    """
    Callback to be notified when a device request for the registered target
    is received.
    """
    print(f"\n- Received request '{request}' for target '{target}'")
    print(f"- Sending response '{DEVICE_REQUEST_RESPONSE}'...")
    return (DEVICE_REQUEST_RESPONSE, DEVICE_REQUEST_STATUS_SUCCESS)

def device_request_status_handler(status, message):
    """
    Callback to be notified when a device request for the registered target
    has finished.
    """
    print(f"- Device request finished! Status: {status} - {message}")

def main():
    """
    Main execution function.
    """
    print(" +---------------------------------+")
    print(" | Digi APIX Device Request Sample |")
    print(" +---------------------------------+\n")

    print(f"- Registering device request for target '{DEVICE_REQUEST_TARGET}'...")
    try:
        register(DEVICE_REQUEST_TARGET, device_request_handler,
                 status_callback=device_request_status_handler)
    except DeviceRequestException as exc:
        print(f"- Error registering device request: {exc}")
        sys.exit(1)

    print("- Waiting for request, press <ENTER> to exit...")
    input()

    print(f"- Unregistering device request for target '{DEVICE_REQUEST_TARGET}'...")
    try:
        unregister(DEVICE_REQUEST_TARGET)
    except DeviceRequestException as exc:
        print(f"- Error unregistering device request: {exc}")
        sys.exit(1)

if __name__ == "__main__":
    main()