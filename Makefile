#
# Copyright 2017-2022, Digi International Inc.
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

PYTHON_BIN ?= python3

BINDINGS_DIR = bindings
BUILD_DIR = build
DIST_DIR = dist
SRC_DIR = src
HEADERS_DIR = $(SRC_DIR)/include
HEADERS_PRIVATE_DIR = $(HEADERS_DIR)/private
HEADERS_PUBLIC_DIR = $(HEADERS_DIR)/public
PYTHON_BINDINGS_DIR = $(BINDINGS_DIR)/python

INSTALL_HEADERS_DIR = /usr/include/lib${NAME}

CFLAGS += -Wall -O2 -fPIC -D_GNU_SOURCE
CFLAGS += -I$(HEADERS_PRIVATE_DIR) -I$(HEADERS_PUBLIC_DIR)
LDFLAGS += -shared -Wl,-soname,lib$(NAME).so.$(MAJOR),--sort-common

# Add 3rd-party library dependencies
CFLAGS += $(shell pkg-config --cflags libsoc libgpiod)
LDLIBS += $(shell pkg-config --libs libsoc libgpiod)

SRCS =  $(SRC_DIR)/adc.c \
	$(SRC_DIR)/common.c \
	$(SRC_DIR)/gpio.c \
	$(SRC_DIR)/i2c.c \
	$(SRC_DIR)/pwm.c \
	$(SRC_DIR)/pwr_management.c \
	$(SRC_DIR)/spi.c \
	$(SRC_DIR)/watchdog.c

PUBLIC_HEADERS = $(HEADERS_PUBLIC_DIR)/adc.h \
		 $(HEADERS_PUBLIC_DIR)/common.h \
		 $(HEADERS_PUBLIC_DIR)/gpio.h \
		 $(HEADERS_PUBLIC_DIR)/i2c.h \
		 $(HEADERS_PUBLIC_DIR)/pwm.h \
		 $(HEADERS_PUBLIC_DIR)/pwr_management.h \
		 $(HEADERS_PUBLIC_DIR)/spi.h \
		 $(HEADERS_PUBLIC_DIR)/watchdog.h

ifeq ($(CONFIG_DISABLE_CAN),)
SRCS += $(SRC_DIR)/can.c \
	$(SRC_DIR)/can_netlink.c
PUBLIC_HEADERS += $(HEADERS_PUBLIC_DIR)/can.h
CFLAGS += $(shell pkg-config --cflags libsocketcan)
LDLIBS += $(shell pkg-config --libs libsocketcan)
endif

OBJS = $(SRCS:.c=.o)

.PHONY: all
all: lib$(NAME).so

lib$(NAME).so: lib$(NAME).so.$(VERSION)
	ln -sf lib$(NAME).so.$(VERSION) lib$(NAME).so.$(MAJOR)
	ln -sf lib$(NAME).so.$(VERSION) lib$(NAME).so

lib$(NAME).so.$(VERSION): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

python-bindings: $(PYTHON_BINDINGS_DIR)/setup.py lib$(NAME).so
	cd $(PYTHON_BINDINGS_DIR); $(PYTHON_BIN) setup.py bdist_wheel

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
	install -m 0644 $(PUBLIC_HEADERS) $(DESTDIR)$(INSTALL_HEADERS_DIR)/

install-python-bindings: python-bindings install
	# Install Python module
	$(PYTHON_BIN) -m pip install --prefix $(DESTDIR)/usr ${PYTHON_BINDINGS_DIR}/${DIST_DIR}/*.whl

.PHONY: clean
clean:
	-rm -f *.so* $(SRC_DIR)/*.o
