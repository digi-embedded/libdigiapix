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

import ctypes
import logging

from ctypes.util import find_library

from digi.apix.exceptions import DigiAPIXException

# Constants.
_LIBDIGIAPIX_LIBRARY = "digiapix"

# Variables.
_log = logging.getLogger(__name__)
_libdigiapix = None


def _load_library():
    """
    Loads the 'digiapix' library in memory.

    Raises:
        DigiAPIXException: if there is any problem loading the library.
    """
    # Use global variable.
    global _libdigiapix
    # Find library in the system.
    path_library = find_library(_LIBDIGIAPIX_LIBRARY)
    if not path_library:
        error = f"Could not find '{_LIBDIGIAPIX_LIBRARY}' library in the system"
        _log.error("%s", error)
        raise DigiAPIXException(error)
    # Load the library.
    try:
        _libdigiapix = ctypes.CDLL(path_library)
    except OSError as exc:
        error = f"Unable to load '{_LIBDIGIAPIX_LIBRARY}' library: {str(exc)}"
        _log.error("%s", error)
        raise DigiAPIXException(error) from exc
    _log.info("%s", f"Library '{_LIBDIGIAPIX_LIBRARY}' loaded successfully")


def get_library():
    """
    Returns the 'digiapix' library.

    Returns:
        :class:`.CDLL`: The loaded library.

    Raises:
        DigiAPIXException: if there is any problem loading the library.
    """
    if _libdigiapix is None:
        _load_library()
    return _libdigiapix
