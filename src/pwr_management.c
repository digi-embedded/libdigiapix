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

#define _GNU_SOURCE

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include "_common.h"
#include "_log.h"
#include "include/public/pwr_management.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define	DISABLED	0
#define	ENABLED		1

// Paths
#define CORES_PATH 				"/sys/devices/system/cpu"
#define CC6_GPU_PATH			"/sys/devices/soc0/soc/130000.gpu/gpu_mult/"
#define CC8X_GPU_PATH			"/sys/devices/platform/80000000.imx8_gpu_ss/"
#define CC8MN_GPU_PATH			"/sys/devices/platform/38000000.gpu/"
#define FREQ_PATH 				"/sys/devices/system/cpu/cpufreq/policy0/"
#define MIN_MULTIPLIER_PATH		"/sys/bus/platform/drivers/galcore/"
#define TEMP_PATH				"/sys/devices/virtual/thermal/thermal_zone0/"
#define CPU_USAGE_PATH			"/proc/stat"

// File entries
#define AVALAIBLE_SCALING_FREQ		"scaling_available_frequencies"
#define AVALAIBLE_SCALING_GOVERNORS "scaling_available_governors"
#define CORES 						"cpu"
#define CRITICAL_TRIP_POINT			"trip_point_1_temp"
#define GPU_MULT					"gpu_mult"
#define MAX_SCALING_FREQ_PATH 		"scaling_max_freq"
#define MAX_FREQ_PATH		  		"cpuinfo_max_freq"
#define MIN_SCALING_FREQ_PATH 		"scaling_min_freq"
#define MIN_FREQ_PATH				"cpuinfo_min_freq"
#define MIN_MULTIPLIER_ENTRY		"gpu3DMinClock"
#define ONLINE 						"online"
#define PASSIVE_TRIP_POINT			"trip_point_0_temp"
#define SCALING_GOVERNOR			"scaling_governor"
#define SCALING_FREQ_PATH			"scaling_setspeed"
#define TEMPERATURE					"temp"

// Commands
#define COMMAND_GET_CORES 			"ls /sys/devices/system/cpu | awk '/cpu[0-9]/' | wc -l"

// Governors Strings
#define GOVERNOR_PERFORMANCE_STRING		"performance"
#define GOVERNOR_POWERSAVE_STRING		"powersave"
#define GOVERNOR_USERSPACE_STRING		"userspace"
#define GOVERNOR_ONDEMAND_STRING		"ondemand"
#define GOVERNOR_CONSERVATIVE_STRING	"conservative"
#define GOVERNOR_INTERACTIVE_STRING		"interactive"
#define	GOVERNOR_SCHEDUTIL_STRING		"schedutil"

/**
 * check_frequency() - Verify that the frequency is valid
 *
 * @freq:	The frequency to check.
 *
 * Return: EXIT_SUCCESS if the freq is valid, EXIT_FAILURE otherwise.
 */
static int check_frequency(int freq)
{
	available_frequencies_t frequencies;
	int i;

	if (freq < ldx_cpu_get_min_freq() || freq > ldx_cpu_get_max_freq()) {
		log_debug("%s: %d is not a valid frequency", __func__, freq);
		return EXIT_FAILURE;
	}

	frequencies = ldx_cpu_get_available_freq();

	for (i = 0; i < frequencies.len; i++) {
		if (freq == frequencies.data[i]) {
			ldx_cpu_free_available_freq(frequencies);
			return EXIT_SUCCESS;
		}
	}

	log_debug("%s: %d is not a valid frequency", __func__, freq);
	ldx_cpu_free_available_freq(frequencies);

	return EXIT_FAILURE;
}

/**
 * get_int_from_path() - Get an integer value from a system path
 *
 * @path:	The path to get the value.
 *
 * Return: The value read from the path, -1 on failure.
 */
static int get_int_from_path(const char* path)
{
	char *data = NULL;
	char *cmd;
	int number;

	asprintf(&cmd, READ_PATH, path);
	if (!cmd) {
		log_error("%s: Unable to allocate memory for the command", __func__);
		return -1;
	}

	data = get_cmd_output(cmd);

	if (data == NULL) {
		log_error("%s: Unable to get the data from path", __func__);
		free(cmd);
		return -1;
	}

	number = atoi(data);

	free(data);
	free(cmd);

	return number;
}

/**
 * get_frequency_from_path() - Get the value of a selected frequency path
 *
 * @path:	The path to get the frequency value.
 *
 * Return: The frequency value read from the path, -1 on failure.
 */
static int get_frequency_from_path(const char* path)
{
	char *dir_path = NULL;
	int frequency;

	dir_path = concat_path(FREQ_PATH, path);
	if (dir_path == NULL)
		return -1;

	frequency = get_int_from_path(dir_path);
	free(dir_path);

	return frequency;
}

/**
 * check_number_of_cores() - Check if a core index is valid or not
 *
 * @core:	The core index to check.
 *
 * Return: EXIT_SUCCESS if the core index is valid, EXIT_FAILURE otherwise.
 */
static int check_core_index(int core)
{
	if ((core > ldx_cpu_get_number_of_cores()) || (core < 0))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

/**
 * ldx_cpu_set_status_core() - Set the selected status of the core
 *
 * @core: 	The core index to set the status. 0 disabled/ 1 enabled
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
static int ldx_cpu_set_status_core(int core, int status)
{
	char *cmd;

	if (check_core_index(core)) {
		log_error("%s: Unable to set the core %d", __func__, core);
		return EXIT_FAILURE;
	}

	asprintf(&cmd, "%s/%s%d/%s", CORES_PATH, CORES, core, ONLINE);
	if (!cmd) {
		log_error("%s: Unable to allocate memory for the command", __func__);
		return -1;
	}

	if (write_file(cmd, "%d", status) != 0) {
		log_error("%s: Unable to set the core status", __func__);
		free(cmd);
		return EXIT_FAILURE;
	}

	free(cmd);

	return EXIT_SUCCESS;
}

const char* ldx_cpu_get_governor_string_from_type(governor_mode_t governor)
{
	switch (governor) {
	case GOVERNOR_PERFORMANCE:
		return GOVERNOR_PERFORMANCE_STRING;
	case GOVERNOR_POWERSAVE:
		return GOVERNOR_POWERSAVE_STRING;
	case GOVERNOR_USERSPACE:
		return GOVERNOR_USERSPACE_STRING;
	case GOVERNOR_ONDEMAND:
		return GOVERNOR_ONDEMAND_STRING;
	case GOVERNOR_CONSERVATIVE:
		return GOVERNOR_CONSERVATIVE_STRING;
	case GOVERNOR_INTERACTIVE:
		return GOVERNOR_INTERACTIVE_STRING;
	case GOVERNOR_SCHEDUTIL:
		return GOVERNOR_SCHEDUTIL_STRING;
	default:
		log_error("%s: Unrecognized governor %d", __func__, governor);
	}

	return NULL;
}

governor_mode_t ldx_cpu_get_governor_type_from_string(const char *governor_string){
	if (strcmp(governor_string, GOVERNOR_PERFORMANCE_STRING) == 0) {
		return GOVERNOR_PERFORMANCE;
	} else if (strcmp(governor_string, GOVERNOR_POWERSAVE_STRING) == 0) {
		return GOVERNOR_POWERSAVE;
	} else if (strcmp(governor_string, GOVERNOR_USERSPACE_STRING) == 0) {
		return GOVERNOR_USERSPACE;
	} else if (strcmp(governor_string, GOVERNOR_ONDEMAND_STRING) == 0) {
		return GOVERNOR_ONDEMAND;
	} else if (strcmp(governor_string, GOVERNOR_CONSERVATIVE_STRING) == 0) {
		return GOVERNOR_CONSERVATIVE;
	} else if (strcmp(governor_string, GOVERNOR_INTERACTIVE_STRING) == 0) {
		return GOVERNOR_INTERACTIVE;
	} else if (strcmp(governor_string, GOVERNOR_SCHEDUTIL_STRING) == 0) {
		return GOVERNOR_SCHEDUTIL;
	}

	return GOVERNOR_INVALID;
}

int ldx_cpu_get_number_of_cores()
{
	char *cores = NULL;
	int num_cores = 0;

	log_debug("%s: Getting number of cores from the CPU", __func__);

	cores = get_cmd_output(COMMAND_GET_CORES);

	if (cores == NULL) {
		log_error("%s: Unable to get the number of CPU cores", __func__);
		return -1;
	}

	num_cores = atoi(cores);
	free(cores);

	return num_cores;
}

int ldx_cpu_get_status_core(int core)
{
	char *cmd;
	int status;

	if (check_core_index(core)) {
		log_error("%s: Unable to get the status of the core %d",
				  __func__, core);
		return -1;
	}

	asprintf(&cmd, "%s/%s%d/%s", CORES_PATH, CORES, core, ONLINE);
	if (!cmd) {
		log_error("%s: Unable to allocate memory for the command", __func__);
		return -1;
	}

	status = get_int_from_path(cmd);
	free(cmd);

	return status;
}

int ldx_cpu_enable_core(int core)
{
	return ldx_cpu_set_status_core(core, ENABLED);
}

int ldx_cpu_disable_core(int core)
{
	return ldx_cpu_set_status_core(core, DISABLED);
}

available_frequencies_t ldx_cpu_get_available_freq()
{
	char *available_frequencies = NULL;
	char *cmd;
	char *ptr;
	available_frequencies_t freq;
	int i = 0;

	asprintf(&cmd, READ_PATH, FREQ_PATH AVALAIBLE_SCALING_FREQ);
	if (!cmd) {
		log_error("%s: Unable to allocate memory for the command", __func__);
		return freq;
	}

	available_frequencies = get_cmd_output(cmd);
	if (available_frequencies == NULL) {
		log_error("%s: Unable to get the available frequencies", __func__);
		free(cmd);
		return freq;
	}

	ptr = available_frequencies;

	/* Count the spaces inside the returned string*/
	while (*ptr) {
		if (*ptr == ' ') {
			i++;
		}
		ptr++;
	}

	/*There is an extra space at the end of the list "900000 1200000 "
	 * So, the list of frequencies is equal to the number of ' '*/
	freq.data = malloc((i) * sizeof(*freq.data));
	freq.len = i;
	i = 0;

	ptr = strtok(available_frequencies, " ");

	while (ptr != NULL) {
		log_debug("%s: Frequency available %s", __func__, ptr);
		freq.data[i] = atoi(ptr);
		ptr = strtok(NULL, " ");
		i++;
	}

	free(available_frequencies);
	free(cmd);

	return freq;
}

void ldx_cpu_free_available_freq(available_frequencies_t freq)
{
	log_debug("%s: Freeing available frequencies", __func__);
	free(freq.data);
}

int ldx_cpu_is_governor_available(governor_mode_t governor)
{
	char *cmd_output = NULL;
	char *cmd;
	char *ptr;

	asprintf(&cmd, READ_PATH, FREQ_PATH AVALAIBLE_SCALING_GOVERNORS);
	if (!cmd) {
		log_error("%s: Unable to allocate memory for the command", __func__);
		return EXIT_FAILURE;
	}

	cmd_output = get_cmd_output(cmd);
	if (cmd_output == NULL) {
		log_error("%s: Unable to get the available governors", __func__);
		free(cmd);
		return EXIT_FAILURE;
	}

	ptr = strtok(cmd_output, " ");

	while (ptr != NULL) {
		if (ldx_cpu_get_governor_type_from_string(ptr) == governor) {
			free(cmd_output);
			free(cmd);
			return EXIT_SUCCESS;
		}
		ptr = strtok(NULL, " ");
	}

	free(cmd_output);
	free(cmd);

	return EXIT_FAILURE;
}

int ldx_cpu_set_governor (governor_mode_t governor)
{
	const char *governor_string = NULL;
	char *cmd;

	governor_string = ldx_cpu_get_governor_string_from_type(governor);

	if (governor_string == NULL){
		return EXIT_FAILURE;
	}

	asprintf(&cmd, "%s/%s", FREQ_PATH, SCALING_GOVERNOR);
	if (!cmd) {
		log_error("%s: Unable to allocate memory for the command", __func__);
		return EXIT_FAILURE;
	}

	if (write_file(cmd, "%s", governor_string) != 0) {
		log_error("%s: Unable to set the governor status", __func__);
		free(cmd);
		return EXIT_FAILURE;
	}

	free(cmd);

	return EXIT_SUCCESS;
}

governor_mode_t ldx_cpu_get_governor()
{
	char *cmd_output = NULL;
	char *cmd;
	char *ptr;
	governor_mode_t governor;

	asprintf(&cmd, READ_PATH, FREQ_PATH SCALING_GOVERNOR);
	if (!cmd) {
		log_error("%s: Unable to allocate memory for the command", __func__);
		return GOVERNOR_INVALID;
	}

	cmd_output = get_cmd_output(cmd);
	if (cmd_output == NULL) {
		log_error("%s: Unable to get the current governor", __func__);
		free(cmd);
		return GOVERNOR_INVALID;
	}

	ptr = strtok(cmd_output, " ");
	if (ptr == NULL) {
		log_error("%s: Unable to get the current governor", __func__);
		free(cmd);
		free(cmd_output);
		return GOVERNOR_INVALID;
	}

	governor = ldx_cpu_get_governor_type_from_string(ptr);
	free(cmd_output);
	free(cmd);

	return governor;
}

int ldx_cpu_get_max_freq()
{
	return get_frequency_from_path(MAX_FREQ_PATH);
}

int ldx_cpu_get_min_freq()
{
	return get_frequency_from_path(MIN_FREQ_PATH);
}

int ldx_cpu_get_max_scaling_freq()
{
	return get_frequency_from_path(MAX_SCALING_FREQ_PATH);
}

int ldx_cpu_get_min_scaling_freq()
{
	return get_frequency_from_path(MIN_SCALING_FREQ_PATH);
}

int ldx_cpu_get_scaling_freq()
{
	return get_frequency_from_path(SCALING_FREQ_PATH);
}

int ldx_cpu_set_min_scaling_freq(int freq)
{
	if (check_frequency(freq)) {
		log_error("%s: Frequency %d is not an available frequency",
			  __func__, freq);
		return EXIT_FAILURE;
	}

	if (freq > ldx_cpu_get_max_scaling_freq()){
		log_error("%s: Frequency %d is higher than the max frequency",
			  __func__, freq);
		return EXIT_FAILURE;
	}

	if (write_file(FREQ_PATH MIN_SCALING_FREQ_PATH, "%d", freq) != 0) {
		log_error("%s: Unable to set the min frequency", __func__);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int ldx_cpu_set_max_scaling_freq(int freq)
{
	if (check_frequency(freq)) {
		log_error("%s: Frequency %d is not an available frequency",
				  __func__, freq);
		return EXIT_FAILURE;
	}

	if (freq < ldx_cpu_get_min_scaling_freq()) {
		log_error("%s: Frequency %d is lower than the min frequency",
				  __func__, freq);
		return EXIT_FAILURE;
	}

	if (write_file(FREQ_PATH MAX_SCALING_FREQ_PATH, "%d", freq) != 0) {
		log_error("%s: Unable to set the max frequency",
				  __func__);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int ldx_cpu_set_scaling_freq(int freq)
{
	if (check_frequency(freq)) {
		log_error("%s: Frequency %d is not an available frequency",
				  __func__, freq);
		return EXIT_FAILURE;
	}

	if (write_file(FREQ_PATH SCALING_FREQ_PATH, "%d", freq) != 0) {
		log_error("%s: Unable to set the frequency %s %d",
				  __func__, FREQ_PATH SCALING_FREQ_PATH ,freq);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int ldx_cpu_get_current_temp()
{
	return get_int_from_path(TEMP_PATH TEMPERATURE);
}

int ldx_cpu_get_passive_trip_point()
{
	return get_int_from_path(TEMP_PATH PASSIVE_TRIP_POINT);
}

int ldx_cpu_get_critical_trip_point()
{
	return get_int_from_path(TEMP_PATH CRITICAL_TRIP_POINT);
}

int ldx_cpu_set_passive_trip_point(int temp)
{
	if (temp <= 0) {
		log_error("%s: temp can not be zero or negative",  __func__);
		return EXIT_FAILURE;
	}

	if (temp > ldx_cpu_get_critical_trip_point()) {
		log_error("%s: passive trip point should be lower than the critical trip point",
				  __func__);
		return EXIT_FAILURE;
	}

	if (write_file(TEMP_PATH PASSIVE_TRIP_POINT, "%d", temp) != 0) {
		log_error("%s: Unable to set the selected temperature %d",
				  __func__, temp);
		return EXIT_FAILURE;
	}

	if (temp != ldx_cpu_get_passive_trip_point()) {
		log_error("%s: Invalid temperature %d",
				  __func__, temp);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int ldx_cpu_set_critical_trip_point(int temp)
{
	if (temp <= 0) {
		log_error("%s: temp can not be zero or negative",
			  __func__);
		return EXIT_FAILURE;
	}

	if (temp < ldx_cpu_get_passive_trip_point()) {
		log_error("%s: critical trip point should be higher than the passive trip point",
			  __func__);
		return EXIT_FAILURE;
	}

	if (write_file(TEMP_PATH CRITICAL_TRIP_POINT, "%d", temp) != 0) {
		log_error("%s: Unable to set the selected temperature %d",
					  __func__, temp);
		return EXIT_FAILURE;
	}

	if (temp != ldx_cpu_get_critical_trip_point()) {
		log_error("%s: Invalid temperature %d",
					  __func__, temp);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int ldx_cpu_get_usage()
{
	long double a[4], b[4];
	FILE *fp;

	fp = fopen(CPU_USAGE_PATH,"r");
	if (fp == NULL) {
		log_error("%s: Unable to get the cpu usage",  __func__);
		return -1;
	}

	if (fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &a[0], &a[1], &a[2], &a[3]) == -1) {
		log_error("%s: Error getting the cpu usage",  __func__);
		fclose(fp);
		return -1;
	}

	fclose(fp);

	sleep(1);

	fp = fopen(CPU_USAGE_PATH,"r");
	if (fp == NULL) {
		log_error("%s: Unable to get the cpu usage",  __func__);
		return -1;
	}

	if (fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &b[0], &b[1], &b[2], &b[3]) == -1) {
			log_error("%s: Error getting the cpu usage",  __func__);
			fclose(fp);
			return -1;
	}

	fclose(fp);

	return (((b[0] + b[1] + b[2]) - (a[0] + a[1] + a[2]))) * 100 /
			((b[0] + b[1] + b[2] + b[3]) - (a[0] + a[1] + a[2] + a[3]));
}

int ldx_gpu_set_multiplier(int multiplier) {

	digi_platform_t platform = get_digi_platform();
	char *path;
	char *dir_path = NULL;

	switch (platform) {
	case CC8X_PLATFORM:
		asprintf(&path, "%s", CC8X_GPU_PATH);
		break;
	case CC8MN_PLATFORM:
		asprintf(&path, "%s", CC8MN_GPU_PATH);
		break;
	case CC6_PLATFORM:
		asprintf(&path, "%s", CC6_GPU_PATH);
		break;
	case CC6UL_PLATFORM:
		log_error("%s: This platform doesn't support GPU management", __func__);
		return -1;
	default:
		log_error("%s: Unsupported platform", __func__);
		return -1;
	}

	if (!path) {
		log_error("%s: Unable to allocate memory for the command", __func__);
		return -1;
	}

	dir_path = concat_path(path, GPU_MULT);

	if (dir_path == NULL) {
		free(path);
		return EXIT_FAILURE;
	}

	if (write_file(dir_path, "%d", multiplier) != 0) {
		log_error("%s: Unable to set the selected multiplier %d",
				  __func__, multiplier);
		free(dir_path);
		free(path);
		return EXIT_FAILURE;
	}

	free(dir_path);
	free(path);

	return EXIT_SUCCESS;
}

int ldx_gpu_get_multiplier()
{
	char *dir_path = NULL;
	char *path;
	digi_platform_t platform;
	int multiplier;

	platform = get_digi_platform();

	switch (platform) {
	case CC8X_PLATFORM:
		asprintf(&path, "%s", CC8X_GPU_PATH);
		break;
	case CC8MN_PLATFORM:
		asprintf(&path, "%s", CC8MN_GPU_PATH);
		break;
	case CC6_PLATFORM:
		asprintf(&path, "%s", CC6_GPU_PATH);
		break;
	case CC6UL_PLATFORM:
		log_error("%s: This platform doesn't support GPU management", __func__);
		return -1;
	default:
		log_error("%s: Unsupported platform", __func__);
		return -1;
	}

	if (!path) {
		log_error("%s: Unable to allocate memory for the path", __func__);
		return -1;
	}

	dir_path = concat_path(path, GPU_MULT);
	if (dir_path == NULL) {
		free(path);
		return -1;
	}

	multiplier = get_int_from_path(dir_path);
	free(dir_path);
	free(path);

	return multiplier;
}

int ldx_gpu_set_min_multiplier(int multiplier)
{
	if (get_digi_platform() == CC6UL_PLATFORM) {
		log_error("%s: This platform doesn't support GPU management", __func__);
		return EXIT_FAILURE;
	}

	if (multiplier <= 0) {
		log_error("%s: multiplier can not be zero or negative",
				  __func__);
		return EXIT_FAILURE;
	}

	if (multiplier > ldx_gpu_get_multiplier()) {
		log_error("%s: gpu multiplier should be lower than %d",
				  __func__, ldx_gpu_get_multiplier());
		return EXIT_FAILURE;
	}

	if (write_file(MIN_MULTIPLIER_PATH MIN_MULTIPLIER_ENTRY
				  , "%d", multiplier) != 0) {
		log_error("%s: Unable to set the selected multiplier %d",
					  __func__, multiplier);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int ldx_gpu_get_min_multiplier()
{
	if (get_digi_platform() == CC6UL_PLATFORM) {
		log_error("%s: This platform doesn't support GPU management", __func__);
		return -1;
	}

	return get_int_from_path(MIN_MULTIPLIER_PATH MIN_MULTIPLIER_ENTRY);
}
