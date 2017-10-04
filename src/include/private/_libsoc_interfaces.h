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

#ifndef _LIBSOC_INTERFACES_H_
#define _LIBSOC_INTERFACES_H_

#include <libsoc_gpio.h>
#include <libsoc_i2c.h>
#include <libsoc_pwm.h>
#include <libsoc_spi.h>

/* GPIO types */
typedef struct gpio_callback libsoc_gpio_callback_t;
typedef gpio libsoc_gpio_t;
typedef gpio_int_ret libsoc_gpio_int_ret_t;
typedef gpio_direction libsoc_gpio_direction_t;
typedef gpio_level libsoc_gpio_level_t;
typedef gpio_edge libsoc_gpio_edge_t;
typedef gpio_mode libsoc_gpio_mode_t;

/* I2C types */
typedef i2c libsoc_i2c_t;

/* PWM types */
typedef pwm libsoc_pwm_t;
typedef pwm_enabled libsoc_pwm_enabled_t;
typedef pwm_polarity libsoc_pwm_polarity_t;
typedef shared_mode libsoc_shared_mode_t;

/* SPI types */
typedef spi libsoc_spi_t;
typedef spi_bpw libsoc_spi_bpw_t;
typedef spi_mode libsoc_spi_mode_t;

#endif /* _LIBSOC_INTERFACES_H_ */
