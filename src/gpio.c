/*
 * Copyright (c) 2017 Digi International Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * =======================================================================
 */

#include <fcntl.h>
#include <stdlib.h>
#include <libsoc_gpio.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "_log.h"
#include "gpio.h"

struct _gpio_t {
	int _mode;
	gpio *_internal_gpio;
};

#define BUFF_SIZE	256

#define _GPIO_DIR_MODES	M(in) \
			M(low) \
			M(high)

#define M(x)	x,
typedef enum {
	_GPIO_DIR_MODES
} _gpio_dir_modes_t;
#undef M

#define M(x)	#x,
static const char * const gpio_dir_strings[] = { _GPIO_DIR_MODES };

static const char * const gpio_mode_strings[] = {
	M(GPIO_INPUT)
	M(GPIO_OUTPUT_LOW)
	M(GPIO_OUTPUT_HIGH)
	M(GPIO_IRQ_EDGE_RISING)
	M(GPIO_IRQ_EDGE_FALLING)
	M(GPIO_IRQ_EDGE_BOTH)
};

static const char * const gpio_value_strings[] = {
	M(GPIO_LOW)
	M(GPIO_HIGH)
};
#undef M

static const char * const gpio_edge_strings[] = {
	"rising",
	"falling",
	"none",
	"both"
};

static int check_gpio(gpio_t *gpio);
static int set_direction(gpio *gpio, _gpio_dir_modes_t dir);
static int check_mode(gpio_mode_t mode);

gpio_t *gpio_request(unsigned int kernel_number, gpio_mode_t mode, request_mode_t request_mode)
{
	gpio_t *new_gpio = NULL;
	struct _gpio_t *data = NULL;
	gpio *internal_gpio = NULL;
	gpio_t init_gpio = { NULL, kernel_number, NULL };

	if (check_mode(mode) != EXIT_SUCCESS)
		return NULL;

	switch (request_mode) {
	case REQUEST_SHARED:
	case REQUEST_GREEDY:
	case REQUEST_WEAK:
		break;
	default:
		request_mode = REQUEST_SHARED;
		log_info("%s: Invalid request mode, setting to 'REQUEST_SHARED'",
			 __func__);
		break;
	}

	log_debug("%s: Requesting GPIO %d [mode '%s' (%d), request mode: %d]",
		  __func__, kernel_number, gpio_mode_strings[mode],
		  mode, request_mode);

	internal_gpio = libsoc_gpio_request(kernel_number, request_mode);
	if (internal_gpio == NULL)
		return NULL;

	data = calloc(1, sizeof(struct _gpio_t));
	if (data == NULL) {
		log_error("%s: Unable to request GPIO %d [mode: '%s' (%d), request mode: %d], cannot allocate memory",
			  __func__, kernel_number, gpio_mode_strings[mode],
			  mode, request_mode);
		libsoc_gpio_free(internal_gpio);
		return NULL;
	}

	new_gpio = calloc(1, sizeof(gpio_t));
	if (new_gpio == NULL) {
		log_error("%s: Unable to request GPIO %d [mode: '%s' (%d), request mode: %d], cannot allocate memory",
			  __func__, kernel_number, gpio_mode_strings[mode],
			  mode, request_mode);
		libsoc_gpio_free(internal_gpio);
		free(data);
		return NULL;
	}

	data->_mode = GPIO_MODE_ERROR;
	data->_internal_gpio = internal_gpio;

	memcpy(new_gpio, &init_gpio, sizeof(gpio_t));
	new_gpio->_data = data;

	if (gpio_set_mode(new_gpio, mode) != EXIT_SUCCESS) {
		gpio_free(new_gpio);
		return NULL;
	}

	return new_gpio;
}

int gpio_free(gpio_t *gpio)
{
	int ret = EXIT_SUCCESS;
	struct _gpio_t *_data = NULL;

	if (gpio == NULL)
		return EXIT_SUCCESS;

	log_debug("%s: Freeing GPIO %d", __func__, gpio->kernel_number);

	_data = gpio->_data;

	if (gpio->_data != NULL && _data->_internal_gpio != NULL)
		ret = libsoc_gpio_free(_data->_internal_gpio);

	free(gpio->_data);
	free(gpio);

	return ret;
}

int gpio_set_mode(gpio_t *gpio, gpio_mode_t mode)
{
	int ret = EXIT_FAILURE;
	struct _gpio_t *_data = NULL;
	_gpio_dir_modes_t dir = in;
	gpio_edge edge = EDGE_ERROR;

	if (check_gpio(gpio) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	if (check_mode(mode) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	log_debug("%s: Setting mode for GPIO %d, mode: '%s' (%d)", __func__,
		  gpio->kernel_number, gpio_mode_strings[mode], mode);

	switch (mode) {
	case GPIO_INPUT:
		dir = in;
		edge = NONE;
		break;
	case GPIO_OUTPUT_LOW:
		dir = low;
		edge = EDGE_ERROR;
		break;
	case GPIO_OUTPUT_HIGH:
		dir = high;
		edge = EDGE_ERROR;
		break;
	case GPIO_IRQ_EDGE_RISING:
		dir = in;
		edge = RISING;
		break;
	case GPIO_IRQ_EDGE_FALLING:
		dir = in;
		edge = FALLING;
		break;
	case GPIO_IRQ_EDGE_BOTH:
		dir = in;
		edge = BOTH;
		break;
	default:
		/* Should not occur */
		return EXIT_FAILURE;
	}

	_data = gpio->_data;

	ret = set_direction(_data->_internal_gpio, dir);
	if (ret != EXIT_SUCCESS) {
		log_error("%s: Unable to set GPIO %d direction to '%s' (%d)",
			  __func__, gpio->kernel_number,
			  gpio_dir_strings[dir], dir);
		return ret;
	}

	if (edge != EDGE_ERROR) {
		ret = libsoc_gpio_set_edge(_data->_internal_gpio, edge);
		if (ret != EXIT_SUCCESS) {
			log_error("%s: Unable to set GPIO %d edge to '%s' (%d)",
				  __func__, gpio->kernel_number,
				  gpio_edge_strings[edge], edge);
		}
	}

	if (ret == EXIT_SUCCESS)
		_data->_mode = mode;

	return ret;
}

gpio_mode_t gpio_get_mode(gpio_t *gpio)
{
	struct _gpio_t *_data = NULL;
	gpio_direction dir = DIRECTION_ERROR;
	gpio_edge edge = EDGE_ERROR;

	if (check_gpio(gpio) != EXIT_SUCCESS)
		return GPIO_MODE_ERROR;

	log_debug("%s: Getting mode of GPIO %d", __func__, gpio->kernel_number);

	_data = gpio->_data;

	dir = libsoc_gpio_get_direction(_data->_internal_gpio);
	if (dir == DIRECTION_ERROR) {
		log_error("%s: Unable to get GPIO %d direction",
			  __func__, gpio->kernel_number);
		return GPIO_MODE_ERROR;
	}

	if (dir == INPUT) {
		edge = libsoc_gpio_get_edge(_data->_internal_gpio);
		if (edge == EDGE_ERROR) {
			log_error("%s: Unable to get GPIO %d edge",
				  __func__, gpio->kernel_number);
			return GPIO_MODE_ERROR;
		}
	}

	switch (dir) {
	case OUTPUT:
		if (_data->_mode == GPIO_OUTPUT_LOW
				|| _data->_mode == GPIO_OUTPUT_HIGH)
			return _data->_mode;
		return GPIO_OUTPUT_LOW;
	case INPUT:
		switch (edge) {
		case RISING:
			_data->_mode = GPIO_IRQ_EDGE_RISING;
			break;
		case FALLING:
			_data->_mode = GPIO_IRQ_EDGE_FALLING;
			break;
		case NONE:
			_data->_mode = GPIO_INPUT;
			break;
		case BOTH:
			_data->_mode = GPIO_IRQ_EDGE_BOTH;
			break;
		default:
			/* Should not occur */
			return GPIO_MODE_ERROR;
		}
		return _data->_mode;
	default:
		/* Should not occur */
		return GPIO_MODE_ERROR;
	}

	return GPIO_MODE_ERROR;
}

int gpio_set_value(gpio_t *gpio, gpio_value_t value)
{
	struct _gpio_t *_data = NULL;
	int ret = EXIT_FAILURE;

	if (check_gpio(gpio) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	switch (value) {
	case GPIO_LOW:
	case GPIO_HIGH:
		break;
	default:
		log_error("%s: Invalid GPIO value, %d. Mode must be '%s' or '%s'",
			  __func__, value,
			  gpio_value_strings[GPIO_LOW],
			  gpio_value_strings[GPIO_HIGH]);
		return EXIT_FAILURE;
	}

	log_debug("%s: Setting value for GPIO %d, value: %d", __func__,
		  gpio->kernel_number, value);

	_data = gpio->_data;

	ret = libsoc_gpio_set_level(_data->_internal_gpio, value);
	if (ret != EXIT_SUCCESS)
		log_error("%s: Unable to set GPIO %d value to %d",
			  __func__, gpio->kernel_number, value);

	return ret;
}

gpio_value_t gpio_get_value(gpio_t *gpio)
{
	struct _gpio_t *_data = NULL;
	gpio_level level = LEVEL_ERROR;

	if (check_gpio(gpio) != EXIT_SUCCESS)
		return GPIO_VALUE_ERROR;

	log_debug("%s: Getting value of GPIO %d", __func__,
		  gpio->kernel_number);

	_data = gpio->_data;

	level = libsoc_gpio_get_level(_data->_internal_gpio);
	if (level == LEVEL_ERROR) {
		log_error("%s: Unable to get GPIO %d value", __func__,
			  gpio->kernel_number);
		return GPIO_VALUE_ERROR;
	}

	return level;
}

gpio_irq_error_t gpio_wait_interrupt(gpio_t *gpio, int timeout)
{
	struct _gpio_t *_data = NULL;
	gpio_int_ret int_ret = GPIO_IRQ_ERROR;

	if (timeout < -1) {
		log_error("%s: Invalid timeout value, %d", __func__, timeout);
		return GPIO_IRQ_ERROR;
	}

	if (check_gpio(gpio) != EXIT_SUCCESS)
		return GPIO_IRQ_ERROR;

	log_debug("%s: Waiting interrupt on GPIO %d (timeout: %d ms)", __func__,
		  gpio->kernel_number, timeout);

	_data = gpio->_data;

	int_ret = libsoc_gpio_wait_interrupt(_data->_internal_gpio, timeout);
	switch (int_ret) {
	case LS_INT_TRIGGERED:
		return GPIO_IRQ_ERROR_NONE;
	case LS_INT_TIMEOUT:
		log_debug("%s: Timeout waiting for interrupt on GPIO %d",
			  __func__, gpio->kernel_number);
		return GPIO_IRQ_ERROR_TIMEOUT;
	case LS_INT_ERROR:
		log_error("%s: Invalid GPIO mode. Mode must be '%s', '%s', or '%s'",
			  __func__,
			  gpio_mode_strings[GPIO_IRQ_EDGE_RISING],
			  gpio_mode_strings[GPIO_IRQ_EDGE_FALLING],
			  gpio_mode_strings[GPIO_IRQ_EDGE_BOTH]);
		return GPIO_IRQ_ERROR;
	default:
		/* Should not occur */
		return GPIO_IRQ_ERROR;
	}
}

int gpio_start_wait_interrupt(gpio_t *gpio, const gpio_interrupt_cb_t interrupt_cb, void *arg)
{
	struct _gpio_t *_data = NULL;
	gpio_edge edge = EDGE_ERROR;

	if (check_gpio(gpio) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	_data = gpio->_data;

	if (libsoc_gpio_get_direction(_data->_internal_gpio) != INPUT) {
		log_error("%s: Invalid GPIO mode. Mode must be '%s', '%s', or '%s'",
			  __func__,
			  gpio_mode_strings[GPIO_IRQ_EDGE_RISING],
			  gpio_mode_strings[GPIO_IRQ_EDGE_FALLING],
			  gpio_mode_strings[GPIO_IRQ_EDGE_BOTH]);
		return EXIT_FAILURE;
	}

	edge = libsoc_gpio_get_edge(_data->_internal_gpio);
	if (edge == EDGE_ERROR || edge == NONE) {
		log_error("%s: Invalid GPIO mode. Mode must be '%s', '%s', or '%s'",
			  __func__,
			  gpio_mode_strings[GPIO_IRQ_EDGE_RISING],
			  gpio_mode_strings[GPIO_IRQ_EDGE_FALLING],
			  gpio_mode_strings[GPIO_IRQ_EDGE_BOTH]);
		return EXIT_FAILURE;
	}

	log_debug("%s: Start waiting for interrupts on GPIO %d", __func__,
		  gpio->kernel_number);

	return libsoc_gpio_callback_interrupt(_data->_internal_gpio, interrupt_cb, arg);
}

int gpio_stop_wait_interrupt(gpio_t *gpio)
{
	struct _gpio_t *_data = NULL;
	int ret = EXIT_FAILURE;

	if (check_gpio(gpio) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	_data = gpio->_data;
	if (_data->_internal_gpio->callback == NULL)
		return EXIT_SUCCESS;

	ret = libsoc_gpio_callback_interrupt_cancel(_data->_internal_gpio);
	if (ret != EXIT_SUCCESS) {
		log_error("%s: Unable to stop waiting for interrupts on GPIO %d",
			  __func__, gpio->kernel_number);
	} else {
		log_debug("%s: Stop waiting for interrupts on GPIO %d",
			  __func__, gpio->kernel_number);
	}

	return ret;
}

/**
 * check_gpio() - Verify that the GPIO pointer is valid
 *
 * @gpio:	The GPIO pointer to check.
 *
 * Return: EXIT_SUCCESS if the GPIO is valid, EXIT_FAILURE otherwise.
 */
static int check_gpio(gpio_t *gpio)
{
	struct _gpio_t *_data = NULL;

	if (gpio == NULL) {
		log_error("%s: GPIO cannot be NULL", __func__);
		return EXIT_FAILURE;
	}

	_data = gpio->_data;
	if (_data == NULL || _data->_internal_gpio == NULL) {
		log_error("%s: Invalid GPIO, %d", __func__,
			  gpio->kernel_number);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/**
 * set_direction() - Set GPIO to input or output
 *
 * @gpio:	A pointer to the requested GPIO to set its direction.
 * @dir:	The desired GPIO direction (_gpio_dir_modes_t).
 *
 * This function configures the direction (_gpio_dir_modes_t) of the given GPIO:
 *    - in: input, its value can be read.
 *    - low: output with low as initial value, its value can be written.
 *    - high: output with high as initial value, its value can be written.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
static int set_direction(gpio *gpio, _gpio_dir_modes_t dir)
{
	int fd;
	char path[BUFF_SIZE];

	sprintf(path, "/sys/class/gpio/gpio%d/direction", gpio->gpio);

	fd = open(path, O_SYNC | O_WRONLY);
	if (fd < 0)
		return EXIT_FAILURE;

	if (write(fd, gpio_dir_strings[dir], BUFF_SIZE) < 0) {
		close(fd);
		return EXIT_FAILURE;
	}

	if (close(fd) < 0)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

/**
 * check_mode() - Verify that the given mode is valid
 *
 * @mode:	The mode to check.
 *
 * Return: EXIT_SUCCESS if the mode is valid, EXIT_FAILURE otherwise.
 */
static int check_mode(gpio_mode_t mode)
{
	switch (mode) {
	case GPIO_INPUT:
	case GPIO_OUTPUT_LOW:
	case GPIO_OUTPUT_HIGH:
	case GPIO_IRQ_EDGE_RISING:
	case GPIO_IRQ_EDGE_FALLING:
	case GPIO_IRQ_EDGE_BOTH:
		return EXIT_SUCCESS;
	default:
		log_error("%s: Invalid GPIO mode, %d. Mode must be '%s', '%s', '%s', '%s', '%s', or '%s'",
			  __func__, mode, gpio_mode_strings[GPIO_INPUT],
			  gpio_mode_strings[GPIO_OUTPUT_LOW],
			  gpio_mode_strings[GPIO_OUTPUT_HIGH],
			  gpio_mode_strings[GPIO_IRQ_EDGE_RISING],
			  gpio_mode_strings[GPIO_IRQ_EDGE_FALLING],
			  gpio_mode_strings[GPIO_IRQ_EDGE_BOTH]);
		return EXIT_FAILURE;
	}
}
