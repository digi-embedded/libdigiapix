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
#include <glob.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/spi/spidev.h>

#include "_common.h"
#include "_libsoc_interfaces.h"
#include "_log.h"
#include "spi.h"

#define MAX_SPI_DEVICES		10
#define MAX_SPI_SLAVES		5
#define HIGH_SPI_BASE		32766

#define M(x)	#x,
static const char * const spi_clk_mode_strings[] = {
	M(SPI_CLK_MODE_0)
	M(SPI_CLK_MODE_1)
	M(SPI_CLK_MODE_2)
	M(SPI_CLK_MODE_3)
};

static const char * const spi_cs_strings[] = {
	M(SPI_CS_ACTIVE_LOW)
	M(SPI_CS_ACTIVE_HIGH)
	M(SPI_CS_NO_CONT)
};

static const char * const spi_bo_strings[] = {
	M(SPI_BO_LSB_FIRST)
	M(SPI_BO_MSB_FIRST)
};

static const char * const spi_bpw_strings[] = {
	M(SPI_BPW_8)
	M(SPI_BPW_16)
};
#undef M

static int check_spi(spi_t *spi);
static int check_transfer_mode(spi_transfer_cfg_t *transfer_mode);
static int check_clock_mode(spi_clk_mode_t clock_mode);
static int check_chip_select(spi_cs_t chip_select);
static int check_bit_order(spi_bo_t bit_order);
static int check_bpw(spi_bpw_t bpw);
static int check_data_buffer(uint8_t *buffer);

spi_t *ldx_spi_request(unsigned int spi_device, unsigned int spi_slave)
{
	libsoc_spi_t *_spi = NULL;
	spi_t *new_spi = NULL;
	spi_t init_spi = { NULL, spi_device, spi_slave, NULL };

	log_debug("%s: Requesting SPI device %d slave %d",
		  __func__, spi_device, spi_slave);

	_spi = libsoc_spi_init(spi_device, spi_slave);
	if (_spi == NULL)
		return NULL;

	new_spi = calloc(1, sizeof(spi_t));
	if (new_spi == NULL) {
		log_error("%s: Unable to request SPI %d:%d, cannot allocate memory",
			  __func__, spi_device, spi_slave);
		libsoc_spi_free(_spi);
		return NULL;
	}

	memcpy(new_spi, &init_spi, sizeof(spi_t));
	((spi_t *)new_spi)->_data = _spi;

	return new_spi;
}

spi_t *ldx_spi_request_by_alias(const char * const spi_alias)
{
	int spi_device, spi_slave;
	spi_t *new_spi = NULL;

	log_debug("%s: Requesting SPI '%s'", __func__, spi_alias);

	spi_device = ldx_spi_get_device(spi_alias);
	if (spi_device == -1) {
		log_error("%s: Invalid SPI alias, '%s'", __func__, spi_alias);
		return NULL;
	}

	spi_slave = ldx_spi_get_slave(spi_alias);
	if (spi_slave == -1) {
		log_error("%s: Invalid SPI alias, '%s'", __func__, spi_alias);
		return NULL;
	}

	new_spi = ldx_spi_request(spi_device, spi_slave);
	if (new_spi != NULL) {
		spi_t init_spi = {spi_alias, spi_device, spi_slave,
				((spi_t *)new_spi)->_data};
		memcpy(new_spi, &init_spi, sizeof(spi_t));
	}

	return new_spi;
}

int ldx_spi_get_device(char const * const spi_alias)
{
	if (config_check_alias(spi_alias) != EXIT_SUCCESS)
		return -1;

	return config_get_spi_device_number(spi_alias);
}

int ldx_spi_get_slave(char const * const spi_alias)
{
	if (config_check_alias(spi_alias) != EXIT_SUCCESS)
		return -1;

	return config_get_spi_slave_number(spi_alias);
}

int ldx_spi_list_available_devices(uint8_t **devices)
{
	uint8_t i, count = 0;
	uint16_t _devices[MAX_SPI_DEVICES] = {0};
	glob_t globresults;
	char path[20];

	for (i = 0; i < MAX_SPI_DEVICES / 2; i++) {
		sprintf(path, "/dev/spidev%d.*", i);
		if (glob(path, 0, NULL, &globresults) == 0) {
			_devices[count] = i;
			count++;
		}

		sprintf(path, "/dev/spidev%d.*", HIGH_SPI_BASE - i);
		if (glob(path, 0, NULL, &globresults) == 0) {
			_devices[count] = HIGH_SPI_BASE - i;
			count++;
		}
	}

	globfree(&globresults);

	if (count == 0)
		return 0;

	*devices = (uint8_t *)calloc(count, sizeof(uint8_t));
	if (*devices == NULL) {
		log_error("%s: Could not allocate memory to list devices", __func__);
		return -1;
	}

	for (i = 0; i < count; i++)
		(*devices)[i] = _devices[i];

	return count;
}

int ldx_spi_list_available_slaves(uint8_t spi_device, uint8_t **slaves)
{
	uint8_t i, count = 0;
	uint8_t _slaves[MAX_SPI_SLAVES] = {0};
	char path[20];

	for (i = 0; i < MAX_SPI_SLAVES; i++) {
		sprintf(path, "/dev/spidev%d.%d", spi_device, i);
		if (access(path, F_OK) == 0) {
			_slaves[count] = i;
			count++;
		}
	}

	if (count == 0)
		return 0;

	*slaves = (uint8_t *)calloc(count, sizeof(uint8_t));
	if (*slaves == NULL) {
		log_error("%s: Could not allocate memory to list slaves", __func__);
		return -1;
	}

	for (i = 0; i < count; i++)
		(*slaves)[i] = _slaves[i];

	return count;
}

int ldx_spi_free(spi_t *spi)
{
	int ret = EXIT_SUCCESS;

	if (spi == NULL)
		return EXIT_SUCCESS;

	log_debug("%s: Freeing SPI %d:%d", __func__, spi->spi_device,
		  spi->spi_slave);

	if (spi->_data != NULL)
		ret = libsoc_spi_free(spi->_data);

	free(spi);

	return ret;
}

int ldx_spi_set_transfer_mode(spi_t *spi_dev, spi_transfer_cfg_t *transfer_mode)
{
	libsoc_spi_t *_spi = NULL;
	uint8_t new_value = 0;

	if (check_spi(spi_dev) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	if (check_transfer_mode(transfer_mode) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	log_debug("%s: Setting SPI %d:%d transfer mode to:\n - Clock mode '%s' (%d)\n - Chip select '%s' (%d)\n - Bit order '%s' (%d)\n",
		  __func__, spi_dev->spi_device, spi_dev->spi_slave,
		  spi_clk_mode_strings[transfer_mode->clk_mode],
		  transfer_mode->clk_mode,
		  spi_cs_strings[transfer_mode->chip_select],
		  transfer_mode->chip_select,
		  spi_bo_strings[transfer_mode->bit_order],
		  transfer_mode->bit_order);

	/* Set clock mode */
	new_value = transfer_mode->clk_mode;

	/* Set chip select */
	switch (transfer_mode->chip_select) {
	case SPI_CS_ACTIVE_HIGH:
		new_value = new_value | SPI_CS_HIGH;
		break;
	case SPI_CS_NO_CONT:
		new_value = new_value | SPI_NO_CS;
		break;
	case SPI_CS_ACTIVE_LOW: /* This value is a 0 */
	default:
		break;
	}

	/* Set bit order */
	switch (transfer_mode->bit_order) {
	case SPI_BO_LSB_FIRST:
		new_value = new_value | SPI_LSB_FIRST;
		break;
	case SPI_BO_MSB_FIRST: /* This value is a 0 */
	default:
		break;
	}

	_spi = (libsoc_spi_t *)spi_dev->_data;

	if (ioctl(_spi->fd, SPI_IOC_WR_MODE, &new_value) == -1) {
		log_error("%s: Unable to set SPI %d:%d transfer mode to to:\n - Clock mode '%s' (%d)\n - Chip select '%s' (%d)\n - Bit order '%s' (%d)\n",
			  __func__, spi_dev->spi_device, spi_dev->spi_slave,
			  spi_clk_mode_strings[transfer_mode->clk_mode],
			  transfer_mode->clk_mode,
			  spi_cs_strings[transfer_mode->chip_select],
			  transfer_mode->chip_select,
			  spi_bo_strings[transfer_mode->bit_order],
			  transfer_mode->bit_order);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int ldx_spi_get_transfer_mode(spi_t *spi, spi_transfer_cfg_t *transfer_mode)
{
	libsoc_spi_t *_spi = NULL;
	uint8_t read_value = 0;

	if (check_spi(spi) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	if (transfer_mode == NULL) {
		log_error("%s: Transfer mode cannot be NULL", __func__);
		return EXIT_FAILURE;
	}

	log_debug("%s: Getting transfer mode of SPI %d:%d", __func__,
		  spi->spi_device, spi->spi_slave);

	_spi = (libsoc_spi_t *)spi->_data;

	if (ioctl(_spi->fd, SPI_IOC_RD_MODE, &read_value) == -1) {
		log_error("%s: Unable to get SPI %d:%d transfer mode",
			  __func__, spi->spi_device, spi->spi_slave);
		return EXIT_FAILURE;
	}

	/* Determine the clock mode */
	switch (read_value) {
	case SPI_MODE_0:
		transfer_mode->clk_mode = SPI_CLK_MODE_0;
		break;
	case SPI_MODE_1:
		transfer_mode->clk_mode = SPI_CLK_MODE_1;
		break;
	case SPI_MODE_2:
		transfer_mode->clk_mode = SPI_CLK_MODE_2;
		break;
	case SPI_MODE_3:
		transfer_mode->clk_mode = SPI_CLK_MODE_3;
		break;
	default:
		log_error("%s: Invalid SPI mode: %d", __func__, read_value);
		transfer_mode->clk_mode = SPI_CLK_MODE_ERROR;
		break;
	}

	/* Determine the chip select */
	switch (read_value & (SPI_CS_HIGH | SPI_NO_CS)) {
	case SPI_CS_HIGH:
		transfer_mode->chip_select = SPI_CS_ACTIVE_HIGH;
		break;
	case SPI_NO_CS:
		transfer_mode->chip_select = SPI_CS_NO_CONT;
		break;
	case SPI_CS_ACTIVE_LOW:
		transfer_mode->chip_select = SPI_CS_ACTIVE_LOW;
		break;
	default:
		transfer_mode->chip_select = SPI_CS_ERROR;
		break;
	}

	/* Determine the bit order */
	switch (read_value & SPI_LSB_FIRST) {
	case SPI_LSB_FIRST:
		transfer_mode->bit_order = SPI_BO_LSB_FIRST;
		break;
	case SPI_BO_MSB_FIRST:
		transfer_mode->bit_order = SPI_BO_MSB_FIRST;
		break;
	default:
		transfer_mode->chip_select = SPI_BO_ERROR;
		break;
	}

	return EXIT_SUCCESS;
}

int ldx_spi_set_bits_per_word(spi_t *spi, spi_bpw_t bpw)
{
	libsoc_spi_bpw_t _bpw = BPW_ERROR;

	if (check_spi(spi) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	if (check_bpw(bpw) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	log_debug("%s: Setting bits-per-word for SPI %d:%d, bits-per-word: '%s' (%d)",
		  __func__, spi->spi_device, spi->spi_slave,
		  spi_bpw_strings[bpw], bpw);

	switch (bpw) {
	case SPI_BPW_8:
		_bpw = BITS_8;
		break;
	case SPI_BPW_16:
		_bpw = BITS_16;
		break;
	default:
		/* Should never happen */
		return EXIT_FAILURE;
	}

	if (libsoc_spi_set_bits_per_word(spi->_data, _bpw) != EXIT_SUCCESS) {
		log_error("%s: Unable to set SPI %d:%d bits-per-word to '%s' (%d)",
			  __func__, spi->spi_device, spi->spi_slave,
			  spi_bpw_strings[bpw], bpw);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

spi_bpw_t ldx_spi_get_bits_per_word(spi_t *spi)
{
	libsoc_spi_bpw_t bpw;

	if (check_spi(spi) != EXIT_SUCCESS)
		return SPI_BPW_ERROR;

	log_debug("%s: Getting bits-per-word of SPI %d:%d", __func__,
		  spi->spi_device, spi->spi_slave);

	bpw = libsoc_spi_get_bits_per_word(spi->_data);
	if (bpw == BPW_ERROR) {
		log_error("%s: Unable to get SPI %d:%d bits-per-word",
			  __func__, spi->spi_device, spi->spi_slave);
		return SPI_BPW_ERROR;
	}

	switch (bpw) {
	case BITS_8:
		return SPI_BPW_8;
	case BITS_16:
		return SPI_BPW_16;
	default:
		/* Should never happen */
		return SPI_BPW_ERROR;
	}
}

int ldx_spi_set_speed(spi_t *spi, unsigned int speed)
{
	if (check_spi(spi) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	log_debug("%s: Setting SPI %d:%d speed to %dHz", __func__,
		  spi->spi_device, spi->spi_slave, speed);

	if (libsoc_spi_set_speed(spi->_data, speed) == EXIT_FAILURE) {
		log_error("%s: Unable to set SPI %d:%d speed to %dHz",
			  __func__, spi->spi_device, spi->spi_slave, speed);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int ldx_spi_get_speed(spi_t *spi)
{
	int speed = -1;

	if (check_spi(spi) != EXIT_SUCCESS)
		return -1;

	log_debug("%s: Getting SPI %d:%d speed", __func__, spi->spi_device,
		  spi->spi_slave);

	speed = libsoc_spi_get_speed(spi->_data);
	if (speed == -1) {
		log_error("%s: Unable to get SPI %d:%d speed", __func__,
			  spi->spi_device, spi->spi_slave);
		return -1;
	}

	return speed;
}

int ldx_spi_write(spi_t *spi, uint8_t *tx_data, unsigned int length)
{
	if (check_spi(spi) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	if (check_data_buffer(tx_data) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	if (length == 0)
		return EXIT_SUCCESS;

	log_debug("%s: Writing %d bytes to SPI %d:%d", __func__, length,
		  spi->spi_device, spi->spi_slave);

	if (libsoc_spi_write(spi->_data, tx_data, length) != EXIT_SUCCESS) {
		log_error("%s: Unable to write %d bytes to SPI %d:%d", __func__,
			  length, spi->spi_device, spi->spi_slave);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int ldx_spi_read(spi_t *spi, uint8_t *rx_data, unsigned int length)
{
	if (check_spi(spi) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	if (check_data_buffer(rx_data) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	if (length == 0)
		return EXIT_SUCCESS;

	log_debug("%s: Reading %d bytes from SPI %d:%d", __func__, length,
		  spi->spi_device, spi->spi_slave);

	if (libsoc_spi_read(spi->_data, rx_data, length) != EXIT_SUCCESS) {
		log_error("%s: Unable to read %d bytes from SPI %d:%d",
			  __func__, length, spi->spi_device, spi->spi_slave);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int ldx_spi_transfer(spi_t *spi, uint8_t *tx_data, uint8_t *rx_data, unsigned int length)
{
	if (check_spi(spi) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	if (check_data_buffer(tx_data) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	if (check_data_buffer(rx_data) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	if (length == 0)
		return EXIT_SUCCESS;

	log_debug("%s: Transferring %d bytes on SPI %d:%d", __func__, length,
		  spi->spi_device, spi->spi_slave);

	if (libsoc_spi_rw(spi->_data, tx_data, rx_data, length) != EXIT_SUCCESS) {
		log_error("%s: Unable to transfer %d bytes on SPI %d:%d",
			  __func__, length, spi->spi_device, spi->spi_slave);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/**
 * check_spi() - Verify that the SPI pointer is valid
 *
 * @spi:	The SPI pointer to check.
 *
 * Return: EXIT_SUCCESS if the SPI is valid, EXIT_FAILURE otherwise.
 */
static int check_spi(spi_t *spi)
{
	if (spi == NULL) {
		log_error("%s: SPI cannot be NULL", __func__);
		return EXIT_FAILURE;
	}

	if (spi->_data == NULL) {
		log_error("%s: Invalid SPI, %d:%d", __func__, spi->spi_device,
			  spi->spi_slave);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/**
 * check_transfer_mode() - Verify that the given SPI transfer mode is valid
 *
 * @transfer_mode:	The transfer mode to check.
 *
 * Return: EXIT_SUCCESS if the transfer mode is valid, EXIT_FAILURE otherwise.
 */
static int check_transfer_mode(spi_transfer_cfg_t *transfer_mode)
{
	if (transfer_mode == NULL) {
		log_error("%s: SPI transfer mode cannot be NULL", __func__);
		return EXIT_FAILURE;
	}

	if (check_clock_mode(transfer_mode->clk_mode) == EXIT_FAILURE
			|| check_chip_select(transfer_mode->chip_select) == EXIT_FAILURE
			|| check_bit_order(transfer_mode->bit_order) == EXIT_FAILURE)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

/**
 * check_clock_mode() - Verify that the given clock mode is valid
 *
 * @clk_mode:	The clock mode to check.
 *
 * Return: EXIT_SUCCESS if the clock mode is valid, EXIT_FAILURE otherwise.
 */
static int check_clock_mode(spi_clk_mode_t clk_mode)
{
	switch (clk_mode) {
	case SPI_CLK_MODE_0:
	case SPI_CLK_MODE_1:
	case SPI_CLK_MODE_2:
	case SPI_CLK_MODE_3:
		return EXIT_SUCCESS;
	default:
		log_error("%s: Invalid SPI clock mode, %d. Clock mode must be '%s', '%s', '%s', or '%s'",
			  __func__, clk_mode,
			  spi_clk_mode_strings[SPI_CLK_MODE_0],
			  spi_clk_mode_strings[SPI_CLK_MODE_1],
			  spi_clk_mode_strings[SPI_CLK_MODE_2],
			  spi_clk_mode_strings[SPI_CLK_MODE_3]);
		return EXIT_FAILURE;
	}
}

/**
 * check_chip_select() - Verify that the given chip select is valid
 *
 * @chip_select:	The chip select to check.
 *
 * Return: EXIT_SUCCESS if the chip select is valid, EXIT_FAILURE otherwise.
 */
static int check_chip_select(spi_cs_t chip_select)
{
	switch (chip_select) {
	case SPI_CS_ACTIVE_LOW:
	case SPI_CS_ACTIVE_HIGH:
	case SPI_CS_NO_CONT:
		return EXIT_SUCCESS;
	default:
		log_error("%s: Invalid SPI chip select, %d. Chip select must be '%s', '%s', or '%s'",
			  __func__, chip_select,
			  spi_cs_strings[SPI_CS_ACTIVE_LOW],
			  spi_cs_strings[SPI_CS_ACTIVE_HIGH],
			  spi_cs_strings[SPI_CS_NO_CONT]);
		return EXIT_FAILURE;
	}
}

/**
 * check_bit_order() - Verify that the given bit order is valid
 *
 * @bit_order:	The bit order to check.
 *
 * Return: EXIT_SUCCESS if the bit order is valid, EXIT_FAILURE otherwise.
 */
static int check_bit_order(spi_bo_t bit_order)
{
	switch (bit_order) {
	case SPI_BO_LSB_FIRST:
	case SPI_BO_MSB_FIRST:
		return EXIT_SUCCESS;
	default:
		log_error("%s: Invalid SPI bit order, %d. Bit order must be '%s', or '%s'",
			  __func__, bit_order, spi_bo_strings[SPI_BO_LSB_FIRST],
			  spi_bo_strings[SPI_BO_MSB_FIRST]);
		return EXIT_FAILURE;
	}
}

/**
 * check_bpw() - Verify that the given bits-per-word is valid
 *
 * @mode:	The bits-per-word to check.
 *
 * Return: EXIT_SUCCESS if the bits-per-word is valid, EXIT_FAILURE otherwise.
 */
static int check_bpw(spi_bpw_t bpw)
{
	switch (bpw) {
	case SPI_BPW_8:
	case SPI_BPW_16:
		return EXIT_SUCCESS;
	default:
		log_error("%s: Invalid SPI bits-per-word, %d. Bits-per-word must be '%s', or '%s'",
			  __func__, bpw, spi_bpw_strings[SPI_BPW_8],
			  spi_bpw_strings[SPI_BPW_16]);
		return EXIT_FAILURE;
	}
}

/**
 * check_data_buffer() - Verify that the given data buffer is valid
 *
 * @buffer:	The data buffer to check.
 *
 * Return: EXIT_SUCCESS if the data buffer is valid, EXIT_FAILURE otherwise.
 */
static int check_data_buffer(uint8_t *buffer)
{
	if (buffer == NULL) {
		log_error("%s: Data buffer cannot be NULL", __func__);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
