/*
 * Copyright (c) 2017 Digi International Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * =======================================================================
 */

#ifndef COMMON_H_
#define COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <syslog.h>

/**
 * ldx_set_log_level() - Set the new log level
 *
 * @level:	New log level.
 */
#define ldx_set_log_level(level)			\
	setlogmask(LOG_UPTO(level))

/**
 * request_mode_t - Defined values for high/low GPIO level
 */
typedef enum {
	REQUEST_SHARED,	/* If the device is already exported it will not be
			 * unexported on free. If it is not exported it will be
			 * unexported on free.
			 */
	REQUEST_GREEDY,	/* The device will be always unexported on free */
	REQUEST_WEAK,	/* If the device is already exported, the request will
			 * fail. It will always be unexported on free.
			 */
} request_mode_t;

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H_ */
