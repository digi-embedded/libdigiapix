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

#ifndef PRIVATE__NETWORK_H_
#define PRIVATE__NETWORK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "network.h"

#define MAX_DNS_ADDRESSES	2

#define CMD_ERROR_PREFIX	"Error:"

/*
 * is_valid_ip() - Check if provided IP is valid
 *
 * @ip:	IP to verify.
 *
 * Return: True if valid, false otherwise.
 */
bool _is_valid_ip(uint8_t ip[IPV4_GROUPS]);

/*
 * _net_check_cfg() - Check whether the network configuration is valid
 *
 * @net_cfg:		Struct with data to configure.
 * @net_state:		Struct to store the current state of the network.
 *
 * Return: NET_STATE_ERROR_NONE on success, any other error code otherwise.
 */
net_state_error_t _net_check_cfg(net_config_t net_cfg, net_state_t *net_state);

/*
 * _net_get_cfg_cmd() - Constructs the command to apply a network configuration
 *
 * @net_cfg:		Struct with data to configure.
 * @net_state:		Struct with the state of the network interface.
 * @is_wifi:		True if it is for WiFi interface configuration.
 * @extra_params:	Extra parameters to be added, NULL if there isn't.
 * @cmd:		Pointer to store the command.
 *
 * Return: NET_STATE_ERROR_NONE on success, any other error code otherwise.
 */
net_state_error_t _net_get_cfg_cmd(net_config_t net_cfg, net_state_t net_state,
	bool is_wifi, char *extra_params, char **cmd);

#ifdef __cplusplus
}
#endif

#endif /* PRIVATE__NETWORK_H_ */
