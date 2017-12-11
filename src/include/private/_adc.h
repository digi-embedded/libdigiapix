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

#ifndef PRIVATE__ADC_H_
#define PRIVATE__ADC_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * adc_driver_t - Defined values for different type of ADC driver
 */
typedef enum {
	ADC_DRIVER_IIO,
	ADC_DRIVER_HWMON,
	ADC_DRIVER_SIZE
} adc_driver_t;

/**
 * adc_callback_t - Data required in the ADC callback
 *
 * @callback_fn:	The function to be called every time the ADC is read.
 * @callback_arg:	Custom argument to pass to the callback function.
 * @thread:		Sampling ADC thread.
 * @ready:		Sampling thread mutex.
 * @interval:		Number of seconds between samples.
 */
typedef struct {
	ldx_adc_read_cb_t callback_fn;
	void *callback_arg;
	pthread_t *thread;
	pthread_mutex_t ready;
	unsigned int interval;
} adc_callback_t;

/**
 * adc_internal_t - Defined values for internal use
 *
 * @driver_type:	The ADC driver type: iio, hwmon, or size. See 'adc_driver_t'.
 * @input_fd:		ADC file descriptor.
 * @scale:		ADC scale.
 * @callback:		ADC callback data for asynchronous sampling.
 */
typedef struct {
	adc_driver_t driver_type;
	int input_fd;
	float scale;
	adc_callback_t *callback;
} adc_internal_t;

#ifdef __cplusplus
}
#endif

#endif /* PRIVATE__ADC_H_ */
