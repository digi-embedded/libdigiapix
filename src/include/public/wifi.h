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

#ifndef WIFI_H_
#define WIFI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "network.h"

/**
 * wifi_state_error_t - Defined error values for WiFi status
 */
typedef enum {
	WIFI_STATE_ERROR_NONE =		NET_STATE_ERROR_NONE,
	WIFI_STATE_ERROR_NO_EXIST =	NET_STATE_ERROR_NO_EXIST,
	WIFI_STATE_ERROR_NO_MEM =	NET_STATE_ERROR_NO_MEM,
	WIFI_STATE_ERROR_NO_IFACES =	NET_STATE_ERROR_NO_IFACES,
	WIFI_STATE_ERROR_STATE =	NET_STATE_ERROR_STATE,
	WIFI_STATE_ERROR_MAC =		NET_STATE_ERROR_MAC,
	WIFI_STATE_ERROR_IP =		NET_STATE_ERROR_IP,
	WIFI_STATE_ERROR_NETMASK =	NET_STATE_ERROR_NETMASK,
	WIFI_STATE_ERROR_GATEWAY =	NET_STATE_ERROR_GATEWAY,
	WIFI_STATE_ERROR_DNS =		NET_STATE_ERROR_DNS,
	WIFI_STATE_ERROR_MTU =		NET_STATE_ERROR_MTU,
	WIFI_STATE_ERROR_STATS =	NET_STATE_ERROR_STATS,
	WIFI_STATE_ERROR_NOT_CONFIG =	NET_STATE_ERROR_NOT_CONFIG,
	WIFI_STATE_ERROR_CONFIG =	NET_STATE_ERROR_CONFIG,
	WIFI_STATE_ERROR_RANGE_INFO,
	WIFI_STATE_ERROR_SSID,
	WIFI_STATE_ERROR_FREQ,
	WIFI_STATE_ERROR_CHANNEL,
	WIFI_STATE_ERROR_SEC_MODE,
	__WIFI_STATE_ERROR_LAST,
} wifi_state_error_t;

/**
 * wifi_sec_mode_t - Defined values for WiFi security mode
 */
typedef enum {
	WIFI_SEC_MODE_ERROR = -1,	/* Error when security mode cannot be determined. */
	WIFI_SEC_MODE_OPEN,		/* Open */
	WIFI_SEC_MODE_WPA,		/* WiFi Protected Access */
	WIFI_SEC_MODE_WPA2,		/* WiFi Protected Access 2 */
	WIFI_SEC_MODE_WPA3,		/* WiFi Protected Access 3 */
} wifi_sec_mode_t;

/**
 * wifi_state_t - Representation of a WiFi interface
 *
 * @net_state:		Interface state.
 * @ssid:		SSID name.
 * @freq:		Frequency.
 * @channel:		Channel.
 * @sec_mode:		WiFi security mode.
 */
typedef struct {
	net_state_t net_state;
	char ssid[IW_ESSID_MAX_SIZE];
	double freq;
	int channel;
	wifi_sec_mode_t sec_mode;
} wifi_state_t;

/**
 * wifi_config_t - WiFi configuration
 *
 * @name:		Interface name.
 * @ssid:		SSID name.
 * @sec_mode:		WiFi security mode.
 * @psk:		Pre-shared-key.
 * @net_config:		Interface configuration.
 */
typedef struct {
	char name[IFNAMSIZ];
	bool set_ssid;
	char ssid[IW_ESSID_MAX_SIZE];
	wifi_sec_mode_t sec_mode;
	char *psk;
	net_config_t net_config;
} wifi_config_t;

/**
 * ldx_wifi_code_to_str() - String that describes code
 *
 * @code:	The code.
 *
 * Return: Returns a pointer to a string that describes the code.
 */
const char *ldx_wifi_code_to_str(wifi_state_error_t code);

/**
 * ldx_wifi_iface_exists() - Check if provided WiFi interface exists or not
 *
 * @iface_name:	Name of the WiFi interface to check.
 *
 * Return: True if interface exists, false otherwise.
 */
bool ldx_wifi_iface_exists(const char* iface_name);

/**
 * ldx_wifi_list_available_ifaces() - Get list of available WiFi interface names
 *
 * @iface_list:	A pointer to store the available WiFi interface names.
 *
 * Return: The number of available WiFi interfaces, -1 on error.
 */
int ldx_wifi_list_available_ifaces(net_names_list_t *iface_list);

/*
 * ldx_wifi_get_iface_info() - Retrieve the given WiFi interface state
 *
 * @iface_name:	WiFi interface name.
 * @wifi_state:	Struct to fill with the WiFi interface state.
 *
 * Return: WIFI_STATE_ERROR_NONE on success, any other error code otherwise.
 */
wifi_state_error_t ldx_wifi_get_iface_state(const char *iface_name, wifi_state_t *wifi_state);

/*
 * ldx_wifi_set_config() - Configure the given WiFi interface
 *
 * @wifi_cfg:		Struct with data to configure.
 *
 * Return: WIFI_STATE_ERROR_NONE on success, any other error code otherwise.
 */
wifi_state_error_t ldx_wifi_set_config(wifi_config_t wifi_cfg);

/**
 * ldx_wifi_list_available_freqs() - List of available frequencies
 *
 * @iface_name:	WiFi interface name.
 * @freqs:	A pointer to store the available frequencies of the interface.
 *
 * This function returns in 'channels' the available WiFi interface frequencies.
 *
 * Memory for the 'freqs' pointer is obtained with 'malloc' and must be freed
 * when return value is greater than 0.
 *
 * Return: The number of available WiFi interface frequencies, -1 on error.
 */
int ldx_wifi_list_available_freqs(const char *iface_name, double **freqs);

/**
 * ldx_wifi_list_available_channels() - List of available channels
 *
 * @iface_name:	WiFi interface name.
 * @channels:	A pointer to store the available channels of the interface.
 *
 * This function returns in 'channels' the available WiFi interface channels.
 *
 * Memory for the 'freqs' pointer is obtained with 'malloc' and must be freed
 * when return value is greater than 0.
 *
 * Return: The number of available WiFi interface channels, -1 on error.
 */
int ldx_wifi_list_available_channels(const char *iface_name, int **channels);

/**
 * ldx_str_sec_mode() - String that describes security mode
 *
 * @mode:	The WiFi security mode.
 *
 * Return: Returns a pointer to a string that describes security mode.
 */
const char *ldx_wifi_sec_mode_to_str(wifi_sec_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_H_ */
