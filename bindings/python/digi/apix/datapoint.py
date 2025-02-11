"""
APIs for uploading data points from the device

This module enables the uploading of various data from
the device:
    from digi.apix import datapoint
    datapoint.upload("mystreamid", "myvalue")

Check help(datapoint.upload) for more details and parameters.
"""
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

import csv
import datetime
import io
import time
import collections.abc
import json
import re
from enum import Enum
from syslog import syslog
from typing import Tuple, List

import digidevice._cc as cc


# The data type is serialized into the CSV message as a string, the NAME must
# be the same as the type accepted by the server
class DataType(Enum):
    """
    You can optionally use the DataType enum as an indicator for the
    data type of the stream. The Python data type does not have to match this
    data type. For example, its perfectly reasonable to upload a string value,
    but mark it as a double if the representation matches a double type.
    """
    INT = 0
    LONG = 1
    FLOAT = 2
    DOUBLE = 3
    STRING = 4
    BINARY = 5
    JSON = 6
    GEOJSON = 7

    def __str__(self):
        return "{}".format(self.value)

    def __int__(self):
        return int(self.value)


class DataPointException(RuntimeError):
    """
    The exception generated when a data point fails to upload
    The message contains any captured output from the command, in the case
    of a server produced error, the value comes from the server.
    """
    def __init__(self, msg: str):
        if msg is None or len(msg) == 0:
            msg = "Unknown cause"
        super().__init__(msg)


class DataPoint:
    """
    A single data point. The stream_id and data parameters are required.
    Data is always uploaded in its string form and converted as appropriate by
    the server.
    """
    _id_regex = re.compile("[_\\-\\[\\]:a-zA-Z0-9.!/]+")

    def __init__(
            self, stream_id: str, data, *, description: str = None,
            timestamp: float = None, units: str = None,
            geo_location: Tuple[float, float, float] = None,
            quality: int = None, data_type: DataType = None
            ):
        """
        :param stream_id: The name of the stream will automatically have the
        Device ID prepended to it by the server
        :param data: A python object representing the data point data. For most
        data types, converted to string for upload by its __str__() method, for
        GEOJSON, a non-string python object may be passed and it is converged
        using json.dumps().
        :param description: Optional: The description of the data point
        :param timestamp: Optional: A datetime object or float timestamp (as
        returned by time.time()) describing the local time the data point was
        recorded (if unset, is automatically set by the client at the time of
        send). Local system timezone information is added to the timestamp
        unless it is specified as a datetime object.
        :param units: Optional: Units is free form text, changes the current
        units recorded in the stream associated with the data point
        :param geo_location: Optional: A sequence describing the (latitude,
        longitude and elevation) associated with the data point. A 2 or 3
        element sequence with float values.
        :param quality: Optional: an integer describing the quality associated
        with this data point
        :param data_type: Optional: The DataType enum constant indicating what
        data type should be used for the stream associated with the data point
        :raises TypeError for  or various connection
        :raises ValueError when the value of parameter doesn't match the
        expected type
        """
        if not DataPoint._id_regex.fullmatch(str(stream_id)):
            raise ValueError("Invalid stream_id. stream_id must match: "
                             f"{DataPoint._id_regex.pattern}")

        if data is None:
            raise ValueError("data to upload must be set")

        # Binary not supported yet.
        if isinstance(data, bytes) or isinstance(data, bytearray):
            raise TypeError((
                    "data value is a bytes-like object and data_type"
                    " DataType.BINARY isn't supported by this API"
                    ))

        if data_type is not None:
            if not isinstance(data_type, DataType):
                if cc._trace:
                    syslog('Bad data_type {!r}'.format(data_type))
                raise TypeError("data_type parameter is not a DataType enum")
            if data_type == DataType.BINARY:
                raise TypeError((
                        "data_type is DataType.BINARY which isn't"
                        "supported by this API"
                        ))
                # raise TypeError(("data value is not a bytes-like object and"
                # "data_type DataType.BINARY was used"))

        if geo_location is not None:
            if (isinstance(geo_location, str) or not
                    isinstance(geo_location, collections.abc.Sequence) or
                    (len(geo_location) != 2 and len(geo_location) != 3)):
                # Note that we actually allow any tuple or list-like object
                # through except strings. That's good and pythonic
                raise ValueError("geo_location should be 2 or 3 sequence")

        if stream_id is None:
            raise ValueError("stream_id for upload must be set")

        self.stream_id = str(stream_id)
        if data_type == DataType.GEOJSON and not isinstance(data, str):
            self.data = json.dumps(data)
        else:
            self.data = data
        self.description = description
        if timestamp is None:
            self.timestamp = time.time()
        else:
            self.timestamp = timestamp
        self.units = units
        if geo_location is not None:
            self.geo_location = [str(float(val)) for val in geo_location]
            if len(self.geo_location) == 2:
                self.geo_location.append("0.0")
        else:
            self.geo_location = None
        if quality is not None:
            self.quality = int(quality)  # cast to force type check
        else:
            self.quality = None
        self.data_type = data_type


def _upload(datapoints: List[DataPoint], timeout: float, timeout_msg: str):
    # The datapoint message the data we send for a datapoint upload request.
    # The format of the data sent is
    # b"<start>(<fieldtype><fielddata>)...<EndOfMessage>"
    msg = cc._encode_string("upload_1_dp")

    # Each CSV row must be in this order. Here's an example, all columns are
    # required, None is valid and is an empty value
    # DATA,TIMESTAMP,QUALITY,DESCRIPTION,LOCATION,DATA_TYPE,UNITS,FORWARD_TO,STREAM_ID
    # -447201564,2017-09-28T12:59:25.980138-06:00,97,Kitties,"43.9041,-92.4916,1302.0",FLOAT,Puppies,,test_datapoint/toCFLuJhXSYgRWnHiEPb
    with io.StringIO() as sw:
        # The excel dialect uses fewer characters due to only using quotes when
        # required.
        w = csv.writer(sw)
        for dp in datapoints:
            if not isinstance(dp, DataPoint):
                raise TypeError("datapoints list must only contain DataPoints")

            columns = list()

            columns.append(dp.data)

            sec_west = time.timezone
            delta = datetime.timedelta(seconds=-sec_west)
            tz = datetime.timezone(delta)

            timestamp = dp.timestamp
            if isinstance(timestamp, datetime.datetime):
                if timestamp.tzinfo is None:
                    # Add timezone information
                    timestamp = datetime.datetime.fromtimestamp(
                            timestamp.timestamp(), tz)
                iso = timestamp.isoformat()
            else:
                # Add timezone information to the timestamp float.
                iso = datetime.datetime.fromtimestamp(float(timestamp),
                                                      tz=tz).isoformat()
            columns.append(iso)

            if dp.quality is not None:
                columns.append(dp.quality)
            else:
                columns.append(None)

            # No particular type requirement, None is fine
            columns.append(dp.description)

            if dp.geo_location is not None:
                columns.append(",".join(dp.geo_location))
            else:
                columns.append(None)

            if dp.data_type is not None:
                columns.append(dp.data_type.name)
            else:
                columns.append(None)

            # No particular type requirement, None is fine
            columns.append(dp.units)

            # forward_to unused here
            columns.append(None)

            columns.append(dp.stream_id)

            w.writerow(columns)

        blob = sw.getvalue()

    # Now add it to the message
    msg += cc._encode_int(int(cc._FieldType.CSVData))
    msg += cc._encode_blob(blob)
    msg += cc._encode_int(int(cc._FieldType.EndOfMessage))

    # For debugging, you can generate the binary message here for writing
    # unit tests and other fun stuff.
    # with open("binaryfile.txt", mode='wb') as f:
    #    f.write(msg)
    try:
        resp = cc._cc_interaction(msg, timeout=timeout,
                                  timeout_msg=timeout_msg)
        if len(resp) == 0:
            # All went OK
            pass
        elif len(resp) == 1 and isinstance(resp[0], str):
            # Expected error message
            raise DataPointException(resp[0]) from None
        else:
            # Unexpected format in CC response
            raise DataPointException("Unexpected answer from cloudconnector: "
                                     f"{resp}") from None
    except RuntimeError as e:
        raise DataPointException(repr(e)) from None


def upload(
           stream_id: str, data, *, description: str = None,
           timestamp: float = None, units: str = None,
           geo_location: Tuple[float, float, float] = None,
           quality: int = None, data_type: DataType = None,
           timeout: float = None
           ):
    """
    Upload a single data point. The stream_id and data parameters are required.
    Data is always uploaded in its string form and converted as appropriate by
    the server. Because the data point is sent to the server and communication
    can fail at any time, it's possible for this request to fail, but the data
    point to still be successfully uploaded.

    Usage::
        datapoint.upload("health/state", 100)
        datapoint.upload("drivestats/speed", 37, units="mph",
                         geo_location=(43.9041, -92.4916, 1302))
        datapoint.upload("heartbeat", "ok", timestamp=time.time())

    :param stream_id: The name of the stream will automatically have the Device
    ID prepended to it by the server
    :param data: A python object representing the data point data. For most
    data types, converted to string for upload by its __str__() method, for
    GEOJSON, a non-string python object may be passed and it is converged using
    json.dumps().
    :param description: Optional: The description of the data point
    :param timestamp: Optional: A datetime object or float timestamp (as
    returned by time.time()) describing the local time the data point was
    recorded (if unset, is automatically set by the client at the time of
    send). Local system timezone information is added to the timestamp unless
    it is specified as a datetime object.
    :param units: Optional: Units is free form text, changes the current units
    recorded in the stream associated with the data point
    :param geo_location: Optional: A sequence describing the (latitude,
    longitude and elevation) associated with the data point. A 2 or 3 element
    sequence with float values.
    :param quality: Optional: an integer describing the quality associated with
    this data point
    :param data_type: Optional: The DataType enum constant indicating what data
    type should be used for the stream associated with the data point
    :param timeout: The timeout in seconds before the client reports an error.
    :raises DataPointException for server or transport problems
    :raises TimeoutError if the request times out
    :raises TypeError for  or various connection
    :raises ValueError when the value of parameter doesn't match the expected
    type
    :return:
    """
    _upload([DataPoint(stream_id, data, description=description,
            timestamp=timestamp, units=units, geo_location=geo_location,
            quality=quality, data_type=data_type)], timeout,
            ("The data point upload for stream {!r} timed out"
             .format(stream_id))
            )


def upload_multiple(datapoints: List[DataPoint], timeout: float = None):
    """
    Upload multiple data points. Because the data point is sent to the server
    and communication can fail at any time, it's possible for this request to
    fail, but the data point to still be successfully uploaded.

    :param datapoints: A list of data points to upload
    :param timeout: Optional: The timeout in seconds before the client reports
    an error.
    :raises DataPointException for server or transport problems
    :raises TimeoutError if the request times out
    :raises TypeError for  or various connection
    :raises ValueError when the value of parameter doesn't match the expected
    type
    """
    _upload(datapoints, timeout, "The data point upload timed out")
