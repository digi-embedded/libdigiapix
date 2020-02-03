/*
 * Copyright 2017-2019, Digi International Inc.
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "_log.h"
#include "_common.h"
#include "common.h"

#define DEFAULT_DIGIAPIX_CFG_FILE	"/etc/libdigiapix.conf"
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define PLATFORM_PATH				"/proc/device-tree/compatible"

#define CC8X_PLATFORM_STRING		"imx8x"
#define CC8MN_PLATFORM_STRING		"imx8mn"
#define CC6UL_PLATFORM_STRING		"imx6ul"
#define CC6_PLATFORM_STRING			"imx6q"
#define CC6DL_PLATFORM_STRING 		"imx6dl"

static void __attribute__ ((constructor(101))) digiapix_init(void);
static void __attribute__ ((destructor(101))) digiapix_fini(void);

static int config_load(void);
static void config_free(void);
static int config_get_csv_integer(const char * const group, const char * const alias, int index);

static board_config *config;

int config_check_alias(const char * const alias)
{
	if (alias == NULL || strlen(alias) == 0) {
		log_error("%s: Invalid alias, it cannot be %s", __func__,
				alias == NULL ? "NULL" : "empty");
		return EXIT_FAILURE;
	}

	if (config == NULL) {
		log_error("%s: Unable get requested alias ('%s')",
				__func__, alias);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int config_get_gpio_kernel_number(const char * const alias)
{
	return libsoc_board_gpio_id(config, alias);
}

int config_get_pwm_chip_number(const char * const alias)
{
	return config_get_csv_integer("PWM", alias, 0);
}

int config_get_pwm_channel_number(const char * const alias)
{
	return config_get_csv_integer("PWM", alias, 1);
}

int config_get_spi_device_number(const char * const alias)
{
	return config_get_csv_integer("SPI", alias, 0);
}

int config_get_spi_slave_number(const char * const alias)
{
	return config_get_csv_integer("SPI", alias, 1);
}

int config_get_i2c_bus(const char * const alias)
{
	return conffile_get_int(config->conf, "I2C", alias, -1);
}

int config_get_adc_chip_number(const char * const alias)
{
	return config_get_csv_integer("ADC", alias, 0);
}

int config_get_adc_channel_number(const char * const alias)
{
	return config_get_csv_integer("ADC", alias, 1);
}

int check_request_mode(request_mode_t request_mode)
{
	switch (request_mode) {
	case REQUEST_SHARED:
	case REQUEST_GREEDY:
	case REQUEST_WEAK:
		return EXIT_SUCCESS;
	default:
		return EXIT_FAILURE;
	}
}

/**
 * digiapix_init() - Initializes the library
 *
 * Executed at the startup of the program, i.e before 'main()' function.
 */
static void digiapix_init(void)
{
	init_logger(LOG_ERR, LOG_CONS | LOG_NDELAY | LOG_PID | LOG_PERROR);

	config_load();
}

/**
 * digiapix_fini() - Releases the library resources
 *
 * Executed just before the program terminates through _exit, i.e after 'main()'
 * function.
 */
static void digiapix_fini(void)
{
	config_free();

	close_logger();
}

/**
 * config_load() - Load board configuration specific values
 *
 * Use 'config_free()' to free the configuration memory.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
static int config_load(void)
{
	if (config == NULL) {
		if (access(DEFAULT_DIGIAPIX_CFG_FILE, R_OK) == 0 &&
		    setenv("LIBSOC_CONF", DEFAULT_DIGIAPIX_CFG_FILE, 1) != 0)
			return EXIT_FAILURE;
		config = libsoc_board_init();
	}

	return config == NULL ? EXIT_FAILURE : EXIT_SUCCESS;
}

/**
 * config_free() - Free up memory for the configuration
 */
static void config_free(void)
{
	if (config != NULL) {
		libsoc_board_free(config);
		config = NULL;
	}
}

/**
 * config_get_csv_integer() - Return the comma-separated integer for the given
 *                            index in the requested configuration value
 *
 * @group: The configuration group.
 * @alias: The alias of the comma-separated configuration value.
 * @index: The index of the comma-separated value to get
 *
 * Return: The selected item, or -1 on error.
 */
static int config_get_csv_integer(const char * const group, const char * const alias, int index)
{
	char *array = NULL;
	char *token = NULL;
	int item_index = 0;
	int item = -1;
	const char *value = conffile_get(config->conf, group, alias, NULL);

	if (value == NULL)
		return -1;

	array = strdup(value);
	if (array == NULL)
		return -1;
	token = strtok(array, ",");

	/* Walk through other tokens */
	while (token && (item_index < index)) {
		token = strtok(NULL, ",");
		item_index++;
	}

	if (token != NULL)
		item = (unsigned int)atoi(token);

	free(array);

	return item;
}


/**
 * get_cmd_output() - Execute the given command and return the output
 *
 * @cmd:	Command to be executed.
 *
 * Return: The first output line of the command execution.
 */
char *get_cmd_output(const char *cmd)
{
	FILE *in;
	char buff[512], *pbuf = NULL;

	in = popen(cmd, "r");
	if (!in)
		return NULL;

	pbuf = fgets(buff, ARRAY_SIZE(buff), in);
	if (pbuf)
		buff[strlen(buff) - 1] = '\0';

	pclose(in);

	if (pbuf)
		return strdup(buff);

	return pbuf;
}

/**
 * write_file() - Write a formatted string to the given file
 *
 * @path:	Full file path.
 * @format:	File name.
 *
 * Return: 0 if the string was successfully written, 1 otherwise.
 */
int write_file(const char *path, const char *format, ...)
{
	va_list argp;
	FILE *f = NULL;
	int len;

	f = fopen(path, "w");
	if (f == NULL)
		return 1;

	va_start(argp, format);

	len = vfprintf(f, format, argp);
	fclose(f);

	va_end(argp);

	if (len < 0)
		return 1;

	return 0;
}

/**
 * concat_path() - Concatenate directory path and file name
 *
 * @dir:	Directory path.
 * @file:	File name.
 *
 * Concatenate the given directory path and file name, and returns a pointer to
 * a new string with the result. If the given directory path does not finish
 * with a '/' it is automatically added.
 *
 * Memory for the new string is obtained with 'malloc' and can be freed with
 * 'free'.
 *
 * Return: A pointer to a new string with the concatenation or NULL if both
 *	   'dir' and 'file' are NULL.
 */
char *concat_path(const char *dir, const char *file)
{
	char *result = NULL;
	int len;

	if (dir == NULL && file == NULL)
		return result;

	if (dir == NULL && file != NULL)
		return strdup(file);

	len = strlen(dir) + (file == NULL ? 0 : strlen(file)) + 1;
	if (dir[strlen(dir) - 1] != '/')
		len++;

	result = malloc(sizeof(*result) * len);
	if (result == NULL)
		return NULL;

	strcpy(result, dir);

	if (dir[strlen(dir) - 1] != '/') {
		result[strlen(dir)] = '/';
		result[strlen(dir) + 1] = '\0';
	}

	if (file != NULL)
		strncat(result, file, len - strlen(result) - 1);

	return result;
}

/**
 * get_digi_platform() - Return the Digi platform
 *
 * Return: a digi_platform_t
 */
digi_platform_t get_digi_platform()
{
	char *cmd_output = NULL;
	char *cmd;
	digi_platform_t platform = INVALID_PLATFORM;

	asprintf(&cmd, READ_PATH, PLATFORM_PATH);
	if (!cmd) {
		log_error("%s: Unable to allocate memory for the command", __func__);
		return INVALID_PLATFORM;
	}

	cmd_output = get_cmd_output(cmd);
	if (cmd_output == NULL) {
		log_error("%s: Unable to get the current platform",
			  __func__);
		free(cmd);
		return INVALID_PLATFORM;
	}

	if (strstr(cmd_output , CC6UL_PLATFORM_STRING) != NULL)
		platform = CC6UL_PLATFORM;
	else if (strstr(cmd_output, CC8MN_PLATFORM_STRING) != NULL)
		platform = CC8MN_PLATFORM;
	else if (strstr(cmd_output, CC8X_PLATFORM_STRING) != NULL)
		platform = CC8X_PLATFORM;
	else if (strstr(cmd_output, CC6_PLATFORM_STRING) != NULL ||
			strstr(cmd_output, CC6DL_PLATFORM_STRING))
		platform = CC6_PLATFORM;

	free(cmd);
	free(cmd_output);

	return platform;
}
