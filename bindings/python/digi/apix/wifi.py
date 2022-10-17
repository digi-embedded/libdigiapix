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

from ctypes import c_bool, c_char, c_char_p, c_int, POINTER, Structure, c_double

from digi.apix import library
from digi.apix.common import _AbstractEnum, INTERFACE_NAME_SIZE
from digi.apix.exceptions import DigiAPIXException
from digi.apix.network import _NetworkError, _NetStateStruct, _NetConfigStruct, \
    _NetNamesListStruct, NetworkInterface, NetworkProfile

# Constants.
SSID_NAME_SIZE = 32

# Variables.
_libdigiapix = None


class WifiException(DigiAPIXException):
    """
    Exception thrown when an error occurs working with WiFi interfaces.
    """


class SecurityMode(_AbstractEnum):
    """
    Enumeration class listing all security modes.
    """
    UNKNOWN = (-1, "Unknown")
    OPEN = (0, "Open")
    WPA = (1, "WPA1")
    WPA2 = (2, "WPA2")
    WPA3 = (3, "WPA3")

    def __init__(self, code, description):
        """
        Class constructor. Instantiates a new `SecurityMode` entry with the
        provided parameters.

        Args:
            code (Integer): Security mode code.
            description (String): Security mode description.
        """
        super().__init__(code, "", description)


class WifiProfile(NetworkProfile):
    """
    Network profile to apply to a WiFi interface.
    """
    def __init__(self):
        super().__init__()
        self.__ssid = None
        self.__sec_mode = None
        self.__psk = None

    @property
    def ssid(self):
        """
         Gets the SSID.

         Returns:
            String: SSID, `None` to do nothing.
        """
        return self.__ssid

    @ssid.setter
    def ssid(self, value):
        """
         Sets the profile SSID.

         Params:
             value (String): New SSID, `None` to do nothing.

        Raises:
            ValueError: If `value` is invalid.
        """
        if value is not None and not isinstance(value, str):
            raise ValueError("Value must be a string")

        self.__ssid = value

    @property
    def sec_mode(self):
        """
         Gets the profile connection security mode.

         Returns:
            :class:`.SecurityMode`: Security mode to connect, `None` to do nothing.
        """
        return self.__sec_mode

    @sec_mode.setter
    def sec_mode(self, value):
        """
         Sets the profile connection mode.

         Params:
            value (:class:`.SecurityMode`): Connection mode, `None` to do nothing.

        Raises:
            ValueError: If `value` is invalid.
        """
        if value is not None and not isinstance(value, SecurityMode):
            raise ValueError("Value must be a SecurityMode")

        self.__sec_mode = value

    @property
    def psk(self):
        """
         Gets the profile connection password.

         Returns:
            String: Password for the connection, `None` to do nothing.
        """
        return self.__psk

    @psk.setter
    def psk(self, value):
        """
         Sets the profile connection password.

         Params:
            value (String): The password. `None` to do nothing.

        Raises:
            ValueError: If `value` is invalid.
        """
        if value is not None and not isinstance(value, str):
            raise ValueError("Value must be a string")

        self.__psk = value

    def _get_cfg_struct(self):
        """
        Returns a :class:`._WifiConfigStruct`.

        Returns:
            :class:`._WifiConfigStruct`: WiFi configuration struct.
        """
        cfg_struct = _WifiConfigStruct()
        cfg_struct.set_ssid = self.__ssid is not None
        cfg_struct.ssid = bytes((0,))
        if cfg_struct.set_ssid:
            cfg_struct.ssid = self.__ssid.encode(encoding="utf-8", errors="ignore")
        cfg_struct.sec_mode = SecurityMode.UNKNOWN.code
        if self.__sec_mode is not None:
            cfg_struct.sec_mode = self.__sec_mode.code
        cfg_struct.psk = bytes((0,))
        if self.__psk is not None:
            cfg_struct.psk = self.__psk.encode(encoding="utf-8", errors="ignore")
        cfg_struct.net_config = super()._get_cfg_struct()

        return cfg_struct


_networkErrorEntries = [(m.name, (m.code, m.title, m.description)) for m in _NetworkError]
_networkErrorEntries.append(("RANGE_INFO", (14, "Range info error",
                                            "Error getting range information")))
_networkErrorEntries.append(("SSID", (15, "SSID error",
                                          "Error getting SSID")))
_networkErrorEntries.append(("FREQ", (16, "Frequency error",
                                          "Error getting frequency")))
_networkErrorEntries.append(("CHANNEL", (17, "Channel error",
                                             "Error getting channel")))
_networkErrorEntries.append(("SEC_MODE", (18, "Security mode",
                                              "Error getting security mode")))
_WifiError = _AbstractEnum('_WifiError', _networkErrorEntries)


class _WifiStateStruct(Structure):
    """
    Internal class to store C library WiFi state struct data.
    """
    _fields_ = [('net', _NetStateStruct),
                ('ssid', c_char * SSID_NAME_SIZE),
                ('freq', c_double),
                ('channel', c_int),
                ('sec_mode', c_int)]


class _WifiConfigStruct(Structure):
    """
    Internal class to store C library WiFi configuration struct data.
    """
    _fields_ = [('name', c_char * INTERFACE_NAME_SIZE),
                ('set_ssid', c_bool),
                ('ssid', c_char * SSID_NAME_SIZE),
                ('sec_mode', c_int),
                ('psk', c_char_p),
                ('net_config', _NetConfigStruct)]


class WifiInterface(NetworkInterface):
    """
    This class represents the information of a WiFi interface.
    """

    __create_key = object()

    def __init__(self, _key, _wifi_state_struct):
        """
        Class constructor. This method should not be called directly, instead
        use `get()` method.
        """
        if _key is not WifiInterface.__create_key:
            raise RuntimeError(
                "WifiInterface objects must be created using get() method")
        super().__init__(NetworkInterface._NetworkInterface__create_key, _wifi_state_struct.net)
        self._wifi_state_struct = _wifi_state_struct

    @staticmethod
    def list_interfaces():
        """
        Lists the available WiFi interfaces.

        Returns:
            List: List of available WiFi interfaces, `None` if no interface
                is available.

        Raises:
            DigiAPIXException: If there is any error loading the library.
        """
        _check_library()

        net_list = _NetNamesListStruct()
        num_ifaces = _libdigiapix.ldx_wifi_list_available_ifaces(net_list)
        if num_ifaces < 1:
            return None

        return [net_list.names[i].value.decode(encoding="ascii", errors="ignore")
                for i in range(net_list.n_ifaces)]

    @classmethod
    def get(cls, name):
        """
        Returns the information of the given WiFi interface.

        Params:
            name (String): Name of the interface to get.

        Returns:
            :class:`.WifiInterface`: The WiFi interface.

        Raises:
            ValueError: If `name` is not a string or is invalid.
            DigiAPIXException: If there is any error loading the library.
            WifiException: If there is any error getting the interface information.
        """
        # Sanity checks.
        if not isinstance(name, str):
            raise ValueError("Name must be a valid string")

        _check_library()

        wifi_state = _WifiStateStruct()
        error = _WifiError.get(_libdigiapix.ldx_wifi_get_iface_state(
            name.encode(encoding="ascii", errors="ignore"), wifi_state))
        if error is not _WifiError.NONE:
            raise WifiException(error.description)

        return WifiInterface(cls.__create_key, wifi_state)

    def configure(self, profile):
        """
        Configures the interface with the provided profile.

        Params:
            profile (:class:`.WifiProfile`): Profile to apply.

        Raises:
            ValueError: If `profile` is invalid.
            DigiAPIXException: If there is any error loading the library.
            WifiException: If there is any error configuring the interface.
        """
        if not isinstance(profile, WifiProfile):
            raise ValueError("Profile must be a WifiProfile")

        cfg_struct = profile._get_cfg_struct()
        cfg_struct.name = self._wifi_state_struct.net.name

        _check_library()

        error = _WifiError.get(_libdigiapix.ldx_wifi_set_config(cfg_struct))
        if error is not _WifiError.NONE:
            raise WifiException(error.description)

    @property
    def ssid(self):
        """
        Returns the SSID name.

        Returns:
            String: SSID name.
        """
        return self._wifi_state_struct.ssid.decode(encoding="ascii")

    @property
    def frequency(self):
        """
        Returns the frequency.

        Returns:
            Float: Frequency value.
        """
        return self._wifi_state_struct.freq

    @property
    def channel(self):
        """
        Returns the channel.

        Returns:
            Integer: Channel value.
        """
        return self._wifi_state_struct.channel

    @property
    def sec_mode(self):
        """
        Returns the security mode.

        Returns:
            :class:`.SecurityMode`: Security mode.
        """
        mode = SecurityMode.get(self._wifi_state_struct.sec_mode)
        if mode is None:
            return SecurityMode.UNKNOWN

        return mode


def _check_library():
    """
    Verifies that the 'digiapix' library is loaded.

    Raises:
        DigiAPIXException: If there is any error loading the library.
    """
    global _libdigiapix
    if _libdigiapix is None:
        _libdigiapix = library.get_library()
        _configure_wifi_ctypes()


def _configure_wifi_ctypes():
    """
    Configures the ctypes for the library network methods
    """
    # Get available interfaces.
    _libdigiapix.ldx_wifi_list_available_ifaces.argtypes = [POINTER(_NetNamesListStruct)]
    _libdigiapix.ldx_wifi_list_available_ifaces.restype = c_int
    # Get interface state.
    _libdigiapix.ldx_wifi_get_iface_state.argtypes = [c_char_p, POINTER(_WifiStateStruct)]
    _libdigiapix.ldx_wifi_get_iface_state.restype = c_int
    # Set interface configuration.
    _libdigiapix.ldx_wifi_set_config.argtypes = [_WifiConfigStruct]
    _libdigiapix.ldx_wifi_set_config.restype = c_int
