/*
 * Copyright 2017-2020, Digi International Inc.
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
#include <errno.h>
#include <gpiod.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>

#include "_common.h"
#include "_libsoc_interfaces.h"
#include "_log.h"
#include "gpio.h"

struct wait_irq_t {
	pthread_t *poll_thread;
	struct poll_ctx_t *poll_ctx;
};

struct poll_ctx_t {
	int fd;
	int sigfd;
	int (*callback_fn) (void *);
	void *arg;
};

struct _gpio_t {
	int _mode;
	gpio_active_mode_t _active_mode;
	libsoc_gpio_t *_internal_gpio;
	struct gpiod_chip *_chip;
	struct gpiod_line *_line;
	struct wait_irq_t *_wait_irq;
};

#define UNDEFINED_SYSFS_GPIO (-1)

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

static const char * const gpio_active_mode_strings[] = {
	"0",
	"1"
};

static const char * const gpio_edge_strings[] = {
	"rising",
	"falling",
	"none",
	"both"
};

static int check_gpio(gpio_t *gpio);
static char * show_gpio(gpio_t *gpio);
static int set_direction(libsoc_gpio_t *gpio, _gpio_dir_modes_t dir);
static int check_mode(gpio_mode_t mode);

gpio_t *ldx_gpio_request(unsigned int kernel_number, gpio_mode_t mode,
			 request_mode_t request_mode)
{
	gpio_t *new_gpio = NULL;
	struct _gpio_t *data = NULL;
	gpio *internal_gpio = NULL;
	gpio_t init_gpio = {
		.alias = NULL,
		.kernel_number = kernel_number,
		.gpio_controller = NULL,
		.gpio_line = -1,
		._data = NULL
	};

	if (check_mode(mode) != EXIT_SUCCESS)
		return NULL;

	if (check_request_mode(request_mode) != EXIT_SUCCESS) {
		request_mode = REQUEST_SHARED;
		log_info("%s: Invalid request mode, setting to 'REQUEST_SHARED'",
			 __func__);
	}

	log_debug("%s: Requesting GPIO %d [mode '%s' (%d), request mode: %d]",
		  __func__, kernel_number, gpio_mode_strings[mode],
		  mode, request_mode);

	internal_gpio = libsoc_gpio_request(kernel_number, request_mode);
	if (internal_gpio == NULL)
		return NULL;

	data = calloc(1, sizeof(struct _gpio_t));
	if (data == NULL) {
		log_error("%s: Unable to request GPIO %d "
			  "[mode: '%s' (%d), request mode: %d], "
			  "cannot allocate memory",
			  __func__, kernel_number, gpio_mode_strings[mode],
			  mode, request_mode);
		libsoc_gpio_free(internal_gpio);
		return NULL;
	}

	new_gpio = calloc(1, sizeof(gpio_t));
	if (new_gpio == NULL) {
		log_error("%s: Unable to request GPIO %d "
			  "[mode: '%s' (%d), request mode: %d], "
			  "cannot allocate memory",
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

	if (ldx_gpio_set_mode(new_gpio, mode) != EXIT_SUCCESS) {
		ldx_gpio_free(new_gpio);
		return NULL;
	}

	return new_gpio;
}

gpio_t *ldx_gpio_request_by_alias(const char * const gpio_alias, gpio_mode_t mode,
				  request_mode_t request_mode)
{
	char *controller_label = NULL;
	int line, kernel_number, controller_ret;
	gpio_t *new_gpio = NULL;

	if (check_mode(mode) != EXIT_SUCCESS)
		return NULL;

	if (check_request_mode(request_mode) != EXIT_SUCCESS) {
		request_mode = REQUEST_SHARED;
		log_info("%s: Invalid request mode, setting to 'REQUEST_SHARED'",
			 __func__);
	}

	log_debug("%s: Requesting GPIO '%s' [mode '%s' (%d), request mode: %d]",
		  __func__, gpio_alias, gpio_mode_strings[mode], mode, request_mode);

	/* Attempt parsing the configuration file as '<alias> = <controller>,<line>' */
	controller_label = calloc(1, MAX_CONTROLLER_LEN);
	if (controller_label == NULL) {
		log_error("%s: Unable to request GPIO. Cannot allocate memory", __func__);
		return NULL;
	}
	controller_ret = ldx_gpio_get_controller(gpio_alias, controller_label);
	line = ldx_gpio_get_line(gpio_alias);
	if ((controller_ret == -1) || (line == -1))  {
		free (controller_label);
		goto attempt_sysfs;
	}

	new_gpio = ldx_gpio_request_by_controller(controller_label, line, mode);
	if (new_gpio != NULL) {
		gpio_t init_gpio = {
			.alias = gpio_alias,
			.kernel_number = UNDEFINED_SYSFS_GPIO,
			.gpio_controller = controller_label,
			.gpio_line = line,
			._data = new_gpio->_data
		};

		memcpy(new_gpio, &init_gpio, sizeof(gpio_t));
	}

	return new_gpio;

attempt_sysfs:
	/* Attempt parsing the configuration file as '<alias> = <kernel_number>' */
	kernel_number = ldx_gpio_get_kernel_number(gpio_alias);
	if (kernel_number == UNDEFINED_SYSFS_GPIO) {
		log_error("%s: Invalid GPIO alias, '%s'", __func__, gpio_alias);
		return NULL;
	}

	new_gpio = ldx_gpio_request(kernel_number, mode, request_mode);
	if (new_gpio != NULL) {
		gpio_t init_gpio = {
			.alias = gpio_alias,
			.kernel_number = kernel_number,
			.gpio_controller = NULL,
			.gpio_line = -1,
			._data = new_gpio->_data
		};

		memcpy(new_gpio, &init_gpio, sizeof(gpio_t));
	}

	return new_gpio;
}

gpio_t *ldx_gpio_request_by_controller(const char * const controller,
				       const unsigned char line_num,
				       gpio_mode_t mode)
{
	gpio_t *new_gpio = NULL;
	struct _gpio_t *data = NULL;
	struct gpiod_chip *chip = NULL;
	struct gpiod_line *line = NULL;
	gpio_t init_gpio = {
		.alias = NULL,
		.kernel_number = UNDEFINED_SYSFS_GPIO,
		.gpio_controller = controller,
		.gpio_line = line_num,
		._data = NULL
	};

	if (check_mode(mode) != EXIT_SUCCESS)
		return NULL;

	log_debug("%s: Requesting GPIO '%s %d' [mode '%s' (%d)",
		  __func__, controller, line_num, gpio_mode_strings[mode], mode);

	chip = gpiod_chip_open_lookup(controller);
	if (!chip) {
		log_error("%s: Unable to request GPIO '%s %d' "
			  "[mode: '%s' (%d)], chip open failed", __func__,
			  controller, line_num, gpio_mode_strings[mode], mode);
		return NULL;
	}
	line = gpiod_chip_get_line(chip, line_num);
	if (!line) {
		log_error("%s: Unable to request GPIO '%s %d' "
			  "[mode: '%s' (%d)], chip get line failed", __func__,
			  controller, line_num, gpio_mode_strings[mode], mode);
		gpiod_chip_close(chip);
		return NULL;
	}

	data = calloc(1, sizeof(struct _gpio_t));
	if (data == NULL) {
		log_error("%s: Unable to request GPIO '%s %d' "
			  "[mode: '%s' (%d)], cannot allocate memory", __func__,
			  controller, line_num, gpio_mode_strings[mode], mode);
		gpiod_chip_close(chip);
		return NULL;
	}

	new_gpio = calloc(1, sizeof(gpio_t));
	if (new_gpio == NULL) {
		log_error("%s: Unable to request GPIO '%s %d' "
			  "[mode: '%s' (%d)], cannot allocate memory", __func__,
			  controller, line_num, gpio_mode_strings[mode], mode);
		gpiod_chip_close(chip);
		free(data);
		return NULL;
	}


	data->_mode = GPIO_MODE_ERROR;
	data->_active_mode = GPIO_ACTIVE_HIGH;
	data->_chip = chip;
	data->_line = line;

	memcpy(new_gpio, &init_gpio, sizeof(gpio_t));
	new_gpio->_data = data;

	if (ldx_gpio_set_mode(new_gpio, mode) != EXIT_SUCCESS) {
		ldx_gpio_free(new_gpio);
		return NULL;
	}

	return new_gpio;
}

int ldx_gpio_get_kernel_number(const char * const gpio_alias)
{
	if (config_check_alias(gpio_alias) != EXIT_SUCCESS)
		return -1;

	return config_get_gpio_kernel_number(gpio_alias);
}

int ldx_gpio_get_controller(const char * const gpio_alias, char * const controller)
{
	if (config_check_alias(gpio_alias) != EXIT_SUCCESS)
		return -1;

	return config_get_gpio_controller(gpio_alias, controller);
}

int ldx_gpio_get_line(const char * const gpio_alias)
{
	if (config_check_alias(gpio_alias) != EXIT_SUCCESS)
		return -1;

	return config_get_gpio_line(gpio_alias);
}

int ldx_gpio_free(gpio_t *gpio)
{
	int ret = EXIT_SUCCESS;
	struct _gpio_t *_data = NULL;

	if (gpio == NULL)
		return EXIT_SUCCESS;

	log_debug("%s: Freeing GPIO %s", __func__, show_gpio(gpio));

	_data = gpio->_data;

	if (gpio->_data == NULL)
		return EXIT_SUCCESS;

	if (_data->_internal_gpio != NULL)
		ret = libsoc_gpio_free(_data->_internal_gpio);

	if (_data->_line != NULL)
		gpiod_line_release(_data->_line);

	if (_data->_chip != NULL)
		gpiod_chip_close(_data->_chip);

	free(gpio->_data);

	/* Free controller label if requested internally by request_by_alias */
	if (gpio->gpio_controller != NULL && gpio->alias != NULL)
		free((char*)gpio->gpio_controller);
	free(gpio);

	return ret;
}

int ldx_gpio_set_debounce(gpio_t *gpio, unsigned int usec)
{
	char buf[BUFF_SIZE];
	int fd, ret = EXIT_SUCCESS;

	if (check_gpio(gpio) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	log_debug("%s: Setting debounce for GPIO %s to: '%u'", __func__,
		  show_gpio(gpio), usec);

	if (gpio->kernel_number == UNDEFINED_SYSFS_GPIO) {
#if defined(GPIO_SET_DEBOUNCE_IOCTL)
		struct _gpio_t *_data = _data = gpio->_data;
		struct gpioline_debounce linedebounce;
		char chip_path[] = "/dev/gpiochipXXX";

		memset(&linedebounce, 0, sizeof(linedebounce));

		linedebounce.line_offset = gpiod_line_offset(_data->_line);
		linedebounce.debounce_usec = usec;

		sprintf(chip_path, "/dev/%s", gpiod_chip_name(_data->_chip));
		fd = open(chip_path, O_RDWR);
		if (fd < 0)
			return EXIT_FAILURE;

		if (ioctl(fd, GPIO_SET_DEBOUNCE_IOCTL, &linedebounce) < 0) {
			log_error("%s: GPIO_SET_DEBOUNCE_IOCTL failed. Err %d",
				  __func__, ret);
			ret = EXIT_FAILURE;
		}

		close(fd);
#else
		ret = EXIT_FAILURE;
#endif
	} else {
		sprintf(buf, "/sys/class/gpio/gpio%d/debounce", gpio->kernel_number);

		fd = open(buf, O_SYNC | O_WRONLY);
		if (fd < 0)
			return EXIT_FAILURE;

		sprintf(buf, "%u", usec);

		if (write(fd, buf, strlen(buf)) != strlen(buf))
			ret = EXIT_FAILURE;

		close(fd);
	}

	return ret;
}

int ldx_gpio_set_mode(gpio_t *gpio, gpio_mode_t mode)
{
	int ret = EXIT_FAILURE;
	struct _gpio_t *_data = NULL;

	if (check_gpio(gpio) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	if (check_mode(mode) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	log_debug("%s: Setting mode for GPIO %s, mode: '%s' (%d)", __func__,
		  show_gpio(gpio), gpio_mode_strings[mode], mode);

	_data = gpio->_data;

	if (gpio->kernel_number == UNDEFINED_SYSFS_GPIO) {
		struct gpiod_line_request_config request_cfg = { 0 };
		int default_val = 0;

		request_cfg.consumer = show_gpio(gpio);
		request_cfg.request_type = GPIOD_LINE_REQUEST_DIRECTION_AS_IS;

		if (_data->_active_mode == GPIO_ACTIVE_LOW)
			request_cfg.flags |= GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;

		switch (mode) {
		case GPIO_INPUT:
			request_cfg.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
			break;
		case GPIO_OUTPUT_LOW:
			request_cfg.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
			default_val = 0;
			break;
		case GPIO_OUTPUT_HIGH:
			request_cfg.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
			default_val = 1;
			break;
		case GPIO_IRQ_EDGE_RISING:
			request_cfg.request_type = GPIOD_LINE_REQUEST_EVENT_RISING_EDGE;
			break;
		case GPIO_IRQ_EDGE_FALLING:
			request_cfg.request_type = GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE;
			break;
		case GPIO_IRQ_EDGE_BOTH:
			request_cfg.request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;
			break;
		default:
			/* Should not occur */
			return EXIT_FAILURE;
		}

		if (gpiod_line_is_used(_data->_line)) {
			/* Check if line was requested by us */
			if (gpiod_line_is_requested(_data->_line)) {
				log_debug("%s: GPIO %s was requested by us",
					  __func__, show_gpio(gpio));

				/* Release it so we can request it again */
				gpiod_line_release(_data->_line);
			} else {
				log_error("%s: GPIO %s was in use by '%s'",
					  __func__, show_gpio(gpio),
					  gpiod_line_consumer(_data->_line));
				return EXIT_FAILURE;
			}
		}

		ret = gpiod_line_request(_data->_line, &request_cfg, default_val);
		if (ret != EXIT_SUCCESS) {
			log_error("%s: Unable to set GPIO %s to mode: '%s' (%d)",
				  __func__, show_gpio(gpio), gpio_mode_strings[mode],
				  mode);
			return ret;
		}
	} else {
		_gpio_dir_modes_t dir = in;
		libsoc_gpio_edge_t edge = EDGE_ERROR;

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

		ret = set_direction(_data->_internal_gpio, dir);
		if (ret != EXIT_SUCCESS) {
			log_error("%s: Unable to set GPIO %d direction to '%s' (%d)",
				  __func__, gpio->kernel_number,
				  gpio_dir_strings[dir], dir);
			return ret;
		}

		if ((edge != EDGE_ERROR) && (edge != NONE)) {
			ret = libsoc_gpio_set_edge(_data->_internal_gpio, edge);
			if (ret != EXIT_SUCCESS) {
				log_error("%s: Unable to set GPIO %d edge to '%s' (%d)",
					  __func__, gpio->kernel_number,
					  gpio_edge_strings[edge], edge);
			}
		}
	}

	if (ret == EXIT_SUCCESS)
		_data->_mode = mode;

	return ret;
}

gpio_mode_t ldx_gpio_get_mode(gpio_t *gpio)
{
	struct _gpio_t *_data = NULL;

	if (check_gpio(gpio) != EXIT_SUCCESS)
		return GPIO_MODE_ERROR;

	log_debug("%s: Getting mode of GPIO %s", __func__, show_gpio(gpio));

	_data = gpio->_data;

	if (gpio->kernel_number == UNDEFINED_SYSFS_GPIO) {
		return _data->_mode;
	} else {
		libsoc_gpio_direction_t dir = DIRECTION_ERROR;
		libsoc_gpio_edge_t edge = EDGE_ERROR;

		dir = libsoc_gpio_get_direction(_data->_internal_gpio);
		if (dir == DIRECTION_ERROR) {
			log_error("%s: Unable to get GPIO %d direction",
				  __func__, gpio->kernel_number);
			return GPIO_MODE_ERROR;
		}

		if (dir == INPUT) {
			edge = libsoc_gpio_get_edge(_data->_internal_gpio);
			if (edge == EDGE_ERROR) {
				log_warning("%s: Unable to get GPIO %d edge",
					    __func__, gpio->kernel_number);
				edge = NONE;
			}
		}

		switch (dir) {
		case OUTPUT:
			if (_data->_mode == GPIO_OUTPUT_LOW ||
				_data->_mode == GPIO_OUTPUT_HIGH)
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
	}

	return GPIO_MODE_ERROR;
}

int ldx_gpio_set_value(gpio_t *gpio, gpio_value_t value)
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

	log_debug("%s: Setting value for GPIO %s, value: %d", __func__,
		  show_gpio(gpio), value);

	_data = gpio->_data;

	if (gpio->kernel_number == UNDEFINED_SYSFS_GPIO)
		ret = gpiod_line_set_value(_data->_line, value);
	else
		ret = libsoc_gpio_set_level(_data->_internal_gpio, value);

	if (ret != EXIT_SUCCESS)
		log_error("%s: Unable to set GPIO %s value to %d",
			  __func__, show_gpio(gpio), value);

	return ret;
}

gpio_value_t ldx_gpio_get_value(gpio_t *gpio)
{
	struct _gpio_t *_data = NULL;
	int level;

	if (check_gpio(gpio) != EXIT_SUCCESS)
		return GPIO_VALUE_ERROR;

	log_debug("%s: Getting value of GPIO %s", __func__,
		  show_gpio(gpio));

	_data = gpio->_data;

	if (gpio->kernel_number == UNDEFINED_SYSFS_GPIO)
		level = gpiod_line_get_value(_data->_line);
	else
		level = libsoc_gpio_get_level(_data->_internal_gpio);

	if (level == LEVEL_ERROR) {
		log_error("%s: Unable to get GPIO %s value", __func__,
			  show_gpio(gpio));
		return GPIO_VALUE_ERROR;
	}
	return level;
}

int ldx_gpio_set_active_mode(gpio_t *gpio, gpio_active_mode_t active_mode)
{
	struct _gpio_t *_data = gpio->_data;

	if (check_gpio(gpio) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	switch (active_mode) {
	case GPIO_ACTIVE_HIGH:
	case GPIO_ACTIVE_LOW:
		break;
	default:
		log_error("%s: Invalid GPIO active_mode value, %d. "
			  "Mode must be '%s' or '%s'",
			  __func__, active_mode,
			  gpio_active_mode_strings[GPIO_ACTIVE_HIGH],
			  gpio_active_mode_strings[GPIO_ACTIVE_LOW]);
		return EXIT_FAILURE;
	}

	log_debug("%s: Setting active_mode for GPIO %s, value: %d", __func__,
		  show_gpio(gpio), active_mode);

	_data->_active_mode = active_mode;

	if (gpio->kernel_number == UNDEFINED_SYSFS_GPIO) {
		int current_active_mode = gpiod_line_active_state(_data->_line);

		/* Check if desired mode is already set */
		if ((current_active_mode == GPIOD_LINE_ACTIVE_STATE_HIGH &&
			active_mode == GPIO_ACTIVE_HIGH) ||
			(current_active_mode == GPIOD_LINE_ACTIVE_STATE_LOW &&
			active_mode == GPIO_ACTIVE_LOW))
			return EXIT_SUCCESS;

		if (ldx_gpio_set_mode(gpio, _data->_mode) != EXIT_SUCCESS)
			return EXIT_FAILURE;
	} else {
		int fd;
		char path[BUFF_SIZE];

		sprintf(path, "/sys/class/gpio/gpio%d/active_low", gpio->kernel_number);

		fd = open(path, O_SYNC | O_WRONLY);

		if (fd < 0) {
			log_error("%s: Unable to set GPIO %d active mode",
					  __func__, gpio->kernel_number);
			return EXIT_FAILURE;
		}

		if (write(fd, gpio_active_mode_strings[active_mode], BUFF_SIZE) < 0) {
			log_error("%s: Unable to change GPIO %d active mode",
					  __func__, gpio->kernel_number);
			close(fd);
			return EXIT_FAILURE;
		}

		if (close(fd) < 0) {
			log_error("%s: Unable to set GPIO %d active mode",
					  __func__, gpio->kernel_number);
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

gpio_active_mode_t ldx_gpio_get_active_mode(gpio_t *gpio)
{
	if (check_gpio(gpio) != EXIT_SUCCESS)
		return GPIO_ACTIVE_MODE_ERROR;

	log_debug("%s: Getting active_low attribute of GPIO %s", __func__,
		  show_gpio(gpio));

	if (gpio->kernel_number == UNDEFINED_SYSFS_GPIO) {
		struct _gpio_t *_data = gpio->_data;
		int current_active_mode;

		current_active_mode = gpiod_line_active_state(_data->_line);

		return (current_active_mode == GPIOD_LINE_ACTIVE_STATE_HIGH) ?
			   GPIO_ACTIVE_HIGH : GPIO_ACTIVE_LOW;
	} else {
		char tmp_str[BUFF_SIZE], level[2];
		int fd;

		sprintf(tmp_str, "/sys/class/gpio/gpio%d/active_low", gpio->kernel_number);

		fd = open(tmp_str, O_RDONLY);

		if (fd < 0) {
			log_error("%s: Unable to get GPIO %d active mode",
					  __func__, gpio->kernel_number);
			return GPIO_ACTIVE_MODE_ERROR;
		}

		lseek(fd, 0, SEEK_SET);

		if (read(fd, level, 2) != 2) {
			log_error("%s: Unable to get GPIO %d active mode",
					  __func__, gpio->kernel_number);
			close(fd);
			return GPIO_ACTIVE_MODE_ERROR;
		}

		if (close(fd) < 0) {
			log_error("%s: Unable to get GPIO %d active mode",
					  __func__, gpio->kernel_number);
			return GPIO_ACTIVE_MODE_ERROR;
		}

		return level[0] == '0' ? GPIO_ACTIVE_HIGH : GPIO_ACTIVE_LOW;
	}
}

gpio_irq_error_t ldx_gpio_wait_interrupt(gpio_t *gpio, int timeout)
{
	struct _gpio_t *_data = NULL;

	if (timeout < -1) {
		log_error("%s: Invalid timeout value, %d", __func__, timeout);
		return GPIO_IRQ_ERROR;
	}

	if (check_gpio(gpio) != EXIT_SUCCESS)
		return GPIO_IRQ_ERROR;

	log_debug("%s: Waiting interrupt on GPIO %s (timeout: %d ms)", __func__,
		  show_gpio(gpio), timeout);

	_data = gpio->_data;

	if (gpio->kernel_number == UNDEFINED_SYSFS_GPIO) {
		struct timespec ts = {
			.tv_sec = timeout / 1000,
			.tv_nsec = (timeout % 1000) * 1000000
		};
		struct gpiod_line_event event;
		int rv;

		if (timeout == -1)
			rv = gpiod_line_event_wait(_data->_line, NULL);
		else
			rv = gpiod_line_event_wait(_data->_line, &ts);
		switch (rv) {
		case 1:
			rv = gpiod_line_event_read(_data->_line, &event);
			return GPIO_IRQ_ERROR_NONE;
		case 0:
			log_debug("%s: Timeout waiting for interrupt on GPIO %s",
					  __func__, show_gpio(gpio));
			return GPIO_IRQ_ERROR_TIMEOUT;
		case -1:
			log_error("%s: Invalid GPIO mode. "
				  "Mode must be '%s', '%s', or '%s'", __func__,
				  gpio_mode_strings[GPIO_IRQ_EDGE_RISING],
				  gpio_mode_strings[GPIO_IRQ_EDGE_FALLING],
				  gpio_mode_strings[GPIO_IRQ_EDGE_BOTH]);
			return GPIO_IRQ_ERROR;
		default:
			/* Should not occur */
			return GPIO_IRQ_ERROR;
		}
	} else {
		libsoc_gpio_int_ret_t int_ret = libsoc_gpio_wait_interrupt(_data->_internal_gpio, timeout);
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
}

void *libgpio_poll_thread(void *data)
{
	struct pollfd pfds;
	struct poll_ctx_t *ctx = data;
	int cnt, ts = -1; /* wait forever */

	pfds.fd = ctx->fd;
	pfds.events = POLLIN | POLLPRI;

	while (1) {
		cnt = poll(&pfds, 1, ts);
		if (cnt < 0) {
			/* Error */
			log_error("%s: error polling GPIO", __func__);
			continue;
		} else if (cnt == 0) {
			/* Timeout */
			continue;
		}

		if (pfds.revents) {
			struct gpiod_line_event event;

			gpiod_line_event_read_fd(ctx->fd, &event);

			ctx->callback_fn(ctx->arg);
		}
	}
}

int ldx_gpio_start_wait_interrupt(gpio_t *gpio, const ldx_gpio_interrupt_cb_t interrupt_cb,
				  void *arg)
{
	struct _gpio_t *_data = NULL;

	if (check_gpio(gpio) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	_data = gpio->_data;

	if (gpio->kernel_number == UNDEFINED_SYSFS_GPIO) {
		struct wait_irq_t *wait_irq = NULL;
		pthread_t *poll_thread = NULL;
		pthread_attr_t pthread_attr;
		struct poll_ctx_t *poll_ctx = NULL;
		int fd, rv;

		switch (_data->_mode) {
		case GPIO_INPUT:
		case GPIO_OUTPUT_LOW:
		case GPIO_OUTPUT_HIGH:
			log_error("%s: Invalid GPIO mode. Mode must be '%s', '%s', or '%s'",
				  __func__,
				  gpio_mode_strings[GPIO_IRQ_EDGE_RISING],
				  gpio_mode_strings[GPIO_IRQ_EDGE_FALLING],
				  gpio_mode_strings[GPIO_IRQ_EDGE_BOTH]);
			return EXIT_FAILURE;
			break;
		case GPIO_IRQ_EDGE_RISING:
		case GPIO_IRQ_EDGE_FALLING:
		case GPIO_IRQ_EDGE_BOTH:
			break;
		default:
			/* Should not occur */
			return EXIT_FAILURE;
		}

		if (_data->_wait_irq != NULL) {
			log_error("%s: irq already in use on GPIO %s", __func__,
				  show_gpio(gpio));
			return EXIT_FAILURE;
		}

		fd = gpiod_line_event_get_fd(_data->_line);
		if (fd == -1) {
			log_error("%s: Error getting file descriptor on GPIO %s",
				  __func__, show_gpio(gpio));
			return EXIT_FAILURE;
		}

		wait_irq = malloc (sizeof (struct wait_irq_t));
		if (wait_irq == NULL) {
			log_error("%s: Error allocating mem for wait_irq on GPIO %s",
				  __func__, show_gpio(gpio));
			return EXIT_FAILURE;
		}

		poll_thread = malloc (sizeof (pthread_t));
		if (poll_thread == NULL) {
			log_error("%s: Error allocating mem for thread on GPIO %s",
				  __func__, show_gpio(gpio));
			free(wait_irq);
			return EXIT_FAILURE;
		}

		poll_ctx = malloc(sizeof(struct poll_ctx_t));
		if (poll_ctx == NULL) {
			log_error("%s: Error allocating mem for poll_ctx on GPIO %s",
				  __func__, show_gpio(gpio));
			free(poll_thread);
			free(wait_irq);
			return EXIT_FAILURE;
		}

		pthread_attr_init(&pthread_attr);
		pthread_attr_setschedpolicy(&pthread_attr, SCHED_FIFO);

		poll_ctx->fd = fd;
		poll_ctx->callback_fn = interrupt_cb;
		poll_ctx->arg = arg;

		wait_irq->poll_ctx = poll_ctx;
		wait_irq->poll_thread = poll_thread;
		_data->_wait_irq = wait_irq;

		rv = pthread_create(poll_thread, NULL, libgpio_poll_thread,
				    poll_ctx);
		if (rv) {
			free(poll_thread);
			free(poll_ctx);
			free(wait_irq);
			_data->_wait_irq = NULL;

			return EXIT_FAILURE;
		}

		log_debug("%s: Start waiting for interrupts on GPIO %s", __func__,
			  show_gpio(gpio));

		return EXIT_SUCCESS;
	} else {
		libsoc_gpio_edge_t edge = EDGE_ERROR;
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

	return EXIT_FAILURE;
}

int ldx_gpio_stop_wait_interrupt(gpio_t *gpio)
{
	struct _gpio_t *_data = NULL;
	int ret = EXIT_FAILURE;

	if (check_gpio(gpio) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	_data = gpio->_data;

	if (gpio->kernel_number == UNDEFINED_SYSFS_GPIO) {
		if (_data->_wait_irq && _data->_wait_irq->poll_thread != NULL) {
			pthread_cancel(*_data->_wait_irq->poll_thread);
			pthread_join(*_data->_wait_irq->poll_thread, NULL);

			free(_data->_wait_irq->poll_ctx);
			free(_data->_wait_irq->poll_thread);
			free(_data->_wait_irq);
			_data->_wait_irq = NULL;

			log_debug("%s: Stop waiting for interrupts on GPIO %s",
				  __func__, show_gpio(gpio));
		} else {
			log_debug("%s: Callback thread was null", __func__);
			ret = EXIT_FAILURE;
		}
	} else {
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
	if (_data == NULL || (_data->_internal_gpio == NULL &&
		_data->_chip == NULL && _data->_line == NULL)) {
		log_error("%s: Invalid GPIO, %s", __func__,
			  show_gpio(gpio));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static char * show_gpio(gpio_t *gpio)
{
	static char _show_gpio[MAX_CONTROLLER_LEN] = "";

	if (gpio->kernel_number == UNDEFINED_SYSFS_GPIO)
		sprintf(_show_gpio, "%s %d", gpio->gpio_controller, gpio->gpio_line);
	else
		sprintf(_show_gpio, "%d", gpio->kernel_number);

	return _show_gpio;
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
static int set_direction(libsoc_gpio_t *gpio, _gpio_dir_modes_t dir)
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
		log_error("%s: Invalid GPIO mode, %d. "
			  "Mode must be '%s', '%s', '%s', '%s', '%s', or '%s'",
			  __func__, mode, gpio_mode_strings[GPIO_INPUT],
			  gpio_mode_strings[GPIO_OUTPUT_LOW],
			  gpio_mode_strings[GPIO_OUTPUT_HIGH],
			  gpio_mode_strings[GPIO_IRQ_EDGE_RISING],
			  gpio_mode_strings[GPIO_IRQ_EDGE_FALLING],
			  gpio_mode_strings[GPIO_IRQ_EDGE_BOTH]);
		return EXIT_FAILURE;
	}
}
