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

#ifndef GPIO_H_
#define GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/**
 * gpio_mode_t - Defined values for GPIO mode.
 */
typedef enum {
	GPIO_MODE_ERROR = -1,	/* Error when GPIO mode cannot be determined */
	GPIO_INPUT,		/* GPIO input mode: its value can be read */
	GPIO_OUTPUT_LOW,	/* GPIO output mode with low as initial value:
				 * its value can be written
				 */
	GPIO_OUTPUT_HIGH,	/* GPIO output mode with high as initial value:
				 * its value can be written
				 */
	GPIO_IRQ_EDGE_RISING,	/* GPIO interrupt rising mode: interrupt is
				 * triggered on rising edge, from low to high
				 */
	GPIO_IRQ_EDGE_FALLING,	/* GPIO interrupt falling mode: interrupt is
				 * triggered on falling edge, from high to low
				 */
	GPIO_IRQ_EDGE_BOTH,	/* GPIO interrupt both mode: interrupt is
				 * triggered on rising and falling edges
				 */
} gpio_mode_t;

/**
 * gpio_value_t - Defined values for high/low GPIO level
 */
typedef enum {
	GPIO_VALUE_ERROR = -1,
	GPIO_LOW,
	GPIO_HIGH,
} gpio_value_t;

/**
 * gpio_irq_error_t - Defined error values for blocked GPIO interrupts
 */
typedef enum {
	GPIO_IRQ_ERROR_NONE = 0,
	GPIO_IRQ_ERROR,
	GPIO_IRQ_ERROR_TIMEOUT,
} gpio_irq_error_t;

/**
 * gpio_t - Representation of a single requested GPIO
 *
 * @alias:		Alias of the GPIO
 * @kernel_number:	GPIO Linux ID number
 * @_data:		Data for internal usage
 */
typedef struct {
	const char * const alias;
	const unsigned int kernel_number;
	void *_data;
} gpio_t;

/**
 * Callback function type used as GPIO interrupt handler
 *
 * See 'gpio_start_wait_interrupt()'.
 */
typedef int (*gpio_interrupt_cb_t)(void *arg);

/**
 * gpio_request() - Request a GPIO to use
 *
 * @kernel_number:	The Linux ID number of the GPIO to request.
 * @mode:		The desired GPIO request mode (gpio_mode_t).
 * @request_mode:	Request mode for opening the GPIO (request_mode_t).
 *
 * This function returns a gpio_t pointer. Memory for the struct is obtained
 * with 'malloc' and must be freed with 'gpio_free()'.
 *
 * Return: A pointer to gpio_t on success, NULL on error.
 */
gpio_t *gpio_request(unsigned int kernel_number, gpio_mode_t mode, request_mode_t request_mode);

/**
 * gpio_request_by_alias() - Request a GPIO to use using its alias name
 *
 * @gpio_alias:		The alias name of the GPIO to request.
 * @mode:		The desired GPIO working mode (gpio_mode_t).
 * @request_mode:	Request mode for opening the GPIO (request_mode_t).
 *
 * This function returns a gpio_t pointer. Memory for the struct is obtained
 * with 'malloc' and must be freed with 'gpio_free()'.
 *
 * Return: A pointer to gpio_t on success, NULL on error.
 */
gpio_t *gpio_request_by_alias(const char * const gpio_alias, gpio_mode_t mode, request_mode_t request_mode);

/**
 * gpio_get_kernel_number() - Retrieve the GPIO Linux ID number of a given alias
 *
 * @gpio_alias:	The alias name of the GPIO.
 *
 * Return: The kernel number associated to the alias, -1 on error.
 */
int gpio_get_kernel_number(const char * const gpio_alias);

/**
 * gpio_free() - Free a previously requested GPIO
 *
 * @gpio:	A pointer to the requested GPIO to free.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int gpio_free(gpio_t *gpio);

/**
 * gpio_set_mode() - Change the given GPIO working mode
 *
 * @gpio:	A requested GPIO to set its working mode.
 * @mode:	Working mode to configure (gpio_mode_t).
 *
 * This function configures the given GPIO working mode to be:
 *   - An input for reading its value: GPIO_INPUT
 *   - An output for setting its value: GPIO_OUTPUT_LOW or GPIO_OUTPUT_HIGH
 *   - An interrupt trigger when there is a value change: GPIO_IRQ_EDGE_RISING,
 *     GPIO_IRQ_EDGE_FALLING, or GPIO_IRQ_EDGE_BOTH
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int gpio_set_mode(gpio_t *gpio, gpio_mode_t mode);

/**
 * gpio_get_mode() - Get the given GPIO working mode
 *
 * @gpio:	A requested GPIO to get its working mode.
 *
 * This function retrieves the GPIO working mode:
 *   - An input: GPIO_INPUT
 *   - An output: GPIO_OUTPUT_LOW or GPIO_OUTPUT_HIGH
 *   - Or interrupt generation: GPIO_IRQ_EDGE_RISING, GPIO_IRQ_EDGE_FALLING, or
 *     GPIO_IRQ_EDGE_BOTH
 *
 * Return: The GPIO working mode (GPIO_INPUT, GPIO_OUTPUT_LOW, GPIO_OUTPUT_HIGH,
 *	   GPIO_IRQ_EDGE_RISING, GPIO_IRQ_EDGE_FALLING, GPIO_IRQ_EDGE_BOTH),
 *	   GPIO_MODE_ERROR if it cannot be retrieved.
 */
gpio_mode_t gpio_get_mode(gpio_t *gpio);

/**
 * gpio_set_value() - Set the given GPIO value to high or low
 *
 * @gpio:	A requested GPIO to set its value.
 * @value:	New GPIO value (gpio_value_t): GPIO_LOW or GPIO_HIGH.
 *
 * This function only has effect if GPIO mode is:
 *   - GPIO_OUTPUT_LOW
 *   - GPIO_OUTPUT_HIGH
 *
 * If GPIO is configured as GPIO_INPUT, GPIO_IRQ_EDGE_RISING,
 * GPIO_IRQ_EDGE_FALLING, or GPIO_IRQ_EDGE_BOTH, this function returns
 * EXIT_FAILURE.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int gpio_set_value(gpio_t *gpio, gpio_value_t value);

/**
 * gpio_get_value() - Get the given GPIO value
 *
 * @gpio:	A requested GPIO to read its value.
 *
 * This function only has effect if GPIO mode is:
 *   - GPIO_INPUT
 *   - GPIO_IRQ_EDGE_RISING
 *   - GPIO_IRQ_EDGE_FALLING
 *   - GPIO_IRQ_EDGE_BOTH
 *
 * If GPIO is configured as GPIO_OUTPU_LOW or GPIO_OUTPUT_HIGH, this function
 * always returns GPIO_LOW.
 *
 * Return: The GPIO value (gpio_value_t) GPIO_LOW or GPIO_HIGH, GPIO_VALUE_ERROR
 * on error.
 */
gpio_value_t gpio_get_value(gpio_t *gpio);

/**
 * gpio_wait_interrupt() - Wait for an interrupt on the given GPIO to occur
 *
 * @gpio:	A requested GPIO to wait for interrupt on.
 * @timeout:	The maximum number of milliseconds to wait for an interrupt, -1
 *		for blocking indefinitely.
 *
 * This function blocks for the given amount of milliseconds (or indefinitely
 * for -1) until an interrupt on the provided GPIO is triggered.
 *
 * The GPIO must be configured as GPIO_IRQ_EDGE_RISING, GPIO_IRQ_EDGE_FALLING,
 * or GPIO_IRQ_EDGE_BOTH, otherwise GPIO_IRQ_ERROR will be returned.
 *
 * To use a non-blocking interrupt mechanism see 'gpio_start_wait_interrupt()'
 * and 'gpio_stop_wait_interrupt()'.
 *
 * Return: GPIO_IRQ_ERROR_NONE when the interrupt is captured,
 *	   GPIO_IRQ_ERROR_TIMEOUT if no interrupt is triggered in the specified
 *	   timeout, or GPIO_IRQ_ERROR on error.
 */
gpio_irq_error_t gpio_wait_interrupt(gpio_t *gpio, int timeout);

/**
 * gpio_start_wait_interrupt() - Start interrupt detection on the given GPIO
 *
 * @gpio:		A pointer to a requested GPIO to set an interrupt
 *			handler.
 * @interrupt_cb:	Callback function used as interrupt handler called when
 *			an interrupt is detected.
 * @arg:		Void casted pointer to pass to the interrupt handler as
 *			parameter.
 *
 * This function waits for interrupts to occur on the GPIO asynchronously.
 * It is a non-blocking function that registers the provided callback to be
 * executed when an interrupt is triggered. After that it continues waiting for
 * new interrupts.
 *
 * To stop listening for interrupts use 'gpio_stop_wait_interrupt()'.
 *
 * This function creates a new thread waiting for GPIO interrupts. This thread
 * executes the callback function when one is triggered.
 *
 * Only one interrupt will be queued if it arrives while the callback function
 * is being executed, so some interrupts may be missed if they happen too fast.
 *
 * The GPIO must be configured as GPIO_IRQ_EDGE_RISING, GPIO_IRQ_EDGE_FALLING,
 * or GPIO_IRQ_EDGE_BOTH, otherwise EXIT_FAILURE will be returned.
 *
 * To use a blocking interrupt mechanism see 'gpio_wait_interrupt()'.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int gpio_start_wait_interrupt(gpio_t *gpio, const gpio_interrupt_cb_t interrupt_cb, void *arg);

/**
 * gpio_stop_wait_interrupt() - Remove the interrupt detection on the given GPIO
 *
 * @gpio:	A pointer to a requested GPIO to stop an interrupt handler.
 *
 * This function stops the previously set interrupt handler on a GPIO using
 * 'gpio_start_wait_interrupt()'.
 *
 * This uses the 'pthread_cancel()' function, so it may cancel mid way through
 * your interrupt handler function.
 *
 * If no interrupt handler is registered for the GPIO, this function does
 * nothing and returns EXIT_SUCCESS.
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int gpio_stop_wait_interrupt(gpio_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H_ */
