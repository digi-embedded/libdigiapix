"""
APIs for registering device request handlers

This module allows to register callbacks for SCI device requests
from DRM:

    from digi.apix import device_request
    
    def handler(target, request):
        print("I received request %s for target %s", request, target)
        print("Sending OK response")
        return "OK"

    device_request.register("my_target", handler)

Check help(device_request.register) and help(device_request.unregister)
for more details and parameters.
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

import atexit
import ctypes
import os
import select
import socket
import sys
import threading
from typing import Callable, Optional

import digi.apix._cc as cc


def _unregister_all():
    targets = list(_registered_callbacks.keys())
    for target in targets:
        try:
            unregister(target)
        except Exception:
            # Do not propagate exceptions to the user, ignore them
            pass

atexit.register(_unregister_all)

class DeviceRequestException(RuntimeError):
    """
    The exception generated when a callback fails to be registered.
    The message contains a description of the problem.
    """

    def __init__(self, msg: str):
        if not msg:
            msg = "Unknown cause"
        super().__init__(msg)


_registered_callbacks = {}
_server_socket = None


class _StoppableThread(threading.Thread):
    """Thread class with a stop() method. The thread itself has to check
    regularly for the stopped() condition."""

    _stop_pipe_r, _stop_pipe_w = os.pipe()

    def __init__(self, *args, **kwargs):
        super(_StoppableThread, self).__init__(*args, **kwargs, daemon=True)

    def stop(self):
        os.write(self._stop_pipe_w, b'!')

    def run(self):
        _server_socket.listen(5)

        while True:
            # Wait for client or for stop signal.
            fds, _, _ = select.select([_server_socket.fileno(), self._stop_pipe_r], [], [])
            if self._stop_pipe_r in fds:
                # On stop signal break out of this main loop to finish the thread.
                break

            (conn, _) = _server_socket.accept()
            conn.settimeout(cc._CC_STATUS_MSG_TIMEOUT)
            stream = conn.makefile('rwb')
            with stream:
                try:
                    cb_type = cc._read_string(stream)   # Callback Type
                    target = cc._read_string(stream)    # Target name
                    try:
                        (request_cb, status_cb, xml_encoding) = _registered_callbacks[target]
                    except KeyError:
                        continue

                    if cb_type == 'request':
                        request = cc._read_blob(stream)  # get the request payload
                        try:
                            request = request.decode(xml_encoding)
                        except UnicodeError:
                            # If the encoding is None, we pass the raw bytes to the callback
                            pass

                    elif cb_type == 'status':
                        err_code = cc._read_int(stream)
                        err_hint = cc._read_string(stream)
                    else:
                        # Ignore other callbacks
                        continue

                    if cb_type == 'request':
                        response = request_cb(target, request)                  # call back to the registered target
                        if not isinstance(response, str):
                            try:
                                response = str(response)
                            except Exception:
                                response = ""

                        cc._write_blob( stream, response.encode(xml_encoding, errors="replace"))  # send the callback response back to the DRM connector
                    elif cb_type == 'status' and status_cb is not None:
                        status_cb(err_code, err_hint)                           # report the status to the callback
                    stream.flush()
                except:
                    # No one can take care of these errors (timeout, incomplete message, ...), just ignore the request.
                    continue

            conn.close()
            
_socket_handler_thread = _StoppableThread()


def _register_callback(target, callback):
    _registered_callbacks[target] = callback

    # If this is the first callback, start background thread
    if not _socket_handler_thread.is_alive():
        _socket_handler_thread.start()


def _send_req_and_validate(req_type, port, target):

    msg = cc._encode_string(req_type) + cc._encode_int(port) + cc._encode_string(target) + cc._encode_int(int(cc._FieldType.EndOfMessage))

    try:
        response = cc._cc_interaction(msg)

        if len(response) == 0:
            # All went OK
            pass;
        elif len(response) == 1 and isinstance(response[0], str):
            # Expected format for error message
            raise DeviceRequestException(response[0]) from None
        else:
            # Unexpected format in CC response
            raise DeviceRequestException("Unexpected answer from cloudconnector: " + str(response)) from None
    except RuntimeError as e:
        raise DeviceRequestException(str(e)) from None


_req_lock = threading.Lock()


def register(target: str, response_callback: Callable[[str, str], Optional[str]],
             status_callback: Callable[[int, str], None] = None, xml_encoding: str = "UTF-8"):
    """
    Register a callback function for a Digi Remote Manager SCI device request for a given target.
    The registered callback function will be called when the device receives a SCI device request
    for the given target. This callback may return a str (or str-convertible object) that will be
    sent back to Digi Remote Manager as response to the SCI device request.
    Optionally, a second callback to receive information about the SCI response sent may be passed.
    The default encoding for SCI device request is UTF-8, but a different enconding may be specified.
    If the encoding is set to None or the decoding of a request from DRM fails, the response_callback
    function will be called with the raw byte array as a parameter, instead of a str.
    For the response_callback return value, characters that cannot be encoded in the given encoding will be replaced.
    Note that only one callback can be associated to any given target: if this method is called with a
    target that is already registered, that call will override previous registered callbacks, which will
    no longer be called. This applies even if the previous call to .register was done from a different
    process.

    Example of RCI request for target "myTarget":
    <sci_request version="1.0">
       <data_service>
         <targets>
           <device id="00000000-00000000-00000000-00000000"/>
         </targets>
         <requests>
              <device_request target_name="myTarget">my payload string</device_request>
         </requests>
       </data_service>
    </sci_request>


    Usage::
        device_request.register("my_target", my_handler)
        device_request.register("my_target", my_handler, status_callback = my_status_cb)
        device_request.register("my_bin_target", my_bin_handler, xml_encoding = None)

    :param target: Value of the "target_name" attribute on the SCI device_request
    :param response_callback: callback function that takes two arguments: the target (str) and the
    request (str). The function may return a str to be sent back to Digi Remote Manager as reply to
    the SCI request.
    :param status_callback: Optional: callback function that provides status about the SCI reply. It takes
    two arguments: an error code (int) and a error hint (str). The error code is 0 for success, and other positive
    values for different errors. The error hint contains an indication of the problem or "Success" for error code 0.
    :param xml_encoding: Optional: Encoding for the SCI request. In case the raw bytes cannot be interpreted using the
    specified encoding, or if the specified encoding is None, then the response_callback will be invoked with the raw
    byte array instead of the str for the request argument. For the response_callback return value, characters that
    cannot be encoded in the given encoding will be replaced. Defaults to UTF-8.
    :raises DeviceRequestException for server or transport problems
    :raises TimeoutError if the request times out
    :raises ValueError when the value of parameter doesn't match the expected type
    :return:
    """
    global _server_socket

    if not isinstance(target, str):
        raise TypeError("paramater target should be str")
    if not callable(response_callback):
        raise TypeError("parameter response_callback should be callable")
    if status_callback is not None and not callable(status_callback):
        raise TypeError("parameter status_callback should be callable")
    if str is not None and not isinstance(xml_encoding, str):
        raise TypeError("optional parameter xml_encoding should be str")

    # If this is a new target for this process, we need to register the process in the cloudconnector
    if target not in _registered_callbacks:
        with _req_lock:
            if _server_socket is None:
                try:
                    _server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    _server_socket.bind((cc.CC_HOST_NAME, 0))
                except Exception as e:
                    raise DeviceRequestException("Unexpected internal error: " + repr(e))

            port = _server_socket.getsockname()[1]
            _send_req_and_validate("register_devicerequest", port, target)

    _register_callback(target, (response_callback, status_callback, xml_encoding))


def unregister(target: str) -> bool:
    """
    Unregisters a previously register callback for a given SCI target.

    Note that you cannot unregister callbacks which were registered by another process.

    Usage::
        device_request.unregister("my_target")

    :param target: Value of the "target_name" attribute on the SCI device_request
    to it by the server
    :raises DeviceRequestException for server or transport problems
    :raises TimeoutError if the request times out
    :raises ValueError when the value of parameter doesn't match the expected type
    :return: True if the target was unregistered, False if the target was not registered by this process.
    """
    global _socket_handler_thread

    if not isinstance(target, str):
        raise TypeError("parameter 'target' should be str")

    with _req_lock:
        try:
            del _registered_callbacks[target]

            # If there are no callbacks, stop the thread
            if len(_registered_callbacks) == 0:
                _socket_handler_thread.stop()

            port = _server_socket.getsockname()[1]
            _send_req_and_validate("unregister_devicerequest", port, target)

            return True
        except KeyError:
            return False
