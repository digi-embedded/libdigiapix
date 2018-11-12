#
# Copyright 2017, Digi International Inc.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

NAME := digiapix

# Version
MAJOR := 1
MINOR := 0
REVISION := 0
VERSION := $(MAJOR).$(MINOR).$(REVISION)

SRC_DIR = src
HEADERS_DIR = $(SRC_DIR)/include
HEADERS_PRIVATE_DIR = $(HEADERS_DIR)/private
HEADERS_PUBLIC_DIR = $(HEADERS_DIR)/public

INSTALL_HEADERS_DIR = /usr/include/lib${NAME}

CFLAGS += -Wall -O2 -fPIC
CFLAGS += -I$(HEADERS_PRIVATE_DIR) -I$(HEADERS_PUBLIC_DIR)
LDFLAGS += -shared -Wl,-soname,lib$(NAME).so.$(MAJOR),--sort-common

# Add 3rd-party library dependences
CFLAGS += $(shell pkg-config --cflags libsoc libsocketcan)
LDLIBS += $(shell pkg-config --libs libsoc libsocketcan)

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:.c=.o)

.PHONY: all
all: lib$(NAME).so

lib$(NAME).so: lib$(NAME).so.$(VERSION)
	ln -sf lib$(NAME).so.$(VERSION) lib$(NAME).so.$(MAJOR)
	ln -sf lib$(NAME).so.$(VERSION) lib$(NAME).so

lib$(NAME).so.$(VERSION): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

.PHONY: install
install: lib$(NAME).so
	# Install library
	install -d $(DESTDIR)/usr/lib/
	install -m 0644 lib$(NAME).so.$(VERSION) $(DESTDIR)/usr/lib/
	ln -sf lib$(NAME).so.$(VERSION) $(DESTDIR)/usr/lib/lib$(NAME).so.$(MAJOR)
	ln -sf lib$(NAME).so.$(VERSION) $(DESTDIR)/usr/lib/lib$(NAME).so
	# Install pkg-config file
	install -d $(DESTDIR)/usr/lib/pkgconfig
	install -m 0644 lib$(NAME).pc $(DESTDIR)/usr/lib/pkgconfig/
	# Install header files
	install -d $(DESTDIR)$(INSTALL_HEADERS_DIR)
	install -m 0644 $(HEADERS_PUBLIC_DIR)/*.h $(DESTDIR)$(INSTALL_HEADERS_DIR)/

.PHONY: clean
clean:
	-rm -f *.so* $(OBJS)
