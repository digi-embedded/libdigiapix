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

#ifndef PRIVATE__LOG_H_
#define PRIVATE__LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <syslog.h>

#define API_ID			"DIGIAPIX"

/**
 * init_logger() - Initialize the logger with the given log level
 *
 * @level:	Log level.
 * @options:	Flags to control the operation of 'openlog()' and subsequent
 *		calls to 'syslog()'.
 */
#define init_logger(level, options)				\
	do {							\
		openlog(API_ID, options, LOG_USER);		\
		setlogmask(LOG_UPTO(level));			\
	} while (0)

/**
 * close_logger() - Closes the descriptor being used to write to the system
 *		    logger.
 */
#define close_logger()						\
	closelog()

/**
 * log_error() - Log the given message as error
 *
 * @format:	Error message to log.
 * @args:	Additional arguments.
 */
#define log_error(format, ...)					\
	syslog(LOG_ERR, "[ERROR] " format, __VA_ARGS__)

/**
 * log_warning() - Log the given message as warning
 *
 * @format:	   Warning message to log.
 * @args:	   Additional arguments.
 */
#define log_warning(format, ...)					\
	syslog(LOG_WARNING, "[WARNING] " format, __VA_ARGS__)

/**
 * log_notice() - Log the given message as notice
 *
 * @format:	  Notice message to log.
 * @args:	  Additional arguments.
 */
#define log_notice(format, ...)					\
	syslog(LOG_NOTICE, "[NOTICE] " format, __VA_ARGS__)

/**
 * log_info() - Log the given message as info
 *
 * @format:	Info message to log.
 * @args:	Additional arguments.
 */
#define log_info(format, ...)					\
	syslog(LOG_INFO, "[INFO] " format, __VA_ARGS__)

/**
 * log_debug() - Log the given message as debug
 *
 * @format:	Debug message to log.
 * @args:	Additional arguments.
 */
#define log_debug(format, ...)					\
	syslog(LOG_DEBUG, "[DEBUG] " format, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* PRIVATE__LOG_H_ */
