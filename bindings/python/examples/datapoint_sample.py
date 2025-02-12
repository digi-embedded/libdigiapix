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
import time

from digi.apix.datapoint import upload, upload_multiple, DataPoint, DataType, DataPointException

STREAM_ID_1 = "sensor/temperature"
STREAM_ID_2 = "sensor/humidity"
STREAM_ID_3 = "sensor/pressure"

DATA_VALUE_1 = 22.5
DATA_VALUE_2 = 55
DATA_VALUE_3 = 1013

DATA_UNITS_1 = "C"
DATA_UNITS_2 = "%"
DATA_UNITS_3 = "hPa"


def main():
    """
    Main execution function.
    """
    print(" +-----------------------------------+")
    print(" | Digi APIX DataPoint Upload Sample |")
    print(" +-----------------------------------+\n")

    print(f"- Uploading single data point: Stream='{STREAM_ID_1}', Data='{DATA_VALUE_1}', Units='{DATA_UNITS_1}'...")
    try:
        upload(STREAM_ID_1, DATA_VALUE_1, units=DATA_UNITS_1, data_type=DataType.FLOAT, timestamp=time.time())
        print("- Data point uploaded successfully.")
    except DataPointException as exc:
        print(f"- Error uploading data point: {exc}")
        sys.exit(1)

    print("\n- Uploading multiple data points...")
    datapoints = [
        DataPoint(STREAM_ID_2, DATA_VALUE_2, units=DATA_UNITS_2, data_type=DataType.INT, timestamp=time.time()),
        DataPoint(STREAM_ID_3, DATA_VALUE_3, units=DATA_UNITS_3, data_type=DataType.INT, timestamp=time.time()),
    ]
    try:
        upload_multiple(datapoints)
        print("- Multiple data points uploaded successfully.")
    except DataPointException as exc:
        print(f"- Error uploading multiple data points: {exc}")
        sys.exit(1)

    print("\n- Data upload complete.")

if __name__ == "__main__":
    main()