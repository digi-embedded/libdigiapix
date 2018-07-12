/*
 * Copyright 2018, Digi International Inc.
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
#include <string.h>
#include <unistd.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>

#include "_common.h"
#include "watchdog.h"
#include "_watchdog.h"
#include "_log.h"

wd_t *ldx_watchdog_request(char const * const wd_device_file)
{
	wd_t *new_wd = NULL;
	wd_t init_wd = {wd_device_file, 0};
	wd_internal_t *internal_data = NULL;

	if (wd_device_file == NULL || strlen(wd_device_file) <= 0) {
		log_error("%s: Invalid watchdog device node", __func__);
		return NULL;
	}

	log_debug("%s: Requesting watchdog: %s",
			__func__, wd_device_file);

	new_wd = calloc(1, sizeof(wd_t));
	internal_data = calloc(1, sizeof(wd_internal_t));
	if (new_wd == NULL || internal_data == NULL) {
		log_error("%s: Unable to request watchdog: %s, cannot allocate memory",
				__func__, wd_device_file);
		free(new_wd);
		free(internal_data);
		return NULL;
	}

	/* Open watchdog, this will start the watchdog counters */
	internal_data->fd = open(wd_device_file, O_RDWR);
	if (internal_data->fd < 0) {
		log_error("%s: Unable to request watchdog: %s, cannot open descriptor",
				__func__, wd_device_file);
		free(new_wd);
		free(internal_data);
		return NULL;
	}
	log_debug("%s: watchdog (%s) opened and started\n",
			__func__, wd_device_file);

	memcpy(new_wd, &init_wd, sizeof(wd_t));
	((wd_t *)new_wd)->_data = internal_data;

	return new_wd;
}

int ldx_watchdog_get_timeout(wd_t *wd)
{
	int timeout;
	wd_internal_t *_wd = NULL;

	if (wd == NULL) {
		log_error("%s: watchdog cannot be NULL", __func__);
		return -1;
	}

	_wd = (wd_internal_t *) wd->_data;

	if (!ioctl(_wd->fd, WDIOC_GETTIMEOUT, &timeout)) {
		log_debug("%s: watchdog timeout interval is %d seconds\n",
				__func__, timeout);
	} else {
		log_error("%s: Failed to get watchdog timeout interval\n",
				__func__);
		return -1;
	}

	return timeout;
}

int ldx_watchdog_set_timeout(wd_t *wd, int timeout)
{
	wd_internal_t *_wd = NULL;

	if (timeout <= 0) {
		log_error("%s: Invalid watchdog timeout", __func__);
		return EXIT_FAILURE;
	}

	if (wd == NULL) {
		log_error("%s: watchdog cannot be NULL", __func__);
		return EXIT_FAILURE;
	}

	_wd = (wd_internal_t *) wd->_data;

	if (!ioctl(_wd->fd, WDIOC_SETTIMEOUT, &timeout)) {
		log_debug("%s: watchdog timeout interval was set to %d seconds\n",
				__func__, timeout);
	} else {
		log_error("%s: Failed to set watchdog timeout interval to %d seconds\n",
				__func__, timeout);
		return EXIT_FAILURE;
	}

	/* We send a keepalive ping to make sure
	 * that the watchdog keeps running and
	 * it takes the new timeout value */
	return ldx_watchdog_refresh(wd);
}

int ldx_watchdog_get_pretimeout(wd_t *wd)
{
	int pretimeout;
	wd_internal_t *_wd = NULL;

	if (wd == NULL) {
		log_error("%s: watchdog cannot be NULL", __func__);
		return -1;
	}

	_wd = (wd_internal_t *) wd->_data;

	if (!ioctl(_wd->fd, WDIOC_GETPRETIMEOUT, &pretimeout)) {
		log_debug("%s: watchdog pretimeout interval is %d seconds\n",
				__func__, pretimeout);
	} else {
		log_error("%s: Failed to get watchdog pretimeout interval\n",
				__func__);
		return -1;
	}

	return pretimeout;
}

int ldx_watchdog_set_pretimeout(wd_t *wd, int pretimeout)
{
	wd_internal_t *_wd = NULL;

	if (pretimeout <= 0) {
		log_error("%s: Invalid watchdog pretimeout", __func__);
		return EXIT_FAILURE;
	}

	if (wd == NULL) {
		log_error("%s: watchdog cannot be NULL", __func__);
		return EXIT_FAILURE;
	}

	_wd = (wd_internal_t *) wd->_data;

	if (!ioctl(_wd->fd, WDIOC_SETPRETIMEOUT, &pretimeout)) {
		log_debug("%s: watchdog pretimeout interval was set to %d seconds\n",
				__func__, pretimeout);
		return EXIT_SUCCESS;
	} else {
		log_error("%s: Failed to set watchdog pretimeout interval to %d seconds\n",
				__func__, pretimeout);
		return EXIT_FAILURE;
	}
}

int ldx_watchdog_get_timeleft(wd_t *wd)
{
	int timeleft;
	wd_internal_t *_wd = NULL;

	if (wd == NULL) {
		log_error("%s: watchdog cannot be NULL", __func__);
		return -1;
	}

	_wd = (wd_internal_t *) wd->_data;

	if (!ioctl(_wd->fd, WDIOC_GETTIMELEFT, &timeleft)) {
		log_debug("%s: watchdog timeout was is %d seconds\n",
				__func__, timeleft);
	} else {
		log_error("%s: Failed to get watchdog timeout\n",
				__func__);
		return -1;
	}

	return timeleft;
}

wd_info_t *ldx_watchdog_get_support(wd_t *wd)
{
	wd_internal_t *_wd = NULL;
	struct watchdog_info *ident;
	wd_info_t *wd_info = NULL;

	if (wd == NULL) {
		log_error("%s: watchdog cannot be NULL", __func__);
		return NULL;
	}

	_wd = (wd_internal_t *) wd->_data;

	ident = calloc(1, sizeof(struct watchdog_info));
	if (ident == NULL) {
		log_error("%s: Unable to request internal watchdog info: %s, cannot allocate memory",
				__func__, wd->node);
		return NULL;
	}

	if (!ioctl(_wd->fd, WDIOC_GETSUPPORT, ident)) {
		log_debug("%s: watchdog support was obtained\n",
				__func__);
	} else {
		log_error("%s: Failed to get watchdog support\n",
				__func__);
		free(ident);
		return NULL;
	}

	wd_info = calloc(1, sizeof(wd_info_t));
	if (wd_info == NULL) {
		log_error("%s: Unable to request watchdog info: %s, cannot allocate memory",
				__func__, wd->node);
		free(ident);
		return NULL;
	}
	memcpy(wd_info->identity, ident->identity, sizeof(ident->identity));
	wd_info->options = ident->options;
	wd_info->firmware_version = ident->firmware_version;

	free(ident);
	return wd_info;
}

int ldx_watchdog_refresh(wd_t *wd)
{
	wd_internal_t *_wd = NULL;

	if (wd == NULL) {
		log_error("%s: watchdog cannot be NULL", __func__);
		return EXIT_FAILURE;
	}

	_wd = (wd_internal_t *) wd->_data;

	if (!ioctl(_wd->fd, WDIOC_KEEPALIVE, 0)) {
		log_debug("%s: watchdog keepalive was sent\n", __func__);
		return EXIT_SUCCESS;
	} else {
		log_error("%s: Failed to send watchdog keepalive\n", __func__);
		return EXIT_FAILURE;
	}
}

int ldx_watchdog_stop(wd_t *wd)
{
	int flags = WDIOS_DISABLECARD;
	wd_internal_t *_wd = NULL;

	if (wd == NULL) {
		log_error("%s: watchdog cannot be NULL", __func__);
		return EXIT_FAILURE;
	}

	_wd = (wd_internal_t *) wd->_data;

	if (!ioctl(_wd->fd, WDIOC_SETOPTIONS, &flags)) {
		log_debug("%s: watchdog timer was disabled\n", __func__);
		return EXIT_SUCCESS;
	} else {
		log_error("%s: Failed to disable watchdog timer\n", __func__);
		return EXIT_FAILURE;
	}
}

int ldx_watchdog_start(wd_t *wd)
{
	int flags = WDIOS_ENABLECARD;
	wd_internal_t *_wd = NULL;

	if (wd == NULL) {
		log_error("%s: watchdog cannot be NULL", __func__);
		return EXIT_FAILURE;
	}

	_wd = (wd_internal_t *) wd->_data;

	if (!ioctl(_wd->fd, WDIOC_SETOPTIONS, &flags)) {
		log_debug("%s: watchdog timer was enabled\n", __func__);
		return EXIT_SUCCESS;
	} else {
		log_error("%s: Failed to enable watchdog timer\n", __func__);
		return EXIT_FAILURE;
	}
}

int ldx_watchdog_free(wd_t *wd)
{
	int ret = EXIT_SUCCESS;
	wd_internal_t *_wd = NULL;

	if (wd == NULL)
		return EXIT_SUCCESS;

	_wd = (wd_internal_t *) wd->_data;

	log_debug("%s: Freeing watchdog %s", __func__,
			wd->node);

	if (close(_wd->fd) < 0) {
		log_error("%s: Error freering watchdog", __func__);
		ret = EXIT_FAILURE;
	}

	free(wd);
	free(_wd);

	return ret;
}
