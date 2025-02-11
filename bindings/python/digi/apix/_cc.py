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

import ctypes
import socket
import io
from enum import Enum
from typing import List

# TODO: Cloud connector stuff in separate module as soon as its used from two places.

CC_HOST_NAME = "127.0.0.1"
CC_PORT = 977
# TODO:2 seconds seems bad, this is probably left over socket to cloud connector timeout, not actual round trip time.
_CC_STATUS_MSG_TIMEOUT = 2.0

_traceReqResp = False

_intsize = ctypes.sizeof(ctypes.c_uint32)

class _FieldType(Enum):
    """
    Internal values for field types in a cloud connector request message.
    The field type is serialized into the request message as a bytes(c_int())
    and must match the field types in the cloud connector.
    See the source for the format of the field data (which must be in the request
    message immediately following the field type).
    """
    EndOfMessage = 0  # Message ends
    CSVData = 1  # data is: c_int length, then length ASCII bytes of CSVdata (including newlines if required)

    def __int__(self):
        return int(self.value)

class _ResponseType(Enum):
    """
    Internal values for field types in a cloud connector request message.
    The field type is serialized into the request message as a bytes(c_int32())
    in network byte order and must match the field types in the cloud connector.
    See the source for the format of the field data (which must be in the request
    message immediately following the field type).
    """
    EndOfMessage = 0  # Message ends
    ErrorText = 1  # data is: c_int length, then length ASCII bytes
    ErrorCode = 2  # data is: c_int error code value

    def __int__(self):
        return int(self.value)

def _parse_response(stream) -> List:
    ret = []

    if _traceReqResp:
        response = stream.read()                # read ALL the data from the stream until EOF
        stream = io.BytesIO(response)           # and make a stream of the response data
        print("Resp: {!r}".format(response))    # dump the content

    while True:
        
        try:
            rtype = _ResponseType( _read_int(stream))
            if rtype == _ResponseType.EndOfMessage:
                break
            elif rtype == _ResponseType.ErrorText:
                msg = _read_blob(stream).decode('ascii')
                ret.append(msg)
            elif rtype == _ResponseType.ErrorCode:
                ret.append(_read_int(stream))
            else:
                raise ValueError("Received an unknown response from the DRM cloud connector {}".format(rtype))
        except:
            raise ValueError("Bad response from cloud connector {}".format(response if _traceReqResp else ""))

    return ret


def _cc_interaction(msg: bytes, timeout: float = _CC_STATUS_MSG_TIMEOUT, timeout_msg: str = None) -> List:
    # TODO: Handle error scenarios reasonably.
    raw_response = b''
    # None means block forever, while 0 would turn on non-blocking mode (this would be kind of bad)
    if timeout is not None and timeout <= 0:
        raise ValueError("The timeout value {} is not valid".format(timeout))
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as fd:
        try:
            fd.connect((CC_HOST_NAME, CC_PORT))
            if _traceReqResp:
                print("Req: {!r}".format(msg))
            fd.sendall(msg)
            fd.settimeout(timeout)
            response = _parse_response(fd.makefile('rb'))
        except ConnectionResetError as e:
            # TODO: Build some smarts into this line flow about the data that is supposed to be
            #       available (i.e. a total length of each message) we would not have to deal with it like this.
            raise RuntimeError("The DRM connector reset the connection prematurely") from None
        except ConnectionRefusedError as e:
            raise RuntimeError("The DRM connector is not running") from None
        except socket.timeout:
            if timeout_msg is None:
                timeout_msg = "The Cloud Connector request timed out"
            raise TimeoutError(timeout_msg) from None
    return response

def _encode_int(value: int) -> bytes:
    return ("i:{}\n".format(value)).encode('ascii')

def _encode_blob(data, str_encoding = "ascii", _type = b'b:') -> bytes:
    """
    Convert a python object to its string representation and serialize
    it to c data bytes using the form <len><ASCII> where len is a c_int,
    and ASCII is the ASCII bytes of the string (with no trailing null).
    :param data: A python variable converted to a string using the standard __str__() method
    :return: The bytes representing this field.
    """
    s = str(data).encode(str_encoding)
    l = len(s)
    if l > 65535:
        raise ValueError("The data size ({}) is too large".format(l))
    return _type + _encode_int(l) + s + b'\n'

def _encode_string(_data, _str_encoding = "ascii") -> bytes:
    result = _encode_blob(_data, _str_encoding, b's:')
    return result

def _readall( stream, length ) -> bytes:
    read = 0;
    result = b''
    while read < length:
        data = stream.read(length - read)
        dl = len(data)
        if dl == 0:
            raise ValueError("Encounted unexpected stream EOF")
        result += data
        read += dl
    return result

def _writeall( stream, data ):
    written = 0;
    total = len(data)
    while written < total:
        nw = stream.write( data[written:total-written])
        if nw is not None:
            written += nw

def _write_blob( stream, data: bytes):
    l = len(data)
    if l > 65535:
        raise ValueError("The data size ({}) is too large".format(l))
    data = b"b:" + _encode_int(l) + data + b'\n'
    _writeall( stream, data )

def _read_int(stream) -> int:
    text = stream.readline()
    if text[0:2] != b'i:':
        raise ValueError("Expecting an int 'i:', received '{}'".format(text))
    return int(text[2:])

def _read_blob(stream, _type = "blob", prefix = b'b:') -> bytes:
    if stream.read(2) != prefix:
        raise ValueError("Error: {} expecting an '{}' at beginning of {}".format(_type,prefix,_type))
    length = _read_int(stream)
    result = _readall( stream, length )
    if _readall( stream, 1) != b'\n':
        raise ValueError("Missing terminator from end of " + _type)
    return result

def _read_string(stream, encoding = 'ascii') -> str:
    return _read_blob(stream,"string", b's:').decode(encoding)

