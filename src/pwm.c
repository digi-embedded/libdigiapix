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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "_common.h"
#include "_libsoc_interfaces.h"
#include "_log.h"
#include "pwm.h"

#define BUFF_SIZE		256
#define SECS_TO_NANOSECS	1000000000.0

#define P(x)	#x
static const char * const pwm_polarity_strings[] = {
	P(PWM_NORMAL),
	P(PWM_INVERSED),
};

static const char * const pwm_enable_strings[] = {
	P(PWM_ENABLED),
	P(PWM_DISABLED),
};
#undef P

static int check_valid_pwm(pwm_t *pwm);

pwm_t *ldx_pwm_request(unsigned int pwm_chip, unsigned int pwm_channel,
		       request_mode_t request_mode)
{
	libsoc_pwm_t *_pwm = NULL;
	pwm_t *new_pwm = NULL;
	pwm_t init_pwm = {NULL, pwm_chip, pwm_channel, NULL};

	if (check_request_mode(request_mode) != EXIT_SUCCESS) {
		request_mode = REQUEST_SHARED;
		log_info("%s: Invalid request mode, setting to 'REQUEST_SHARED'",
			 __func__);
	}

	log_debug("%s: Requesting PWM chip %d, channel %d [request mode: %d]",
		  __func__, pwm_chip, pwm_channel, request_mode);

	_pwm = libsoc_pwm_request(pwm_chip, pwm_channel, request_mode);

	if (_pwm == NULL)
		return NULL;

	new_pwm = calloc(1, sizeof(pwm_t));
	if (new_pwm == NULL) {
		log_error("%s: Unable to request PWM %d:%d [request mode: %d], cannot allocate memory",
			  __func__, pwm_chip, pwm_channel, request_mode);
		libsoc_pwm_free(_pwm);
		return NULL;
	}

	memcpy(new_pwm, &init_pwm, sizeof(pwm_t));
	((pwm_t *)new_pwm)->_data = _pwm;

	return new_pwm;
}

pwm_t *ldx_pwm_request_by_alias(char const * const pwm_alias, request_mode_t request_mode)
{
	int pwm_chip_number;
	int pwm_channel_number;
	void *new_pwm = NULL;

	log_debug("%s: Requesting PWM '%s' [request mode: %d]",
		  __func__, pwm_alias, request_mode);

	pwm_chip_number = ldx_pwm_get_chip(pwm_alias);
	if (pwm_chip_number == -1) {
		log_error("%s: Invalid PWM alias, '%s'", __func__, pwm_alias);
		return NULL;
	}

	pwm_channel_number = ldx_pwm_get_channel(pwm_alias);
	if (pwm_channel_number == -1) {
		log_error("%s: Invalid PWM alias, '%s'", __func__, pwm_alias);
		return NULL;
	}

	new_pwm = ldx_pwm_request(pwm_chip_number, pwm_channel_number, request_mode);
	if (new_pwm != NULL) {
		pwm_t init_pwm = {pwm_alias, pwm_chip_number, pwm_channel_number,
				((pwm_t *)new_pwm)->_data};
		memcpy(new_pwm, &init_pwm, sizeof(pwm_t));
	}

	return new_pwm;
}

int ldx_pwm_get_chip(char const * const pwm_alias)
{
	if (config_check_alias(pwm_alias) != EXIT_SUCCESS)
		return -1;

	return config_get_pwm_chip_number(pwm_alias);
}

int ldx_pwm_get_channel(char const * const pwm_alias)
{
	if (config_check_alias(pwm_alias) != EXIT_SUCCESS)
		return -1;

	return config_get_pwm_channel_number(pwm_alias);
}

int ldx_pwm_get_number_of_channels(unsigned int pwm_chip)
{
	int fd, nbytes;
	char path[BUFF_SIZE];
	char channels[5];
	long nchan = -1;

	log_debug("%s: Getting number of channels of PWM %d", __func__, pwm_chip);

	sprintf(path, "/sys/class/pwm/pwmchip%d/npwm", pwm_chip);
	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;

	nbytes = read(fd, channels, sizeof(channels) - 1);
	if (nbytes < 0)
		goto err_out;
	/* Remove newline character read from the sysfs */
	channels[nbytes - 1] = 0;

	errno = 0;
	nchan = strtol(channels, NULL, 10);
	if ((errno == ERANGE && (nchan == LONG_MAX || nchan == LONG_MIN))
	    || (errno != 0 && nchan == 0))
		nchan = -1;

err_out:
	close(fd);

	return (int)nchan;
}

int ldx_pwm_get_number_of_channels_by_alias(char const * const pwm_alias)
{
	int chip = config_get_pwm_chip_number(pwm_alias);

	if (chip < 0)
		return -1;

	return ldx_pwm_get_number_of_channels(chip);
}

int ldx_pwm_free(pwm_t *pwm)
{
	int ret = EXIT_SUCCESS;

	if (pwm == NULL)
		return EXIT_SUCCESS;

	log_debug("%s: Freeing PWM %d:%d", __func__, pwm->chip, pwm->channel);

	if (pwm->_data != NULL)
		ret = libsoc_pwm_free(pwm->_data);

	free(pwm);

	return ret;
}

pwm_config_error_t ldx_pwm_set_period(pwm_t *pwm, unsigned int period)
{
	pwm_config_error_t ret = PWM_CONFIG_ERROR;
	int duty_cycle = -1;

	if (check_valid_pwm(pwm) != EXIT_SUCCESS)
		return PWM_CONFIG_ERROR_INVALID;

	if (period > INT_MAX) {
		log_error("%s: Invalid period for PWM %d:%d, it must be between 1 and %d",
			  __func__, pwm->chip, pwm->channel, INT_MAX);
		return PWM_CONFIG_ERROR_INVALID;
	}

	duty_cycle = ldx_pwm_get_duty_cycle(pwm);
	if (duty_cycle > -1 && (int)period < duty_cycle) {
		log_error("%s: The duty cycle (%d ns) is greater than period (%d ns) "
			  "that you are setting. Change the duty cycle "
			  "before setting the period.",
			  __func__, duty_cycle, period);
		return PWM_CONFIG_ERROR_INVALID;
	}

	log_debug("%s: Setting period for PWM %d:%d: %d ns", __func__,
		pwm->chip, pwm->channel, period);

	ret = libsoc_pwm_set_period(pwm->_data, period);

	if (ret != EXIT_SUCCESS) {
		log_error("%s: Unable to set PWM %d:%d period to %d ns",
			  __func__, pwm->chip, pwm->channel, period);
		ret = PWM_CONFIG_ERROR;
	} else {
		ret = PWM_CONFIG_ERROR_NONE;
	}

	return ret;
}

int ldx_pwm_get_period(pwm_t *pwm)
{
	int period;

	if (check_valid_pwm(pwm) != EXIT_SUCCESS)
		return -1;

	log_debug("%s: Getting period of PWM %d:%d", __func__, pwm->chip,
		  pwm->channel);

	period = libsoc_pwm_get_period(pwm->_data);
	if (period == -1)
		log_error("%s: Unable to get the PWM %d:%d period",
			  __func__, pwm->chip, pwm->channel);

	return period;
}

pwm_config_error_t ldx_pwm_set_freq(pwm_t *pwm, unsigned long freq_hz)
{
	unsigned int period;

	if (freq_hz == 0 || freq_hz > SECS_TO_NANOSECS)
		return PWM_CONFIG_ERROR_INVALID;

	/* The API only allows to set the period in nanosecs*/
	period = (SECS_TO_NANOSECS / freq_hz) + 0.5;

	log_debug("%s: Setting frequency of PWM %d:%d: %lu Hz", __func__,
		  pwm->chip, pwm->channel, freq_hz);

	return ldx_pwm_set_period(pwm, period);
}

long ldx_pwm_get_freq(pwm_t *pwm)
{
	int period;

	log_debug("%s: Getting frequency of PWM %d:%d", __func__, pwm->chip, pwm->channel);

	period = ldx_pwm_get_period(pwm);

	return (period > 0) ? (SECS_TO_NANOSECS / period) + 0.5 : -1;
}

pwm_config_error_t ldx_pwm_set_duty_cycle(pwm_t *pwm, unsigned int duty_cycle)
{
	int current_period;

	log_debug("%s: Setting duty cycle of PWM %d:%d: %d ns", __func__,
		  pwm->chip, pwm->channel, duty_cycle);

	current_period = ldx_pwm_get_period(pwm);
	if (current_period > -1 && duty_cycle > (unsigned int)current_period) {
		log_error("%s: Invalid duty cycle value, %d ns. Duty cycle must"
			  " be less than the current period (%d ns)",
			  __func__, duty_cycle, current_period);
		return PWM_CONFIG_ERROR_INVALID;
	}

	return (libsoc_pwm_set_duty_cycle(pwm->_data, duty_cycle) == EXIT_SUCCESS) ?
			PWM_CONFIG_ERROR_NONE : PWM_CONFIG_ERROR;
}

int ldx_pwm_get_duty_cycle(pwm_t *pwm)
{
	if (check_valid_pwm(pwm) != EXIT_SUCCESS)
		return -1;

	log_debug("%s: Getting duty cycle of PWM %d:%d", __func__, pwm->chip, pwm->channel);

	return libsoc_pwm_get_duty_cycle(pwm->_data);
}

pwm_config_error_t ldx_pwm_set_duty_cycle_percentage(pwm_t *pwm, unsigned int percentage)
{
	int current_period;

	if (percentage > 100) {
		log_error("%s: Invalid duty cycle percentage %d%%. It must be between 0 and 100",
			  __func__, percentage);
		return PWM_CONFIG_ERROR_INVALID;
	}

	log_debug("%s: Setting duty cycle percentage of PWM %d:%d: %d%%",
		  __func__, pwm->chip, pwm->channel, percentage);

	current_period = ldx_pwm_get_period(pwm);
	if (current_period == -1)
		return PWM_CONFIG_ERROR;

	return ldx_pwm_set_duty_cycle(pwm, (current_period / 100.0 * percentage) + 0.5);
}

int ldx_pwm_get_duty_percentage(pwm_t *pwm)
{
	int duty_cycle;
	int period;

	log_debug("%s: Getting duty cycle percentage of PWM %d:%d", __func__,
		  pwm->chip, pwm->channel);

	duty_cycle = ldx_pwm_get_duty_cycle(pwm);
	period = ldx_pwm_get_period(pwm);
	if (duty_cycle > 0 && period > 0)
		return (duty_cycle * 1.0 / period * 100) + 0.5;

	return -1;
}

int ldx_pwm_set_polarity(pwm_t *pwm, pwm_polarity_t polarity)
{
	if (check_valid_pwm(pwm) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	switch (polarity) {
	case PWM_NORMAL:
		break;
	case PWM_INVERSED:
		break;
	default:
		log_error("%s: Invalid PWM polarity, %d. Polarity must be '%s', or '%s'",
			  __func__, polarity, pwm_polarity_strings[PWM_NORMAL],
			  pwm_polarity_strings[PWM_INVERSED]);
		return EXIT_FAILURE;
	}

	log_debug("%s: Setting polarity of PWM %d:%d: '%s' (%d)", __func__,
		  pwm->chip, pwm->channel,
		  pwm_polarity_strings[polarity], polarity);

	return libsoc_pwm_set_polarity(pwm->_data, polarity);
}

pwm_polarity_t ldx_pwm_get_polarity(pwm_t *pwm)
{
	libsoc_pwm_polarity_t polarity = POLARITY_ERROR;

	if (check_valid_pwm(pwm) != EXIT_SUCCESS)
		return PWM_POLARITY_ERROR;

	log_debug("%s: Getting polarity of PWM %d:%d", __func__, pwm->chip, pwm->channel);

	polarity = libsoc_pwm_get_polarity(pwm->_data);

	switch (polarity) {
	case POLARITY_ERROR:
		log_error("%s: Unable to get PWM %d:%d polarity",
			  __func__, pwm->chip, pwm->channel);
		return PWM_POLARITY_ERROR;
	case NORMAL:
		return PWM_NORMAL;
	case INVERSED:
		return PWM_INVERSED;
	default:
		/* Should not occur */
		return PWM_POLARITY_ERROR;
	}
}

int ldx_pwm_enable(pwm_t *pwm, pwm_enabled_t enabled)
{
	if (check_valid_pwm(pwm) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	switch (enabled) {
	case PWM_ENABLED:
		break;
	case PWM_DISABLED:
		break;
	default:
		log_error("%s: Invalid PWM enabled value, %d. Must be '%s' or '%s'",
			  __func__, enabled, pwm_enable_strings[PWM_ENABLED],
			  pwm_enable_strings[PWM_DISABLED]);
		return EXIT_FAILURE;
	}

	log_debug("%s: %s PWM %d:%d", __func__, enabled == PWM_ENABLED ?
		  "Enabling" : "Disabling", pwm->chip, pwm->channel);

	return libsoc_pwm_set_enabled(pwm->_data, enabled);
}

pwm_enabled_t ldx_pwm_is_enabled(pwm_t *pwm)
{
	libsoc_pwm_enabled_t enabled = ENABLED_ERROR;

	if (check_valid_pwm(pwm) != EXIT_SUCCESS)
		return PWM_ENABLED_ERROR;

	log_debug("%s: Checking if PWM %d:%d is enabled", __func__, pwm->chip, pwm->channel);

	enabled = libsoc_pwm_get_enabled(pwm->_data);

	switch (enabled) {
	case ENABLED_ERROR:
		log_error("%s: Unable to get PWM %d:%d polarity",
			  __func__, pwm->chip, pwm->channel);
		return PWM_ENABLED_ERROR;
	case ENABLED:
		return PWM_ENABLED;
	case DISABLED:
		return PWM_DISABLED;
	default:
		/* Should not occur */
		return PWM_ENABLED_ERROR;
	}
}

static int check_valid_pwm(pwm_t *pwm)
{
	if (pwm == NULL) {
		log_error("%s: PWM cannot be NULL", __func__);
		return EXIT_FAILURE;
	}

	if (pwm->_data == NULL) {
		log_error("%s: Invalid PWM, %d:%d", __func__, pwm->chip, pwm->channel);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
