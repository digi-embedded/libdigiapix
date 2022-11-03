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

from ctypes import c_bool, c_char, c_int, c_uint8, c_uint16, c_uint32, POINTER, Structure, byref

from digi.apix import library
from digi.apix.common import _AbstractEnum, MacAddress, INTERFACE_NAME_SIZE, MAC_ADDRESS_GROUPS
from digi.apix.exceptions import DigiAPIXException

# Constants.
_BT_NAME_MAX_LENGTH = 248

# Variables.
_libdigiapix = None


class BluetoothException(DigiAPIXException):
    """
    Exception thrown when an error occurs working with Bluetooth devices.
    """


class _BluetoothError(_AbstractEnum):
    """
    Enumeration class listing all the Bluetooth errors.
    """
    NONE = (0, "None", "No error")
    NO_EXIST = (1, "Device does not exist", "The requested device does not exist")
    NO_MEM = (2, "No memory", "Not enough memory to read the Bluetooth state")
    HCI_INFO = (3, "HCI error", "Error reading the HCI information")
    ENABLE = (4, "Enable error", "Error reading the HCI information")
    LOCAL_NAME = (5, "MAC error", "Error reading the interface MAC address")
    CONFIG = (6, "Configuration error", "Error configuring the Bluetooth device")

    def __init__(self, code, title, description):
        """
        Class constructor. Instantiates a new `_BluetoothError` entry with the provided parameters.

        Args:
            code (Integer): Bluetooth error code.
            title (String): Bluetooth error title.
            description (String): Bluetooth error description.
        """
        super().__init__(code, title, description)


class _BluetoothEnable(_AbstractEnum):
    """
    Enumeration class listing all Bluetooth enable status.
    """
    ERROR = (-1, "Enable error")
    DISABLED = (0, "Disabled")
    ENABLED = (1, "Enabled")

    def __init__(self, code, description):
        """
        Class constructor. Instantiates a new `_BluetoothEnable` entry with the provided
        parameters.

        Args:
            code (Integer): Bluetooth enable code.
            description (String): Bluetooth enable description.
        """
        super().__init__(code, "", description)


class BluetoothProfile:
    """
    Bluetooth profile to apply to a Bluetooth device.
    """
    def __init__(self):
        self.__enable = None
        self.__advert_name = None

    @property
    def enable(self):
        """
        Gets the profile enable option.

        Returns:
            Boolean: `True` to enable the Bluetooth device, `False` to disable,
                     `None` to do nothing.
        """
        return self.__enable

    @enable.setter
    def enable(self, value):
        """
        Sets the profile enable option.

        Params:
            value (Boolean): `True` to enable, `False` to disable,
                             `None` to do nothing.

        Raises:
            ValueError: If `value` is invalid.
        """
        if value is not None and not isinstance(value, bool):
            raise ValueError("Value must be a boolean")

        self.__enable = value

    @property
    def advert_name(self):
        """
        Gets the profile advertising name.

        Returns:
            String: The advertising name, `None` to do nothing.
        """
        return self.__advert_name

    @advert_name.setter
    def advert_name(self, value):
        """
        Sets the profile advertising name.

        Params:
            value (String): The advertising name, `None` to do nothing.

        Raises:
            ValueError: If `value` is invalid.
        """
        if value is not None and not isinstance(value, str):
            raise ValueError("Value must be a string")

        self.__advert_name = value

    def _get_cfg_struct(self):
        """
        Returns a :class:`._BluetoothConfigStruct`.

        Returns:
            :class:`._BluetoothConfigStruct`: Bluetooth configuration struct.
        """
        cfg_struct = _BluetoothConfigStruct()

        cfg_struct.enable = self.__enable if not None else -1

        cfg_struct.set_name = self.__advert_name is not None
        cfg_struct.name = bytes((0,))
        if cfg_struct.set_name:
            cfg_struct.name = self.__advert_name.encode(encoding="utf-8", errors="ignore")

        return cfg_struct


class _BluetoothStatsStruct(Structure):
    """
    Internal class to store C library Bluetooth statistics struct data.
    """
    _fields_ = [('rx_bytes', c_uint32),
                ('rx_errors', c_uint32),
                ('rx_acl', c_uint32),
                ('rx_sco', c_uint32),
                ('rx_events', c_uint32),
                ('tx_bytes', c_uint32),
                ('tx_errors', c_uint32),
                ('tx_acl', c_uint32),
                ('tx_sco', c_uint32),
                ('tx_cmds', c_uint32)]


class _BluetoothStateStruct(Structure):
    """
    Internal class to store C library Bluetooth state struct data.
    """
    _fields_ = [('dev_id', c_uint16),
                ('dev_name', c_char * INTERFACE_NAME_SIZE),
                ('name', c_char * (_BT_NAME_MAX_LENGTH + 1)),
                ('mac', c_uint8 * MAC_ADDRESS_GROUPS),
                ('enable', c_int),
                ('running', c_bool)]


class _BluetoothConfigStruct(Structure):
    """
    Internal class to store C library Bluetooth configuration struct data.
    """
    _fields_ = [('dev_id', c_uint16),
                ('enable', c_int),
                ('set_name', c_bool),
                ('name', c_char * (_BT_NAME_MAX_LENGTH + 1))]


class BluetoothStats:
    """
    This class represents the statistics of a Bluetooth device.
    Use `get_statistics()` method from your `BluetoothDevice`to get statistics.
    """

    __create_key = object()

    def __init__(self, _key, device_id):
        """
        Class constructor. This method should not be called directly, instead
        use `get_statistics()` method from your `BluetoothDevice`to get statistics.
        """
        if _key is not BluetoothStats.__create_key:
            raise RuntimeError(
                "BluetoothStats objects must be created using BluetoothDevice.get_statistics()")
        self._device_id = device_id
        self._bt_stats_struct = None

    @property
    def rx_bytes(self):
        """
        Returns the number of good received bytes by the Bluetooth device.

        Returns:
            Integer: Number of good received bytes by the Bluetooth device.
        """
        return self._bt_stats_struct.rx_bytes

    @property
    def rx_errors(self):
        """
        Returns the total number of bad packets received by the Bluetooth device.

        Returns:
            Integer: Total number of bad packets received by the Bluetooth device.
        """
        return self._bt_stats_struct.rx_errors

    @property
    def rx_acl(self):
        """
        Returns the number of good received ACL packets by the Bluetooth device.

        Returns:
            Integer: Number of good received ACL packets by the Bluetooth device.
        """
        return self._bt_stats_struct.rx_acl

    @property
    def rx_sco(self):
        """
        Returns the number of good received SCO packets by the Bluetooth device.

        Returns:
            Integer: Number of good received SCO packets by the Bluetooth device.
        """
        return self._bt_stats_struct.rx_sco

    @property
    def rx_events(self):
        """
        Returns the number of good received events by the Bluetooth device.

        Returns:
            Integer: Number of good received events by the Bluetooth device.
        """
        return self._bt_stats_struct.rx_events

    @property
    def tx_bytes(self):
        """
        Returns the number of good transmitted bytes by the Bluetooth device.

        Returns:
            Integer: Number of good transmitted bytes by the Bluetooth device.
        """
        return self._bt_stats_struct.tx_bytes

    @property
    def tx_errors(self):
        """
        Returns the total number of transmit problems in the Bluetooth device.

        Returns:
            Integer: Total number of transmit problems in the Bluetooth device.
        """
        return self._bt_stats_struct.tx_errors

    @property
    def tx_acl(self):
        """
        Returns the number of good transmitted ACL packets by the Bluetooth device.

        Returns:
            Integer: Number of good transmitted ACL packets by the Bluetooth device.
        """
        return self._bt_stats_struct.tx_acl

    @property
    def tx_sco(self):
        """
        Returns the number of good transmitted SCO packets by the Bluetooth device.

        Returns:
            Integer: Number of good transmitted SCO packets by the Bluetooth device.
        """
        return self._bt_stats_struct.tx_sco

    @property
    def tx_commands(self):
        """
        Returns the number of good transmitted commands by the Bluetooth device.

        Returns:
            Integer: Number of good transmitted commands by the Bluetooth device.
        """
        return self._bt_stats_struct.tx_cmds

    def update(self):
        """
        Reads and updates the Bluetooth device statistics.

        Raises:
            DigiAPIXException: If there is any error loading the library.
            BluetoothException: If there is any error reading the Bluetooth device statistics.
        """
        _check_library()

        bt_stats = _BluetoothStatsStruct()
        error = _BluetoothError.get(_libdigiapix.ldx_bt_get_stats(self._device_id,  bt_stats))
        if error is not _BluetoothError.NONE:
            raise BluetoothException(error.description)

        self._bt_stats_struct = bt_stats


class BluetoothDevice:
    """
    This class represents the information of a Bluetooth devices. Instances of BluetoothDevice
    should be created using the 'get()' method.
    """

    __create_key = object()

    def __init__(self, _key, _bt_state_struct):
        """
        Class constructor. This method should not be called directly, instead use the
        'get()' method.
        """
        if _key is not BluetoothDevice.__create_key:
            raise RuntimeError(
                "BluetoothDevice objects must be created using get() method")
        self._bt_state_struct = _bt_state_struct
        self._bt_stats = None

    @staticmethod
    def list_devices():
        """
        Lists the available Bluetooth devices.

        Returns:
            List: the list with the names of available Bluetooth devices,
                  `None` if no Bluetooth device is available.

        Raises:
            BluetoothException: if there is any error listing the Bluetooth devices.
            DigiAPIXException: if there is any error loading the library.
        """
        # Get the list of available devices with their information.
        bt_devices = BluetoothDevice._get_bluetooth_states()
        if not bt_devices:
            return None

        return [bt_device.dev_name.decode(encoding="ascii", errors="ignore")
                for bt_device in bt_devices]

    @classmethod
    def get(cls, device_name):
        """
        Returns the Bluetooth device with the given name.

        Params:
            device_name (String): Name of the Bluetooth device to get.

        Returns:
            :class:`.BluetoothDevice`: The Bluetooth device.

        Raises:
            ValueError: If `device_name` is not a string.
            DigiAPIXException: If there is any error loading the library.
            BluetoothException: If there is any error getting the Bluetooth device information.
        """
        # Sanity checks.
        if not isinstance(device_name, str):
            raise ValueError("Device name must be a string")
        _check_library()

        # Get the list of available devices with their information.
        bt_devices = BluetoothDevice._get_bluetooth_states()
        if not bt_devices:
            raise BluetoothException("No Bluetooth devices available")
        # Look for the requested device.
        for bt_device in bt_devices:
            if bt_device.dev_name.decode(encoding="ascii", errors="ignore") == device_name:
                return BluetoothDevice(cls.__create_key, bt_device)
        raise BluetoothException(f"Bluetooth device '{device_name}' not found")

    @staticmethod
    def _get_bluetooth_states():
        """
        Lists the available Bluetooth devices information.

        Returns:
            List: the list with the available Bluetooth devices information,
                  `None` if no Bluetooth device is available.

        Raises:
            DigiAPIXException: if there is any error loading the library.
        """
        # Sanity checks.
        _check_library()

        native_devices_list = POINTER(c_uint16)()
        num_devices = _libdigiapix.ldx_bt_list_available_devices(byref(native_devices_list))
        if num_devices < 1:
            return None
        device_ids = [native_devices_list[i] for i in range(num_devices)]
        if not device_ids:
            return None
        # Get the state struct for each available device.
        bt_devices = []
        for device_id in device_ids:
            bt_state = _BluetoothStateStruct()
            error = _BluetoothError.get(_libdigiapix.ldx_bt_get_state(device_id, bt_state))
            if error is not _BluetoothError.NONE:
                continue
            bt_devices.append(bt_state)

        return bt_devices

    def configure(self, profile):
        """
        Configures the Bluetooth device with the provided profile.

        Params:
            profile (:class:`.BluetoothProfile`): Profile to apply.

        Raises:
            ValueError: If `profile` is invalid.
            DigiAPIXException: If there is any error loading the library.
            BluetoothException: If there is any error configuring the Bluetooth device.
        """
        if not isinstance(profile, BluetoothProfile):
            raise ValueError("Profile must be a BluetoothProfile")

        cfg_struct = profile._get_cfg_struct()
        cfg_struct.dev_id = self._bt_state_struct.dev_id

        _check_library()

        error = _BluetoothError.get(_libdigiapix.ldx_bt_set_config(cfg_struct))
        if error is not _BluetoothError.NONE:
            raise BluetoothException(error.description)

    @property
    def device_id(self):
        """
        Returns the ID of the Bluetooth device.

        Returns:
            Integer: ID of the Bluetooth device.
        """
        return self._bt_state_struct.dev_id

    @property
    def device_name(self):
        """
        Returns the Bluetooth device name.

        Returns:
            String: Bluetooth device name.
        """
        return self._bt_state_struct.dev_name.decode(encoding="ascii", errors="ignore")

    @property
    def advert_name(self):
        """
        Returns the Bluetooth device advertising name.

        Returns:
            String: Bluetooth device advertising name.
        """
        return self._bt_state_struct.name.decode(encoding="ascii", errors="ignore")

    @property
    def mac(self):
        """
        Returns the Bluetooth device MAC address.

        Returns:
            :class:`.MacAddress`: The Bluetooth device MAC address.
        """
        return MacAddress(self._bt_state_struct.mac)

    @property
    def is_enabled(self):
        """
        Returns whether the Bluetooth device is enabled or not.

        Returns:
            Boolean: `True` if the Bluetooth device is enabled, `False` otherwise.
        """
        return self._bt_state_struct.enable == _BluetoothEnable.ENABLED.code

    @property
    def is_running(self):
        """
        Returns whether the Bluetooth device is running or not.

        Returns:
            Boolean: `True` if the Bluetooth device is running, `False` otherwise.
        """
        return self._bt_state_struct.running

    def get_statistics(self):
        """
        Returns the Bluetooth device statistics.

        Returns:
            :class:`.BluetoothStats`: The Bluetooth device statistics.

        Raises:
            DigiAPIXException: If there is any error loading the library.
            BluetoothException: If there is any error reading the Bluetooth device statistics.
        """
        if self._bt_stats is None:
            device_id = self._bt_state_struct.dev_id
            self._bt_stats = BluetoothStats(BluetoothStats._BluetoothStats__create_key, device_id)
            self._bt_stats.update()

        return self._bt_stats


def _check_library():
    """
    Verifies that the 'digiapix' library is loaded.

    Raises:
        DigiAPIXException: if there is any error loading the library.
    """
    # Use global variable.
    global _libdigiapix
    # Load the library.
    if _libdigiapix is None:
        _libdigiapix = library.get_library()
        # Configure the CTypes of the Bluetooth methods to use.
        _configure_bluetooth_ctypes()


def _configure_bluetooth_ctypes():
    """
    Configures the ctypes for the library Bluetooth methods
    """
    # Get available devices.
    _libdigiapix.ldx_bt_list_available_devices.argtypes = [POINTER(POINTER(c_uint16))]
    _libdigiapix.ldx_bt_list_available_devices.restype = c_int
    # Get device state.
    _libdigiapix.ldx_bt_get_state.argtypes = [c_uint16, POINTER(_BluetoothStateStruct)]
    _libdigiapix.ldx_bt_get_state.restype = c_int
    # Get device statistics.
    _libdigiapix.ldx_bt_get_stats.argtypes = [c_uint16, POINTER(_BluetoothStatsStruct)]
    _libdigiapix.ldx_bt_get_stats.restype = c_int
    # Set configuration.
    _libdigiapix.ldx_bt_set_config.argtypes = [_BluetoothConfigStruct]
    _libdigiapix.ldx_bt_set_config.restype = c_int
