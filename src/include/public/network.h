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

#ifndef NETWORK_H_
#define NETWORK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "common.h"

#define IPV4_GROUPS		4
#define MAX_NET_IFACES		32

/**
 * net_status_t - Network status values
 */
typedef enum {
	NET_STATUS_CONNECTED = 0,
	NET_STATUS_DISCONNECTED,
	NET_STATUS_UNMANAGED,
	NET_STATUS_UNAVAILABLE,
	NET_STATUS_UNKNOWN,
	__NET_STATUS_LAST,
} net_status_t;

/**
 * net_state_error_t - Defined error values for network status
 */
typedef enum {
	NET_STATE_ERROR_NONE = 0,
	NET_STATE_ERROR_NO_EXIST,
	NET_STATE_ERROR_NO_MEM,
	NET_STATE_ERROR_NO_IFACES,
	NET_STATE_ERROR_STATE,
	NET_STATE_ERROR_MAC,
	NET_STATE_ERROR_IP,
	NET_STATE_ERROR_NETMASK,
	NET_STATE_ERROR_GATEWAY,
	NET_STATE_ERROR_DNS,
	NET_STATE_ERROR_MTU,
	NET_STATE_ERROR_STATS,
	NET_STATE_ERROR_NOT_CONFIG,
	NET_STATE_ERROR_CONFIG,
	__NET_STATE_ERROR_LAST,
} net_state_error_t;

/**
 * net_enabled_t - Defined values for the status of the network interface
 */
typedef enum {
	NET_ENABLED_ERROR = -1,
	NET_DISABLED,
	NET_ENABLED,
} net_enabled_t;

/**
 * net_names_list_t - List of network interfaces names
 *
 * @n_ifaces:		Number of interfaces.
 * @names:		Array of interface names.
 */
typedef struct {
	int n_ifaces;
	char names[MAX_NET_IFACES][IFNAMSIZ];
} net_names_list_t;

/**
 * net_stats_t - Representation of network statistics
 *
 * @rx_packets:		Total packets received.
 * @tx_packets:		Total packets transmitted.
 * @rx_bytes:		Total bytes received.
 * @tx_bytes:		Total bytes transmitted.
 * @rx_errors:		Bad packets receive.
 * @tx_errors:		Packet transmit problems.
 * @rx_dropped:		No space in linux buffers.
 * @tx_dropped:		No space available in linux.
 * @multicast:		Multicast packets received.
 * @collisions:		Number of collisions.
 * @rx_length_errors:	Received packets with length error.
 * @rx_over_errors:	Receiver ring buff overflow.
 * @rx_crc_errors:	Received packet with crc error.
 * @rx_frame_errors:	Received frame alignment error.
 * @rx_fifo_errors:	Receiver fifo overrun.
 * @rx_missed_errors:	Receiver missed packet.
 * @tx_aborted_errors:	Aborted transmissions.
 * @tx_carrier_errors:	Carrier errors.
 * @tx_fifo_errors:	Transmitter fifo overrun.
 * @tx_heartbeat_errors:Heartbeat errors.
 * @tx_window_errors:	Window errors.
 * @rx_compressed:	Received compressed packets.
 * @tx_compressed:	Transmitted compressed packets.
 * @rx_nohandler:	Dropped, no handler found.
 */
typedef struct {
	uint32_t rx_packets;
	uint32_t tx_packets;
	uint32_t rx_bytes;
	uint32_t tx_bytes;
	uint32_t rx_errors;
	uint32_t tx_errors;
	uint32_t rx_dropped;
	uint32_t tx_dropped;
	uint32_t multicast;
	uint32_t collisions;

	/* Detailed rx_errors: */
	uint32_t rx_length_errors;
	uint32_t rx_over_errors;
	uint32_t rx_crc_errors;
	uint32_t rx_frame_errors;
	uint32_t rx_fifo_errors;
	uint32_t rx_missed_errors;

	/* Detailed tx_errors */
	uint32_t tx_aborted_errors;
	uint32_t tx_carrier_errors;
	uint32_t tx_fifo_errors;
	uint32_t tx_heartbeat_errors;
	uint32_t tx_window_errors;

	/* For cslip etc */
	uint32_t rx_compressed;
	uint32_t tx_compressed;

	uint32_t rx_nohandler;
} net_stats_t;

/**
 * net_state_t - Representation of a network interface state
 *
 * @name:		Interface name.
 * @mac:		Mac address.
 * @status:		Interface status.
 * @is_dhcp:		DHCP or static configuration.
 * @ipv4:		Interface IP.
 * @gateway:		Gateway IP.
 * @netmask:		Network mask.
 * @broadcast:		Broadcast address.
 * @mtu:		MTU (Maximum Transfer Unit).
 * @dns1:		Primary DNS server.
 * @dns2:		Secondary DNS server.
 */
typedef struct {
	char name[IFNAMSIZ];
	uint8_t mac[MAC_ADDRESS_GROUPS];

	net_status_t status;
	net_enabled_t is_dhcp;
	uint8_t ipv4[IPV4_GROUPS];
	uint8_t gateway[IPV4_GROUPS];
	uint8_t netmask[IPV4_GROUPS];
	uint8_t broadcast[IPV4_GROUPS];
	int mtu;
	uint8_t dns1[IPV4_GROUPS];
	uint8_t dns2[IPV4_GROUPS];
} net_state_t;

/**
 * net_config_t - Network configuration
 *
 * @name:		Interface name.
 * @status:		Interface status.
 * @is_dhcp:		DHCP or static configuration.
 * @ipv4:		Interface IP.
 * @gateway:		Gateway IP.
 * @netmask:		Network mask.
 * @n_dns:		Number of DNS to configure.
 * @dns1:		Primary DNS server.
 * @dns2:		Secondary DNS server.
 */
typedef struct {
	char name[IFNAMSIZ];
	net_status_t status;
	net_enabled_t is_dhcp;
	bool set_ip;
	uint8_t ipv4[IPV4_GROUPS];
	bool set_gateway;
	uint8_t gateway[IPV4_GROUPS];
	bool set_netmask;
	uint8_t netmask[IPV4_GROUPS];
	uint8_t n_dns;
	uint8_t dns1[IPV4_GROUPS];
	uint8_t dns2[IPV4_GROUPS];
} net_config_t;

/**
 * ldx_net_code_to_str() - String that describes code
 *
 * @code:	The code.
 *
 * Return: Returns a pointer to a string that describes the code.
 */
const char *ldx_net_code_to_str(net_state_error_t code);

/**
 * ldx_net_iface_exists() - Check if provided interface exists or not
 *
 * @iface_name:	Name of the network interface to check.
 *
 * Return: True if interface exists, false otherwise.
 */
bool ldx_net_iface_exists(const char *iface_name);

/**
 * ldx_net_list_available_ifaces() - Get list of available network names
 *
 * @iface_list:	A pointer to store the available network interface names.
 *
 * Return: The number of available network interfaces, -1 on error.
 */
int ldx_net_list_available_ifaces(net_names_list_t *iface_list);

/*
 * ldx_net_get_iface_state() - Retrieve the given network interface state
 *
 * @iface_name:		Network interface name.
 * @net_state_t:	Struct to fill with the network interface state.
 *
 * Return: NET_STATE_ERROR_NONE on success, any other error code otherwise.
 */
net_state_error_t ldx_net_get_iface_state(const char *iface_name, net_state_t *net_state);

/**
 * ldx_net_get_iface_stats() - Get the network interface statistics
 *
 * @iface_name:	Name of the network interface to get its statistics.
 * @net_stats:	Struct to fill with the network interface statistics.
 *
 * Return: NET_STATE_ERROR_NONE on success, any other error code otherwise.
 */
net_state_error_t ldx_net_get_iface_stats(const char *iface_name, net_stats_t *net_stats);

/*
 * ldx_net_set_config() - Configure the given network interface
 *
 * @net_cfg:		Struct with data to configure.
 *
 * Return: NET_STATE_ERROR_NONE on success, any other error code otherwise.
 */
net_state_error_t ldx_net_set_config(net_config_t net_cfg);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_H_ */
