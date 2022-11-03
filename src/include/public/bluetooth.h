/*
 * Copyright 2022, Digi International Inc.
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

#ifndef BLUETOOTH_H_
#define BLUETOOTH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "common.h"

#define BT_NAME_MAX_LEN		248

/**
 * bt_state_error_t - Defined error values for Bluetooth status
 */
typedef enum {
	BT_STATE_ERROR_NONE = 0,
	BT_STATE_ERROR_NO_EXIST,
	BT_STATE_ERROR_NO_MEM,
	BT_STATE_ERROR_HCI_INFO,
	BT_STATE_ERROR_ENABLE,
	BT_STATE_ERROR_LOCAL_NAME,
	BT_STATE_ERROR_CONFIG,
	__BT_STATE_ERROR_LAST,
} bt_state_error_t;

/**
 * bt_enabled_t - Defined values for the status of the Bluetooth interface
 */
typedef enum {
	BT_ENABLED_ERROR = -1,
	BT_DISABLED,
	BT_ENABLED,
} bt_enabled_t;

/**
 * bt_stats_t - Representation of Bluetooth statistics.
 *
 * @rx_bytes:		Number of received bytes.
 * @rx_errors:		Number of reception errors.
 * @rx_acl:		Number of received ACL frames.
 * @rx_sco:		Number of received SCO frames.
 * @rx_events:		Number of received events.
 * @tx_bytes:		Number of transmitted bytes.
 * @tx_errors:		Number of transmit errors.
 * @tx_acl:		Number of transmitted ACL frames.
 * @tx_sco:		Number of transmitted SCO frames.
 * @tx_cmds:		Number of transmit commands.
 */
typedef struct {
	uint32_t rx_bytes;
	uint32_t rx_errors;
	uint32_t rx_acl;
	uint32_t rx_sco;
	uint32_t rx_events;
	uint32_t tx_bytes;
	uint32_t tx_errors;
	uint32_t tx_acl;
	uint32_t tx_sco;
	uint32_t tx_cmds;
} bt_stats_t;

/**
 * bt_state_t - Representation of a Bluetooth interface state
 *
 * @dev_id:		Bluetooth identifier.
 * @dev_name:		Bluetooth device name.
 * @name:		Interface name.
 * @mac:		Mac address.
 * @enable:		Interface enablement state.
 * @running:		Interface is running.
 */
typedef struct {
	uint16_t dev_id;
	char dev_name[IFNAMSIZ];
	char name[BT_NAME_MAX_LEN + 1];
	uint8_t mac[MAC_ADDRESS_GROUPS];

	bt_enabled_t enable;
	bool running;
} bt_state_t;

/**
 * bt_config_t - Bluetooth configuration
 *
 * @dev_id:		Bluetooth identifier.
 * @enable:		Interface enablement state.
 * @set_name:		True to configure name, false otherwise.
 * @name:		Interface name.
 */
typedef struct {
	uint16_t dev_id;
	bt_enabled_t enable;
	bool set_name;
	char name[BT_NAME_MAX_LEN + 1];
} bt_config_t;

/**
 * ldx_bt_code_to_str() - String that describes code
 *
 * @code:	The code.
 *
 * Return: Returns a pointer to a string that describes the code.
 */
const char *ldx_bt_code_to_str(bt_state_error_t code);

/**
 * ldx_bt_device_exists() - Check if provided device id exists
 *
 * @dev_id:	Bluetooth device identifier.
 *
 * Return: True if device exists, false otherwise.
 */
bool ldx_bt_device_exists(uint16_t dev_id);

/**
 * ldx_bt_list_available_devices() - Get list of available Bluetooth devices
 *
 * @ids:	A pointer to store the available Bluetooth device identifiers.
 *
 * This function returns in 'ids' the available Bluetooth device identifiers.
 *
 * Memory for the 'ids' pointer is obtained with 'malloc' and must be freed
 * when return value is > 0.
 *
 * Return: The number of available Bluetooth devices, -1 on error.
 */
int ldx_bt_list_available_devices(uint16_t **ids);

/*
 * ldx_bt_get_state() - Retrieve the given Bluetooth device state
 *
 * @dev_id:		Bluetooth device identifier.
 * @bt_state:		Struct to fill with the Bluetooth interface state.
 *
 * Return: BT_STATE_ERROR_NONE on success, any other error code otherwise.
 */
bt_state_error_t ldx_bt_get_state(uint16_t dev_id, bt_state_t *bt_state);

/*
 * ldx_bt_get_stats() - Get the network interface statistics
 *
 * @dev_id:		Bluetooth device identifier.
 * @bt_state:		Struct to fill with the Bluetooth interface statistics.
 *
 * Return: BT_STATE_ERROR_NONE on success, any other error code otherwise.
 */
bt_state_error_t ldx_bt_get_stats(uint16_t dev_id, bt_stats_t *bt_stats);

/*
 * ldx_bt_set_config() - Configured the Bluetooth device
 *
 * @bt_cfg:		Struct with data to configure.
 *
 * Return: BT_STATE_ERROR_NONE on success, any other error code otherwise.
 */
bt_state_error_t ldx_bt_set_config(bt_config_t bt_cfg);

#ifdef __cplusplus
}
#endif

#endif /* BLUETOOTH_H_ */
