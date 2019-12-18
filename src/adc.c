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

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "_common.h"
#include "adc.h"
#include "_adc.h"
#include "_libsoc_interfaces.h"
#include "_log.h"

#define BUFF_SIZE		256

static float get_scale(adc_driver_t driver_type, unsigned int adc_chip);

adc_t *ldx_adc_request(unsigned int adc_chip, unsigned int adc_channel)
{
	adc_t *new_adc = NULL;
	adc_t init_adc = {NULL, adc_chip, adc_channel, NULL};
	adc_internal_t *internal_data = NULL;
	char path[BUFF_SIZE];
	float scale;

	log_debug("%s: Requesting ADC chip: %d channel: %d",
			__func__, adc_chip, adc_channel);

	new_adc = calloc(1, sizeof(adc_t));
	internal_data = calloc(1, sizeof(adc_internal_t));

	if (new_adc == NULL || internal_data == NULL) {
		log_error("%s: Unable to request ADC chip: %d channel: %d, "
				"cannot allocate memory", __func__, adc_chip,
				adc_channel);
		free(new_adc);
		free(internal_data);
		return NULL;
	}

	for (int i = 0; i <= ADC_DRIVER_SIZE; i++) {
		switch (i) {
		case ADC_DRIVER_IIO:
			sprintf(path, "/sys/bus/iio/devices/iio:device%d"
					"/in_voltage%d_raw", adc_chip,
					adc_channel);
			internal_data->driver_type = ADC_DRIVER_IIO;
			break;

		case ADC_DRIVER_HWMON:
			sprintf(path, "/sys/class/hwmon/hwmon%d/device/"
					"in%d_input", adc_chip, adc_channel);
			internal_data->driver_type = ADC_DRIVER_HWMON;
			break;

		default:
			log_error("%s: Unable to find the requested ADC "
					"chip: %d channel: %d", __func__,
					adc_chip, adc_channel);
			free(new_adc);
			free(internal_data);
			return NULL;
		}
		internal_data->input_fd = open(path, O_SYNC | O_RDONLY);
		if (internal_data->input_fd > 0)
			break;
	}

	if (internal_data->input_fd <= 0) {
		log_error("%s: Unable to find the requested ADC chip: %d "
				"channel: %d", __func__, adc_chip, adc_channel);
		free(new_adc);
		free(internal_data);
		return NULL;
	}

	scale = get_scale(internal_data->driver_type, adc_chip);
	if (scale == -1) {
		printf("%s: Unable to find the scale for the ADC chip: %d\n",
				__func__, adc_chip);
	}

	internal_data->scale = scale;
	internal_data->callback = NULL;

	memcpy(new_adc, &init_adc, sizeof(adc_t));
	((adc_t *)new_adc)->_data = internal_data;

	return new_adc;
}

adc_t *ldx_adc_request_by_alias(char const * const adc_alias)
{
	int adc_channel, adc_chip;
	adc_t *new_adc = NULL;

	log_debug("%s: Requesting ADC '%s'", __func__, adc_alias);

	adc_channel = ldx_adc_get_channel(adc_alias);
	adc_chip = ldx_adc_get_chip(adc_alias);
	if (adc_channel < 0 || adc_chip < 0) {
		log_error("%s: Invalid ADC alias, '%s'", __func__, adc_alias);
		return NULL;
	}

	new_adc = ldx_adc_request(adc_chip, adc_channel);

	if (new_adc != NULL) {
		adc_t init_adc = {adc_alias, adc_chip, adc_channel,
				((adc_t *)new_adc)->_data};
		memcpy(new_adc, &init_adc, sizeof(adc_t));
	}

	return new_adc;
}

int ldx_set_scale(adc_t *adc, float scale)
{
	adc_internal_t *_adc = NULL;

	if (scale <= 0) {
		log_error("%s: Invalid scale for the adc", __func__);
		return EXIT_FAILURE;
	}
	_adc = (adc_internal_t *) adc->_data;
	_adc->scale = scale;
	return EXIT_SUCCESS;
}

int ldx_adc_free(adc_t *adc)
{
	int ret = EXIT_SUCCESS;
	adc_internal_t *_adc = NULL;

	if (adc == NULL)
		return EXIT_SUCCESS;

	_adc = (adc_internal_t *) adc->_data;

	log_debug("%s: Freeing ADC chip: %d channel: %d", __func__,
			adc->chip, adc->channel);

	if (close(_adc->input_fd) < 0) {
		log_error("%s: Error closing input file", __func__);
		ret = EXIT_FAILURE;
	}

	free(adc);
	free(_adc);

	return ret;
}

int ldx_adc_get_chip(char const * const adc_alias)
{
	if (config_check_alias(adc_alias) != EXIT_SUCCESS)
		return -1;

	return config_get_adc_chip_number(adc_alias);
}

int ldx_adc_get_channel(char const * const adc_alias)
{
	if (config_check_alias(adc_alias) != EXIT_SUCCESS)
		return -1;

	return config_get_adc_channel_number(adc_alias);
}

int ldx_adc_get_sample(adc_t *adc)
{
	char value[BUFF_SIZE];
	int int_value = 0, nbytes;
	adc_internal_t *_adc = NULL;

	if (adc == NULL) {
		log_error("%s: ADC cannot be NULL", __func__);
		return -1;
	}

	_adc = (adc_internal_t *) adc->_data;

	log_info("%s: Reading ADC value.", __func__);

	/* We need to restart the pointer in each read */
	lseek(_adc->input_fd, 0, SEEK_SET);

	nbytes = read(_adc->input_fd, value, sizeof(value) - 1);
	if (nbytes < 0) {
		log_error("%s: Error reading input", __func__);
		return -1;
	}
	/* Remove newline character read from the sysfs */
	value[nbytes - 1] = 0;

	errno = 0;
	int_value = strtol(value, NULL, 10);
	if ((errno == ERANGE && (int_value == LONG_MAX || int_value == LONG_MIN))
	    || (errno != 0 && int_value == 0)) {
		log_error("%s: ADC value can't be lower than 0", __func__);
		return -1;
	}

	log_debug("%s: Value read in ADC chip: %d ", __func__, int_value);

	return int_value;
}

float ldx_adc_convert_sample_to_mv(adc_t *adc, int sample)
{
	adc_internal_t *_adc = NULL;
	_adc = (adc_internal_t *) adc->_data;
	if (sample * _adc->scale < 0) {
		log_error("%s: Scale should be a number greater than 0", __func__);
		return -1;
	}

	return sample * _adc->scale;

}
void *ldx_sampling_callback_thread(void *callback_adc)
{
	adc_t *adc = callback_adc;
	adc_internal_t *_adc = NULL;

	_adc = (adc_internal_t *) adc->_data;

	pthread_mutex_unlock(&_adc->callback->ready);

	while (1) {
		int sample = ldx_adc_get_sample(adc);
		_adc->callback->callback_fn(sample,
				_adc->callback->callback_arg);
		sleep(_adc->callback->interval);
	}
}

int ldx_adc_stop_sampling(adc_t *adc)
{
	adc_internal_t *_adc = NULL;
	int ret = EXIT_SUCCESS;

	if (adc == NULL) {
		log_error("%s: ADC cannot be NULL", __func__);
		return EXIT_SUCCESS;
	}

	_adc = (adc_internal_t *) adc->_data;
	if (_adc->callback == NULL)
		return EXIT_SUCCESS;

	if (_adc->callback->thread != NULL) {
		pthread_cancel(*_adc->callback->thread);
		pthread_join(*_adc->callback->thread, NULL);
	} else {
		log_debug("%s: Callback thread was null", __func__);
		ret = EXIT_FAILURE;
	}

	pthread_mutex_unlock(&_adc->callback->ready);
	pthread_mutex_destroy(&_adc->callback->ready);

	free(_adc->callback->thread);
	free(_adc->callback);

	_adc->callback = NULL;

	log_debug("%s: Callback thread was stopped", __func__);

	return ret;
}

int ldx_adc_start_sampling(adc_t *adc, const ldx_adc_read_cb_t read_cb,
			unsigned int interval, void *arg)
{
	pthread_t *poll_thread = NULL;
	pthread_attr_t pthread_attr;
	adc_internal_t *_adc = NULL;
	adc_callback_t *new_adc_callback;

	if (adc == NULL) {
		log_error("%s: ADC cannot be NULL", __func__);
		return EXIT_SUCCESS;
	}

	poll_thread = malloc (sizeof (pthread_t));
	if (poll_thread == NULL) {
		log_error("%s: Unable to start sampling ADC chip: %d channel: %d,"
				" cannot allocate memory", __func__, adc->chip,
				adc->channel);
		return EXIT_FAILURE;
	}

	log_debug("%s: Start waiting for samples on ADC chip: %d channel: %d"
			, __func__, adc->chip, adc->channel);

	_adc = (adc_internal_t *) adc->_data;

	pthread_attr_init(&pthread_attr);
	pthread_attr_setschedpolicy(&pthread_attr, SCHED_FIFO);

	log_debug("%s: Creating new callback", __func__);

	new_adc_callback = malloc(sizeof(adc_callback_t));
	if (new_adc_callback == NULL) {
		log_error("%s: Unable to start sampling ADC chip: %d channel: %d,"
				" cannot allocate memory", __func__, adc->chip,
				adc->channel);
		free(poll_thread);
		return EXIT_FAILURE;
	}

	new_adc_callback->callback_fn = read_cb;
	new_adc_callback->callback_arg = arg;
	new_adc_callback->thread = poll_thread;

	_adc->callback = new_adc_callback;
	_adc->callback->interval = interval;

	pthread_mutex_init(&new_adc_callback->ready, NULL);

	int ret = pthread_create(poll_thread, NULL,
			ldx_sampling_callback_thread, adc);

	if (ret == 0) {
		/* Wait for thread to be initialized and ready */
		pthread_mutex_lock(&new_adc_callback->ready);
	} else {
		pthread_mutex_unlock(&_adc->callback->ready);
		pthread_mutex_destroy(&_adc->callback->ready);
		free(_adc->callback->thread);
		free(_adc->callback);

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static float get_scale(adc_driver_t driver_type, unsigned int adc_chip)
{
	char scale_path[BUFF_SIZE];
	char value[16];		/* Enough for values reported in the sysfs */
	float scale_factor = -1;
	int fd, nbytes;

	if (driver_type == ADC_DRIVER_IIO) {
		sprintf(scale_path, "/sys/bus/iio/devices/iio:device%d/in_voltage_scale"
				, adc_chip);
		fd = open(scale_path, O_SYNC | O_RDONLY);
		if (fd < 0) {
			log_error("%s: Unable to find the scale for the ADC chip: %d\n"
					, __func__, adc_chip);
			return -1;
		}

		nbytes = read(fd, value, sizeof(value) - 1);
		if (nbytes < 0) {
			log_error("%s: Error reading scale factor", __func__);
			close(fd);
			return -1;
		}
		/* Remove newline character read from the sysfs */
		value[nbytes - 1] = 0;

		scale_factor = atof(value);
		if (close(fd) < 0) {
			log_error("%s: Error closing input file", __func__);
			return -1;
		}
	} else if (driver_type == ADC_DRIVER_HWMON) {
		/* In this driver we read directly in mV so the scale factor is 1 */
		scale_factor = 1;
	}

	return scale_factor;
}

