/*
 * Copyright 2019, Digi International Inc.
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

#ifndef PWR_MANAGEMENT_H
#define PWR_MANAGEMENT_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include "common.h"

/**
 * governor_mode_t - Defined values for governors mode.
 */
typedef enum {
	GOVERNOR_INVALID = -1,	/* Error when Governor mode cannot be determined */
	GOVERNOR_PERFORMANCE,	/* Sets CPU to the highest frequency within the
							 * borders of max scaling and min scaling frequencies.
							 */
	GOVERNOR_POWERSAVE,		/* Sets CPU to the lowest frequency within the borders
						 	 * of max scaling and min scaling frequencies
						 	 */
	GOVERNOR_USERSPACE,		/* Allows the user to set the CPU to a
						 	 * specific frequency
						 	 */
	GOVERNOR_ONDEMAND,		/* Sets the CPU depending on the current usage
				 	 	 	 */
	GOVERNOR_CONSERVATIVE,	/* Sets the CPU depending on the current usage.
	 	 	 	 	 	  	 * It gracefully increases and decreases the CPU
	 	 	 	 	 	 	 *  speed rather than jumping to max speed.
	 	 	 	 	 	 	 */
	GOVERNOR_INTERACTIVE,	/* Sets the CPU speed depending on usage,
	 	 	 	 	 	  	 * but is more aggressive when scaling
	 	 	 	 	 	 	 * the CPU speed up in response to CPU-intensive
	 	 	 	 	 	 	 * activity.
	 	 	 	 	 	 	 */
	GOVERNOR_SCHEDUTIL,		/* uses CPU utilization data available from the CPU
	 	 	 	 	 	 	 * scheduler. It generally is regarded as a part of
	 	 	 	 	 	 	 * the CPU scheduler, so it can access the scheduler's
	 	 	 	 	 	 	 * internal data structures directly
	 	 	 	 	 	 	 */
	MAX_GOVERNORS,
} governor_mode_t;

/**
 * available_frequencies_t - Struct to store the frequencies
 *
 * @data:	array with the frequencies
 * @len:	len of the array.
 */
typedef struct {
	int *data;
	size_t len;
} available_frequencies_t;

/**
 * ldx_cpu_get_number_of_cores() - Get the number of CPU cores
 *
 * This function returns the number of the CPU cores in your device.
 *
 * Return: CPU cores on success, -1 otherwise.
 */
int ldx_cpu_get_number_of_cores();

/**
 * ldx_cpu_get_status_core() - Get the status of the CPU core.
 *
 * @core:	The index of the CPU core to get the status.
 *
 * This function gets the status of a CPU core provided by its index.
 * The first core has the index 0.
 *
 * Return: The status of the core (0 disabled, 1 enabled or -1 on error).
 */
int ldx_cpu_get_status_core(int core);

/**
 * ldx_cpu_disable_core() - Disable the given core
 *
 * @core:	The index of the CPU core to disable.
 *
 * This function disables the given core. You can disable all the cores except one.
 * If you try to disable the last remaining core, you will receive an error.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_cpu_disable_core(int core);

/**
 * ldx_cpu_enable_core() - Enable the given core
 *
 * @core:	The index of the CPU core to enable.
 *
 * This function enables the given core.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_cpu_enable_core(int core);

/**
 * ldx_cpu_get_available_freq() - Get the available frequencies.
 *
 * This function returns a available_frequencies_t.
 * Memory for the struct is obtained with 'malloc' and must be freed
 * with 'ldx_cpu_free_available_freq()'.
 *
 * Return: An struct available_frequencies_t with the frequencies available.
 */
available_frequencies_t ldx_cpu_get_available_freq();

/**
 * ldx_cpu_free_available_freq() - Free a previously requested frequency struct
 *
 * @freq:	The requested available_frequencies_t to free.
 *
 */
void ldx_cpu_free_available_freq(available_frequencies_t freq);

/**
 * ldx_cpu_get_max_freq() - Get the max frequency supported by the CPU
 *
 * This function returns the maximum frequency supported by the CPU.
 *
 * Return: the maximum frequency on success, -1 otherwise.
 */
int ldx_cpu_get_max_freq();

/**
 * ldx_cpu_get_min_freq() - Get the min frequency supported by the CPU
 *
 * This function returns the minimum frequency supported by the CPU.
 *
 * Return: The minimum frequency on success, -1 otherwise.
 */
int ldx_cpu_get_min_freq();

/**
 * ldx_cpu_get_max_scaling_freq() - Get the max frequency scaling from the CPU
 *
 * This function returns the maximum scaling frequency configured in the CPU.
 *
 * Return: The maximum scaling frequency on success, -1 otherwise.
 */
int ldx_cpu_get_max_scaling_freq();

/**
 * ldx_cpu_get_min_scaling_freq() - Get the min frequency scaling from the CPU
 *
 * This function returns the minimum scaling frequency configured in the CPU.
 *
 * Return: The minimum scaling frequency on success, -1 otherwise.
 */
int ldx_cpu_get_min_scaling_freq();

/**
 * ldx_cpu_set_min_scaling_freq() - Set the min scaling frequency of the CPU
 *
 * @freq:	The frequency to set.
 *
 * This function configures the min scaling frequency of the CPU.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_cpu_set_min_scaling_freq(int freq);

/**
 * ldx_cpu_set_max_scaling_freq() - Set the max scaling frequency of the CPU
 *
 * @freq:	The frequency to set.
 *
 * This function configures the max scaling frequency of the CPU.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_cpu_set_max_scaling_freq(int freq);

/**
 * ldx_cpu_is_governor_available() - Verify if the governor is available or not
 *
 * @governor:	The governor to check availability.
 *
 * Return: EXIT_SUCCESS if the governor is supported, EXIT_FAILURE otherwise.
 */
int ldx_cpu_is_governor_available(governor_mode_t governor);

/**
 * ldx_cpu_set_governor() - Set the selected governor
 *
 * @governor:	The governor to set.
 *
 * This function configures the CPU governor from one of the available governors:
 *
 * 		-	GOVERNOR_PERFORMANCE
 *      -	GOVERNOR_POWERSAVE
 *		-	GOVERNOR_USERSPACE
 *		-	GOVERNOR_ONDEMAND
 *		-	GOVERNOR_CONSERVATIVE
 *		-	GOVERNOR_INTERACTIVE
 *		-	GOVERNOR_SCHEDUTIL
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_cpu_set_governor (governor_mode_t governor);

/**
 * ldx_cpu_get_governor() - Get the configured CPU governor
 *
 * This function returns the CPU governor.
 *
 * Return: Return a governor_mode_t with the configured governor.
 */
governor_mode_t ldx_cpu_get_governor();

/**
 * ldx_cpu_governor_type_from_string() - Get a governor mode providing an string
 *
 * @governor_string: 	A string with the governor to get the mode
 *
 * Return: The governor mode, GOVERNOR_INVALID otherwise.
 */
governor_mode_t ldx_cpu_get_governor_type_from_string(const char *governor_string);

/**
 * ldx_cpu_get_governor_string_from_type() - Return a string with the governor
 *
 * @governor: 	The governor mode to get the string
 *
 * Return: An string if a valid governor is provided, NULL otherwise.
 */
const char * ldx_cpu_get_governor_string_from_type(governor_mode_t governor);

/**
 * ldx_cpu_get_current_temp() - Get the current temperature in mºC
 *
 *
 * Return: The temperature on success. -1 on failure
 */
int ldx_cpu_get_current_temp();

/**
 * ldx_cpu_get_passive_trip_point() - Get the current passive trip point in mºC
 *
 *
 * Return: The value of the passive trip point on success. -1 on failure
 */
int ldx_cpu_get_passive_trip_point();

/**
 * ldx_cpu_get_critical_trip_point() - Get the current critical trip point in mºC
 *
 *
 * Return: The value of the critical trip point on success. -1 on failure
 */
int ldx_cpu_get_critical_trip_point();

/**
 * ldx_cpu_set_critical_trip_point() - Get the current critical trip point in mºC
 *
 * Critical temperature is the temperature limit at which system will
 * reduce CPU and GPU frequency to avoid system overheating.
 *
 * @temp: temperature in mºC to set
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_cpu_set_critical_trip_point(int temp);

/**
 * ldx_cpu_set_passive_trip_point() - Get the current passive trip point in mºC
 *
 * Passive CPU temperature is the temperature limit at which system will
 * reduce CPU and GPU frequency to avoid system overheating.
 *
 * @temp: temperature in mºC to set
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_cpu_set_passive_trip_point(int temp);

/**
 * ldx_cpu_get_cpu_usage() - Get the current CPU usage in %
 *
 *
 * Return: The usage of the CPU in %, -1 on error.
 */
int ldx_cpu_get_usage();

/**
 * ldx_cpu_get_gpu_min_multiplier() - Get the min multiplier
 *
 * This multiplier is applied when the passive trip point is reached in order
 * to cool the device.
 *
 * Return: The multiplier on success, -1 otherwise.
 */
int ldx_gpu_get_min_multiplier();

/**
 * ldx_gpu_set_min_multiplier() - Set the min multiplier
 *
 * This multiplier is applied when the passive trip point is reached in order
 * to cool the device.
 *
 * @multiplier: The multiplier to apply
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_gpu_set_min_multiplier(int multiplier);

/**
 * ldx_cpu_set_scaling_freq() - Set the scaling frequency
 *
 * @freq: The frequency to set.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_cpu_set_scaling_freq(int freq);

/**
 * ldx_cpu_get_scaling_freq() - Get the scaling frequency
 *
 * This function returns the scaling frequency configured in the CPU.
 *
 * Return: The scaling frequency on success, -1 otherwise.
 */
int ldx_cpu_get_scaling_freq();

/**
 * ldx_cpu_get_gpu_multiplier() - Set the GPU multiplier
 *
 * This multiplier is used in order to determine the max frequency of the GPU,
 * being the maximum 64, and the minimum 1.
 *
 * This may cause negative effects in the frame rate of your application.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_gpu_set_multiplier(int multiplier);

/**
 * ldx_cpu_get_gpu_multiplier() - Get the GPU multiplier
 *
 * This multiplier is used in order to determine the max frequency of the GPU
 * being the maximum 64, and the minimum 1.
 *
 * Return: The multiplier on success, -1 otherwise.
 */
int ldx_gpu_get_multiplier();

#ifdef __cplusplus
}
#endif

#endif /* PWR_MANAGEMENT_H */
