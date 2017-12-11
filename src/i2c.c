/*
 * Copyright 2017, Digi International Inc.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "_common.h"
#include "_libsoc_interfaces.h"
#include "_log.h"
#include "i2c.h"

#define MAX_I2C_BUSES	5

static int check_i2c(i2c_t *i2c);

i2c_t *ldx_i2c_request(unsigned int i2c_bus)
{
	libsoc_i2c_t *_i2c = NULL;
	i2c_t *new_i2c = NULL;
	i2c_t init_i2c = { NULL, i2c_bus, NULL };

	log_debug("%s: Requesting I2C bus %d", __func__, i2c_bus);

	_i2c = libsoc_i2c_init(i2c_bus, 0);

	if (_i2c == NULL)
		return NULL;

	new_i2c = calloc(1, sizeof(i2c_t));
	if (new_i2c == NULL) {
		log_error("%s: Unable to request I2C %d, cannot allocate memory",
			  __func__, i2c_bus);
		libsoc_i2c_free(_i2c);
		return NULL;
	}

	memcpy(new_i2c, &init_i2c, sizeof(i2c_t));
	((i2c_t *)new_i2c)->_data = _i2c;

	return new_i2c;
}

i2c_t *ldx_i2c_request_by_alias(char const * const i2c_alias)
{
	int i2c_bus;
	void *new_i2c = NULL;

	log_debug("%s: Requesting I2C '%s'", __func__, i2c_alias);

	i2c_bus = ldx_i2c_get_bus(i2c_alias);

	new_i2c = ldx_i2c_request((unsigned int)i2c_bus);
	if (new_i2c != NULL) {
		i2c_t init_i2c = {i2c_alias, i2c_bus, ((i2c_t *)new_i2c)->_data};

		memcpy(new_i2c, &init_i2c, sizeof(i2c_t));
	}

	return new_i2c;
}

int ldx_i2c_get_bus(char const * const i2c_alias)
{
	if (config_check_alias(i2c_alias) != EXIT_SUCCESS)
		return -1;

	return config_get_i2c_bus(i2c_alias);
}

int ldx_i2c_list_available_buses(uint8_t **buses)
{
	uint8_t i, next = 0;
	uint8_t _buses[MAX_I2C_BUSES] = {0};
	char path[20];

	if (buses == NULL) {
		log_error("%s: Unable to list I2C buses", __func__);
		return EXIT_FAILURE;
	}

	for (i = 0; i < MAX_I2C_BUSES; i++) {
		snprintf(path, sizeof(path), "/dev/i2c-%d", i);
		if (access(path, F_OK) == 0) {
			_buses[next] = i;
			next++;
		}
	}

	if (next == 0)
		return 0;

	*buses = (uint8_t *)calloc(next, sizeof(uint8_t));
	if (*buses == NULL) {
		log_error("%s: Could not allocate memory to list buses", __func__);
		return -1;
	}

	for (i = 0; i < next; i++)
		(*buses)[i] = _buses[i];

	return next;
}

int ldx_i2c_free(i2c_t *i2c)
{
	int ret = EXIT_SUCCESS;

	if (i2c == NULL)
		return EXIT_SUCCESS;

	log_debug("%s: Freeing I2C %d", __func__, i2c->bus);

	if (i2c->_data != NULL)
		ret = libsoc_i2c_free(i2c->_data);

	free(i2c);

	return ret;
}

int ldx_i2c_set_timeout(i2c_t *i2c, unsigned int timeout)
{
	if (check_i2c(i2c) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	log_debug("%s: Setting I2C %d timeout to %d", __func__, i2c->bus, timeout);

	if (libsoc_i2c_set_timeout(i2c->_data, timeout) != EXIT_SUCCESS) {
		log_error("%s: Unable to set I2C-%d timeout", __func__,
			  i2c->bus);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int ldx_i2c_set_retries(i2c_t *i2c, unsigned int retry)
{
	libsoc_i2c_t *_i2c = NULL;

	if (check_i2c(i2c) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	_i2c = i2c->_data;

	log_debug("%s: Setting I2C %d bus retries to %d", __func__, i2c->bus, retry);

	if (ioctl(_i2c->fd, I2C_RETRIES, retry) < 0) {
		log_error("%s: Unable to set I2C bus retries on I2C-%d",
			  __func__, i2c->bus);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int ldx_i2c_read(i2c_t *i2c, unsigned int i2c_address, uint8_t *buffer,
		 uint16_t length)
{
	libsoc_i2c_t *_i2c = NULL;

	if (check_i2c(i2c) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	_i2c = i2c->_data;

	if (length == 0)
		return EXIT_SUCCESS;

	if (buffer == NULL) {
		log_error("%s: Unable to read data from I2C-%d slave 0x%x",
			  __func__, i2c->bus, i2c_address);
		return EXIT_FAILURE;
	}

	_i2c->address = i2c_address;

	log_debug("%s: Reading %d bytes from I2C-%d at address %d", __func__,
		  length, i2c->bus, i2c_address);

	if (libsoc_i2c_read(_i2c, buffer, length) != EXIT_SUCCESS) {
		log_error("%s: Unable to read data from I2C-%d slave 0x%x",
			  __func__, i2c->bus, i2c_address);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int ldx_i2c_write(i2c_t *i2c, unsigned int i2c_address, uint8_t *buffer,
		  uint16_t length)
{
	libsoc_i2c_t *_i2c = NULL;

	if (check_i2c(i2c) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	_i2c = i2c->_data;

	if (length == 0)
		return EXIT_SUCCESS;

	if (buffer == NULL) {
		log_error("%s: Unable to write data from I2C-%d slave 0x%x",
			  __func__, i2c->bus, i2c_address);
		return EXIT_FAILURE;
	}

	_i2c->address = i2c_address;

	log_debug("%s: Writing %d bytes to I2C-%d at address %d", __func__,
		  length, i2c->bus, i2c_address);

	if (libsoc_i2c_write(_i2c, buffer, length) != EXIT_SUCCESS) {
		log_error("%s: Unable to write data from I2C-%d slave 0x%x",
			  __func__, i2c->bus, i2c_address);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int ldx_i2c_transfer(i2c_t *i2c, unsigned int i2c_address,
		     uint8_t *buffer_to_write, uint16_t w_length,
		     uint8_t *buffer_to_read, uint16_t r_length)
{
	libsoc_i2c_t *_i2c = NULL;

	if (check_i2c(i2c) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	_i2c = i2c->_data;
	_i2c->address = i2c_address;

	log_debug("%s: Transferring data with I2C-%d at address %d: Writing %d bytes and reading %d bytes",
		  __func__, i2c->bus, i2c_address, w_length, r_length);

	if ((buffer_to_write != NULL) && (w_length > 0)) {
		if (libsoc_i2c_write(_i2c, buffer_to_write, w_length) != EXIT_SUCCESS) {
			log_error("%s: Unable to transfer data to the I2C-%d slave 0x%x",
				  __func__, i2c->bus, i2c_address);
			return EXIT_FAILURE;
		}
	}

	if ((buffer_to_read != NULL) && (r_length > 0)) {
		if (libsoc_i2c_read(_i2c, buffer_to_read, r_length) != EXIT_SUCCESS) {
			log_error("%s: Unable to transfer data to the I2C-%d slave 0x%x",
				  __func__, i2c->bus, i2c_address);
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

/**
 * check_i2c() - Verify that the I2C pointer is valid
 *
 * @i2c:	The I2C pointer to check.
 *
 * Return: EXIT_SUCCESS if the I2C is valid, EXIT_FAILURE otherwise.
 */
static int check_i2c(i2c_t *i2c)
{
	if (i2c == NULL) {
		log_error("%s: I2C cannot be NULL", __func__);
		return EXIT_FAILURE;
	}

	if (i2c->_data == NULL) {
		log_error("%s: Invalid I2C, %d", __func__,
			  i2c->bus);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
