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


class RequestMode(Enum):
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
        self.__code = code
        self.__title = title
        self.__description = description

    @property
    def code(self):
        """
        Returns the Request mode code.

        Returns:
            Integer: Request mode code.
        """
        return self.__code

    @property
    def title(self):
        """
        Returns the Request mode title.

        Returns:
            String: Request mode title.
        """
        return self.__title

    @property
    def description(self):
        """
        Returns the Request mode description.

        Returns:
            String: Request mode description.
        """
        return self.__description

    @classmethod
    def get(cls, code):
        """
        Returns the Request mode corresponding to the given code.

        Args:
            code (Integer): Request mode code.

        Returns:
            :class:`.RequestMode`: Request mode corresponding to the given code, `None` if not
                                   found.
        """
        for request_mode in cls:
            if code == request_mode.code:
                return request_mode
        return None
