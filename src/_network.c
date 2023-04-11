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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "_log.h"
#include "_network.h"
#include "process.h"

#define CMD_CONN_EXISTS		"nmcli connection show %s >/dev/null 2>&1"
#define CMD_ENABLE		"nmcli device %s %s"
#define CMD_CONN_ADD		"nmcli connection add type %s connection.id %s connection.interface-name %s"
#define CMD_CONN_MOD		"nmcli connection modify %s"
#define CMD_CONN_METHOD		" ipv4.method %s"
#define CMD_CONN_IP		" ipv4.addresses %d.%d.%d.%d/%d"
#define CMD_CONN_GATEWAY	" ipv4.gateway %d.%d.%d.%d"
#define CMD_CONN_ADD_DNS	" +ipv4.dns %d.%d.%d.%d"
#define CMD_CONN_DEL_DNS	" -ipv4.dns %d.%d.%d.%d"
#define CMD_GET_NM_NAME 	"o=\"$(nmcli -m tab -t -f GENERAL.IP-IFACE,GENERAL.%s device show)\" && { echo \"${o}\" | awk NF=NF RS='' FS='\n' OFS=':' ORS='\n' | grep %s | cut -d':' -f2; }"

/*
 * is_valid_netmask() - Check if provided network mask is valid
 *
 * @netmask:	Network mask to verify.
 *
 * Return: True if valid, false otherwise.
 */
static bool is_valid_netmask(uint8_t netmask[IPV4_GROUPS])
{
	uint32_t nm;

	if (netmask == NULL)
		return false;

	nm = netmask[3] | (netmask[2] << 8) | (netmask[1] << 16) | (netmask[0] << 24);
	if (nm == 0)
		return false;

	return (nm & (~nm >> 1)) == 0;
}

/*
 * get_cidr() - Returns the CIDR of a network mask
 *
 * @netmask:	Network mask to get its CIDR.
 *
 * Return: The CIDR, -1 if error.
 */
static int get_cidr(uint8_t netmask[IPV4_GROUPS])
{
	int cidr = 0;
	uint32_t nm;

	if (!is_valid_netmask(netmask))
		return -1;

	nm = netmask[3] | (netmask[2] << 8) | (netmask[1] << 16) | (netmask[0] << 24);
	while (nm) {
		cidr += (nm & 0x01);
		nm >>= 1;
	}

	return cidr;
}

/*
 * check_conn_exists() - Checks if a connection exists for the interface
 *
 * @iface_name:	Network interface name.
 *
 * Return: True if there is a connection for the interface.
 */
static bool check_conn_exists(const char *iface_name)
{
	bool exists = false;
	char *cmd = NULL, *resp = NULL;
	int rc, len = snprintf(NULL, 0, CMD_CONN_EXISTS, iface_name);

	cmd = calloc(len + 1, sizeof(*cmd));
	if (cmd == NULL)
		return false;
	sprintf(cmd, CMD_CONN_EXISTS, iface_name);

	rc = ldx_process_execute_cmd(cmd, &resp, 1);
	exists = (rc == 0);
	if (rc == 127) /* Command not found */
		log_debug("%s: 'nmcli' not found", __func__);

	free(cmd);
	free(resp);

	return exists;
}

/*
 * _get_nm_name() - Returns the device/connection name for network manager
 *
 * @iface_name:		Network interface name.
 * @type:		"DEVICE" or "CONNECTION".
 * @name:		Name of the device/connection for network manager
 *
 * 'name' must be freed in case of success.
 *
 * Return: 0 on success, 1 otherwise.
 */
static int _get_nm_name(const char *iface_name, const char *type, char **name)
{
	int len, ret = 1;
	char *cmd = NULL, *resp = NULL;

	*name = NULL;

	len = snprintf(NULL, 0, CMD_GET_NM_NAME, type, iface_name);
	cmd = calloc(len + 1, sizeof(*cmd));
	if (cmd == NULL) {
		log_debug("%s: Unable to get '%s' nmcli %s name: Out of memory",
			  __func__, iface_name, type);
		return ret;
	}

	sprintf(cmd, CMD_GET_NM_NAME, type, iface_name);
	len = ldx_process_execute_cmd(cmd, &resp, 2);
	if (len != 0 || resp == NULL) {
		if (len == 127) /* Command not found */
			log_debug("%s: 'nmcli' not found", __func__);
		else if (resp != NULL)
			log_debug("%s: Unable to get '%s' nmcli %s name: %s",
				  __func__, iface_name, type, resp);
		else
			log_debug("%s: Unable to get '%s' nmcli %s name",
				  __func__, iface_name, type);
		goto done;
	}

	resp[strlen(resp) - 1] = '\0';  /* Remove the last line feed */

	if (strlen(resp) == 0)
		goto done;

	*name = calloc(strlen(resp) + 1, sizeof(**name));
	if (*name == NULL) {
		log_debug("%s: Unable to get '%s' nmcli %s name: Out of memory",
			  __func__, iface_name, type);
		goto done;
	}

	strcpy(*name, resp);
	ret = 0;

done:
	free(cmd);
	free(resp);

	return ret;
}

bool _is_valid_ip(uint8_t ip[IPV4_GROUPS])
{
	if (ip == NULL)
		return false;

	return ip[3] + (ip[2] << 8) + (ip[1] << 16) + (ip[0] << 24) != 0;
}

int _get_nm_dev_name(const char *iface_name, char **name)
{
	return _get_nm_name(iface_name, "DEVICE", name);
}

int _get_nm_conn_name(const char *iface_name, char **name)
{
	return _get_nm_name(iface_name, "CONNECTION", name);
}

net_state_error_t _net_check_cfg(net_config_t net_cfg, net_state_t *net_state)
{
	char *iface_name = net_cfg.name;
	net_state_error_t ret = NET_STATE_ERROR_CONFIG;

	if (!ldx_net_iface_exists(iface_name))
		return NET_STATE_ERROR_NO_EXIST;

	if (net_state->status == NET_STATUS_UNMANAGED || net_state->status == NET_STATUS_UNAVAILABLE)
		return NET_STATE_ERROR_NOT_CONFIG;

	if (net_cfg.set_ip && !_is_valid_ip(net_cfg.ipv4))
		return NET_STATE_ERROR_IP;

	if (net_cfg.set_netmask && !is_valid_netmask(net_cfg.netmask))
		return NET_STATE_ERROR_NETMASK;

	if (net_cfg.set_gateway && !_is_valid_ip(net_cfg.gateway))
		return NET_STATE_ERROR_GATEWAY;

	ret = ldx_net_get_iface_state(iface_name, net_state);
	if (ret != NET_STATE_ERROR_NONE && ret != NET_STATE_ERROR_GATEWAY
		&& ret != NET_STATE_ERROR_DNS && ret != NET_STATE_ERROR_MTU) {
		log_debug("%s: Unable to set network config for '%s': Cannot read current state",
			  __func__, iface_name);
		return ret;
	}

	if (net_state->status == NET_STATUS_UNMANAGED || net_state->status == NET_STATUS_UNAVAILABLE)
		return NET_STATE_ERROR_NOT_CONFIG;

	return NET_STATE_ERROR_NONE;
}

net_state_error_t _net_get_cfg_cmd(net_config_t net_cfg, net_state_t net_state,
	bool is_wifi, char *extra_params, char **cmd)
{
	net_state_error_t ret = NET_STATE_ERROR_CONFIG;
	bool is_new = false;
	char *iface_name = net_cfg.name;
	char *nm_name = NULL, *cname = (char *)iface_name;
	uint8_t *dns[] = { net_cfg.dns1, net_cfg.dns2 };
	uint8_t *del_dns[] = { NULL, NULL };
	char *tmp = NULL, *enable_cmd = NULL;
	int len, i, valid_dns = 0;

	if (net_cfg.n_dns > MAX_DNS_ADDRESSES) {
		log_warning("%s: Maximum number of DNS to configure %d",
			    __func__, MAX_DNS_ADDRESSES);
		net_cfg.n_dns = 2;
	}

	for (i = 0; i < net_cfg.n_dns; i++) {
		if (_is_valid_ip(dns[i]))
			valid_dns++;
	}
	if (valid_dns != net_cfg.n_dns) {
		ret = NET_STATE_ERROR_DNS;
		log_debug("%s: Unable to set network config for '%s': %s",
			  __func__, iface_name, ldx_net_code_to_str(ret));
		return ret;
	}

	if (net_cfg.n_dns > 0)
		del_dns[0] = net_state.dns1;
	if (net_cfg.n_dns > 1)
		del_dns[1] = net_state.dns2;

	if (_get_nm_conn_name(iface_name, &nm_name) == 0)
		cname = nm_name;

	is_new = !check_conn_exists(cname);
	if (is_new) {
		len = snprintf(NULL, 0, CMD_CONN_ADD,
			is_wifi ? "802-11-wireless" : "802-3-ethernet", cname, cname);
		*cmd = calloc(len + 1, sizeof(**cmd));
		if (*cmd == NULL) {
			ret = NET_STATE_ERROR_NO_MEM;
			log_debug("%s: Unable to set network config of '%s': %s",
				  __func__, iface_name, ldx_net_code_to_str(ret));
			return ret;
		}
		sprintf(*cmd, CMD_CONN_ADD,
			is_wifi ? "802-11-wireless" : "802-3-ethernet", cname, cname);
	} else if ((extra_params != NULL && strlen(extra_params) > 0)
		|| net_cfg.is_dhcp != NET_ENABLED_ERROR
		|| net_cfg.set_ip || net_cfg.set_netmask
		|| net_cfg.set_gateway || valid_dns) {
		len = snprintf(NULL, 0, CMD_CONN_MOD, cname);
		*cmd = calloc(len + 1, sizeof(**cmd));
		if (*cmd == NULL) {
			ret = NET_STATE_ERROR_NO_MEM;
			log_debug("%s: Unable to set network config of '%s': %s",
				  __func__, iface_name, ldx_net_code_to_str(ret));
			return ret;
		}
		sprintf(*cmd, CMD_CONN_MOD, cname);
	}

	/* Connection method: static/DHCP */
	if (net_cfg.is_dhcp != NET_ENABLED_ERROR) {
		len = snprintf(NULL, 0, CMD_CONN_METHOD,
			net_cfg.is_dhcp == NET_ENABLED ? "auto ipv4.address \"\" ipv4.gateway \"\"" : "manual");
		tmp = realloc(*cmd, (strlen(*cmd) + len + 1) * sizeof(**cmd));
		if (tmp == NULL) {
			ret = NET_STATE_ERROR_NO_MEM;
			log_debug("%s: Unable to set network config of '%s': %s",
				  __func__, iface_name, ldx_net_code_to_str(ret));
			goto error;
		}

		*cmd = tmp;
		sprintf(*cmd + strlen(*cmd), CMD_CONN_METHOD,
			net_cfg.is_dhcp == NET_ENABLED ? "auto ipv4.address \"\" ipv4.gateway \"\"" : "manual");
	}

	/* IP and netmask configuration */
	if (net_cfg.set_ip || net_cfg.set_netmask) {
		uint8_t *ip = net_cfg.ipv4;
		uint8_t *netmask = net_cfg.netmask;
		int cidr = -1;

		if (!net_cfg.set_ip) {
			ip = net_state.ipv4;
			if (!_is_valid_ip(ip)) {
				ret = NET_STATE_ERROR_IP;
				log_debug("%s: Unable to set network config for '%s': Invalid IP",
					  __func__, iface_name);
				goto error;
			}
		}

		if (!net_cfg.set_netmask) {
			netmask = net_state.netmask;
			if (!is_valid_netmask(netmask)) {
				ret = NET_STATE_ERROR_NETMASK;
				log_debug("%s: Unable to set network config for '%s': Invalid network mask",
					  __func__, iface_name);
				goto error;
			}
		}

		cidr = get_cidr(netmask);

		len = snprintf(NULL, 0, CMD_CONN_IP, ip[0], ip[1], ip[2], ip[3], cidr);
		tmp = realloc(*cmd, (strlen(*cmd) + len + 1) * sizeof(**cmd));
		if (tmp == NULL) {
			ret = NET_STATE_ERROR_NO_MEM;
			log_debug("%s: Unable to set network config of '%s': %s",
				  __func__, iface_name, ldx_net_code_to_str(ret));
			goto error;
		}

		*cmd = tmp;
		sprintf(*cmd + strlen(*cmd), CMD_CONN_IP, ip[0], ip[1], ip[2], ip[3], cidr);
	}

	/* Gateway */
	if (net_cfg.set_gateway) {
		uint8_t *gateway = net_cfg.gateway;

		len = snprintf(NULL, 0, CMD_CONN_GATEWAY,
			gateway[0], gateway[1], gateway[2], gateway[3]);
		tmp = realloc(*cmd, (strlen(*cmd) + len + 1) * sizeof(**cmd));
		if (tmp == NULL) {
			ret = NET_STATE_ERROR_NO_MEM;
			log_debug("%s: Unable to set network config of '%s': %s",
				  __func__, iface_name, ldx_net_code_to_str(ret));
			goto error;
		}

		*cmd = tmp;
		sprintf(*cmd + strlen(*cmd), CMD_CONN_GATEWAY,
			gateway[0], gateway[1], gateway[2], gateway[3]);
	}

	/* DNS */
	for (i = 0; i < net_cfg.n_dns; i++) {
		if (!_is_valid_ip(dns[i]))
			continue;

		/* Add new DNS */
		len = snprintf(NULL, 0, CMD_CONN_ADD_DNS,
			dns[i][0], dns[i][1], dns[i][2], dns[i][3]);
		tmp = realloc(*cmd, (strlen(*cmd) + len + 1) * sizeof(**cmd));
		if (tmp == NULL) {
			ret = NET_STATE_ERROR_NO_MEM;
			log_debug("%s: Unable to set network config of '%s': %s",
				  __func__, iface_name, ldx_net_code_to_str(ret));
			goto error;
		}

		*cmd = tmp;
		sprintf(*cmd + strlen(*cmd), CMD_CONN_ADD_DNS,
			dns[i][0], dns[i][1], dns[i][2], dns[i][3]);

		/* Remove old DNS */
		if (!_is_valid_ip(del_dns[i]))
			continue;

		len = snprintf(NULL, 0, CMD_CONN_DEL_DNS,
			del_dns[i][0], del_dns[i][1], del_dns[i][2], del_dns[i][3]);
		tmp = realloc(*cmd, (strlen(*cmd) + len + 1) * sizeof(**cmd));
		if (tmp == NULL) {
			ret = NET_STATE_ERROR_NO_MEM;
			log_debug("%s: Unable to set network config of '%s': %s",
				  __func__, iface_name, ldx_net_code_to_str(ret));
			goto error;
		}

		*cmd = tmp;
		sprintf(*cmd + strlen(*cmd), CMD_CONN_DEL_DNS,
			del_dns[i][0], del_dns[i][1], del_dns[i][2], del_dns[i][3]);
	}

	/* Extra parameters */
	if (extra_params != NULL && strlen(extra_params) > 0) {
		len = strlen(extra_params) + 1; /* white space */ 
		tmp = realloc(*cmd, (strlen(*cmd) + len + 1) * sizeof(**cmd));
		if (tmp == NULL) {
			ret = NET_STATE_ERROR_NO_MEM;
			log_debug("%s: Unable to set network config of '%s': %s",
				  __func__, iface_name, ldx_net_code_to_str(ret));
			goto error;
		}

		*cmd = tmp;
		sprintf(*cmd + strlen(*cmd), " %s", extra_params);
	}

	/* Enable/disable the interface */
	if ((net_state.status == NET_STATUS_CONNECTED && net_cfg.status != NET_STATUS_DISCONNECTED)
		|| (net_state.status != NET_STATUS_CONNECTED && net_cfg.status == NET_STATUS_CONNECTED))
		enable_cmd = "connect";
	else if (net_state.status != NET_STATUS_DISCONNECTED && net_cfg.status == NET_STATUS_DISCONNECTED)
		enable_cmd = "disconnect";

	if (enable_cmd != NULL) {
		char *dname = (char *)iface_name;
		bool empty_cmd = (*cmd == NULL);

		free(nm_name);
		if (_get_nm_dev_name(iface_name, &nm_name) == 0)
			dname = nm_name;

		len = snprintf(NULL, 0, CMD_ENABLE, enable_cmd, dname);
		if (!empty_cmd)
			len = len + strlen(*cmd) + 4; /* ' && ' */

		tmp = realloc(*cmd, (len + 1) * sizeof(**cmd));
		if (tmp == NULL) {
			ret = NET_STATE_ERROR_NO_MEM;
			log_debug("%s: Unable to set network config of '%s': %s",
				  __func__, iface_name, ldx_net_code_to_str(ret));
			goto error;
		}

		*cmd = tmp;
		if (!empty_cmd)
			sprintf(*cmd + strlen(*cmd), " && " CMD_ENABLE, enable_cmd, dname);
		else
			sprintf(*cmd, CMD_ENABLE, enable_cmd, dname);
	}

	ret = NET_STATE_ERROR_NONE;

	return ret;

error:
	free(nm_name);
	free(*cmd);
	*cmd = NULL;

	return ret;
}
