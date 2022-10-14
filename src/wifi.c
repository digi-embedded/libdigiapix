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
#include <errno.h>
#include <linux/wireless.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "_common.h"
#include "_log.h"
#include "_network.h"
#include "process.h"
#include "wifi.h"

#define CMD_GET_SECURITY	"nmcli -g IN-USE,SSID,SECURITY device wifi list ifname %s --rescan no | grep '*:' | awk -F'[:]' '{print $3}'"
#define CMD_DEL_CONN		"nmcli connection delete %s"
#define CMD_CONN_SSID		" 802-11-wireless.ssid \"%s\" 802-11-wireless.hidden true"
#define CMD_CONN_KEY_MGMT	" 802-11-wireless-security.key-mgmt %s"	/* none, wpa-psk */
#define CMD_CONN_AUTH_ALG	" 802-11-wireless-security.auth-alg %s"	/* open */
#define CMD_CONN_PSK		" 802-11-wireless-security.psk \"%s\""	/* pwd, only if key-mgmt=wpa-psk */
#define CMD_CONN_PSK_FLAGS	" 802-11-wireless-security.psk-flags %d"	/* 0, only if key-mgmt=wpa-psk */

#define UNKOWN_SECURITY_MODE	"Unknown WiFi security mode"

#define UNKOWN_CODE	"Unknown WiFi state error"

static const char* wifi_state_error_descs[] = {
	"No error",
	"Interface not found",
	"Out of memory",
	"Unable to get WiFi interfaces",
	"Unable to get WiFi interface state",
	"Unable to get MAC of interface",
	"Unable to get/set IP of interface",
	"Unable to get/set network mask of interface",
	"Unable to get/set gateway of interface",
	"Unable to get/set DNS of interface",
	"Unable to get MTU of interface",
	"Unable to get network statistics of interface",
	"Interface not configurable",
	"Unable to configure network interface",
	"Unable to get range information",
	"Unable to get SSID",
	"Unable to get frequency",
	"Unable to get channel",
	"Unable to get security mode",
};

static const char* wifi_sec_mode_names[] = {
	"Open",
	"WPA1",
	"WPA2",
	"WPA3",
};

/*
 * wifi_ioctl() - Execute WiFi IOCL
 *
 * @sock:	Socket to the kernel.
 * @iface_name:	Name of the wireless interface.
 * @request:	Wireless extension request code.
 * @wreq:	In/out request parameter.
 *
 * Return: ioctl code.
 */
static int wifi_ioctl(int sock, const char *iface_name, int request, struct iwreq *wreq) {
	strncpy(wreq->ifr_name, iface_name, IFNAMSIZ - 1);

	return ioctl(sock, request, wreq);
}

/*
 * freq2float() - Converts a frequency to a float value in Hertz.
 *
 * @freq:	The frequency to convert.
 *
 * Return: The frequency value.
 */
static double freq2float(struct iw_freq freq)
{
	double res = (double) freq.m;
	int i = 0;

	for(i = 0; i < freq.e; i++)
		res *= 10;

	return res;
}

/*
 * get_range_info() - Retrieve the range information of the provided interface
 *
 * @iface_name:	Name of the wireless interface to retrieve the information.
 * @sock:	Socket for the IOCTL.
 * @range:	Struct to store the range information.
 *
 * Return: WIFI_STATE_ERROR_NONE on success, WIFI_STATE_ERROR_RANGE_INFO otherwise.
 */
static wifi_state_error_t get_range_info(const char *iface_name, int sock, struct iw_range *range)
{
	struct iwreq wreq;
	char buffer[sizeof(struct iw_range) * 2]; /* Large enough */
	wifi_state_error_t ret = WIFI_STATE_ERROR_RANGE_INFO;

	memset(buffer, 0, sizeof(buffer));

	wreq.u.data.pointer = (caddr_t) buffer;
	wreq.u.data.length = sizeof(buffer);
	wreq.u.data.flags = 0;
	if(wifi_ioctl(sock, iface_name, SIOCGIWRANGE, &wreq) >= 0) {
		ret = WIFI_STATE_ERROR_NONE;
		memcpy((char *)range, buffer, sizeof(struct iw_range));
	} else {
		log_debug("%s: %s of '%s': %s (%d)", __func__,
			  ldx_wifi_code_to_str(ret), iface_name, strerror(errno), errno);
	}

	return ret;
}

/*
 * get_ssid() - Retrieve the SSID to which the interface is connected to
 *
 * @iface_name:	Name of the wireless interface to retrieve its connected SSID.
 * @sock:	Socket for the IOCTL.
 * @ssid:	String to store the SSID value.
 *
 * Return: WIFI_STATE_ERROR_NONE on success, WIFI_STATE_ERROR_SSID otherwise.
 */
static wifi_state_error_t get_ssid(const char *iface_name, int sock, char *ssid)
{
	struct iwreq wreq;
	wifi_state_error_t ret = WIFI_STATE_ERROR_SSID;

	memset(&wreq, 0, sizeof(struct iwreq));
	wreq.u.essid.length = IW_ESSID_MAX_SIZE;
	wreq.u.essid.pointer = ssid;
	if (wifi_ioctl(sock, iface_name, SIOCGIWESSID, &wreq) >= 0) {
		ret = WIFI_STATE_ERROR_NONE;
	} else {
		ssid = NULL;
		log_debug("%s: %s of '%s': %s (%d)", __func__,
			  ldx_wifi_code_to_str(ret), iface_name, strerror(errno), errno);
	}

	return ret;
}

/*
 * get_freq() - Retrieve the current frequency
 *
 * @iface_name:	Name of the wireless interface to retrieve its frequency.
 * @sock:	Socket for the IOCTL.
 * @freq:	Double to store the frequency value.
 *
 * Return: WIFI_STATE_ERROR_NONE on success, WIFI_STATE_ERROR_FREQ otherwise.
 */
static wifi_state_error_t get_freq(const char *iface_name, int sock, double *freq)
{
	struct iwreq wreq;
	wifi_state_error_t ret = WIFI_STATE_ERROR_FREQ;

	memset(&wreq, 0, sizeof(struct iwreq));
	if (wifi_ioctl(sock, iface_name, SIOCGIWFREQ, &wreq) >= 0) {
		*freq = freq2float(wreq.u.freq);
		ret = WIFI_STATE_ERROR_NONE;
	} else {
		*freq = -1;
		log_debug("%s: %s of '%s': %s (%d)", __func__,
			  ldx_wifi_code_to_str(ret), iface_name, strerror(errno), errno);
	}

	return ret;
}

/*
 * get_channel() - Retrieve the current channel
 *
 * @iface_name:	Name of the wireless interface to retrieve its channel.
 * @sock:	Socket for the IOCTL.
 * @freq:	Current frequency value.
 * @channel:	Integer to store the channel value.
 *
 * Return: WIFI_STATE_ERROR_NONE on success, WIFI_STATE_ERROR_CHANNEL or
 * WIFI_STATE_ERROR_RANGE_INFO otherwise.
 */
static wifi_state_error_t get_channel(const char *iface_name, int sock, double freq, int *channel)
{
	struct iw_range range;
	wifi_state_error_t ret;
	int i = 0;

	*channel = -1;

	ret = get_range_info(iface_name, sock, &range);
	if (ret != WIFI_STATE_ERROR_NONE)
		return ret;

	if (range.num_frequency <= 0) {
		ret = WIFI_STATE_ERROR_CHANNEL;
		log_debug("%s: %s of '%s' ", __func__,
			  ldx_wifi_code_to_str(ret), iface_name);
		return ret;
	}

	for(i = 0; i < range.num_frequency; i++) {
		if (freq == freq2float(range.freq[i])) {
			*channel = range.freq[i].i;
			ret = WIFI_STATE_ERROR_NONE;
			break;
		}
	}

	return ret;
}

/*
 * get_sec_mode() - Retrieve the WiFi security mode
 *
 * @iface_name:	Name of the wireless interface to retrieve its security mode.
 * @freq:	WiFi security mode.
 *
 * Return: WIFI_STATE_ERROR_NONE on success, WIFI_STATE_ERROR_CHANNEL or
 * WIFI_STATE_ERROR_NO_MEM otherwise.
 */
static wifi_state_error_t get_sec_mode(const char *iface_name, wifi_sec_mode_t *mode)
{
	char *cmd = NULL, *resp = NULL, *token = NULL;
	char delim[] = " ";
	wifi_state_error_t ret = WIFI_STATE_ERROR_SEC_MODE;
	size_t len;

	*mode = WIFI_SEC_MODE_ERROR;

	len = snprintf(NULL, 0, CMD_GET_SECURITY, iface_name);
	cmd = calloc(len + 1, sizeof(char));
	if (cmd == NULL) {
		ret = WIFI_STATE_ERROR_NO_MEM;
		log_debug("%s: Unable to get security mode of '%s': %s",
			  __func__, iface_name, ldx_wifi_code_to_str(ret));
		return ret;
	}

	sprintf(cmd, CMD_GET_SECURITY, iface_name);

	if (ldx_process_execute_cmd(cmd, &resp, 2) != 0) {
		if (resp != NULL) {
			log_debug("%s: %s of '%s': %s", __func__,
				  ldx_wifi_code_to_str(ret), iface_name, resp);
			free(resp);
		} else {
			log_debug("%s: %s of '%s'", __func__,
				  ldx_wifi_code_to_str(ret), iface_name);
		}
		goto done;
	}

	if (resp == NULL || strlen(resp) == 0)
		goto done;

	resp[strlen(resp) - 1] = '\0';  /* Remove the last line feed */
	if (strlen(resp) == 0) { /* Empty is an open access point */
		*mode = 0;
		ret = WIFI_STATE_ERROR_NONE;
		goto done;
	}

	token = strtok(resp, delim);
	while (token) {
		int i;

		for (i = 0; i < ARRAY_SIZE(wifi_sec_mode_names); i++) {
			if (strncmp(token, wifi_sec_mode_names[i], strlen(wifi_sec_mode_names[i])) == 0
				&& *mode < i) {
				*mode = i;
				ret = WIFI_STATE_ERROR_NONE;
				break;
			}
		}
		token = strtok(NULL, delim);
	}
done:
	free(cmd);

	return ret;
}

const char *ldx_wifi_code_to_str(wifi_state_error_t code)
{
	if (code < 0 || code >= __WIFI_STATE_ERROR_LAST)
		return UNKOWN_CODE;

	return wifi_state_error_descs[code];
}

bool ldx_wifi_iface_exists(const char* iface_name)
{
	bool exists = false;
	int sock = -1;
	struct iwreq wreq;

	if (!ldx_net_iface_exists(iface_name))
		return false;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (sock == -1) {
		log_debug("%s: Unable to check if interface '%s' exists: %s (%d)",
			  __func__, iface_name, strerror(errno), errno);
		return false;
	}

	memset(&wreq, 0, sizeof(wreq));
	exists = wifi_ioctl(sock, iface_name, SIOCGIWNAME, &wreq) >= 0;

	close(sock);

	return exists;
}

int ldx_wifi_list_available_ifaces(net_names_list_t *iface_list)
{
	net_names_list_t net_list;
	int i;
	int n_ifaces = ldx_net_list_available_ifaces(&net_list);

	memset(iface_list, 0, sizeof(*iface_list));

	if (n_ifaces <= 0)
		return n_ifaces;

	for (i = 0; i < n_ifaces; i++) {
		if (ldx_wifi_iface_exists(net_list.names[i])) {
			strncpy(iface_list->names[iface_list->n_ifaces], net_list.names[i], IFNAMSIZ - 1);
			iface_list->n_ifaces++;
		}
	}

	return iface_list->n_ifaces;
}

wifi_state_error_t ldx_wifi_get_iface_state(const char *iface_name, wifi_state_t *wifi_state)
{
	int sock = -1;
	wifi_state_error_t ret, err;

	memset(wifi_state, 0, sizeof(*wifi_state));
	wifi_state->freq = -1;
	wifi_state->channel = -1;
	wifi_state->sec_mode = WIFI_SEC_MODE_ERROR;

	if (!ldx_wifi_iface_exists(iface_name)) {
		ret = WIFI_STATE_ERROR_NO_EXIST;
		log_debug("%s: Unable to get state of '%s': %s", __func__,
			  iface_name, ldx_wifi_code_to_str(ret));
		return ret;
	}

	ret = ldx_net_get_iface_state(iface_name, &(wifi_state->net_state));
	if (ret != WIFI_STATE_ERROR_NONE)
		return ret;

	if (wifi_state->net_state.status != NET_STATUS_CONNECTED)
		wifi_state->sec_mode = WIFI_SEC_MODE_ERROR;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (sock == -1) {
		ret = WIFI_STATE_ERROR_STATE;
		log_debug("%s: %s of '%s': %s (%d)", __func__,
			  ldx_wifi_code_to_str(ret), iface_name, strerror(errno), errno);
		goto done;
	}

	ret = get_ssid(iface_name, sock, wifi_state->ssid);

	err = get_freq(iface_name, sock, &wifi_state->freq);
	if (err == WIFI_STATE_ERROR_NONE)
		err = get_channel(iface_name, sock, wifi_state->freq, &wifi_state->channel);

	if (ret == WIFI_STATE_ERROR_NONE)
		ret = err;

	err = get_sec_mode(iface_name, &wifi_state->sec_mode);
	if (ret == WIFI_STATE_ERROR_NONE)
		ret = err;
done:
	if (sock > -1)
		close(sock);

	return ret;
}

wifi_state_error_t ldx_wifi_set_config(wifi_config_t wifi_cfg)
{
	char *iface_name = wifi_cfg.name;
	wifi_state_error_t ret = WIFI_STATE_ERROR_CONFIG;
	wifi_state_t wifi_state;
	char *extra = NULL, *cmd = NULL, *resp = NULL, *tmp = NULL;
	int rc, len;

	if (!ldx_wifi_iface_exists(iface_name)) {
		ret = WIFI_STATE_ERROR_NO_EXIST;
		log_debug("%s: Unable to set config of '%s': %s",
			  __func__, iface_name, ldx_wifi_code_to_str(ret));
		return ret;
	}

	strncpy(wifi_cfg.net_config.name, iface_name, sizeof(wifi_cfg.net_config.name));

	ret = _net_check_cfg(wifi_cfg.net_config, &wifi_state.net_state);
	if (ret != WIFI_STATE_ERROR_NONE) {
		log_debug("%s: Unable to set config of '%s': %s",
			  __func__, iface_name, ldx_net_code_to_str(ret));
		return ret;
	}

	/* Security */
	if (wifi_cfg.sec_mode == WIFI_SEC_MODE_OPEN) {
		/* If current config has security enabled, we need to remove
		 * the connection and create a new one.
		 */
		len = snprintf(NULL, 0, CMD_DEL_CONN, iface_name);
		cmd = calloc(len + 1, sizeof(*cmd));
		if (cmd == NULL) {
			ret = WIFI_STATE_ERROR_NO_MEM;
			log_debug("%s: Unable to set config of '%s': %s",
				  __func__, iface_name, ldx_wifi_code_to_str(ret));
			return ret;
		}
		sprintf(cmd, CMD_DEL_CONN, iface_name);

		rc = ldx_process_execute_cmd(cmd, &resp, 10);
		if (rc == 127) /* Command not found */
			log_debug("%s: 'nmcli' not found", __func__);

		free(cmd);
		cmd = NULL;
	} else if (wifi_cfg.sec_mode > WIFI_SEC_MODE_OPEN) {
		char *mode = "none";

		if (wifi_cfg.sec_mode >= WIFI_SEC_MODE_WPA
			&& wifi_cfg.sec_mode <= WIFI_SEC_MODE_WPA3)
			mode = "wpa-psk";

		len = snprintf(NULL, 0, CMD_CONN_KEY_MGMT CMD_CONN_AUTH_ALG, mode, "open");
		extra = calloc(len + 1, sizeof(*extra));
		if (extra == NULL) {
			ret = WIFI_STATE_ERROR_NO_MEM;
			log_debug("%s: Unable to set config of '%s': %s",
				  __func__, iface_name, ldx_wifi_code_to_str(ret));
			goto done;
		}
		sprintf(extra, CMD_CONN_KEY_MGMT CMD_CONN_AUTH_ALG, mode, "open");
	}

	/* SSID */
	if (wifi_cfg.set_ssid) {
		int extra_len = extra != NULL ? strlen(extra) : 0;

		len = snprintf(NULL, 0, CMD_CONN_SSID, wifi_cfg.ssid);
		tmp = realloc(extra, (extra_len + len + 1) * sizeof(*extra));
		if (tmp == NULL) {
			ret = WIFI_STATE_ERROR_NO_MEM;
			log_debug("%s: Unable to set config of '%s': %s",
				  __func__, iface_name, ldx_wifi_code_to_str(ret));
			goto done;
		}
		extra = tmp;

		sprintf(extra + extra_len, CMD_CONN_SSID, wifi_cfg.ssid);
	}

	/* PSK */
	if (wifi_cfg.psk != NULL
		&& (wifi_cfg.sec_mode == WIFI_SEC_MODE_ERROR		/* security not provided */
			|| (wifi_cfg.sec_mode >= WIFI_SEC_MODE_WPA	/* or security is wpa */
				&& wifi_cfg.sec_mode <= WIFI_SEC_MODE_WPA3))) {
		int extra_len = extra != NULL ? strlen(extra) : 0;

		len = snprintf(NULL, 0, CMD_CONN_PSK CMD_CONN_PSK_FLAGS, wifi_cfg.psk, 0);
		tmp = realloc(extra, (extra_len + len + 1) * sizeof(*extra));
		if (tmp == NULL) {
			ret = WIFI_STATE_ERROR_NO_MEM;
			log_debug("%s: Unable to set config of '%s': %s",
				  __func__, iface_name, ldx_wifi_code_to_str(ret));
			goto done;
		}
		extra = tmp;

		sprintf(extra + extra_len, CMD_CONN_PSK CMD_CONN_PSK_FLAGS, wifi_cfg.psk, 0);
	}

	ret = _net_get_cfg_cmd(wifi_cfg.net_config, wifi_state.net_state, true, extra, &cmd);
	if (ret != WIFI_STATE_ERROR_NONE)
		goto done;

	log_debug("nmcli cmd: %s\n", cmd);
	rc = ldx_process_execute_cmd(cmd, &resp, 30);
	if (rc != 0
		|| (resp != NULL && strncmp(CMD_ERROR_PREFIX, resp, strlen(CMD_ERROR_PREFIX)) == 0)) {
		if (rc == 127) { /* Command not found */
			log_debug("%s: 'nmcli' not found", __func__);
		} else if (resp != NULL) {
			log_debug("%s: Unable to set config for '%s': %s",
				  __func__, iface_name, resp);
		} else {
			log_debug("%s: Unable to set config for '%s'",
				  __func__, iface_name);
		}
		ret = WIFI_STATE_ERROR_CONFIG;
		goto done;
	}

	ret = WIFI_STATE_ERROR_NONE;

done:
	free(extra);
	free(cmd);
	free(resp);

	return ret;
}

int ldx_wifi_list_available_freqs(const char *iface_name, double **freqs)
{
	struct iw_range range;
	int sock = -1, n_freqs = -1, i = 0;

	*freqs = NULL;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (sock == -1) {
		log_error("%s: Unable to get available frequencies of '%s': %s (%d)",
			  __func__, iface_name, strerror(errno), errno);
		goto done;
	}

	if (get_range_info(iface_name, sock, &range) != 0)
		goto done;

	n_freqs = range.num_frequency;
	if (n_freqs <= 0)
		goto done;

	*freqs = calloc(n_freqs, sizeof(**freqs));
	if (*freqs == NULL) {
		n_freqs = -1;
		log_error("%s: Unable to get available frequencies of '%s': Out of memory",
			  __func__, iface_name);
		goto done;
	}

	for(i = 0; i < n_freqs; i++)
		(*freqs)[i] = freq2float(range.freq[i]);

done:
	if (sock > -1)
		close(sock);

	return n_freqs;
}

int ldx_wifi_list_available_channels(const char *iface_name, int **channels)
{
	struct iw_range range;
	int sock = -1, n_channels = -1, i = 0;

	*channels = NULL;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (sock == -1) {
		log_error("%s: Unable to get available channels of '%s': %s (%d)",
			  __func__, iface_name, strerror(errno), errno);
		goto done;
	}

	if (get_range_info(iface_name, sock, &range) != 0)
		goto done;

	n_channels = range.num_channels;
	if (n_channels <= 0)
		goto done;

	*channels = calloc(n_channels, sizeof(**channels));
	if (*channels == NULL) {
		n_channels = -1;
		log_error("%s: Unable to get available channels of '%s': Out of memory",
			  __func__, iface_name);
		goto done;
	}

	for(i = 0; i < n_channels; i++)
		(*channels)[i] = range.freq[i].i;

done:
	if (sock > -1)
		close(sock);

	return n_channels;
}

const char *ldx_wifi_sec_mode_to_str(wifi_sec_mode_t mode)
{
	if (mode < 0 || mode >= ARRAY_SIZE(wifi_sec_mode_names))
		return UNKOWN_SECURITY_MODE;

	return wifi_sec_mode_names[mode];
}
