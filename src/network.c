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

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "_log.h"
#include "_network.h"
#include "network.h"
#include "process.h"

#define CMD_GET_GATEWAY		"route -n | grep %s | grep 'UG[ \t]' | awk '{print $2}'"
#define CMD_IS_DHCP		"ip route | grep %s | grep default | awk '{print $7}'"
#define CMD_GET_DNS		"nmcli -g IP4.DNS device show %s"
#define CMD_IFACE_STATE		"nmcli -g GENERAL.STATE device show %s | awk -F'[()]' '{print $2}'"

#define UNKOWN_CODE	"Unknown network state error"

static const char* net_state_error_descs[] = {
	"No error",
	"Interface not found",
	"Out of memory",
	"Unable to get network interfaces",
	"Unable to get network interface state",
	"Unable to get MAC of interface",
	"Unable to get/set IP of interface",
	"Unable to get/set network mask of interface",
	"Unable to get/set gateway of interface",
	"Unable to get/set DNS of interface",
	"Unable to get MTU of interface",
	"Unable to get network statistics of interface",
	"Interface not configurable",
	"Unable to configure network interface",
};

/*
 * get_socket() - Opens a socket
 *
 * @action_msg:	The final action to log.
 * @iface_name:	Name of the interface.
 *
 * Return: The opened socket file descriptor.
 */
static int get_socket(const char *action_msg, const char *iface_name)
{
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

	if (sock == -1 && action_msg != NULL && iface_name != NULL)
		log_debug("%s: Unable to %s of interface '%s': %s (%d)",
			  __func__, action_msg, iface_name, strerror(errno), errno);

	return sock;
}

/*
 * net_ioctl() - Execute net IOCL
 *
 * @sock:	Socket to the kernel.
 * @iface_name:	Name of the interface.
 * @request:	Request code.
 * @wreq:	In/out request parameter.
 *
 * Return: ioctl code.
 */
static int net_ioctl(int sock, const char *iface_name, int request, struct ifreq *ifr) {
	strncpy(ifr->ifr_name, iface_name, IFNAMSIZ - 1);

	return ioctl(sock, request, ifr);
}

/**
 * _get_mac() - Get the MAC address of the given interface
 *
 * @iface_name:	Name of the network interface to retrieve its MAC address.
 * @mac:	Pointer to store the MAC address.
 *
 * Return: NET_STATE_ERROR_NONE on success, NET_STATE_ERROR_MAC otherwise.
 */
static net_state_error_t _get_mac(const char *iface_name, uint8_t (*mac)[MAC_ADDRESS_GROUPS], int sock)
{
	struct ifreq ifr;
	net_state_error_t ret = NET_STATE_ERROR_MAC;
	bool internal_sock = (sock < 0);

	if (internal_sock)
		sock = get_socket("get MAC", iface_name);

	if (sock == -1)
		goto done;

	memset(&ifr, 0, sizeof(ifr));
	if (net_ioctl(sock, iface_name, SIOCGIFHWADDR, &ifr) < 0) {
		log_debug("%s: %s '%s': %s (%d)", __func__,
			  ldx_net_code_to_str(ret), iface_name, strerror(errno), errno);
		goto done;
	}
	memcpy(mac, ifr.ifr_hwaddr.sa_data, MAC_ADDRESS_GROUPS);
	ret = NET_STATE_ERROR_NONE;

done:
	if (internal_sock && sock >= 0)
		close(sock);

	return ret;
}

/**
 * _get_ip() - Get the IP of the given interface
 *
 * @iface_name:	Name of the network interface to retrieve its IP address.
 * @ip:		Pointer to store the IP address.
 *
 * Return: NET_STATE_ERROR_NONE on success, NET_STATE_ERROR_IP otherwise.
 */
static net_state_error_t _get_ip(const char *iface_name, uint8_t (*ip)[IPV4_GROUPS], int sock)
{
	struct ifreq ifr;
	net_state_error_t ret = NET_STATE_ERROR_IP;
	bool internal_sock = (sock < 0);

	if (internal_sock)
		sock = get_socket("get IP", iface_name);

	if (sock == -1)
		goto done;

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_addr.sa_family = AF_INET;
	if (net_ioctl(sock, iface_name, SIOCGIFADDR, &ifr) < 0) {
		log_debug("%s: %s '%s': %s (%d)", __func__,
			  ldx_net_code_to_str(ret), iface_name, strerror(errno), errno);
		goto done;
	}

	ret = NET_STATE_ERROR_NONE;
	if (ifr.ifr_addr.sa_family == AF_INET) {
		struct sockaddr_in *sa = (struct sockaddr_in *) &ifr.ifr_addr;

		memcpy(ip, &(sa->sin_addr), IPV4_GROUPS);
	}

done:
	if (internal_sock && sock >= 0)
		close(sock);

	return ret;
}

/**
 * _get_mtu() - Get the MTU (Maximum Transfer Unit) of the given interface
 *
 * @iface_name:	Name of the network interface to retrieve its MTU.
 * @mtu:	Pointer to store the MTU.
 *
 * Return: NET_STATE_ERROR_NONE on success, NET_STATE_ERROR_MTU otherwise.
 */
static net_state_error_t _get_mtu(const char *iface_name, int *mtu, int sock)
{
	struct ifreq ifr;
	net_state_error_t ret = NET_STATE_ERROR_MTU;
	bool internal_sock = (sock < 0);

	if (internal_sock)
		sock = get_socket("get MTU", iface_name);

	if (sock == -1)
		goto done;

	memset(&ifr, 0, sizeof(ifr));
	if (net_ioctl(sock, iface_name, SIOCGIFMTU, &ifr) < 0) {
		log_debug("%s: %s '%s': %s (%d)", __func__,
			  ldx_net_code_to_str(ret), iface_name, strerror(errno), errno);
		goto done;
	}
	memcpy(mtu, &ifr.ifr_mtu, sizeof(*mtu));
	ret = NET_STATE_ERROR_NONE;

done:
	if (internal_sock && sock >= 0)
		close(sock);

	return ret;
}

/**
 * is_dhcp() - Check if provided interface uses DHCP or not
 *
 * @iface_name:	Name of the network interface to retrieve its DHCP status.
 *
 * Return: NET_ENABLED if interface uses DHCP, NET_DISABLED otherwise, and
 * NET_ENABLED_ERROR on error.
 */
static net_enabled_t is_dhcp(const char *iface_name)
{
	char *cmd = NULL, *resp = NULL;
	int len;
	net_enabled_t ret = NET_ENABLED_ERROR;

	if (strcmp(iface_name, "lo") == 0)
		return NET_DISABLED;

	len = snprintf(NULL, 0, CMD_IS_DHCP, iface_name);
	cmd = calloc(len + 1, sizeof(*cmd));
	if (cmd == NULL) {
		log_debug("%s: Unable to check '%s' DHCP: Out of memory",
			  __func__, iface_name);
		goto done;
	}

	sprintf(cmd, CMD_IS_DHCP, iface_name);

	if (ldx_process_execute_cmd(cmd, &resp, 2) != 0 || resp == NULL) {
		if (resp != NULL)
			log_debug("%s: Unable to check '%s' DHCP: %s", __func__,
				  iface_name, resp);
		else
			log_debug("%s: Unable to check '%s' DHCP", __func__,
				  iface_name);
		goto done;
	}

	ret = strncmp(resp, "dhcp", 4) == 0;

done:
	free(cmd);
	free(resp);

	return ret;
}

/**
 * get_dns() - Get the DNS addresses of the given interface
 *
 * @iface_name:	Name of the network interface to retrieve DNS.
 * @dns1:	Pointer to store the primary DNS address.
 * @dns2:	Pointer to store the secondary DNS address.
 *
 * Return: NET_STATE_ERROR_NONE on success, NET_STATE_ERROR_DNS, or
 * NET_STATE_ERROR_NO_MEM otherwise.
 */
static net_state_error_t get_dns(const char *iface_name, uint8_t (*dns1)[IPV4_GROUPS], uint8_t (*dns2)[IPV4_GROUPS])
{
	int len, i = 0;
	net_state_error_t ret = NET_STATE_ERROR_DNS;
	char *cmd = NULL, *resp = NULL, *token = NULL;
	char delim[] = " | ", dns_str[20];
	struct in_addr iaddr;

	len = snprintf(NULL, 0, CMD_GET_DNS, iface_name);
	cmd = calloc(len + 1, sizeof(*cmd));
	if (cmd == NULL) {
		ret = NET_STATE_ERROR_NO_MEM;
		log_debug("%s: Unable to get '%s' DNS: %s", __func__,
			  iface_name, ldx_net_code_to_str(ret));
		return ret;
	}

	sprintf(cmd, CMD_GET_DNS, iface_name);
	len = ldx_process_execute_cmd(cmd, &resp, 2);
	if (len != 0 || resp == NULL) {
		if (len == 127) /* Command not found */
			log_debug("%s: 'nmcli' not found", __func__);
		else if (resp != NULL)
			log_debug("%s: %s '%s': %s", __func__,
				  ldx_net_code_to_str(ret), iface_name, resp);
		else
			log_debug("%s: %s '%s'", __func__,
				  ldx_net_code_to_str(ret), iface_name);
		goto done;
	}

	if (strlen(resp) == 0)
		goto done;

	resp[strlen(resp) - 1] = '\0';  /* Remove the last line feed */

	token = strtok(resp, delim);
	/* Walk through other tokens */
	while (token && i < MAX_DNS_ADDRESSES) {
		snprintf(dns_str, sizeof(dns_str), "%s", token);
		if (inet_aton(dns_str, &iaddr)) {
			if (i == 0)
				memcpy(dns1, &iaddr, IPV4_GROUPS);
			else
				memcpy(dns2, &iaddr, IPV4_GROUPS);
		} else {
			log_debug("%s: Unable to convert '%s' into a valid IP",
				  __func__, dns_str);
		}
		i += 1;
		token = strtok(NULL, delim);
	}
	ret = NET_STATE_ERROR_NONE;

done:
	free(cmd);
	free(resp);

	return ret;
}

/**
 * get_gateway() - Get the gateway address of the given interface
 *
 * @iface_name:	Name of the network interface to retrieve its gateway address.
 * @gateway:	Pointer to store the gateway address.
 *
 * Return: NET_STATE_ERROR_NONE on success, NET_STATE_ERROR_GATEWAY or
 * NET_STATE_ERROR_NO_MEM otherwise.
 */
static net_state_error_t get_gateway(const char *iface_name, uint8_t (*gateway)[IPV4_GROUPS])
{
	struct in_addr iaddr;
	char *cmd = NULL, *resp = NULL;
	int len;
	net_state_error_t ret = NET_STATE_ERROR_GATEWAY;

	len = snprintf(NULL, 0, CMD_GET_GATEWAY, iface_name);
	cmd = calloc(len + 1, sizeof(*cmd));
	if (cmd == NULL) {
		ret = NET_STATE_ERROR_NO_MEM;
		log_debug("%s: Unable to get '%s' gateway: %s", __func__,
			  iface_name, ldx_net_code_to_str(ret));
		return ret;
	}

	sprintf(cmd, CMD_GET_GATEWAY, iface_name);

	if (ldx_process_execute_cmd(cmd, &resp, 2) != 0 || resp == NULL) {
		if (resp != NULL)
			log_debug("%s: %s '%s': %s", __func__,
				  ldx_net_code_to_str(ret), iface_name, resp);
		else
			log_debug("%s: %s '%s'", __func__,
				  ldx_net_code_to_str(ret), iface_name);
		goto done;
	}

	if (strlen(resp) > 0)
		resp[strlen(resp) - 1] = '\0';  /* Remove the last line feed */

	if (!inet_aton(resp, &iaddr)) {
		log_debug("%s: %s '%s': Invalid IP", __func__,
			  ldx_net_code_to_str(ret), iface_name);
		goto done;
	}

	memcpy(gateway, &iaddr, IPV4_GROUPS);
	ret = NET_STATE_ERROR_NONE;

done:
	free(cmd);
	free(resp);

	return ret;
}

static net_status_t get_device_status(const char *iface_name)
{
	net_status_t status = NET_STATUS_UNKNOWN;
	char *cmd = NULL, *resp = NULL;
	int rc;

	rc = snprintf(NULL, 0, CMD_IFACE_STATE, iface_name);
	cmd = calloc(rc + 1, sizeof(*cmd));
	if (cmd == NULL) {
		log_debug("%s: Unable to determine if '%s' is configurable: Out of memory",
			  __func__, iface_name);
		return NET_STATUS_UNKNOWN;
	}

	sprintf(cmd, CMD_IFACE_STATE, iface_name);
	rc = ldx_process_execute_cmd(cmd, &resp, 2);
	if (rc != 0) {
		if (rc == 127) /* Command not found */
			log_debug("%s: 'nmcli' not found", __func__);
		else if (resp != NULL)
			log_debug("%s: Unable to determine if '%s' is configurable: %s",
				  __func__, iface_name, resp);
		else
			log_debug("%s: Unable to determine if '%s' is configurable",
				  __func__, iface_name);
		goto done;
	}

	if (strlen(resp) == 0)
		goto done;

	resp[strlen(resp) - 1] = '\0';  /* Remove the last line feed */
	if (strcmp(resp, "connected") == 0)
		status = NET_STATUS_CONNECTED;
	else if (strcmp(resp, "disconnected") == 0)
		status = NET_STATUS_DISCONNECTED;
	else if (strcmp(resp, "unmanaged") == 0)
		status = NET_STATUS_UNMANAGED;
	else if (strcmp(resp, "unavailable") == 0)
		status = NET_STATUS_UNAVAILABLE;

done:
	free(cmd);
	free(resp);

	return status;
}

const char *ldx_net_code_to_str(net_state_error_t code)
{
	if (code < 0 || code >= __NET_STATE_ERROR_LAST)
		return UNKOWN_CODE;

	return net_state_error_descs[code];
}

bool ldx_net_iface_exists(const char *iface_name)
{
	struct if_nameindex *if_ni = NULL, *iface = NULL;
	bool exists = false;

	if (iface_name == NULL || strlen(iface_name) == 0)
		return false;

	if_ni = if_nameindex();
	if (if_ni == NULL) {
		log_debug("%s: Unable to check if interface '%s' exists: %s (%d)",
			  __func__, iface_name, strerror(errno), errno);
		return false;
	}

	for (iface = if_ni; iface->if_index != 0 || iface->if_name != NULL; iface++) {
		if (strncmp(iface_name, iface->if_name, strlen(iface_name)) == 0) {
			exists = true;
			break;
		}
	}

	if_freenameindex(if_ni);

	return exists;
}

int ldx_net_list_available_ifaces(net_names_list_t *iface_list)
{
	struct if_nameindex *if_ni = NULL, *iface = NULL;

	memset(iface_list, 0, sizeof(*iface_list));

	if_ni = if_nameindex();
	if (if_ni == NULL) {
		log_error("%s: Unable to get network interfaces: %s (%d)",
			  __func__, strerror(errno), errno);
		return -1;
	}

	for (iface = if_ni; iface->if_index != 0 || iface->if_name != NULL; iface++) {
		strncpy(iface_list->names[iface_list->n_ifaces], iface->if_name, IFNAMSIZ - 1);
		iface_list->n_ifaces++;
		if (iface_list->n_ifaces == MAX_NET_IFACES) {
			log_warning("%s: Number of interfaces (%d) bigger than allowed maximum (%d)",
				  __func__, iface_list->n_ifaces, MAX_NET_IFACES);
			break;
		}
	}

	if_freenameindex(if_ni);

	return iface_list->n_ifaces;
}

net_state_error_t ldx_net_get_iface_state(const char *iface_name, net_state_t *net_state)
{
	struct ifreq ifr;
	net_state_error_t ret = NET_STATE_ERROR_NONE, tmp_ret;
	short flags = 0;
	int sock = -1;

	memset(net_state, 0, sizeof(*net_state));
	net_state->status = NET_STATUS_UNKNOWN;
	net_state->is_dhcp = NET_ENABLED_ERROR;

	if (!ldx_net_iface_exists(iface_name)) {
		ret = NET_STATE_ERROR_NO_EXIST;
		log_debug("%s: Unable to get state for '%s': %s",
			  __func__, iface_name, ldx_net_code_to_str(ret));
		return ret;
	}

	/* Fill interface name */
	strncpy(net_state->name, iface_name, IFNAMSIZ - 1);

	sock = get_socket("get state", iface_name);
	if (sock == -1)
		return NET_STATE_ERROR_STATE;

	/* Get MAC address */
	ret = _get_mac(iface_name, &net_state->mac, sock);

	/* Get flags */
	memset(&ifr, 0, sizeof(ifr));
	if (net_ioctl(sock, iface_name, SIOCGIFFLAGS, &ifr) >= 0) {
		flags = ifr.ifr_flags;
	} else {
		log_debug("%s: Unable to get flags of interface '%s': %s (%d)",
			  __func__, iface_name, strerror(errno), errno);
	}

	net_state->status = get_device_status(iface_name);
	if (net_state->status == NET_STATUS_CONNECTED || flags & IFF_LOOPBACK) {
		/* Get IP */
		tmp_ret = _get_ip(iface_name, &net_state->ipv4, sock);
		if (ret == NET_STATE_ERROR_NONE)
			ret = tmp_ret;

		if (tmp_ret == NET_STATE_ERROR_NONE && _is_valid_ip(net_state->ipv4)) {
			/* Get Broadcast address */
			memset(&ifr, 0, sizeof(ifr));
			if (net_ioctl(sock, iface_name, SIOCGIFBRDADDR, &ifr) >= 0) {
				struct sockaddr_in *sa = (struct sockaddr_in *) &ifr.ifr_broadaddr;

				memcpy(net_state->broadcast, &(sa->sin_addr), IPV4_GROUPS);
			}

			/* Get netmask */
			memset(&ifr, 0, sizeof(ifr));
			if (net_ioctl(sock, iface_name, SIOCGIFNETMASK, &ifr) >= 0) {
				struct sockaddr_in *sa = (struct sockaddr_in *) &ifr.ifr_netmask;

				memcpy(net_state->netmask, &(sa->sin_addr), IPV4_GROUPS);
			}
		}

		if (!(flags & IFF_LOOPBACK)) {
			tmp_ret = get_gateway(iface_name, &net_state->gateway);
			if (ret == NET_STATE_ERROR_NONE)
				ret = tmp_ret;
			tmp_ret = get_dns(iface_name, &net_state->dns1, &(net_state->dns2));
			if (ret == NET_STATE_ERROR_NONE)
				ret = tmp_ret;
			net_state->is_dhcp = is_dhcp(iface_name);
		}
	}

	/* Get MTU */
	tmp_ret = _get_mtu(iface_name, &net_state->mtu, sock);
	if (ret == NET_STATE_ERROR_NONE)
		ret = tmp_ret;

	if (sock >= 0)
		close(sock);

	return ret;
}

net_state_error_t ldx_net_get_iface_stats(const char *iface_name, net_stats_t *net_stats)
{
	struct ifaddrs *ifaddr = NULL, *ifa = NULL;
	net_state_error_t ret = NET_STATE_ERROR_STATS;

	memset(net_stats, 0, sizeof(*net_stats));

	if (!ldx_net_iface_exists(iface_name)) {
		ret = NET_STATE_ERROR_NO_EXIST;
		log_debug("%s: Unable to get network statistics of '%s': %s",
			  __func__, iface_name, ldx_net_code_to_str(ret));
		return ret;
	}

	if (getifaddrs(&ifaddr) == -1) {
		ret = NET_STATE_ERROR_NO_IFACES;
		log_debug("%s: %s: %s (%d)", __func__,
			  ldx_net_code_to_str(ret), strerror(errno), errno);
		return ret;
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		struct rtnl_link_stats *stats = NULL;

		if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_PACKET)
			continue;

		if (strncmp(iface_name, ifa->ifa_name, strlen(iface_name)) != 0
			|| ifa->ifa_addr == NULL)
			continue;

		if (ifa->ifa_data == NULL) {
			log_debug("%s: %s '%s'", __func__,
				  ldx_net_code_to_str(ret), iface_name);
			goto done;
		}

		stats = ifa->ifa_data;
		memcpy(net_stats, stats, sizeof(*stats));
		break;
	}
	ret = NET_STATE_ERROR_NONE;
done:
	if (ifaddr != NULL)
		freeifaddrs(ifaddr);

	return ret;
}

net_state_error_t ldx_net_set_config(net_config_t net_cfg)
{
	char *iface_name = net_cfg.name;
	net_state_t net_state;
	char *cmd = NULL, *resp = NULL;
	int rc;
	net_state_error_t ret = _net_check_cfg(net_cfg, &net_state);

	if (ret != NET_STATE_ERROR_NONE) {
		log_debug("%s: Unable to set network config of '%s': %s",
			  __func__, iface_name, ldx_net_code_to_str(ret));
		return ret;
	}

	ret = _net_get_cfg_cmd(net_cfg, net_state, false, NULL, &cmd);
	if (ret != NET_STATE_ERROR_NONE)
		goto done;

	log_debug("nmcli cmd: %s\n", cmd);
	rc = ldx_process_execute_cmd(cmd, &resp, 30);
	if (rc != 0
		|| (resp != NULL && strncmp(CMD_ERROR_PREFIX, resp, strlen(CMD_ERROR_PREFIX)) == 0)) {
		if (rc == 127) { /* Command not found */
			log_debug("%s: 'nmcli' not found", __func__);
		} else if (resp != NULL) {
			log_debug("%s: Unable to set network config for '%s': %s",
				  __func__, iface_name, resp);
		} else {
			log_debug("%s: Unable to set network config for '%s'",
				  __func__, iface_name);
		}
		ret = NET_STATE_ERROR_CONFIG;
		goto done;
	}

	ret = NET_STATE_ERROR_NONE;

done:
	free(cmd);
	free(resp);

	return ret;
}
