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
import time

from digi.apix.common import RequestMode
from digi.apix.exceptions import GPIOException, DigiAPIXException
from digi.apix.gpio import GPIOMode, GPIOValue, GPIO


def interrupt_callback():
    """
    Callback to be notified when an interrupt occurs in the registered GPIO.
    """
    print("  - Interrupt occurred!")


def main():
    """
    Main execution function.
    """
    print(" +-----------------------+")
    print(" | Digi APIX GPIO Sample |")
    print(" +-----------------------+\n")

    # Create 'USER_LED' GPIO.
    try:
        user_led = GPIO.create_from_alias("USER_LED",
                                          GPIOMode.OUTPUT_LOW,
                                          RequestMode.SHARED)
    except DigiAPIXException as exc:
        print(f"{str(exc)}")
        sys.exit(1)
    print("'USER_LED' GPIO created.")

    # Blink LED.
    print("'Blinking 'USER_LED'...")
    for _ in range(0, 5):
        try:
            time.sleep(1)
            user_led.set_value(GPIOValue.HIGH)
            time.sleep(1)
            user_led.set_value(GPIOValue.LOW)
        except GPIOException as exc:
            print(f"{str(exc)}")
            sys.exit(1)

    # Create 'USER_BUTTON' GPIO.
    try:
        user_button = GPIO.create_from_alias("USER_BUTTON",
                                             GPIOMode.IRQ_EDGE_FALLING,
                                             RequestMode.SHARED)
    except DigiAPIXException as exc:
        print(f"{str(exc)}")
        sys.exit(1)
    print("'USER_BUTTON' GPIO created.")

    # Wait for interrupt synchronously.
    try:
        print("Waiting for interrupt on 'USER_BUTTON' synchronously...")
        user_button.wait_for_interrupt(-1)
        print("  - Interrupt captured!")
    except GPIOException as exc:
        print(f"{str(exc)}")

    # Wait for interrupts asynchronously.
    try:
        user_button.register_interrupt_callback(interrupt_callback)
        print("Waiting for interrupts on 'USER_BUTTON' asynchronously... (Press <ENTER> to exit)")
        input('> ')
        print('Stopping...')
    except KeyboardInterrupt:
        sys.exit(1)
    except GPIOException as exc:
        print(f"{str(exc)}")
        sys.exit(1)
    user_button.remove_interrupt_callback(interrupt_callback)


if __name__ == "__main__":
    main()
