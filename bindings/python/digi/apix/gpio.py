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

from ctypes import c_char_p, c_int, c_ubyte, c_uint32, c_void_p, CFUNCTYPE, POINTER, Structure

from digi.apix import library
from digi.apix.common import RequestMode
from digi.apix.exceptions import DigiAPIXException

# Constants.
_ERROR_ACTIVE_MODE = "Active mode must be a valid GPIOActiveMode entry"
_ERROR_ALIAS = "Alias must be a valid string"
_ERROR_CONTROLLER = "Controller must be a valid string"
_ERROR_DEBOUNCE = "Debounce time must be positive integer"
_ERROR_KERNEL_NUMBER = "Kernel number must be a positive integer"
_ERROR_LINE = "Line must be a positive integer"
_ERROR_MODE = "Mode must be a valid GPIOMode entry"
_ERROR_REQUEST_MODE = "Request mode must be a valid RequestMode entry"
_ERROR_VALUE = "Value must be a valid GPIOValue entry"

_IRQ_CALLBACK = CFUNCTYPE(c_int, c_void_p)

# Variables.
_libdigiapix = None


class GPIOException(DigiAPIXException):
    """
    Exception thrown when an error occurs working with GPIOs
    """


class GPIOMode(Enum):
    """
    Enumeration class listing all the GPIO modes.
    """
    INPUT = (0, "Input", "GPIO value can be read")
    OUTPUT_LOW = (1, "Output low", "Output mode with low as initial value, output can be written")
    OUTPUT_HIGH = (2, "Output high", "Output mode with high as initial value, output can be"
                                     "written")
    IRQ_EDGE_RISING = (3, "Interrupt rising edge", "Interrupt is triggered on rising edge, from"
                                                   "low to high")
    IRQ_EDGE_FALLING = (4, "Interrupt falling edge", "Interrupt is triggered on falling edge, from"
                                                     "high to low")
    IRQ_EDGE_BOTH = (5, "Interrupt both edges", "Interrupt is triggered on rising and falling"
                                                "edges")

    def __init__(self, code, title, description):
        """
        Class constructor. Instantiates a new `GPIOMode` entry with the provided parameters.

        Args:
            code (Integer): GPIO mode code.
            title (String): GPIO mode title.
            description (String): GPIO mode description.
        """
        self.__code = code
        self.__title = title
        self.__description = description

    @property
    def code(self):
        """
        Returns the GPIO mode code.

        Returns:
            Integer: GPIO mode code.
        """
        return self.__code

    @property
    def title(self):
        """
        Returns the GPIO mode title.

        Returns:
            String: GPIO mode title.
        """
        return self.__title

    @property
    def description(self):
        """
        Returns the GPIO mode description.

        Returns:
            String: GPIO mode description.
        """
        return self.__description

    @classmethod
    def get(cls, code):
        """
        Returns the GPIO mode corresponding to the given code.

        Args:
            code (Integer): GPIO mode code.

        Returns:
            :class:`.GPIOMode`: GPIO mode corresponding to the given code, `None` if not found.
        """
        for gpio_mode in cls:
            if code == gpio_mode.code:
                return gpio_mode
        return None


class GPIOValue(Enum):
    """
    Enumeration class listing all the GPIO values.
    """
    LOW = (0, "Low")
    HIGH = (1, "High")

    def __init__(self, code, description):
        """
        Class constructor. Instantiates a new `GPIOValue` entry with the provided parameters.

        Args:
            code (Integer): GPIO value code.
            description (String): GPIO value description.
        """
        self.__code = code
        self.__description = description

    @property
    def code(self):
        """
        Returns the GPIO value code.

        Returns:
            Integer: GPIO value code.
        """
        return self.__code

    @property
    def description(self):
        """
        Returns the GPIO value description.

        Returns:
            String: GPIO value description.
        """
        return self.__description

    @classmethod
    def get(cls, code):
        """
        Returns the GPIO value corresponding to the given code.

        Args:
            code (Integer): GPIO value code.

        Returns:
            :class:`.GPIOValue`: GPIO value corresponding to the given code, `None` if not found.
        """
        for gpio_value in cls:
            if code == gpio_value.code:
                return gpio_value
        return None


class GPIOActiveMode(Enum):
    """
    Enumeration class listing all the GPIO active modes.
    """
    HIGH = (0, "High")
    LOW = (1, "Low")

    def __init__(self, code, description):
        """
        Class constructor. Instantiates a new `GPIOActiveMode` entry with the provided parameters.

        Args:
            code (Integer): GPIO active mode code.
            description (String): GPIO active mode description.
        """
        self.__code = code
        self.__description = description

    @property
    def code(self):
        """
        Returns the GPIO active mode code.

        Returns:
            Integer: GPIO active mode code.
        """
        return self.__code

    @property
    def description(self):
        """
        Returns the GPIO active mode description.

        Returns:
            String: GPIO active mode description.
        """
        return self.__description

    @classmethod
    def get(cls, code):
        """
        Returns the GPIO active mode corresponding to the given code.

        Args:
            code (Integer): GPIO active mode code.

        Returns:
            :class:`.GPIOActiveMode`: GPIO active mode corresponding to the given code, `None` if
                                      not found.
        """
        for gpio_active_mode in cls:
            if code == gpio_active_mode.code:
                return gpio_active_mode
        return None


class _GPIOStruct(Structure):
    """
    Internal class to store C library GPIO struct data.
    """
    _fields_ = [('alias', c_char_p),
                ('kernel_number', c_uint32),
                ('gpio_controller', c_char_p),
                ('gpio_line', c_uint32),
                ('data', c_void_p)]


class GPIO:
    """
    This class represents a system GPIO. Instances of GPIO should be created using the
    'GPIO.create_from_<x>' family of functions.
    """

    def __init__(self, gpio_struct):
        """
        Class constructor. This method should not be called directly, instead use the
        'GPIO.create_from_<x>' family of functions.
        """
        self._gpio_struct = gpio_struct
        self._irq_callback = None
        self._irq_callbacks = []
        self._irq_thread_running = False

    def __del__(self):
        """
        Executed when garbage collector collects this item.
        """
        # Check if interrupt thread is running.
        if self._irq_thread_running:
            self._stop_interrupts_thread()
        # Free GPIO struct.
        _libdigiapix.ldx_gpio_free(self._gpio_struct)

    @classmethod
    def create_from_controller(cls, controller, line, mode):
        """
        Creates a new GPIO using the given controller information.

        Args:
            controller (String): The controller name or alias of the GPIO to request.
            line (Integer): The line number of the GPIO to request.
            mode (:class:`.GPIOMode`): The desired GPIO working mode.

        Returns:
            :class:`.GPIO`: The instantiated GPIO.

        Raises:
            ValueError: if `controller` is not a valid string or
                        if `line` is not a positive integer or
                        if `mode` is not a valid :class:`.GPIOMode`.
            DigiAPIXException: if there is any error loading the library.
            GPIOException: if there is any error creating the GPIO.
        """
        # Sanity checks.
        if not isinstance(controller, str):
            raise ValueError(_ERROR_CONTROLLER)
        if not isinstance(line, int) or line < 0:
            raise ValueError(_ERROR_LINE)
        if not isinstance(mode, GPIOMode):
            raise ValueError(_ERROR_MODE)
        _check_library()

        gpio_struct = _libdigiapix.ldx_gpio_request_by_controller(
            controller.encode(encoding="ascii", errors="ignore"), line, mode.code)
        if not gpio_struct:
            raise GPIOException("Error creating GPIO")
        return cls(gpio_struct)

    @classmethod
    def create_from_kernel_number(cls, kernel_number, mode, request_mode):
        """
        Requests a new GPIO using the given kernel number.

        Args:
            kernel_number (Integer): The Linux ID number of the GPIO to request.
            mode (:class:`.GPIOMode`): The desired GPIO working mode.
            request_mode (:class:`.RequestMode`): Request mode for opening the GPIO.

        Returns:
            :class:`.GPIO`: The instantiated GPIO.

        Raises:
            ValueError: if `kernel_number` is not a positive integer or
                        if `mode` is not a valid :class:`.GPIOMode` or
                        if `request_mode` is not a valid :class:`.RequestMode`.
            DigiAPIXException: if there is any error loading the library.
            GPIOException: if there is any error creating the GPIO.
        """
        # Sanity checks.
        if not isinstance(kernel_number, int) or kernel_number < 0:
            raise ValueError(_ERROR_KERNEL_NUMBER)
        if not isinstance(mode, GPIOMode):
            raise ValueError(_ERROR_MODE)
        if not isinstance(request_mode, RequestMode):
            raise ValueError(_ERROR_REQUEST_MODE)
        _check_library()

        gpio_struct = _libdigiapix.ldx_gpio_request(kernel_number,
                                                    mode.code,
                                                    request_mode.code)
        if not gpio_struct:
            raise GPIOException("Error creating GPIO")
        return cls(gpio_struct)

    @classmethod
    def create_from_alias(cls, alias, mode, request_mode):
        """
        Creates a GPIO using the given alias information.

        Args:
            alias (String): The alias name of the GPIO to request.
            mode (:class:`.GPIOMode`): The desired GPIO working mode.
            request_mode (:class:`.RequestMode`): Request mode for opening the GPIO. This field is
                                                  ignored if alias is in libgpiod format.

        Returns:
            :class:`.GPIO`: The instantiated GPIO.

        Raises:
            ValueError: if `alias` is not a valid string or
                        if `mode` is not a valid :class:`.GPIOMode` or
                        if `request_mode` is not a valid :class:`.RequestMode`.
            DigiAPIXException: if there is any error loading the library.
            GPIOException: if there is any error creating the GPIO.
        """
        if not isinstance(alias, str):
            raise ValueError(_ERROR_ALIAS)
        if not isinstance(mode, GPIOMode):
            raise ValueError(_ERROR_MODE)
        if not isinstance(request_mode, RequestMode):
            raise ValueError(_ERROR_REQUEST_MODE)
        _check_library()

        gpio_struct = _libdigiapix.ldx_gpio_request_by_alias(
            alias.encode(encoding="ascii", errors="ignore"), mode.code, request_mode.code)
        if not gpio_struct:
            raise GPIOException("Error creating GPIO")
        return cls(gpio_struct)

    @property
    def alias(self):
        """
        Returns the GPIO alias.

        Returns:
            String: GPIO alias.
        """
        return self._gpio_struct.contents.alias.decode(encoding="ascii", errors="ignore")

    @property
    def kernel_number(self):
        """
        Returns the GPIO kernel number.

        Returns:
            Integer: GPIO kernel number.
        """
        return self._gpio_struct.contents.kernel_number

    @property
    def controller(self):
        """
        Returns the GPIO controller.

        Returns:
            String: GPIO controller.
        """
        return self._gpio_struct.contents.gpio_controller.decode(encoding="ascii", errors="ignore")

    @property
    def line(self):
        """
        Returns the GPIO line.

        Returns:
            Integer: GPIO line.
        """
        return self._gpio_struct.contents.gpio_line

    def set_mode(self, mode):
        """
        Changes the GPIO working mode.

        This function configures the given GPIO working mode to be:
            - An input for reading its value: GPIOMode.INPUT
            - An output for setting its value: GPIOMode.OUTPUT_LOW or GPIOMode.OUTPUT_HIGH
            - An interrupt trigger when there is a value change: GPIOMode.IRQ_EDGE_RISING,
              GPIOMode.IRQ_EDGE_FALLING, or GPIOMode.IRQ_EDGE_BOTH

        Args:
            mode (:class:`.GPIOMode`): Working mode to configure.

        Raises:
            ValueError: if `mode` is not a valid :class:`.GPIOMode`.
            GPIOException: if there is any error setting the GPIO mode.
        """
        # Sanity checks.
        if not isinstance(mode, GPIOMode):
            raise ValueError(_ERROR_MODE)

        # Check if interrupts thread is running before changing mode.
        if self._irq_thread_running and mode not in (GPIOMode.IRQ_EDGE_FALLING,
                                                     GPIOMode.IRQ_EDGE_RISING,
                                                     GPIOMode.IRQ_EDGE_BOTH):
            # Stop the interrupt thread as new mode won't support interrupts.
            self._stop_interrupts_thread()
        # Set the new mode.
        if _libdigiapix.ldx_gpio_set_mode(self._gpio_struct, mode.code) != 0:
            raise GPIOException("Error configuring GPIO mode")

    def get_mode(self):
        """
        Gets the GPIO working mode.

        This function retrieves the GPIO working mode:
            - An input: GPIOMode.INPUT
            - An output: GPIOMode.OUTPUT_LOW or GPIOMode.OUTPUT_HIGH
            - Or interrupt generator: GPIOMode.IRQ_EDGE_RISING,
              GPIOMode.IRQ_EDGE_FALLING, or GPIOMode.IRQ_EDGE_BOTH

        Returns:
            :class:`.GPIOMode`: The GPIO working mode.

        Raises:
            GPIOException: if there is any error reading the GPIO mode.
        """
        mode = GPIOMode.get(_libdigiapix.ldx_gpio_get_mode(self._gpio_struct))
        if not mode:
            raise GPIOException("Error reading GPIO mode")
        return mode

    def set_value(self, value):
        """
        Sets the GPIO value.

        This function only has effect if GPIO mode is:
            - GPIOMode.OUTPUT_LOW
            - GPIOMode.OUTPUT_HIGH

        Args:
            value (:class:`.GPIOValue`): The new GPIO value.

        Raises:
            ValueError: if `value` is not a valid :class:`.GPIOValue`.
            GPIOException: if there is any error setting the GPIO value.
        """
        # Sanity checks.
        if not isinstance(value, GPIOValue):
            raise ValueError(_ERROR_VALUE)

        if _libdigiapix.ldx_gpio_set_value(self._gpio_struct, value.code) != 0:
            raise GPIOException("Error setting GPIO value")

    def get_value(self):
        """
        Gets the GPIO value.

        This function only has effect if GPIO mode is:
            - GPIOMode.INPUT
            - GPIOMode.IRQ_EDGE_RISING
            - GPIOMode.IRQ_EDGE_FALLING
            - GPIOMode.IRQ_EDGE_BOTH

        Returns:
            :class:`.GPIOValue`: The GPIO value.

        Raises:
            GPIOException: if there is any error reading the GPIO value.
        """
        value = GPIOMode.get(_libdigiapix.ldx_gpio_get_value(self._gpio_struct))
        if not value:
            raise GPIOException("Error reading GPIO value")
        return value

    def set_active_mode(self, active_mode):
        """
        Sets the GPIO active mode.

        Args:
            active_mode (:class:`.GPIOActiveMode`): New GPIO active mode.

        Raises:
            ValueError: if `mode` is not a valid :class:`.GPIOActiveMode`.
            GPIOException: if there is any error setting the GPIO active mode.
        """
        # Sanity checks.
        if not isinstance(active_mode, GPIOActiveMode):
            raise ValueError(_ERROR_ACTIVE_MODE)

        if _libdigiapix.ldx_gpio_set_active_mode(self._gpio_struct, active_mode.code) != 0:
            raise GPIOException("Error setting GPIO active mode")

    def get_active_mode(self):
        """
        Gets the GPIO active mode.

        Returns:
            :class:`.GPIOActiveMode`: The GPIO active mode.

        Raises:
            GPIOException: if there is any error reading the GPIO active mode.
        """
        active_mode = GPIOMode.get(_libdigiapix.ldx_gpio_get_active_mode(self._gpio_struct))
        if not active_mode:
            raise GPIOException("Error reading GPIO active mode")
        return active_mode

    def set_debounce(self, debounce_time):
        """
        Sets debounce time of the GPIO

        Args:
            debounce_time (Integer): Debounce time in microseconds.

        Raises:
            ValueError: if `debounce_time` is not a positive integer.
            GPIOException: if there is any error setting the debouncing time.
        """
        # Sanity checks.
        if not isinstance(debounce_time, int) or debounce_time < 0:
            raise ValueError(_ERROR_DEBOUNCE)

        if _libdigiapix.ldx_gpio_set_debounce(self._gpio_struct, debounce_time) != 0:
            raise GPIOException("Error setting GPIO debouncing time")

    def wait_for_interrupt(self, timeout):
        """
        Waits synchronously for an interrupt to occur in the GPIO.

        This function blocks for the given amount of milliseconds (or indefinitely for -1) until
        an interrupt on the GPIO is triggered.

        The GPIO must be configured as GPIOMode.IRQ_EDGE_RISING, GPIOMode.IRQ_EDGE_FALLING, or
        GPIOMode.IRQ_EDGE_BOTH, otherwise a 'GPIOException' will be thrown.

        To use a non-blocking interrupt mechanism see 'start_wait_interrupt()' and
        'stop_wait_interrupt()'.

        Args:
            timeout (Integer): The maximum number of milliseconds to wait for an interrupt, -1 for
                               blocking indefinitely.

        Raises:
            GPIOException: if there is an error waiting for interrupt.
        """
        res = _libdigiapix.ldx_gpio_wait_interrupt(self._gpio_struct, timeout)
        if res == 2:
            raise GPIOException("Timeout waiting for interrupt")
        if res != 0:
            raise GPIOException("Error waiting for interrupt")

    def register_interrupt_callback(self, callback):
        """
        Registers the given callback to be notified asynchronously when an interrupt occurs.

        The GPIO must be configured as GPIOMode.IRQ_EDGE_RISING, GPIOMode.IRQ_EDGE_FALLING, or
        GPIOMode.IRQ_EDGE_BOTH, otherwise a 'GPIOException' will be thrown.

        To use a blocking interrupt mechanism see 'wait_for_interrupt()'.

        Args:
            callback (Function): The callback function.

        Raises:
            GPIOException: if there is an error registering the interrupt callback.
        """
        # Sanity checks.
        if callback is None:
            raise GPIOException("Callback cannot be 'None'")
        if callback in self._irq_callbacks:
            raise GPIOException("Callback already registered")

        # Add callback to the list.
        self._irq_callbacks.append(callback)
        # Check if irq thread is running.
        if self._irq_thread_running:
            return
        # Start interrupts thread.
        self._start_interrupts_thread()

    def remove_interrupt_callback(self, callback):
        """
        Removes the registered interrupt callback.

        The GPIO must be configured as GPIOMode.IRQ_EDGE_RISING, GPIOMode.IRQ_EDGE_FALLING, or
        GPIOMode.IRQ_EDGE_BOTH, otherwise a 'GPIOException' will be thrown.

        To use a blocking interrupt mechanism see 'wait_for_interrupt()'.

        Args:
            callback (Function): The callback function.

        Raises:
            GPIOException: if there is an error removing the interrupt callback.
        """
        # Sanity checks.
        if callback is None:
            raise GPIOException("Callback cannot be 'None'")
        if callback not in self._irq_callbacks:
            raise GPIOException("Callback not registered")

        # Remove callback from the list.
        self._irq_callbacks.remove(callback)
        # Check if should stop interrupts thread.
        if self._irq_thread_running and not self._irq_callbacks:
            self._stop_interrupts_thread()

    def _start_interrupts_thread(self):
        """
        Starts waiting for interrupts.

        Raises:
            GPIOException: if there is any error waiting for interrupts.
        """
        # Sanity checks.
        if self._irq_thread_running:
            return

        # Create callback.
        self._irq_callback = _IRQ_CALLBACK(self._interrupt_triggered)
        # Call library.
        res = _libdigiapix.ldx_gpio_start_wait_interrupt(self._gpio_struct,
                                                         self._irq_callback,
                                                         None)
        # Check return value.
        if res != 0:
            raise GPIOException("Error starting interrupts thread.")
        # Flag running.
        self._irq_thread_running = True

    def _stop_interrupts_thread(self):
        """
        Stops waiting for interrupts.

        Raises:
            GPIOException: if there is any error stopping waiting for interrupts.
        """
        # Sanity checks.
        if not self._irq_thread_running:
            return

        # Call library.
        _res = _libdigiapix.ldx_gpio_stop_wait_interrupt(self._gpio_struct)
        # Flag running.
        self._irq_callback = None
        self._irq_thread_running = False

    def _interrupt_triggered(self, _data):
        """
        Method to be notified when an interrupt occurs.

        Args:
            _data (object): Callback data.
        """
        # Notify registered callbacks.
        for callback in self._irq_callbacks:
            callback()

        return 0


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
        # Configure the CTypes of the GPIO methods to use.
        _configure_gpio_ctypes()


def _configure_gpio_ctypes():
    """
    Configures the ctypes for the library GPIO methods
    """
    # Free GPIO.
    _libdigiapix.ldx_gpio_free.argtypes = [POINTER(_GPIOStruct)]
    _libdigiapix.ldx_gpio_free.restype = c_int
    # Request GPIO.
    _libdigiapix.ldx_gpio_request.argtypes = [c_uint32, c_int, c_int]
    _libdigiapix.ldx_gpio_request.restype = POINTER(_GPIOStruct)
    # Request by controller.
    _libdigiapix.ldx_gpio_request_by_controller.argtypes = [c_char_p, c_ubyte, c_int]
    _libdigiapix.ldx_gpio_request_by_controller.restype = POINTER(_GPIOStruct)
    # Request by alias.
    _libdigiapix.ldx_gpio_request_by_alias.argtypes = [c_char_p, c_int, c_int]
    _libdigiapix.ldx_gpio_request_by_alias.restype = POINTER(_GPIOStruct)
    # Set mode.
    _libdigiapix.ldx_gpio_set_mode.argtypes = [POINTER(_GPIOStruct), c_int]
    _libdigiapix.ldx_gpio_set_mode.restype = c_int
    # Get mode.
    _libdigiapix.ldx_gpio_get_mode.argtypes = [POINTER(_GPIOStruct)]
    _libdigiapix.ldx_gpio_get_mode.restype = c_int
    # Set value.
    _libdigiapix.ldx_gpio_set_value.argtypes = [POINTER(_GPIOStruct), c_int]
    _libdigiapix.ldx_gpio_set_value.restype = c_int
    # Get value.
    _libdigiapix.ldx_gpio_get_value.argtypes = [POINTER(_GPIOStruct)]
    _libdigiapix.ldx_gpio_get_value.restype = c_int
    # Set active mode.
    _libdigiapix.ldx_gpio_set_active_mode.argtypes = [POINTER(_GPIOStruct), c_int]
    _libdigiapix.ldx_gpio_set_active_mode.restype = c_int
    # Get active mode.
    _libdigiapix.ldx_gpio_get_active_mode.argtypes = [POINTER(_GPIOStruct)]
    _libdigiapix.ldx_gpio_get_active_mode.restype = c_int
    # Set debounce time.
    _libdigiapix.ldx_gpio_set_debounce.argtypes = [POINTER(_GPIOStruct), c_uint32]
    _libdigiapix.ldx_gpio_set_debounce.restype = c_int
    # Wait interrupt.
    _libdigiapix.ldx_gpio_wait_interrupt.argtypes = [POINTER(_GPIOStruct), c_int]
    _libdigiapix.ldx_gpio_wait_interrupt.restype = c_int
    # Start wait interrupt.
    _libdigiapix.ldx_gpio_start_wait_interrupt.argtypes = [POINTER(_GPIOStruct), _IRQ_CALLBACK,
                                                           c_void_p]
    _libdigiapix.ldx_gpio_start_wait_interrupt.restype = c_int
    # Stop wait interrupt.
    _libdigiapix.ldx_gpio_stop_wait_interrupt.argtypes = [POINTER(_GPIOStruct)]
    _libdigiapix.ldx_gpio_stop_wait_interrupt.restype = c_int
