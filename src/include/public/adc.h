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

#ifndef ADC_H_
#define ADC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/**
 * Callback function type used as ADC interrupt handler
 *
 * See 'ldx_adc_start_sampling()'.
 */
typedef int (*ldx_adc_read_cb_t)(int sample, void *arg);

/**
 * adc_t - Representation of a single requested ADC
 *
 * @alias:	Alias of the ADC.
 * @chip:	ADC chip which control the adc driver.
 * @channel:	ADC channel that you want to use.
 * @_data:	Data for internal usage.
 */
typedef struct {
	const char * const alias;
	const unsigned int chip;
	const unsigned int channel;
	void *_data;
} adc_t;

/**
 * ldx_adc_request() - Request an ADC to use
 *
 * @adc_chip:		The desired ADC chip.
 * @adc_channel:	The desired ADC channel.
 *
 * This function returns an adc_t pointer. Memory for the 'struct' is obtained
 * with 'malloc' and must be freed with 'ldx_adc_free()'.
 *
 * Return: A pointer to 'adc_t' on success, NULL on error.
 */
adc_t *ldx_adc_request(unsigned int adc_chip, unsigned int adc_channel);

/**
 * ldx_adc_request_by_alias() - Request an ADC to use using its alias name
 *
 * @adc_alias:		The alias name of the ADC to request.
 *
 * This function returns an adc_t pointer. Memory for the 'struct' is obtained
 * with 'malloc' and must be freed with 'ldx_adc_free()'.
 *
 * The alias in the configuration file has the following format:
 *	<alias_name> = <adc_chip_number>,<adc_channel_number>
 *
 * Return: A pointer to 'adc_t' on success, NULL on error.
 */
adc_t *ldx_adc_request_by_alias(char const * const adc_alias);

/**
 * ldx_adc_get_chip() - Get the ADC chip of a given alias
 *
 * @adc_alias:	The alias of the ADC.
 *
 * Return: The Linux ADC chip number associated to the alias, -1 on error.
 */
int ldx_adc_get_chip(char const * const adc_alias);

/**
 * ldx_adc_get_channel() - Get the ADC channel of a given alias
 *
 * @adc_alias:	The alias of the ADC.
 *
 * Return: The ADC channel number associated to the alias, -1 on error.
 */
int ldx_adc_get_channel(char const * const adc_alias);

/**
 * ldx_adc_free() - Free a previously requested ADC
 *
 * @adc:	A pointer to the requested ADC to free.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_adc_free(adc_t *adc);

/**
 * ldx_adc_get_sample() - Read the value of an ADC channel
 *
 * @adc:	A requested ADC to get its value.
 *
 * Return: The value of the ADC channel, -1 on error.
 */
int ldx_adc_get_sample(adc_t *adc);

/**
 * ldx_adc_convert_sample_to_mv() - Convert the sample to mV
 *
 * @adc:	A requested ADC to get its value in mV.
 * @sample:	The sample to convert in mV.
 *
 * Return: The value of the ADC channel in mV, -1 on error.
 */
float ldx_adc_convert_sample_to_mv(adc_t *adc, int sample);

/**
 * ldx_adc_start_sampling() - Start sampling in the requested ADC
 *
 * @adc:		A pointer to a requested ADC to start sampling.
 * @ldx_adc_read_cb_t:	Callback function to be called each time interval.
 * @interval:		Sampling interval in seconds.
 * @arg:		Void casted pointer to pass to the interrupt handler as
 *			parameter.
 *
 * This function waits for interval time to get an ADC sample asynchronously.
 * It is a non-blocking function that registers the provided callback to be
 * executed each time interval. After that it continues waiting until the next
 * time interval.
 *
 * To stop the sampling use 'ldx_adc_stop_sampling()'.
 *
 * This function creates a new thread waiting for ADC reads. This thread
 * executes the callback function each time that the time interval has passed.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_adc_start_sampling(adc_t *adc, const ldx_adc_read_cb_t read_cb,
			unsigned int interval, void *arg);

/**
 * ldx_adc_stop_sampling() - Stop the sampling on the given ADC
 *
 * @adc:	A pointer to a requested ADC to stop the sampling.
 *
 * This function stops the previously set sampling handler on a ADC using
 * 'ldx_adc_start_sampling()'.
 *
 * This uses the 'pthread_cancel()' function, so it may cancel mid way through
 * your interrupt handler function.
 *
 * If no sampling handler is registered for the ADC, this function does
 * nothing and returns EXIT_SUCCESS.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_adc_stop_sampling(adc_t *adc);

/**
 * ldx_adc_set_scale() - Set the scaling factor for the ADC sampling
 *
 * @adc:	The alias of the ADC.
 * @scale:	The scale factor to multiply the raw samples.
 *
 * Return: EXIT_SUCCESS  on success, EXIT_FAILURE otherwise.
 */
int ldx_adc_set_scale(adc_t *adc, float scale);

#ifdef __cplusplus
}
#endif

#endif /* ADC_H_ */
