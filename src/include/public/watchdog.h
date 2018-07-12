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

#ifndef WATCHDOG_H_
#define WATCHDOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "common.h"

/**
 * wd_t - Representation of a single requested watchdog
 *
 * @node:	watchdog device node.
 * @_data:	Data for internal usage.
 */
typedef struct {
	const char * const node;
	void *_data;
} wd_t;

/**
 * wd_info_t - Watchdog support info
 *
 * @options:		Options the card/driver supports.
 * @firmware_version:	Firmware version of the card.
 * @identity:		Identity of the board.
 */
typedef struct {
	uint32_t options;
	uint32_t firmware_version;
	unsigned char identity[32];
} wd_info_t;

/**
 * ldx_watchdog_request() - Request a watchdog to use
 *
 * @wd_device_file:	Absolute watchdog device node path.
 *
 * This function returns an wd_t pointer. Memory for the 'struct' is obtained
 * with 'malloc' and must be freed with 'ldx_watchdog_free()'.
 *
 * Return: A pointer to 'wd_t' on success, NULL on error.
 */
wd_t *ldx_watchdog_request(char const * const wd_device_file);

/**
 * ldx_watchdog_get_timeout() - Get the timeout of a given watchdog
 *
 * @wd:		A requested watchdog to get its timeout value.
 *
 * Return: The timeout associated to the watchdog, -1 on error.
 */
int ldx_watchdog_get_timeout(wd_t *wd);

/**
 * ldx_watchdog_set_timeout() - Change the given watchdog timeout
 *
 * @wd:		A requested watchdog to set its timeout value.
 * @timeout:	Timeout (in seconds) to configure.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_watchdog_set_timeout(wd_t *wd, int timeout);

/**
 * ldx_watchdog_get_pretimeout() - Get the pretimeout of a given watchdog
 *
 * @wd:		A requested watchdog to get its pretimeout value.
 *
 * Return: The pretimeout associated to the watchdog, -1 on error.
 */
int ldx_watchdog_get_pretimeout(wd_t *wd);

/**
 * ldx_watchdog_set_pretimeout() - Change the given watchdog pretimeout
 *
 * @wd:		A requested watchdog to set its pretimeout value.
 * @timeout:	Pretimeout (in seconds) to configure.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_watchdog_set_pretimeout(wd_t *wd, int pretimeout);

/**
 * ldx_watchdog_get_timeleft() - Get the remaining time before the system will reboot
 *
 * @wd:		A requested watchdog to get its remaining time value.
 *
 * Return: The remaining time (in seconds) associated to the watchdog, -1 on error.
 */
int ldx_watchdog_get_timeleft(wd_t *wd);

/**
 * ldx_watchdog_get_support() - Get the watchdog support info of a given watchdog
 *
 * @wd:		A requested watchdog to get its support info.
 *
 * Return: A pointer to 'wd_info_t' on success, NULL on error.
 */
wd_info_t *ldx_watchdog_get_support(wd_t *wd);

/**
 * ldx_watchdog_refresh() - Refreshing the given watchdog
 *
 * @wd:		A requested watchdog to refresh it.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_watchdog_refresh(wd_t *wd);

/**
 * ldx_watchdog_stop() - Disable timer of given watchdog
 *
 * @wd:		A requested watchdog to disable the timer.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_watchdog_stop(wd_t *wd);

/**
 * ldx_watchdog_start() - Enable timer of given watchdog
 *
 * @wd:		A requested watchdog to enable the timer.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_watchdog_start(wd_t *wd);

/**
 * ldx_watchdog_free() - Free a previously requested watchdog
 *
 * @wd:	A pointer to the requested watchdog to free.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int ldx_watchdog_free(wd_t *wd);

#ifdef __cplusplus
}
#endif

#endif /* WATHDOG_H_ */
