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

#ifndef SPI_H_
#define SPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include <stdint.h>

/**
 * spi_clk_mode_t - Defined values for SPI clock mode.
 */
typedef enum {
	SPI_CLK_MODE_ERROR = -1,/* Error when the SPI clock mode cannot be read */
	SPI_CLK_MODE_0,	/* CPOL=0 CPHA=0 - The data must be available before the
			 * first clock signal rising. The clock idle state is zero.
			 * The data on MISO and MOSI lines must be stable while
			 * clock is high and can be changed when clock is low. The
			 * data is captured on the clock's low-to-high transition
			 * and propagated on high-to-low clock transition.
			 */
	SPI_CLK_MODE_1,	/* CPOL=0 CPHA=1 - The first clock signal rising can be used
			 * to prepare the data. The clock idle state is zero. The
			 * data on MISO and MOSI lines must be stable while
			 * clock is low and can be changed when clock is high. The
			 * data is captured on the clock's high-to-low transition
			 * and propagated on low-to-high clock transition.
			 */
	SPI_CLK_MODE_2,	/* CPOL=1 CPHA=0 - The data must be available before the
			 * first clock signal falling. The clock idle state is one.
			 * The data on MISO and MOSI lines must be stable while
			 * clock is low and can be changed when clock is high. The
			 * data is captured on the clock's high-to-low transition
			 * and propagated on low-to-high clock transition.
			 */
	SPI_CLK_MODE_3	/* CPOL=1 CPHA=1 - The first clock signal falling can be
			 * used to prepare the data. The clock idle state is one.
			 * The data on MISO and MOSI lines must be stable while
			 * clock is high and can be changed when clock is low. The
			 * data is captured on the clock's low-to-high transition
			 * and propagated on high-to-low clock transition.
			 */
} spi_clk_mode_t;

/**
 * spi_cs_t - Defined values for SPI chip select configuration
 */
typedef enum {
	SPI_CS_ERROR = -1,	/* Error when the chip select cannot be determined */
	SPI_CS_ACTIVE_LOW,	/* Chip select active at low level */
	SPI_CS_ACTIVE_HIGH,	/* Chip select active at high level */
	SPI_CS_NO_CONT		/* Chip select not controlled */
} spi_cs_t;

/**
 * spi_bo_t - Defined values for SPI bit order configuration
 */
typedef enum {
	SPI_BO_ERROR = -1,	/* Error when the bit order cannot be determined */
	SPI_BO_MSB_FIRST,	/* Most significant bit first */
	SPI_BO_LSB_FIRST	/* Less significant bit first */
} spi_bo_t;

/**
 * spi_bpw_t - Defined values for SPI bits-per-word
 */
typedef enum {
	SPI_BPW_ERROR = -1,	/* Error when the SPI bits-per-word cannot be read */
	SPI_BPW_8,		/* 8 bits-per-word */
	SPI_BPW_16		/* 16 bits-per-word */
} spi_bpw_t;

/**
 * spi_transfer_cfg_t- Representation of a SPI transfer mode configuration
 *
 * @clk_mode:		SPI clock mode
 * @chip_select:	SPI chip select configuration
 * @bit_order:		SPI bit order
 */
typedef struct {
	spi_clk_mode_t clk_mode;
	spi_cs_t chip_select;
	spi_bo_t bit_order;
} spi_transfer_cfg_t;

/**
 * spi_t - Representation of a single SPI
 *
 * @alias:	Alias of the SPI
 * @spi_device:	SPI device index
 * @spi_slave:	SPI slave index
 * @_data:	Data for internal usage
 */
typedef struct {
	const char * const alias;
	const unsigned int spi_device;
	const unsigned int spi_slave;
	void *_data;
} spi_t;

/**
 * ldx_spi_request() - Request a SPI to use
 *
 * @spi_device:	The SPI device index to use.
 * @spi_slave:	The SPI slave index to use.
 *
 * This function returns a spi_t pointer. Memory for the struct is obtained
 * with 'malloc' and must be freed with 'ldx_spi_free()'.
 *
 * Return: A pointer to spi_t on success, NULL on error.
 */
spi_t *ldx_spi_request(unsigned int spi_device, unsigned int spi_slave);

/**
 * ldx_spi_request_by_alias() - Request a SPI to use using its alias name
 *
 * @spi_alias:	The alias name of the SPI to use.
 *
 * This function returns a spi_t pointer. Memory for the struct is obtained
 * with 'malloc' and must be freed with 'ldx_spi_free()'.
 *
 * Return: A pointer to spi_t on success, NULL on error.
 */
spi_t *ldx_spi_request_by_alias(const char * const spi_alias);

/**
 * ldx_spi_get_device() - Get the SPI device number of a given alias
 *
 * @spi_alias:	The alias of the SPI.
 *
 * Return: The SPI device number associated to the alias, -1 on error.
 */
int ldx_spi_get_device(char const * const spi_alias);

/**
 * spi_get_slave() - Get the SPI slave number of a given alias
 *
 * @spi_alias:	The alias of the SPI.
 *
 * Return: The SPI slave number associated to the alias, -1 on error.
 */
int ldx_spi_get_slave(char const * const spi_alias);

/**
 * ldx_spi_list_available_devices() - Get the list of available SPI device
 *				      indexes
 *
 * @devices:	A pointer to store the available SPI device indexes.
 *
 * This function returns in 'devices' the available Linux SPI device indexes.
 *
 * Memory for the 'devices' pointer is obtained with 'malloc' and must be
 * freed only when return value is > 0.
 *
 * Return: The number of available SPI devices, -1 on error
 */
int ldx_spi_list_available_devices(uint8_t **devices);

/**
 * ldx_spi_list_available_slaves() - Get the list of the available SPI slave
 *				     device indexes for the given SPI device
 *
 * @spi_device:	The index of the SPI device to check for slave devices.
 * @slaves:	A pointer to store the SPI slave device indexes.
 *
 * This function returns in 'slaves' the available Linux SPI device slave
 * indexes for the given SPI device.
 *
 * Memory for the 'slaves' pointer is obtained with 'malloc' and must be freed
 * only when return value is > 0.
 *
 * Return: The number of available slave devices for the given SPI device,
 *	   -1 on error
 */
int ldx_spi_list_available_slaves(uint8_t spi_device, uint8_t **slaves);

/**
 * ldx_spi_free() - Free a previously requested SPI
 *
 * @spi:	A pointer to the requested SPI to free.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_spi_free(spi_t *spi);

/**
 * ldx_spi_set_transfer_mode() - Change the given SPI transfer mode
 *
 * @spi:		A requested SPI to set its transfer mode.
 * @transfer_mode:	The new SPI transfer mode (spi_transfer_cfg_t).
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_spi_set_transfer_mode(spi_t *spi, spi_transfer_cfg_t *transfer_mode);

/**
 * ldx_spi_get_transfer_mode() - Get the given SPI transfer mode
 *
 * @spi:		A requested SPI to get its transfer mode.
 * @transfer_mode:	Struct to store the read transfer mode (spi_transfer_cfg_t).
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_spi_get_transfer_mode(spi_t *spi, spi_transfer_cfg_t *transfer_mode);

/**
 * ldx_spi_set_bits_per_word() - Change the given SPI bits-per-word
 *
 * @spi:	A requested SPI to configure the bits-per-word.
 * @bpw:	Bits-per-word to configure (spi_bpw_t).
 *
 * This function configures the given SPI bits-per-word to be:
 *	- SPI_BPW_8: 8 bits-per-word
 *	- SPI_BPW_16: 16 bits-per-word
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_spi_set_bits_per_word(spi_t *spi, spi_bpw_t bpw);

/**
 * ldx_spi_get_bits_per_word() - Get the given SPI configured bits-per-word
 *
 * @spi:	A requested SPI to get the configured bits-per-word.
 *
 * This function retrieves the SPI bits-per-word:
 *	- SPI_BPW_8: 8 bits-per-word
 *	- SPI_BPW_16: 16 bits-per-word
 *
 * Return: The configured SPI bits-per-word (SPI_BPW_8, SPI_BPW_16) or
 *	   SPI_BPW_ERROR if it cannot be retrieved.
 */
spi_bpw_t ldx_spi_get_bits_per_word(spi_t *spi);

/**
 * ldx_spi_set_speed() - Change the SPI bus max speed
 *
 * @spi:	A requested SPI to configure the bus speed.
 * @speed:	The new speed to set in Hz.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_spi_set_speed(spi_t *spi, unsigned int speed);

/**
 * ldx_spi_get_speed() - Get the SPI configured max speed
 *
 * @spi:	A requested SPI to get the max speed.
 *
 * Return: The configured SPI max speed in Hz, -1 on error.
 */
int ldx_spi_get_speed(spi_t *spi);

/**
 * ldx_spi_write() - Write data to the SPI bus
 *
 * @spi:	A requested SPI to write data to.
 * @tx_data:	Array of bytes to write.
 * @length:	Number of bytes to write.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_spi_write(spi_t *spi, uint8_t *tx_data, unsigned int length);

/**
 * ldx_spi_read() - Read data from the SPI bus
 *
 * @spi:	A requested SPI to read data from.
 * @rx_data:	Array of bytes to store read data into.
 * @length:	Number of bytes to read.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_spi_read(spi_t *spi, uint8_t *rx_data, unsigned int length);

/**
 * ldx_spi_transfer() - Write and read data from the SPI bus simultaneously
 *
 * @spi:	A requested SPI to write and read data from.
 * @tx_data:	Array of bytes to write.
 * @rx_data:	Array of bytes to store read data into.
 * @length:	Number of bytes to transfer.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_spi_transfer(spi_t *spi, uint8_t *tx_data, uint8_t *rx_data,
		     unsigned int length);

#ifdef __cplusplus
}
#endif

#endif /* SPI_H_ */
