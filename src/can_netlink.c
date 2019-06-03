/*
 * Copyright 2018, Digi International Inc.
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <errno.h>
#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/error.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "can.h"
#include "_can.h"
#include "_log.h"

int ldx_can_get_state(const can_if_t *cif, enum can_state *state)
{
	int ret;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	ret = can_get_state(cif->name, (int *)state);
	if (ret) {
		log_error("%s: Unable to get %s interface state",
			  __func__, cif->name);
		return -CAN_ERROR_NL_GET_STATE;
	}

	return CAN_ERROR_NONE;
}

int ldx_can_get_dev_stats(const can_if_t *cif, struct can_device_stats *cds)
{
	int ret;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	ret = can_get_device_stats(cif->name, cds);
	if (ret) {
		log_error("%s: Unable to get %s device stats",
			  __func__, cif->name);
		return -CAN_ERROR_NL_GET_DEV_STATS;
	}

	return CAN_ERROR_NONE;
}

int ldx_can_get_bit_error_counter(const can_if_t *cif, struct can_berr_counter *bc)
{
	int ret;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	ret = can_get_berr_counter(cif->name, bc);
	if (ret) {
		log_error("%s: Unable to get %s bit error counter",
			  __func__, cif->name);
		return -CAN_ERROR_NL_GET_BIT_ERR_CNT;
	}

	return CAN_ERROR_NONE;
}

int ldx_can_start(const can_if_t *cif)
{
	int ret, state;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	ret = can_do_start(cif->name);
	if (ret) {
		log_error("%s: Unable to start %s interface",
			  __func__, cif->name);
		return -CAN_ERROR_NL_START;
	}

	if (cif->cfg.nl_cmd_verify) {
		ret = can_get_state(cif->name, &state);
		if (ret) {
			log_error("%s: Unable to get %s interface state",
				  __func__, cif->name);
			return -CAN_ERROR_NL_GET_STATE;
		}

		if (state != CAN_STATE_ERROR_ACTIVE) {
			log_error("%s: Unexpected state %d, in %s interface",
				  __func__, state, cif->name);
			return -CAN_ERROR_NL_STATE_MISSMATCH;
		}
	}

	return CAN_ERROR_NONE;
}

int ldx_can_stop(const can_if_t *cif)
{
	int ret, state;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	ret = can_do_stop(cif->name);
	if (ret) {
		log_error("%s: Unable to stop %s interface",
			  __func__, cif->name);
		return -CAN_ERROR_NL_STOP;
	}

	if (cif->cfg.nl_cmd_verify) {
		ret = can_get_state(cif->name, &state);
		if (ret) {
			log_error("%s: Unable to get %s interface state",
				  __func__, cif->name);
			return -CAN_ERROR_NL_GET_STATE;
		}

		if (state != CAN_STATE_STOPPED) {
			log_error("%s: Unexpected state %d, in %s interface",
				  __func__, state, cif->name);
			return -CAN_ERROR_NL_STATE_MISSMATCH;
		}
	}

	return CAN_ERROR_NONE;
}

int ldx_can_restart(const can_if_t *cif)
{
	int ret, state;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	ret = can_do_restart(cif->name);
	if (ret) {
		log_error("%s: Unable to restart %s interface",
			  __func__, cif->name);
		return -CAN_ERROR_NL_RESTART;
	}

	if (cif->cfg.nl_cmd_verify) {
		ret = can_get_state(cif->name, &state);
		if (ret) {
			log_error("%s: Unable to get %s interface state",
				  __func__, cif->name);
			return -CAN_ERROR_NL_GET_STATE;
		}

		if (state != CAN_STATE_ERROR_ACTIVE) {
			log_error("%s: Unexpected state %d, in %s interface",
				  __func__, state, cif->name);
			return -CAN_ERROR_NL_STATE_MISSMATCH;
		}
	}

	return CAN_ERROR_NONE;
}

int ldx_can_set_bitrate(can_if_t *cif, uint32_t bitrate)
{
	int ret;
	struct can_bittiming bt;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	ret = can_set_bitrate(cif->name, bitrate);
	if (ret) {
		log_error("%s: Unable to set bitrate to %u",
			  __func__, bitrate);
		return -CAN_ERROR_NL_BITRATE;
	}

	if (cif->cfg.nl_cmd_verify) {
		ret = can_get_bittiming(cif->name, &bt);
		if (ret) {
			log_error("%s: Unable to get %s bit timing info",
				  __func__, cif->name);
			return -CAN_ERROR_NL_GET_BIT_TIMING;
		}

		if (bt.bitrate != bitrate) {
			log_error("%s: on %s bitrate set does not match bitrate read",
				  __func__, cif->name);
			return -CAN_ERROR_NL_BR_MISSMATCH;
		}
	}

	return CAN_ERROR_NONE;
}

int ldx_can_set_data_bitrate(can_if_t *cif, uint32_t dbitrate)
{
	int ret;
	struct can_bittiming dbt;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	ret = can_set_data_bitrate(cif->name, dbitrate);
	if (ret) {
		log_error("%s: Unable to set data bitrate to %u",
			  __func__, dbitrate);
		return -CAN_ERROR_NL_BITRATE;
	}

	if (cif->cfg.nl_cmd_verify) {
		ret = can_get_data_bittiming(cif->name, &dbt);
		if (ret) {
			log_error("%s: Unable to get %s bit timing info",
				  __func__, cif->name);
			return -CAN_ERROR_NL_GET_BIT_TIMING;
		}

		if (dbt.bitrate != dbitrate) {
			log_error("%s: on %s data bitrate set does not match data bitrate read",
				  __func__, cif->name);
			return -CAN_ERROR_NL_BR_MISSMATCH;
		}
	}

	return CAN_ERROR_NONE;
}

int ldx_can_set_restart_ms(can_if_t *cif, uint32_t restart_ms)
{
	int ret;
	uint32_t read_restart_ms;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	ret = can_set_restart_ms(cif->name, restart_ms);
	if (ret) {
		log_error("%s: Unable to set restart ms to %u",
			  __func__, restart_ms);
		return -CAN_ERROR_NL_SET_RESTART_MS;
	}

	if (cif->cfg.nl_cmd_verify) {
		ret = can_get_restart_ms(cif->name, &read_restart_ms);
		if (ret) {
			log_error("%s: Unable read restart ms value",
				  __func__);
			return -CAN_ERROR_NL_GET_RESTART_MS;
		}

		if (restart_ms != read_restart_ms)
			return -CAN_ERROR_NL_RSTMS_MISSMATCH;
	}

	return CAN_ERROR_NONE;
}

int ldx_can_set_bit_timing(can_if_t *cif, struct can_bittiming *bt)
{
	int ret;
	struct can_bittiming bt_read;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	ret = can_set_bittiming(cif->name, bt);
	if (ret) {
		log_error("%s: Unable to set bit timing on %s",
			  __func__, cif->name);
		return -CAN_ERROR_NL_SET_BIT_TIMING;
	}

	if (cif->cfg.nl_cmd_verify) {
		ret = can_get_bittiming(cif->name, &bt_read);
		if (ret) {
			log_error("%s: Unable to get %s bit timing info",
				  __func__, cif->name);
			return -CAN_ERROR_NL_GET_BIT_TIMING;
		}

		ret = memcmp(bt, &bt_read, sizeof(bt_read));
		if (ret) {
			log_error("%s: on %s bit timing set does not match bit timing read",
				  __func__, cif->name);
			return -CAN_ERROR_NL_BT_MISSMATCH;
		}
	}

	return CAN_ERROR_NONE;
}

int ldx_can_set_data_bit_timing(can_if_t *cif, struct can_bittiming *dbt)
{
	int ret;
	struct can_bittiming dbt_read;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	ret = can_set_data_bittiming(cif->name, dbt);
	if (ret) {
		log_error("%s: Unable to set data bit timing on %s",
			  __func__, cif->name);
		return -CAN_ERROR_NL_SET_BIT_TIMING;
	}

	if (cif->cfg.nl_cmd_verify) {
		ret = can_get_data_bittiming(cif->name, &dbt_read);
		if (ret) {
			log_error("%s: Unable to get %s data bit timing info",
				  __func__, cif->name);
			return -CAN_ERROR_NL_GET_BIT_TIMING;
		}

		ret = memcmp(dbt, &dbt_read, sizeof(dbt_read));
		if (ret) {
			log_error("%s: on %s data bit timing set does not match bit timing read",
				  __func__, cif->name);
			return -CAN_ERROR_NL_BT_MISSMATCH;
		}
	}

	return CAN_ERROR_NONE;
}

int ldx_can_set_ctrlmode(can_if_t *cif, struct can_ctrlmode *cm)
{
	int ret;
	struct can_ctrlmode cm_read;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	ret = can_set_ctrlmode(cif->name, cm);
	if (ret) {
		log_error("%s: Unable to set control mode on %s",
			  __func__, cif->name);
		return -CAN_ERROR_NL_SET_CTRL_MODE;
	}

	if (cif->cfg.nl_cmd_verify) {
		ret = can_get_ctrlmode(cif->name, &cm_read);
		if (ret) {
			log_error("%s: Unable to get %s ctrlmode info",
					__func__, cif->name);
			return -CAN_ERROR_NL_GET_CTRL_MODE;
		}

		ret = memcmp(cm, &cm_read, sizeof(cm_read));
		if (ret) {
			log_error("%s: on %s control mode set does not match with control mode read",
					__func__, cif->name);
			return -CAN_ERROR_NL_CTRL_MISSMATCH;
		}
	}

	return CAN_ERROR_NONE;
}
