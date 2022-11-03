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
import re

from enum import Enum

# Constants.
INTERFACE_NAME_SIZE = 16
MAC_ADDRESS_GROUPS = 6
MAX_IFACES = 32


class MacAddress:
    """
    This class represents a MAC address.

    The address is a unique device address assigned during manufacturing.
    This address is unique to each physical device.
    """

    __PATTERN = "^(0[xX])?[0-9a-fA-F]{1,16}$"
    """
    64-bit address string pattern.
    """

    __REGEXP = re.compile(__PATTERN)

    def __init__(self, address):
        """
        Class constructor. Instantiates a new :class:`.MacAddress` object
        with the provided parameters.

        Args:
            address (Bytearray): the MAC address as byte array.

        Raise:
            ValueError: if `address` is `None` or its length less than 1 or greater than 8.
        """
        if not address:
            raise ValueError("Address must contain at least 1 byte")
        if len(address) > 8:
            raise ValueError("Address cannot contain more than 8 bytes")

        self.__address = bytearray(8)
        diff = 8 - len(address)
        for i in range(diff):
            self.__address[i] = 0
        for i in range(diff, 8):
            self.__address[i] = address[i - diff]

    @classmethod
    def from_hex_string(cls, address):
        """
        Class constructor. Instantiates a new :class:`.MacAddress`
        object from the provided hex string.

        Args:
            address (String): The address as a string.

        Raises:
            ValueError: if the address' length is less than 1 or does not match
                with the pattern: `(0[xX])?[0-9a-fA-F]{1,16}`.
        """
        if not address:
            raise ValueError("Address must contain at least 1 byte")
        addr = address.replace(':', '')
        if not cls.__REGEXP.match(addr):
            raise ValueError("Address must follow this pattern: " + cls.__PATTERN)

        aux = int(addr, 16)
        return cls(aux.to_bytes((aux.bit_length() + 7) // 8, byteorder='big'))

    @classmethod
    def from_bytes(cls, *args):
        """
        Class constructor. Instantiates a new :class:`.MacAddress`
        object from the provided bytes.

        Args:
            args (8 Integers): 8 integers that represent the bytes 1 to 8 of
                this MacAddress.

        Raises:
            ValueError: if the amount of arguments is not 8 or if any of the
                arguments is not between 0 and 255.
        """
        if len(args) != 8:
            raise ValueError("Number of bytes given as arguments must be 8.")
        for i, val in enumerate(args):
            if val > 255 or val < 0:
                raise ValueError("Byte " + str(i + 1) + " must be between 0 and 255")

        return cls(bytearray(args))

    @classmethod
    def is_valid(cls, address):
        """
        Checks if the provided hex string is a valid MAC address.

        Args:
            address (String, Bytearray, or :class:`.MacAddress`):
                String: String with the address only with hex digits without
                blanks. Minimum 1 character, maximum 16 (64-bit).
                Bytearray: Address as byte array. Must be 1-8 digits.

        Returns
            Boolean: `True` for a valid 64-bit address, `False` otherwise.
        """
        if isinstance(address, MacAddress):
            return True

        if isinstance(address, bytearray):
            return 1 <= len(address) <= 8

        if isinstance(address, str):
            addr = address.replace(':', '')
            return bool(cls.__REGEXP.match(addr))

        return False

    def __str__(self):
        """
        Called by the str() built-in function and by the print statement to
        compute the "informal" string representation of an object. This differs
        from __repr__() in that it does not have to be a valid Python
        expression: a more convenient or concise representation may be used instead.

        Returns:
            String: "informal" representation of this XBee64BitAddress.
        """
        return "".join(["%02X" % i for i in self.__address])

    def __hash__(self):
        """
        Returns a hash code value for the object.

        Returns:
            Integer: hash code value for the object.
        """
        res = 23
        for byte in self.__address:
            res = 31 * (res + byte)
        return res

    def __eq__(self, other):
        """
        Operator ==

        Args:
            other: another MacAddress.

        Returns:
            Boolean: `True` if self and other have the same value and type, `False` in other case.
        """
        if other is None:
            return False
        if not isinstance(other, MacAddress):
            return False

        return self.address == other.address

    def __iter__(self):
        """
        Gets an iterator class of this instance address.

        Returns:
            Iterator: iterator of this address.
        """
        return self.__address.__iter__()

    @property
    def address(self):
        """
        Returns a bytearray representation of this MacAddress.

        Returns:
            Bytearray: bytearray representation of this MacAddress.
        """
        return bytearray(self.__address)


class _AbstractEnum(Enum):
    """
    Abstract enumeration class.
    """

    def __init__(self, code, title, description):
        """
        Class constructor. Instantiates a new `_AbstractEnum` entry with the provided parameters.

        Args:
            code (Integer): Enum entry code.
            title (String): Enum entry title.
            description (String): Enum entry description.
        """
        self.__code = code
        self.__title = title
        self.__description = description

    @property
    def code(self):
        """
        Returns the enum entry code.

        Returns:
            Integer: Enum entry code.
        """
        return self.__code

    @property
    def title(self):
        """
        Returns the enum entry title.

        Returns:
            String: Enum entry title.
        """
        return self.__title

    @property
    def description(self):
        """
        Returns the enum entry description.

        Returns:
            String: Enum entry description.
        """
        return self.__description

    @classmethod
    def get(cls, code):
        """
        Returns the enum entry corresponding to the given code.

        Args:
            code (Integer): Enum entry code.

        Returns:
            :class:`._AbstractEnum`: Enum entry corresponding to the given
                code, `None` if not found.
        """
        for enum_entry in cls:
            if code == enum_entry.code:
                return enum_entry

        return None


class RequestMode(_AbstractEnum):
    """
    Enumeration class listing all the request modes.
    """
    SHARED = (0, "Shared", "If the device is already exported it will not be unexported on free")
    GREEDY = (1, "Greedy", "The device will be always unexported on free")
    WEAK = (2, "Weak", "If the device is already exported, the request will fail")

    def __init__(self, code, title, description):
        """
        Class constructor. Instantiates a new `RequestMode` entry with the provided parameters.

        Args:
            code (Integer): Request mode code.
            title (String): Request mode title.
            description (String): Request mode description.
        """
        super().__init__(code, title, description)
