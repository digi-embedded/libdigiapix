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
from enum import Enum

from ctypes import c_bool, c_char, c_char_p, c_int, c_uint8, c_uint32, POINTER, Structure
from ipaddress import IPv4Address, IPv4Interface

from digi.apix import library
from digi.apix.common import MacAddress, INTERFACE_NAME_SIZE, MAC_ADDRESS_GROUPS, MAX_IFACES
from digi.apix.exceptions import DigiAPIXException

# Constants.
_IPV4_GROUPS = 4

# Variables.
_libdigiapix = None


class NetworkException(DigiAPIXException):
    """
    Exception thrown when an error occurs working with network interfaces
    """


class IPMode(Enum):
    """
    Enumeration class listing all the IP modes.
    """
    UNKNOWN = (-1, "Unknown")
    STATIC = (0, "Static")
    DHCP = (1, "DHCP")

    def __init__(self, code, description):
        """
        Class constructor. Instantiates a new `IPMode` entry with the provided
        parameters.

        Args:
            code (Integer): IP mode code.
            description (String): IP mode description.
        """
        self.__code = code
        self.__description = description

    @property
    def code(self):
        """
        Returns the IP mode code.

        Returns:
            Integer: IP mode code.
        """
        return self.__code

    @property
    def description(self):
        """
        Returns the IP mode description.

        Returns:
            String: IP mode description.
        """
        return self.__description

    @classmethod
    def get(cls, code):
        """
        Returns the IP mode corresponding to the given code.

        Args:
            code (Integer): IP mode code.

        Returns:
            :class:`.IPMode`: IP mode corresponding to the given code,
                `None` if not found.
        """
        for mode in cls:
            if code == mode.code:
                return mode

        return None


class NetStatus(Enum):
    """
    Enumeration class listing all network status.
    """
    CONNECTED = (0, "Connected")
    DISCONNECTED = (1, "Disconnected")
    UNMANAGED = (2, "Unmanaged")
    UNAVAILABLE = (3, "Unavailable")
    UNKNOWN = (4, "Unknown")

    def __init__(self, code, description):
        """
        Class constructor. Instantiates a new `NetStatus` entry with the provided parameters.

        Args:
            code (Integer): Network status code.
            description (String): Network status description.
        """
        self.__code = code
        self.__desc = description

    @property
    def code(self):
        """
        Returns the network status code.

        Returns:
            Integer: Network status code.
        """
        return self.__code

    @property
    def description(self):
        """
        Returns the network status description.

        Returns:
            String: Network status description.
        """
        return self.__desc

    @classmethod
    def get(cls, code):
        """
        Returns the network status corresponding to the given code.

        Args:
            code (Integer): Network status code.

        Returns:
            :class:`.NetStatus`: Network status corresponding to the given code,
                `None` if not found.
        """
        for network_status in cls:
            if code == network_status.code:
                return network_status

        return None


class NetworkProfile:
    """
    Network profile to apply to an interface.
    """
    def __init__(self):
        self.__connect = None
        self.__mode = None
        self.__ipv4 = None
        self.__netmask = None
        self.__gateway = None
        self.__dns1 = None
        self.__dns2 = None

    @property
    def connect(self):
        """
         Gets the profile connection.

         Returns:
            Boolean: `True` to connect, `False` to disconnect, `None` to do nothing.
        """
        return self.__connect

    @connect.setter
    def connect(self, value):
        """
         Sets the profile connection.

         Params:
             value (Boolean): `True` to connect, `False` to disconnect,
                `None` to do nothing.

        Raises:
            ValueError: If `value` is invalid.
        """
        if value is not None and not isinstance(value, bool):
            raise ValueError("Value must be a boolean")

        self.__connect = value

    @property
    def mode(self):
        """
         Gets the profile connection mode.

         Returns:
            :class:`.IPMode`: Mode to connect, `None` to do nothing.
        """
        return self.__mode

    @mode.setter
    def mode(self, value):
        """
         Sets the profile connection mode.

         Params:
            value (:class:`.IPMode`): Connection mode, `None` to do nothing.

        Raises:
            ValueError: If `value` is invalid.
        """
        if value is not None and not isinstance(value, IPMode):
            raise ValueError("Value must be a IPMode")

        self.__mode = value

    @property
    def ipv4(self):
        """
         Gets the profile connection IPv4.

         Returns:
            :class:`.IPv4Address`: IPv4 to configure, `None` to do nothing.
        """
        return self.__ipv4

    @ipv4.setter
    def ipv4(self, value):
        """
         Sets the profile connection IPv4.

         Params:
            value (:class:`.IPv4Address`, String, or Integer): A string, a
                32-bits integer, or a `IPv4Address` object. `None` to do nothing.

        Raises:
            AddressValueError: If `value` is an invalid IPv4 address.
        """
        if value is not None and not isinstance(value, IPv4Address):
            self.__ipv4 = IPv4Address(value)
        else:
            self.__ipv4 = value

    @property
    def netmask(self):
        """
         Gets the profile connection network mask.

         Returns:
            :class:`.IPv4Address`: Network mask to configure, `None` to do nothing.
        """
        return self.__netmask

    @netmask.setter
    def netmask(self, value):
        """
         Sets the profile connection network mask.

         Params:
            value (:class:`.IPv4Address`, String or Integer): An integer
                representing the prefix length (e.g. 24) or a string
                representing the prefix mask (e.g. 255.255.255.0) or a
                `IPv4Address` object. `None` to do nothing.

        Raises:
            AddressValueError: If `value` is an invalid IPv4 address.
            NetmaskValueError: If `value` is an invalid netmask address.
        """
        if value is not None:
            if isinstance(value, IPv4Address):
                value = str(value)
            self.__netmask = IPv4Interface(("127.0.0.1", value)).netmask
        else:
            self.__netmask = value

    @property
    def gateway(self):
        """
         Gets the profile default gateway IPv4.

         Returns:
            :class:`.IPv4Address`: Gateway IPv4 to configure, `None` to do nothing.
        """
        return self.__gateway

    @gateway.setter
    def gateway(self, value):
        """
         Sets the profile default gateway IPv4.

         Params:
            value (:class:`.IPv4Address`, String, or Integer): A string, a
                32-bits integer, or a `IPv4Address` object. `None` to do nothing.

        Raises:
            AddressValueError: If `value` is an invalid IPv4 address.
        """
        if value is not None and not isinstance(value, IPv4Address):
            self.__gateway = IPv4Address(value)
        else:
            self.__gateway = value

    @property
    def dns1(self):
        """
         Gets the profile DNS1 IPv4.

         Returns:
            :class:`.IPv4Address`: DNS1 IPv4 to configure, `None` to do nothing.
        """
        return self.__dns1

    @dns1.setter
    def dns1(self, value):
        """
         Sets the profile DNS1 IPv4.

         Params:
            value (:class:`.IPv4Address`, String, or Integer): A string, a
                32-bits integer, or a `IPv4Address` object. `None` to do nothing.

        Raises:
            AddressValueError: If `value` is an invalid IPv4 address.
        """
        if value is not None and not isinstance(value, IPv4Address):
            self.__dns1 = IPv4Address(value)
        else:
            self.__dns1 = value

    @property
    def dns2(self):
        """
         Gets the profile DNS2 IPv4.

         Returns:
            :class:`.IPv4Address`: DNS1 IPv4 to configure, `None` to do nothing.
        """
        return self.__dns2

    @dns2.setter
    def dns2(self, value):
        """
         Sets the profile DNS2 IPv4.

         Params:
            value (:class:`.IPv4Address`, String, or Integer): A string, a
                32-bits integer, or a `IPv4Address` object. `None` to do nothing.

        Raises:
            AddressValueError: If `value` is an invalid IPv4 address.
        """
        if value is not None and not isinstance(value, IPv4Address):
            self.__dns2 = IPv4Address(value)
        else:
            self.__dns2 = value

    def _get_cfg_struct(self):
        """
        Returns a :class:`._NetConfigStruct`.

        Returns:
            :class:`._NetConfigStruct`: Network configuration struct.
        """
        cfg_struct = _NetConfigStruct()

        val = NetStatus.UNKNOWN
        if self.__connect is not None:
            val = NetStatus.get(int(not self.__connect))
        cfg_struct.status = val.code if not None else NetStatus.UNKNOWN.code

        cfg_struct.is_dhcp = IPMode.UNKNOWN.code
        if self.__mode is not None:
            cfg_struct.is_dhcp = self.__mode.code

        cfg_struct.set_ip = self.__ipv4 is not None
        array = c_uint8 * _IPV4_GROUPS
        cfg_struct.ipv4 = array(*bytes((0, 0, 0, 0)))
        if cfg_struct.set_ip:
            cfg_struct.ipv4 = array(*self.__ipv4.packed)

        cfg_struct.set_netmask = self.__netmask is not None
        cfg_struct.netmask = array(*bytes((0, 0, 0, 0)))
        if cfg_struct.set_netmask:
            cfg_struct.netmask = array(*self.__netmask.packed)

        cfg_struct.set_gateway = self.__gateway is not None
        cfg_struct.gateway = array(*bytes((0, 0, 0, 0)))
        if cfg_struct.set_gateway:
            cfg_struct.gateway = array(*self.__gateway.packed)

        cfg_struct.n_dns = 0
        cfg_struct.dns1 = array(*bytes((0, 0, 0, 0)))
        if self.__dns1:
            cfg_struct.dns1 = array(*self.__dns1.packed)
            cfg_struct.n_dns += 1

        cfg_struct.dns2 = array(*bytes((0, 0, 0, 0)))
        if self.__dns2:
            cfg_struct.dns2 = array(*self.__dns2.packed)
            cfg_struct.n_dns += 1

        return cfg_struct


class _NetworkError(Enum):
    """
    Enumeration class listing all the network errors.
    """
    NONE = (0, "None", "No error")
    NO_EXIST = (1, "Interface does not exist", "The requested interface does not exist")
    NO_MEM = (2, "No memory", "Not enough memory to read the network state")
    NO_IFACES = (3, "Interfaces not available", "There is not any available interface")
    STATE_ERROR = (4, "State error", "Unable to get network interface state")
    MAC = (5, "MAC error", "Error getting the interface MAC address")
    IP = (6, "IP error", "Error getting/setting the interface IP address")
    NETMASK = (7, "Network mask error", "Error getting/setting the interface network mask")
    GATEWAY = (8, "Gateway error", "Error getting/setting the default gateway address")
    DNS = (9, "DNS error", "Error getting/setting the DNS address")
    MTU = (10, "MTU error", "Error reading the MTU value")
    STATS = (11, "Statistics error", "Error reading the interface statistics")
    NO_CONFIGURABLE = (12, "No configurable", "The interface is not configurable")
    CONFIG_ERROR = (13, "Configuration error", "Unable to configure network interface")

    def __init__(self, code, title, description):
        """
        Class constructor. Instantiates a new `_NetworkError` entry with the provided parameters.

        Args:
            code (Integer): Network error code.
            title (String): Network error title.
            description (String): Network error description.
        """
        self.__code = code
        self.__title = title
        self.__description = description

    @property
    def code(self):
        """
        Returns the network error code.

        Returns:
            Integer: Network error code.
        """
        return self.__code

    @property
    def title(self):
        """
        Returns the network error title.

        Returns:
            String: Network error title.
        """
        return self.__title

    @property
    def description(self):
        """
        Returns the network error description.

        Returns:
            String: Network error description.
        """
        return self.__description

    @classmethod
    def get(cls, code):
        """
        Returns the network error corresponding to the given code.

        Args:
            code (Integer): Network error code.

        Returns:
            :class:`._NetworkError`: Network error corresponding to the given
                code, `None` if not found.
        """
        for network_error in cls:
            if code == network_error.code:
                return network_error

        return None


class _NetNamesListStruct(Structure):
    """
    Internal class to store C library network interface names list struct data.
    """
    _fields_ = [('n_ifaces', c_int),
                ('names', (c_char * INTERFACE_NAME_SIZE) * MAX_IFACES)]


class _NetStatsStruct(Structure):
    """
    Internal class to store C library network statistics struct data.
    """
    _fields_ = [('rx_packets', c_uint32),
                ('tx_packets', c_uint32),
                ('rx_bytes', c_uint32),
                ('tx_bytes', c_uint32),
                ('rx_errors', c_uint32),
                ('tx_errors', c_uint32),
                ('rx_dropped', c_uint32),
                ('tx_dropped', c_uint32),
                ('multicast', c_uint32),
                ('collisions', c_uint32),
                # Detailed rx_errors.
                ('rx_length_errors', c_uint32),
                ('rx_over_errors', c_uint32),
                ('rx_crc_errors', c_uint32),
                ('rx_frame_errors', c_uint32),
                ('rx_fifo_errors', c_uint32),
                ('rx_missed_errors', c_uint32),
                # Detailed tx_errors.
                ('tx_aborted_errors', c_uint32),
                ('tx_carrier_errors', c_uint32),
                ('tx_fifo_errors', c_uint32),
                ('tx_heartbeat_errors', c_uint32),
                ('tx_window_errors', c_uint32),
                # For cslip etc.
                ('rx_compressed', c_uint32),
                ('tx_compressed', c_uint32),
                # Other.
                ('rx_nohandler', c_uint32)]


class _NetStateStruct(Structure):
    """
    Internal class to store C library network state struct data.
    """
    _fields_ = [('name', c_char * INTERFACE_NAME_SIZE),
                ('mac', c_uint8 * MAC_ADDRESS_GROUPS),
                ('status', c_int),
                ('is_dhcp', c_int),
                ('ipv4', c_uint8 * _IPV4_GROUPS),
                ('gateway', c_uint8 * _IPV4_GROUPS),
                ('netmask', c_uint8 * _IPV4_GROUPS),
                ('broadcast', c_uint8 * _IPV4_GROUPS),
                ('mtu', c_int),
                ('dns1', c_uint8 * _IPV4_GROUPS),
                ('dns2', c_uint8 * _IPV4_GROUPS)]


class _NetConfigStruct(Structure):
    """
    Internal class to store C library network configuration struct data.
    """
    _fields_ = [('name', c_char * INTERFACE_NAME_SIZE),
                ('status', c_int),
                ('is_dhcp', c_int),
                ('set_ip', c_bool),
                ('ipv4', c_uint8 * _IPV4_GROUPS),
                ('set_gateway', c_bool),
                ('gateway', c_uint8 * _IPV4_GROUPS),
                ('set_netmask', c_bool),
                ('netmask', c_uint8 * _IPV4_GROUPS),
                ('n_dns', c_uint8),
                ('dns1', c_uint8 * _IPV4_GROUPS),
                ('dns2', c_uint8 * _IPV4_GROUPS)]


class NetworkStats:
    """
    This class represents the statistics of a network interface.
    Use `get_statistics()` method from your `NetworkInterface`to get statistics.
    """

    __create_key = object()

    def __init__(self, iface_name, _key):
        """
        Class constructor. This method should not be called directly, instead
        use `get_statistics()` method from your `NetworkInterface`to get statistics.
        """
        if _key is not NetworkStats.__create_key:
            raise RuntimeError(
                "NetworkStats objects must be created using NetworkInterface.get_statistics()")
        self._iface_name = iface_name
        self._net_stats_struct = None

    @property
    def rx_packets(self):
        """
        Returns the number of good packets received by the interface

        Returns:
            Integer: Number of good packets received by the interface
        """
        return self._net_stats_struct.rx_packets

    @property
    def tx_packets(self):
        """
        Returns the number of interface sent packets.

        Returns:
            Integer: Number of interface sent packets.
        """
        return self._net_stats_struct.tx_packets

    @property
    def rx_bytes(self):
        """
        Returns the number of good received bytes by the interface,
        corresponding to `rx_packets`.

        Returns:
            Integer: Number of good received bytes.
        """
        return self._net_stats_struct.rx_bytes

    @property
    def tx_bytes(self):
        """
        Returns the number of good transmitted bytes by the interface,
        corresponding to `tx_packets`.

        Returns:
            Integer: Number of good transmitted bytes.
        """
        return self._net_stats_struct.tx_bytes

    @property
    def rx_errors(self):
        """
        Returns the total number of received bad packets by the interface.

        Returns:
            Integer: Total number of received bad packets.
        """
        return self._net_stats_struct.rx_errors

    @property
    def tx_errors(self):
        """
        Returns the total number of transmit problems in the interface.

        Returns:
            Integer: Total number of transmit problems in the interface.
        """
        return self._net_stats_struct.tx_errors

    @property
    def rx_dropped_packets(self):
        """
        Returns the number of packets received but not processed by the interface.

        Returns:
            Integer: Number of received packets not processed by the interface.
        """
        return self._net_stats_struct.rx_dropped

    @property
    def tx_dropped_packets(self):
        """
        Returns the number of packets dropped on their way to transmission in
        the interface.

        Returns:
            Integer: Number of packets dropped on their way to transmission.
        """
        return self._net_stats_struct.tx_dropped

    @property
    def rx_multicast_packets(self):
        """
        Returns the number of multicast packets received by the interface.

        Returns:
            Integer: Number of multicast packets received by the interface.
        """
        return self._net_stats_struct.multicast

    @property
    def tx_collisions(self):
        """
        Returns the number of collisions during packet transmissions in the
        interface.

        Returns:
            Integer: Number of collisions during packet transmissions.
        """
        return self._net_stats_struct.collisions

    @property
    def rx_length_errors(self):
        """
        Returns the number of received packets dropped due to invalid length by
        the interface.

        Returns:
            Integer: Number of received packets dropped due to invalid length.
        """
        return self._net_stats_struct.rx_length_errors

    @property
    def rx_over_errors(self):
        """
        Returns the number of overflow count events received by the interface.
        This statistic corresponds to hardware events and is not commonly used
        on software devices.

        Returns:
            Integer: Number of overflow count events received by the interface.
        """
        return self._net_stats_struct.rx_over_errors

    @property
    def rx_crc_errors(self):
        """
        Returns the number of received packets with a CRC error by the interface.

        Returns:
            Integer: Number of packets received with a CRC error.
        """
        return self._net_stats_struct.rx_crc_errors

    @property
    def rx_frame_errors(self):
        """
        Returns the number of received packets with a frame alignment error by
        the interface.

        Returns:
            Integer: Number of packets received with a frame alignment error.
        """
        return self._net_stats_struct.rx_frame_errors

    @property
    def rx_fifo_errors(self):
        """
        Returns the number of FIFO error count events received by the interface.
        This statistic is used on software devices, e.g. to count software
        packet queue overflow (can) or sequencing errors (GRE).

        Returns:
            Integer: Number of FIFO error count events during reception.
        """
        return self._net_stats_struct.rx_fifo_errors

    @property
    def rx_missed_errors(self):
        """
        Returns the number of packets dropped by the interface due to lack
        of buffer space.

        Returns:
            Integer: Number of packets dropped due to lack of buffer space.
        """
        return self._net_stats_struct.rx_missed_errors

    @property
    def tx_aborted_errors(self):
        """
        Returns the number of packets not sent by the interface due to an
        abort error.

        Returns:
            Integer: Number of packets not sent due to an abort error.
        """
        return self._net_stats_struct.tx_aborted_errors

    @property
    def tx_carrier_errors(self):
        """
        Returns the number of frame transmission errors due to loss of carrier
        during transmission in the interface.

        Returns:
            Integer: Number of frame transmission errors due to loss of carrier.
        """
        return self._net_stats_struct.tx_carrier_errors

    @property
    def tx_fifo_errors(self):
        """
        Returns the number of frame transmission errors due to FIFO
        underrun/underflow in the interface.

        Returns:
            Integer: Number of frame transmission errors due to FIFO underrun/underflow.
        """
        return self._net_stats_struct.tx_fifo_errors

    @property
    def tx_heartbeat_errors(self):
        """
        Returns the number of Heartbeat/SQE Test errors for old half-duplex
        Ethernet interfaces.

        Returns:
            Integer: Number of Heartbeat/SQE Test errors.
        """
        return self._net_stats_struct.tx_heartbeat_errors

    @property
    def tx_window_errors(self):
        """
        Returns the number of frame transmission errors due to late collisions
        in the interface.

        Returns:
            Integer: Number of frame transmission errors due to late collisions.
        """
        return self._net_stats_struct.tx_window_errors

    @property
    def rx_compressed_packets(self):
        """
        Returns the number of correctly compressed packets received by the interface.

        Returns:
            Integer: Number of correctly compressed packets received.
        """
        return self._net_stats_struct.rx_compressed

    @property
    def tx_compressed_packets(self):
        """
        Returns the number of compressed packets transmitted by the interface.

        Returns:
            Integer: Number of compressed packets transmitted.
        """
        return self._net_stats_struct.tx_compressed

    @property
    def rx_nohandler_packets(self):
        """
        Returns the number of received packets but dropped by the networking
        stack because the device is not designated to receive packets.

        Returns:
            Integer: Number of received packets but dropped by the networking stack.
        """
        return self._net_stats_struct.rx_nohandler

    def update(self):
        """
        Reads and updates the interface statistics.

        Raises:
            DigiAPIXException: If there is any error loading the library.
            NetworkException: If there is any error reading the interface statistics.
        """
        _check_library()

        net_stats = _NetStatsStruct()
        error = _NetworkError.get(
            _libdigiapix.ldx_net_get_iface_stats(
                self._iface_name.encode(encoding="ascii", errors="ignore"), net_stats))
        if error is not _NetworkError.NONE:
            raise NetworkException(error.description)

        self._net_stats_struct = net_stats


class NetworkInterface:
    """
    This class represents the information of a network interface.
    """

    __create_key = object()

    def __init__(self, _key, _net_state_struct):
        """
        Class constructor. This method should not be called directly, instead
        use `get()` method.
        """
        if _key is not NetworkInterface.__create_key:
            raise RuntimeError(
                "NetworkInterface objects must be created using get() method")
        self._net_state_struct = _net_state_struct
        self._net_stats = None
        self._ipv4_iface = IPv4Interface(
            (bytes(self._net_state_struct.ipv4), self._address_to_string(self._net_state_struct.netmask)))

    @staticmethod
    def list_interfaces():
        """
        Lists the available network interfaces.

        Returns:
            List: List of available network interfaces, `None` if no interface
                is available.

        Raises:
            DigiAPIXException: If there is any error loading the library.
        """
        _check_library()

        net_list = _NetNamesListStruct()
        num_ifaces = _libdigiapix.ldx_net_list_available_ifaces(net_list)
        if num_ifaces < 1:
            return None

        return [net_list.names[i].value.decode(encoding="ascii", errors="ignore")
                for i in range(net_list.n_ifaces)]

    @classmethod
    def get(cls, name):
        """
        Returns the information of the given interface.

        Params:
            name (String): Name of the interface to get.

        Returns:
            :class:`.NetworkInterface`: The network interface.

        Raises:
            ValueError: If `name` is not a string or is invalid.
            DigiAPIXException: If there is any error loading the library.
            NetworkException: If there is any error getting the interface information.
        """
        # Sanity checks.
        if not isinstance(name, str):
            raise ValueError("Interface name must be a valid string")

        _check_library()

        net_state = _NetStateStruct()
        error = _NetworkError.get(_libdigiapix.ldx_net_get_iface_state(
            name.encode(encoding="ascii", errors="ignore"), net_state))
        if error is not _NetworkError.NONE:
            raise NetworkException(error.description)

        return NetworkInterface(cls.__create_key, net_state)

    def configure(self, profile):
        """
        Configures the interface with the provided profile.

        Params:
            profile (:class:`.NetworkProfile`): Profile to apply.

        Raises:
            ValueError: If `profile` is invalid.
            DigiAPIXException: If there is any error loading the library.
            NetworkException: If there is any error configuring the interface.
        """
        if not isinstance(profile, NetworkProfile):
            raise ValueError("Profile must be a NetworkProfile")

        cfg_struct = profile._get_cfg_struct()
        cfg_struct.name = self._net_state_struct.name

        _check_library()

        error = _NetworkError.get(_libdigiapix.ldx_net_set_config(cfg_struct))
        if error is not _NetworkError.NONE:
            raise NetworkException(error.description)

    @property
    def name(self):
        """
        Returns the interface name.

        Returns:
            String: Interface name.
        """
        return self._net_state_struct.name.decode(encoding="ascii")

    @property
    def mac(self):
        """
        Returns the interface MAC address.

        Returns:
            :class:`.MacAddress`: The interface MAC address.
        """
        return MacAddress(self._net_state_struct.mac)

    @property
    def status(self):
        """
        Returns the interface status.

        Returns:
            :class:`.NetStatus`: The interface status.
        """
        status = NetStatus.get(self._net_state_struct.status)
        if status is None:
            return NetStatus.UNKNOWN

        return status

    @property
    def ip_mode(self):
        """
        Returns the IP mode used by the interface.

        Returns:
            :class:`.IPMode`: The IP mode.
        """
        mode = IPMode.get(self._net_state_struct.is_dhcp)
        if mode is None:
            return IPMode.UNKNOWN

        return mode

    @property
    def ipv4(self):
        """
        Returns the IP address of the interface.

        Returns:
            :class:`.IPv4Address`: The interface IP address.
        """
        return self._ipv4_iface.ip

    @property
    def gateway(self):
        """
        Returns the default gateway address of the interface.

        Returns:
            :class:`.IPv4Address`: The interface default gateway address.
        """
        return IPv4Address(bytes(self._net_state_struct.gateway))

    @property
    def netmask(self):
        """
        Returns the netmask address of the interface.

        Returns:
            :class:`.IPv4Address`: The interface netmask address.
        """
        return self._ipv4_iface.netmask

    @property
    def broadcast(self):
        """
        Returns the broadcast address of the interface.

        Returns:
            :class:`.IPv4Address`: The interface broadcast address.
        """
        return IPv4Address(bytes(self._net_state_struct.broadcast))

    @property
    def dns1(self):
        """
        Returns the DNS 1 address of the interface.

        Returns:
            :class:`.IPv4Address`: The interface DNS 1 address.
        """
        return IPv4Address(bytes(self._net_state_struct.dns1))

    @property
    def dns2(self):
        """
        Returns the DNS 2 address of the interface.

        Returns:
            :class:`.IPv4Address`: The interface DNS 2 address.
        """
        return IPv4Address(bytes(self._net_state_struct.dns2))

    @property
    def mtu(self):
        """
        Returns the interface MTU (Maximum Transmission Unit).

        Returns:
            Integer: The interface MTU value.
        """
        return self._net_state_struct.mtu

    @classmethod
    def _address_to_string(cls, address):
        """
        Converts the given address to string.

        Params:
            address (:class:`.c_ubyte_Array`): Address to convert.

        Returns:
            String: The given address as string.
        """
        return ".".join(f"{block}" for block in address)

    def get_statistics(self):
        """
        Returns the statistics of the given interface.

        Returns:
            :class:`.NetworkStats: The interface statistics.

        Raises:
            DigiAPIXException: If there is any error loading the library.
            NetworkException: If there is any error reading the interface statistics.
        """
        if self._net_stats is None:
            name = self._net_state_struct.name.decode(encoding="ascii")
            self._net_stats = NetworkStats(name, NetworkStats._NetworkStats__create_key)
            self._net_stats.update()

        return self._net_stats


def _check_library():
    """
    Verifies that the 'digiapix' library is loaded.

    Raises:
        DigiAPIXException: If there is any error loading the library.
    """
    global _libdigiapix
    if _libdigiapix is None:
        _libdigiapix = library.get_library()
        _configure_network_ctypes()


def _configure_network_ctypes():
    """
    Configures the ctypes for the library network methods
    """
    # Get available interfaces.
    _libdigiapix.ldx_net_list_available_ifaces.argtypes = [POINTER(_NetNamesListStruct)]
    _libdigiapix.ldx_net_list_available_ifaces.restype = c_int
    # Get interface state.
    _libdigiapix.ldx_net_get_iface_state.argtypes = [c_char_p, POINTER(_NetStateStruct)]
    _libdigiapix.ldx_net_get_iface_state.restype = c_int
    # Get interface statistics.
    _libdigiapix.ldx_net_get_iface_stats.argtypes = [c_char_p, POINTER(_NetStatsStruct)]
    _libdigiapix.ldx_net_get_iface_stats.restype = c_int
    # Set interface configuration.
    _libdigiapix.ldx_net_set_config.argtypes = [_NetConfigStruct]
    _libdigiapix.ldx_net_set_config.restype = c_int
