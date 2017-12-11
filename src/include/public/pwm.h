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

#ifndef PWM_H_
#define PWM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/**
 * pwm_enabled_t - Defined values for the status of the PWM
 */
typedef enum {
	PWM_ENABLED_ERROR = -1,
	PWM_DISABLED,
	PWM_ENABLED,
} pwm_enabled_t;

/**
 * pwm_polarity_t - Defined values for the polarity of the PWM
 */
typedef enum {
	PWM_POLARITY_ERROR = -1,
	PWM_NORMAL,
	PWM_INVERSED,
} pwm_polarity_t;

/**
 * pwm_config_error_t - Defined error values for PWM configuration
 */
typedef enum {
	PWM_CONFIG_ERROR_NONE = 0,
	PWM_CONFIG_ERROR,
	PWM_CONFIG_ERROR_INVALID,
} pwm_config_error_t;

/**
 * pwm_t - Representation of a single requested PWM
 *
 * @alias:		Alias of the PWM.
 * @channel:		PWM channel that you want to use.
 * @chip:		PWM chip which control the PWM.
 * @_data:	Data for internal usage.
 */
typedef struct {
	const char * const alias;
	const unsigned int channel;
	const unsigned int chip;
	void *_data;
} pwm_t;

/**
 * ldx_pwm_request() - Request a PWM to use
 *
 * @pwm_chip:		The Linux chip number of the PWM.
 * @channel:		The desired channel for the PWM.
 * @request_mode:	Request mode for opening the PWM (request_mode_t).
 *
 * This function returns a pwm_t pointer. Memory for the 'struct' is obtained
 * with 'malloc' and must be freed with 'ldx_pwm_free()'.
 *
 * Return: A pointer to 'pwm_t' on success, NULL on error.
 */
pwm_t *ldx_pwm_request(unsigned int pwm_chip, unsigned int channel, request_mode_t request_mode);

/**
 * ldx_pwm_request_by_alias() - Request a PWM to use using its alias name
 *
 * @pwm_alias:		The alias name of the PWM to request.
 * @request_mode:	Request mode for opening the PWM (request_mode_t).
 *
 * This function returns a pwm_t pointer. Memory for the 'struct' is obtained
 * with 'malloc' and must be freed with 'ldx_pwm_free()'.
 *
 * The alias in the configuration file has the following format:
 *	<alias_name> = <pwm_chip_number>,<pwm_channel_number>
 *
 * Return: A pointer to 'pwm_t' on success, NULL on error.
 */
pwm_t *ldx_pwm_request_by_alias(char const * const pwm_alias, request_mode_t request_mode);

/**
 * ldx_pwm_get_chip() - Get the PWM chip of a given alias
 *
 * @pwm_alias:	The alias of the PWM.
 *
 * This function returns the number of the PWM chip.
 *
 * Return: The Linux PWM chip number associated to the alias, -1 on error.
 */
int ldx_pwm_get_chip(char const * const pwm_alias);

/**
 * ldx_pwm_get_channel() - Get the PWM channel of a given alias
 *
 * @pwm_alias:	The alias of the PWM.
 *
 * Return: The PWM channel number associated to the alias, -1 on error.
 */
int ldx_pwm_get_channel(char const * const pwm_alias);

/**
 * ldx_pwm_get_number_of_channels() - Get the number of PWM channels that the
 *				      given chip supports
 *
 * @pwm_chip:	The Linux chip number of the PWM.
 *
 * Return: The number of the PWM channels that the chip supports, -1 on error.
 */
int ldx_pwm_get_number_of_channels(unsigned int pwm_chip);

/**
 * ldx_pwm_get_number_of_channels_by_alias() - Get the number of PWM channels
 *					       that the given chip supports
 *
 * @pwm_alias:	The alias of the PWM.
 *
 * This function retrieves the number of PWM channels that the chip associated
 * with the given alias supports.
 *
 * Return: The number of the PWM channels that the chip supports, -1 on error.
 */
int ldx_pwm_get_number_of_channels_by_alias(char const * const pwm_alias);

/**
 * ldx_pwm_free() - Free a previously requested PWM
 *
 * @pwm:	A pointer to the requested PWM to free.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_pwm_free(pwm_t *pwm);

/**
 * ldx_pwm_set_freq() - Change the frequency of the signal in the given PWM
 *
 * @pwm:	A requested PWM to set the signal frequency.
 * @freq_hz:	The frequency to set in Hz.
 *
 * The resulting period of the signal must be greater than the current duty
 * cycle, otherwise PWM_CONFIG_ERROR_INVALID will be returned.
 *
 * Return: PWM_CONFIG_ERROR_NONE on success, PWM_CONFIG_ERROR_INVALID for an
 *	   invalid frequency, or PWM_CONFIG_ERROR on error.
 */
pwm_config_error_t ldx_pwm_set_freq(pwm_t *pwm, unsigned long freq_hz);

/**
 * ldx_pwm_get_freq() - Get the frequency (in Hz) of a PWM signal
 *
 * @pwm:	A requested PWM to get its frequency.
 *
 * Return: The frequency in Hz of the PWM signal, -1 on error.
 */
long ldx_pwm_get_freq(pwm_t *pwm);

/**
 * ldx_pwm_set_period() - Change the period of the signal in the given PWM
 *
 * @pwm:	A requested PWM to set the signal period.
 * @period:	The period to set in nanoseconds.
 *
 * The period of the signal must be greater than the current duty cycle,
 * otherwise PWM_CONFIG_ERROR_INVALID will be returned.
 *
 * Return: PWM_CONFIG_ERROR_NONE on success, PWM_CONFIG_ERROR_INVALID for an
 *	   invalid period, or PWM_CONFIG_ERROR on error.
 */
pwm_config_error_t ldx_pwm_set_period(pwm_t *pwm, unsigned int period);

/**
 * ldx_pwm_get_period() - Get the period (in ns) of a PWM signal
 *
 * @pwm:	A requested PWM to get its period.
 *
 * Return: The period in nanoseconds of the PWM signal, -1 on error.
 */
int ldx_pwm_get_period(pwm_t *pwm);

/**
 * ldx_pwm_set_duty_cycle_percentage() - Change the duty cycle percentage of a
 *					 PWM signal
 *
 * @pwm:	A requested PWM to set the duty cycle.
 * @percentage:	The duty cycle percentage of the signal (0 to 100).
 *
 * Return: PWM_CONFIG_ERROR_NONE on success, PWM_CONFIG_ERROR_INVALID for an
 *	   invalid duty cycle, or PWM_CONFIG_ERROR on error.
 */
pwm_config_error_t ldx_pwm_set_duty_cycle_percentage(pwm_t *pwm, unsigned int percentage);

/**
 * ldx_pwm_get_duty_percentage() - Get the duty cycle percentage of a PWM signal
 *
 * @pwm:	A requested PWM to get the duty cycle.
 *
 * Return: The duty cycle percentage (0 to 100%) for the given PWM channel, -1
 *	   on error.
 */
int ldx_pwm_get_duty_cycle_percentage(pwm_t *pwm);

/**
 * ldx_pwm_set_duty_cycle() - Set the duty cycle (in ns) of a PWM signal
 *
 * @pwm:	A requested PWM to set the duty cycle.
 * @duty:	The active time in ns of the PWM signal.
 *
 * The duty cycle value must be less than the period of the current PWM signal,
 * otherwise PWM_CONFIG_ERROR_INVALID will be returned.
 *
 * Return: PWM_CONFIG_ERROR_NONE on success, PWM_CONFIG_ERROR_INVALID for an
 *	   invalid duty cycle, or PWM_CONFIG_ERROR on error.
 */
pwm_config_error_t ldx_pwm_set_duty_cycle(pwm_t *pwm, unsigned int duty);

/**
 * ldx_pwm_get_duty_cycle() - Get the duty cycle in ns of a PWM signal
 *
 * @pwm:	A requested PWM to get the duty cycle.
 *
 * Return: The duty cycle in nanoseconds for the given PWM channel, -1 on error.
 */
int ldx_pwm_get_duty_cycle(pwm_t *pwm);

/**
 * ldx_pwm_set_polarity() - Change the polarity of a PWM channel
 *
 * @pwm:	A requested PWM to set the polarity.
 * @polarity:	Polarity mode to configure (pwm_polarity_t).
 *
 * This function configures the given PWM polarity to be:
 *	- Normal polarity: PWM_NORMAL
 *	- Inverted polarity: PWM_INVERSED
 *
 * Polarity can only be changed if the PWM is not enabled, otherwise
 * EXIT_FAILURE is returned.
 *
 * If this property is not supported EXIT_FAILURE is returned.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_pwm_set_polarity(pwm_t *pwm, pwm_polarity_t polarity);

/**
 * ldx_pwm_get_polarity() - Get the polarity of a PWM channel
 *
 * @pwm:	A requested PWM to get the polarity.
 *
 * This function retrieves the polarity of the give PWM:
 *	- Normal polarity: PWM_NORMAL
 *	- Inverted polarity: PWM_INVERSED
 *	- Error retrieving the value: PWM_POLARITY_ERROR
 *
 * Return: The PWM polarity (PWM_NORMAL, PWM_INVERSED), PWM_POLARITY_ERROR if it
 *	   cannot be retrieved.
 */
pwm_polarity_t ldx_pwm_get_polarity(pwm_t *pwm);

/**
 * ldx_pwm_enable() - Enable the given PWM
 *
 * @pwm:	A requested PWM to enable.
 * @enabled:	pwm_enabled_t with the value to set.
 *
 * This function enable or disable the PWM channel:
 *	- Enable the PWM channel: PWM_ENABLED
 *	- Disable the PWM channel: PWM_DISABLED
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_pwm_enable(pwm_t *pwm, pwm_enabled_t enabled);

/**
 * ldx_pwm_is_enabled() - Check if the PWM is enabled
 *
 * @pwm:	A requested PWM to check.
 *
 * Return: The PWM status (PWM_ENABLED, PWM_DISABLED), PWM_ENABLED_ERROR if it
 *	   cannot be retrieved.
 */
pwm_enabled_t ldx_pwm_is_enabled(pwm_t *pwm);

#ifdef __cplusplus
}
#endif

#endif /* PWM_H_ */
