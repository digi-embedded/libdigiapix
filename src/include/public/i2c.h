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

#ifndef I2C_H_
#define I2C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/**
 * i2c_t - Representation of a single requested I2C
 *
 * @alias:		Alias of the I2C
 * @bus:		I2C Linux bus number
 * @_data:		Data for internal usage
 */
typedef struct {
	const char * const alias;
	const unsigned int bus;
	void *_data;
} i2c_t;

/**
 * ldx_i2c_request() - Request a I2C bus to use
 *
 * @i2c_bus:	The Linux bus number of the I2C to request.
 *
 * This function returns a i2c_t pointer. Memory for the struct is obtained
 * with 'malloc' and must be freed with 'ldx_i2c_free()'.
 *
 * Return: A pointer to i2c_t on success, NULL on error.
 */
i2c_t *ldx_i2c_request(unsigned int i2c_bus);

/**
 * ldx_i2c_request_by_alias() - Request a I2C bus using its alias name
 *
 * @i2c_alias:		The alias name of the I2C to request.
 *
 * This function returns a i2c_t pointer. Memory for the struct is obtained
 * with 'malloc' and must be freed with 'ldx_i2c_free()'.
 *
 * Return: A pointer to i2c_t on success, NULL on error.
 */
i2c_t *ldx_i2c_request_by_alias(const char * const i2c_alias);

/**
 * ldx_i2c_free() - Free a previously requested I2C
 *
 * @i2c:	A pointer to the requested I2C to free.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_i2c_free(i2c_t *i2c);

/**
 * ldx_i2c_get_bus() - Get the given I2C bus index using its alias name
 *
 * i2c_alias:	The alias of the I2C.
 *
 * Return: The Linux bus index or -1 on error.
 */
int ldx_i2c_get_bus(const char * const i2c_alias);

/**
 * ldx_i2c_list_available_buses() - Get list of available I2C buses
 *
 * @buses:	A pointer to store the available I2C bus indexes.
 *
 * This function returns in 'buses' the available Linux I2C bus indexes.
 *
 * Memory for the 'buses' pointer is obtained with 'malloc' and must be
 * freed only when return value is > 0.
 *
 * Return: The number of available I2C buses, -1 on error.
 */
int ldx_i2c_list_available_buses(uint8_t **buses);

/**
 * ldx_i2c_set_timeout() - Set the I2C bus timeout in milliseconds
 *
 * @i2c:	A requested I2C to set the timeout.
 * @timeout:	I2C bus timeout value.
 *
 * The function sets the I2C bus timeout. The value will be multiplied by 10.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_i2c_set_timeout(i2c_t *i2c, unsigned int timeout);

/**
 * ldx_i2c_set_retries() - Set the I2C bus poll retries
 *
 * @i2c:	A requested I2C to set the device polling retries.
 * @retry:	I2C bus retry value.
 *
 * The function sets the I2C bus retries to poll a device when not acknowledged.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_i2c_set_retries(i2c_t *i2c, unsigned int retry);

/**
 * ldx_i2c_read() - Read data from the I2C slave device
 *
 * @i2c:		A requested I2C bus to read from.
 * @i2c_address:	Address of the I2C slave device to read from
 * @buffer:		A pointer to the read buffer
 * @length:		Length of the data that should be read over the I2C bus
 *
 * This function reads data from a I2C device connected to the requested I2C
 * bus.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_i2c_read(i2c_t *i2c, unsigned int i2c_address, uint8_t *buffer, uint16_t length);

/**
 * ldx_i2c_write() - Send data to an I2C slave device
 *
 * @i2c:		A requested I2C bus to write from.
 * @i2c_address:	Address of the I2C slave device to write to.
 * @buffer:		A pointer to the write buffer.
 * @length:		Length of the data that should be written over the I2C
 *			bus.
 *
 * This function writes data from a I2C device connected to the requested I2C
 * bus.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_i2c_write(i2c_t *i2c, unsigned int i2c_address, uint8_t *buffer, uint16_t length);

/**
 * ldx_i2c_transfer() - Transfer data with an the I2C slave device
 *
 * @i2c:		A requested I2C bus to transfer to/from.
 * @i2c_address:	Address of the I2C slave device to transfer data with.
 * @buffer_to_write:	A pointer to the data should be written to the I2C
 *			device.
 * @w_length:		Length of the data that should be written over the I2C
 *			bus.
 * @buffer_to_read:	A pointer to the data should be read from the I2C
 *			device.
 * @r_length:		Length of the data that should be read over the I2C bus.
 *
 * This function transfers data to and from a I2C device connected to the
 * requested I2C bus.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_i2c_transfer(i2c_t *i2c, unsigned int i2c_address, uint8_t *buffer_to_write,
		uint16_t w_length, uint8_t *buffer_to_read, uint16_t r_length);

#ifdef __cplusplus
}
#endif

#endif /* I2C_H_ */
