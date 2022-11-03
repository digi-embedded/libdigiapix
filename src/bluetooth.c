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

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "_log.h"
#include "bluetooth.h"

#define UNKOWN_CODE	"Unknown Bluetooth state error"

static const char* bt_state_error_descs[] = {
	"No error",
	"Bluetooth device not found",
	"Out of memory",
	"Unable to get Bluetooth device info",
	"Unable to open/close Bluetooth device",
	"Unable to get/set Bluetooth name",
	"Unable to configure Bluetooth device"
};

/*
 * get_hci_dev_info() - Retrieve the HCI Bluetooth device
 *
 * @dev_id:		Bluetooth device identifier.
 * @hci_dev_info:	The HCI Bluetooth device info.
 *
 * Return: BT_STATE_ERROR_NONE on success, any other error code otherwise.
 */
static bt_state_error_t get_hci_dev_info(uint16_t dev_id, struct hci_dev_info *dev_info)
{
	if (hci_devinfo(dev_id, dev_info) < 0) {
		bt_state_error_t ret = BT_STATE_ERROR_HCI_INFO;
		log_debug("%s: %s of '%d': %s (%d)", __func__,
			  ldx_bt_code_to_str(ret), dev_id, strerror(errno), errno);
		return ret;
	}

	return BT_STATE_ERROR_NONE;
}

/*
 * get_name() - Retrieve the Bluetooth interface name
 *
 * @dev_id:	Bluetooth device identifier.
 * @name:	String to store the name value.
 *
 * Return: BT_STATE_ERROR_NONE on success, any other error code otherwise.
 */
static bt_state_error_t get_name(uint16_t dev_id, char *name)
{
	bt_state_error_t ret = BT_STATE_ERROR_LOCAL_NAME;
	int sock = hci_open_dev(dev_id);

	if (sock < 0) {
		log_debug("%s: %s of '%d': %s (%d)", __func__,
			  ldx_bt_code_to_str(ret), dev_id, strerror(errno), errno);
		return ret;
	}

	if (hci_read_local_name(sock, BT_NAME_MAX_LEN + 1, name, 1000) < 0) {  /* timeout: 1s */
		log_debug("%s: %s of '%d': %s (%d)", __func__,
			  ldx_bt_code_to_str(ret), dev_id, strerror(errno), errno);
		goto done;
	}

	name[BT_NAME_MAX_LEN] = '\0';
	ret = BT_STATE_ERROR_NONE;

done:
	hci_close_dev(sock);

	return ret;
}

const char *ldx_bt_code_to_str(bt_state_error_t code)
{
	if (code < 0 || code >= __BT_STATE_ERROR_LAST)
		return UNKOWN_CODE;

	return bt_state_error_descs[code];
}

bool ldx_bt_device_exists(uint16_t dev_id)
{
	bool exists = false;
	uint16_t *bt_devs = NULL;
	int i = 0;
	int n_bt_devs = ldx_bt_list_available_devices(&bt_devs);

	if (n_bt_devs <= 0)
		return false;

	for (i = 0; i < n_bt_devs; i++) {
		if (bt_devs[i] == dev_id) {
			exists = true;
			break;
		}
	}

	free(bt_devs);

	return exists;
}

int ldx_bt_list_available_devices(uint16_t **ids)
{
	struct hci_dev_list_req *dev_list = NULL;
	struct hci_dev_req *dev_req = NULL;
	int i, sock, n_dev = -1;

	sock = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);
	if (sock < 0) {
		log_error("%s: Unable to get Bluetooth interfaces: %s (%d)",
			  __func__, strerror(errno), errno);
		return -1;
	}

	dev_list = malloc(HCI_MAX_DEV * sizeof(*dev_req) + sizeof(*dev_list));
	if (dev_list == NULL) {
		log_error("%s: Unable to get Bluetooth interfaces: Out of memory",
			  __func__);
		goto done;
	}

	memset(dev_list, 0, HCI_MAX_DEV * sizeof(*dev_req) + sizeof(*dev_list));

	dev_list->dev_num = HCI_MAX_DEV;
	dev_req = dev_list->dev_req;
	if (ioctl(sock, HCIGETDEVLIST, dev_list) < 0) {
		log_error("%s: Unable to get Bluetooth interfaces: %s (%d)",
			  __func__, strerror(errno), errno);
		goto free;
	}

	n_dev = dev_list->dev_num;
	if (n_dev == 0)
		goto free;

	*ids = calloc(n_dev, sizeof(**ids));
	if (*ids == NULL) {
		log_error("%s: Unable to get Bluetooth interfaces: Out of memory",
			  __func__);
		n_dev = -1;
		goto free;
	}

	for (i = 0; i < dev_list->dev_num; i++, dev_req++)
		(*ids)[i] = dev_req->dev_id;

free:
	free(dev_list);
done:
	close(sock);

	return n_dev;
}

bt_state_error_t ldx_bt_get_state(uint16_t dev_id, bt_state_t *bt_state)
{
	struct hci_dev_info dev_info;
	bt_state_error_t ret = BT_STATE_ERROR_NO_EXIST;
	int i = 0;

	memset(bt_state, 0, sizeof(*bt_state));
	bt_state->enable = BT_ENABLED_ERROR;

	if (!ldx_bt_device_exists(dev_id)) {
		log_debug("%s: Unable to get state for '%d': %s", __func__,
			  dev_id, ldx_bt_code_to_str(ret));
		return ret;
	}

	bt_state->dev_id = dev_id;

	ret = get_hci_dev_info(dev_id, &dev_info);
	if (ret != BT_STATE_ERROR_NONE)
		return ret;

	strncpy(bt_state->dev_name, dev_info.name, IFNAMSIZ - 1);

	for (i = 0; i < MAC_ADDRESS_GROUPS; i++)
		bt_state->mac[i] = dev_info.bdaddr.b[MAC_ADDRESS_GROUPS - i - 1];

	bt_state->enable = hci_test_bit(HCI_UP, &dev_info.flags);
	bt_state->running = hci_test_bit(HCI_RUNNING, &dev_info.flags);

	if (bt_state->enable == BT_ENABLED)
		ret = get_name(dev_id, bt_state->name);

	return ret;
}

bt_state_error_t ldx_bt_get_stats(uint16_t dev_id, bt_stats_t *bt_stats)
{
	struct hci_dev_info dev_info;
	bt_state_error_t ret = BT_STATE_ERROR_NO_EXIST;

	memset(bt_stats, 0, sizeof(*bt_stats));

	if (!ldx_bt_device_exists(dev_id)) {
		log_debug("%s: Unable to get stats for '%d': %s", __func__,
			  dev_id, ldx_bt_code_to_str(ret));
		return ret;
	}

	ret = get_hci_dev_info(dev_id, &dev_info);
	if (ret != BT_STATE_ERROR_NONE)
		return ret;

	bt_stats->rx_bytes = dev_info.stat.byte_rx;
	bt_stats->rx_errors = dev_info.stat.err_rx;
	bt_stats->rx_acl = dev_info.stat.acl_rx;
	bt_stats->rx_sco = dev_info.stat.sco_rx;
	bt_stats->rx_events = dev_info.stat.evt_rx;
	bt_stats->tx_bytes = dev_info.stat.byte_tx;
	bt_stats->tx_errors = dev_info.stat.err_tx;
	bt_stats->tx_acl = dev_info.stat.acl_tx;
	bt_stats->tx_sco = dev_info.stat.sco_tx;
	bt_stats->tx_cmds = dev_info.stat.cmd_tx;

	return BT_STATE_ERROR_NONE;
}

bt_state_error_t ldx_bt_set_config(bt_config_t bt_cfg)
{
	bt_state_error_t ret = BT_STATE_ERROR_NO_EXIST;
	uint16_t dev_id = bt_cfg.dev_id;
	int sock = -1;

	if (!ldx_bt_device_exists(dev_id)) {
		log_debug("%s: Unable to set Bluetooth config for '%d': %s",
			  __func__, dev_id, ldx_bt_code_to_str(ret));
		return ret;
	}

	sock = hci_open_dev(dev_id);
	if (sock < 0) {
		ret = BT_STATE_ERROR_CONFIG;
		log_debug("%s: %s '%d': %s (%d)", __func__,
			  ldx_bt_code_to_str(ret), dev_id, strerror(errno), errno);
		return ret;
	}

	if (bt_cfg.enable == BT_ENABLED && ioctl(sock, HCIDEVUP, dev_id) < 0) {
		if (errno != EALREADY) {
			ret = BT_STATE_ERROR_ENABLE;
			log_debug("%s: %s '%d': %s (%d)", __func__,
				  ldx_bt_code_to_str(ret), dev_id, strerror(errno), errno);
			goto done;
		}
	}

	if (bt_cfg.set_name) {
		if (hci_write_local_name(sock, bt_cfg.name, 2000) < 0) {  /* timeout: 2s */
			ret = BT_STATE_ERROR_LOCAL_NAME;
			log_debug("%s: %s of '%d' to '%s': %s (%d)", __func__,
				  ldx_bt_code_to_str(ret), dev_id, bt_cfg.name,
				  strerror(errno), errno);
			goto done;
		}
	}

	if (bt_cfg.enable == BT_DISABLED && ioctl(sock, HCIDEVDOWN, dev_id) < 0) {
		if (errno != EALREADY) {
			ret = BT_STATE_ERROR_ENABLE;
			log_debug("%s: %s '%d': %s (%d)", __func__,
				  ldx_bt_code_to_str(ret), dev_id, strerror(errno), errno);
			goto done;
		}
	}

	ret = BT_STATE_ERROR_NONE;
done:
	hci_close_dev(sock);

	return ret;
}
