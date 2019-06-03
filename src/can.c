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
#include <linux/can/error.h>
#include <linux/net_tstamp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "can.h"
#include "_can.h"
#include "_log.h"

/* map the sanitized data length to an appropriate data length code */
#define CAN_LEN2DLC(len)		len > 64 ? 0xF : len2dlc[len]

/* CAN DLC to real data length conversion helpers */
static const unsigned char dlc2len[] = {0, 1, 2, 3, 4, 5, 6, 7,
					8, 12, 16, 20, 24, 32, 48, 64};

/* get data length from can_dlc with sanitized can_dlc */
static unsigned char can_dlc2len(unsigned char can_dlc)
{
	return dlc2len[can_dlc & 0x0F];
}

static const unsigned char len2dlc[] = {0, 1, 2, 3, 4, 5, 6, 7, 8,		/* 0 - 8 */
					9, 9, 9, 9,				/* 9 - 12 */
					10, 10, 10, 10,				/* 13 - 16 */
					11, 11, 11, 11,				/* 17 - 20 */
					12, 12, 12, 12,				/* 21 - 24 */
					13, 13, 13, 13, 13, 13, 13, 13,		/* 25 - 32 */
					14, 14, 14, 14, 14, 14, 14, 14,		/* 33 - 40 */
					14, 14, 14, 14, 14, 14, 14, 14,		/* 41 - 48 */
					15, 15, 15, 15, 15, 15, 15, 15,		/* 49 - 56 */
					15, 15, 15, 15, 15, 15, 15, 15};	/* 57 - 64 */



const char *const __can_error_str[CAN_ERROR_MAX + 1] = {
	[CAN_ERROR_NONE]			= "Success",
	[CAN_ERROR_NULL_INTERFACE]	= "CAN interface is NULL",
	[CAN_ERROR_IFR_IDX]		= "Interface index error",
	[CAN_ERROR_NO_MEM]		= "No memory",
	[CAN_ERROR_NL_GET_STATE]		= "Get netlink interface state",
	[CAN_ERROR_NL_START]		= "Start interface",
	[CAN_ERROR_NL_STOP]		= "Stop interface",
	[CAN_ERROR_NL_STATE_MISSMATCH]	= "Netlink state set does not match value read",
	[CAN_ERROR_NL_BITRATE]		= "Set interface bitrate",
	[CAN_ERROR_NL_RESTART]		= "Restart interface error",
	[CAN_ERROR_NL_SET_RESTART_MS]	= "Set restart ms error",
	[CAN_ERROR_NL_GET_RESTART_MS]	= "Get restart ms error",
	[CAN_ERROR_NL_RSTMS_MISSMATCH]	= "Restart ms value set does not match value read",
	[CAN_ERROR_NL_SET_CTRL_MODE]	= "Set ctrl mode error",
	[CAN_ERROR_NL_GET_CTRL_MODE]	= "Get ctrl mode error",
	[CAN_ERROR_NL_CTRL_MISSMATCH]	= "Get ctrl mode value set does not match value read",
	[CAN_ERROR_NL_GET_DEV_STATS]	= "Get device statistics error",
	[CAN_ERROR_NL_SET_BIT_TIMING]	= "Set bit timing error",
	[CAN_ERROR_NL_GET_BIT_TIMING]	= "Get bit timing error",
	[CAN_ERROR_NL_BT_MISSMATCH]	= "Bit timing value set does not match value read",
	[CAN_ERROR_NL_GET_BIT_ERR_CNT]	= "Get bit error counter error",
	[CAN_ERROR_NL_BR_MISSMATCH]	= "Bitrate value set does not match value read",

	[CAN_ERROR_TX_SKT_CREATE]		= "Socket create error",
	[CAN_ERROR_TX_SKT_WR]		= "Socket write error",
	[CAN_ERROR_TX_SKT_BIND]		= "Socket bind error",
	[CAN_ERROR_TX_RETRY_LATER]	= "TX retry later",
	[CAN_ERROR_INCOMP_FRAME]		= "Incomplete TX frame",

	[CAN_ERROR_SETSKTOPT_RAW_FLT]	= "setsocketopt CAN_RAW_FILTER error",
	[CAN_ERROR_SETSKTOPT_ERR_FLT]	= "setsocketopt CAN_RAW_ERR_FILTER error",
	[CAN_ERROR_SETSKTOPT_CANFD]	= "setsocketopt CAN_RAW_FD_FRAMES error",
	[CAN_ERROR_SETSKTOPT_TIMESTAMP]	= "setsocketopt SO_TIMESTAMP error",
	[CAN_ERROR_SETSKTOPT_SNDBUF]	= "setsocketopt SO_SNDBUF error",
	[CAN_ERROR_GETSKTOPT_SNDBUF]	= "getsocketopt SO_SNDBUF error",
	[CAN_ERROR_SETSKTOPT_RCVBUF]	= "setsocketopt SO_RCVBUF error",
	[CAN_ERROR_GETSKTOPT_RCVBUF]	= "getsocketopt SO_RCVBUF error",

	[CAN_ERROR_DROPPED_FRAMES]		= "Dropped frames",
};

static void ldx_can_default_error_handler(int error, void *data)
{
	/* Default error handler, used to log information */
	log_error("%s: error: %d, %s", __func__, error, ldx_can_strerror(error));
}

const char * ldx_can_strerror(int error)
{
	const char *err_string = NULL;

	if (error > 0 && error < CAN_ERROR_MAX)
		err_string = __can_error_str[error];

	return err_string;
}

void ldx_can_set_defconfig(can_if_cfg_t *cfg)
{
	cfg->nl_cmd_verify	= true;
	cfg->canfd_enabled	= false;
	cfg->process_header	= true;
	cfg->hw_timestamp	= false;
	cfg->bitrate		= LDX_CAN_INVALID_BITRATE;
	cfg->dbitrate		= LDX_CAN_INVALID_BITRATE;
	cfg->restart_ms		= LDX_CAN_INVALID_RESTART_MS;
	cfg->ctrl_mode.mask	= LDX_CAN_UNCONFIGURED_MASK;
	cfg->error_mask		= CAN_ERR_TX_TIMEOUT |
				  CAN_ERR_CRTL |
				  CAN_ERR_BUSOFF |
				  CAN_ERR_BUSERROR |
				  CAN_ERR_RESTARTED;
}

static void process_can_process_msgheader(struct msghdr *msg, struct timeval *tv, uint32_t *df)
{
	struct cmsghdr *cmsg;
	struct timespec *stamp;

	for (cmsg = CMSG_FIRSTHDR(msg);
		 cmsg && (cmsg->cmsg_level == SOL_SOCKET);
		 cmsg = CMSG_NXTHDR(msg, cmsg)) {

		switch (cmsg->cmsg_type) {
		case SO_RXQ_OVFL:
			memcpy(df, CMSG_DATA(cmsg), sizeof(uint32_t));
			break;

		case SO_TIMESTAMP:
			memcpy(tv, CMSG_DATA(cmsg), sizeof(*tv));
			break;

		case SO_TIMESTAMPING:
			/*
			 * stamp[0] is the software timestamp
			 * stamp[1] is deprecated
			 * stamp[2] is the raw hardware timestamp
			 * See chapter 2.1.2 Receive timestamps in
			 * linux/Documentation/networking/timestamping.txt
			 */
			stamp = (struct timespec *)CMSG_DATA(cmsg);
			tv->tv_sec = stamp[2].tv_sec;
			tv->tv_usec = stamp[2].tv_nsec / 1000;
			break;

		default:
			break;
		}
	}
}

static void ldx_can_call_err_cb(const can_if_t *cif, int error, void *data)
{
	can_priv_t *pdata = cif->_data;
	can_err_cb_t *err_cb;

	list_for_each_entry(err_cb, &pdata->err_cb_list_head, list) {
		if (err_cb->handler)
			err_cb->handler(error, data);
	}
}

static int ldx_can_process_tx_socket(const can_if_t *cif)
{
	can_priv_t *pdata = cif->_data;
	struct msghdr msg;
	struct canfd_frame frame;
	struct iovec iov;
	char ctrlmsg[CMSG_SPACE(sizeof(struct timeval) + 3 *sizeof(struct timespec) + sizeof(__u32))];
	int ret = EXIT_SUCCESS, nbytes;
	bool done = false;

	/* Lets do some initializations out of the main loop... */
	iov.iov_base = &frame;
	iov.iov_len = sizeof(frame);

	msg.msg_name = &pdata->addr;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = &ctrlmsg;
	msg.msg_namelen = sizeof(struct sockaddr_can);
	msg.msg_controllen = sizeof(ctrlmsg);
	msg.msg_flags = 0;

	while (!done) {
		nbytes = recvmsg(pdata->tx_skt, &msg, 0);
		if (nbytes < 0) {
			if (errno == ENETDOWN) {
				log_error("%s: CAN network is down", __func__);
				ret = -CAN_ERROR_NETWORK_DOWN;
			}
			return ret;
		}

		if (nbytes == 0) {
			done = true;
			continue;
		}

		if (frame.can_id & CAN_ERR_FLAG) {
			can_err_cb_t *err_cb;

			list_for_each_entry(err_cb, &pdata->err_cb_list_head, list) {
				if (err_cb->handler)
					err_cb->handler(frame.can_id, NULL);
			}
		}
	}

	return ret;
}

static int ldx_can_process_rx_socket(can_if_t *cif, can_cb_t *rx_cb)
{
	can_priv_t *pdata = cif->_data;
	struct timeval tstamp;
	struct msghdr msg;
	struct canfd_frame frame;
	struct iovec iov;
	char ctrlmsg[CMSG_SPACE(sizeof(struct timeval) + 3 *sizeof(struct timespec) + sizeof(__u32))];
	int ret = EXIT_SUCCESS, nbytes;
	bool done = false;

	/* Lets do some initializations out of the main loop... */
	iov.iov_base = &frame;
	iov.iov_len = sizeof(frame);

	msg.msg_name = &pdata->addr;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = &ctrlmsg;
	msg.msg_namelen = sizeof(struct sockaddr_can);
	msg.msg_controllen = sizeof(ctrlmsg);
	msg.msg_flags = 0;

	while (!done) {
		nbytes = recvmsg(rx_cb->rx_skt, &msg, 0);
		if (nbytes < 0) {
			if (errno == ENETDOWN) {
				log_error("%s: CAN network is down", __func__);
				ret = -CAN_ERROR_NETWORK_DOWN;
			}
			return ret;
		}

		if (nbytes == 0) {
			done = true;
			continue;
		}

		if(cif->cfg.process_header) {
			uint32_t dropf = 0;

			process_can_process_msgheader(&msg, &tstamp, &dropf);
			if (dropf) {
				log_error("%s: CAN frames dropped", __func__);
				cif->dropped_frames = dropf;
				ldx_can_call_err_cb(cif, CAN_ERROR_DROPPED_FRAMES, NULL);
			}
		}

		if (frame.can_id & CAN_ERR_FLAG) {
			can_err_cb_t *err_cb;

			list_for_each_entry(err_cb, &pdata->err_cb_list_head, list) {
				if (err_cb->handler)
					err_cb->handler(frame.can_id, NULL);
			}
		}

		if (rx_cb->handler)
			rx_cb->handler(&frame, &tstamp);
	}

	return ret;
}

static void *ldx_can_thr(void *arg)
{
	can_if_t *cif = (can_if_t *)arg;
	can_priv_t *pdata = cif->_data;
	int ret;

	while (pdata->run_thr) {
		fd_set fds;
		struct timeval tout;

		pthread_mutex_lock(&pdata->mutex);

		memcpy(&fds, &pdata->can_fds, sizeof(fds));
		memcpy(&tout, &pdata->can_tout, sizeof(tout));

		ret = select(pdata->maxfd + 1, &fds, NULL, NULL, &tout);
		if (ret < 0 && errno != EINTR) {
			log_error("%s|%s: select error (%d|%d)",
					  cif->name, __func__, ret, errno);
			ldx_can_call_err_cb(cif, errno, NULL);
		} else if (ret > 0) {
			can_cb_t *rx_cb;

			/*
			 * Check the socket for each registered rx handler and
			 * trigger the callback acordingly
			 */
			list_for_each_entry(rx_cb, &pdata->rx_cb_list_head, list) {
				if (FD_ISSET(rx_cb->rx_skt, &fds)) {
					ret = ldx_can_process_rx_socket(cif, rx_cb);
					if (ret) {
						log_error("%s|%s: select error (%d|%d)",
								  cif->name, __func__, ret, errno);
					}
				}
			}

			/* Check also the tx socket to detect errors */
			if (FD_ISSET(pdata->tx_skt, &fds)) {
				ret = ldx_can_process_tx_socket(cif);
				if (ret)
					log_error("%s|%s: select error (%d|%d)",
							  cif->name, __func__, ret, errno);
			}
		}

		pthread_mutex_unlock(&pdata->mutex);

		sched_yield();
	}
	return NULL;
}

int ldx_can_init(can_if_t *cif, can_if_cfg_t *cfg)
{
	int ret = 0;
	can_priv_t *pdata = NULL;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	pdata = cif->_data;
	cif->cfg = *cfg;

	/* Set bitrate if required */
	if (cfg->bitrate != LDX_CAN_INVALID_BITRATE) {
		ret = ldx_can_set_bitrate(cif, cfg->bitrate);
		if (ret)
			return ret;

		cif->cfg.bitrate = cfg->bitrate;
	}

	/* Set data bitrate if required */
	if (cfg->dbitrate != LDX_CAN_INVALID_BITRATE) {
		ret = ldx_can_set_data_bitrate(cif, cfg->dbitrate);
		if (ret)
			return ret;

		cif->cfg.bitrate = cfg->bitrate;
	}

	/* Set restart ms if required */
	if (cfg->restart_ms != LDX_CAN_INVALID_RESTART_MS) {
		ret = ldx_can_set_restart_ms(cif, cfg->restart_ms);
		if (ret)
			return ret;

		cif->cfg.restart_ms = cfg->restart_ms;
	}

	/* Configure the bit_timing setting */
	if (cfg->bit_timing.bitrate) {
		ret = ldx_can_set_restart_ms(cif, cfg->restart_ms);
		if (ret)
			return ret;

		cif->cfg.restart_ms = cfg->restart_ms;
	}

	/* Configure the control mode setting */
	if (cfg->ctrl_mode.mask != LDX_CAN_UNCONFIGURED_MASK) {
		ret = ldx_can_set_ctrlmode(cif, &cfg->ctrl_mode);
		if (ret)
			return ret;

		cif->cfg.ctrl_mode = cfg->ctrl_mode;
	}

	/* Once configured, start the interface */
	ret = ldx_can_start(cif);
	if (ret)
		return ret;

	pdata->tx_skt = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (pdata->tx_skt < 0) {
		log_error("%s: Unable to create socket", __func__);
		return -CAN_ERROR_TX_SKT_CREATE;
	}

	strncpy(pdata->ifr.ifr_name, cif->name, IFNAMSIZ - 1);
	pdata->ifr.ifr_name[IFNAMSIZ - 1] = 0;
	pdata->ifr.ifr_ifindex = if_nametoindex(pdata->ifr.ifr_name);
	if (!pdata->ifr.ifr_ifindex) {
		log_error("%s: Unable to get interface index on %s",
			  __func__, cif->name);
		ret = -CAN_ERROR_IFR_IDX;
		goto err_skt_close;
	}

	pdata->addr.can_family = AF_CAN;
	pdata->addr.can_ifindex = pdata->ifr.ifr_ifindex;

	/*
	 * Set the socket as non blocking. Anyway, it does not seem to have any
	 * real effect setting it as blocking or non blocking.
	 */
	ret = fcntl(pdata->tx_skt, F_SETFL, O_NONBLOCK);
	if (ret)
		goto err_skt_close;

	if (cif->cfg.canfd_enabled) {
		int canfd_en = 1;

		ret = ioctl(pdata->tx_skt, SIOCGIFMTU, &pdata->ifr);
		if (ret < 0) {
			log_error("%s: error on ioctl SIOCGIFMTU on %s",
				  __func__, cif->name);
			ret = -CAN_ERROR_SIOCGIFMTU;
			goto err_skt_close;
		}

		if (pdata->ifr.ifr_mtu != CANFD_MTU) {
			log_error("%s: CAN FD mtu not supported on %s",
				  __func__, cif->name);
			ret = CAN_ERROR_NOT_CANFD;
			goto err_skt_close;
		}

		ret = setsockopt(pdata->tx_skt, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,
				 &canfd_en, sizeof(canfd_en));
		if (ret < 0) {
			log_error("%s: setsockopt CAN_RAW_FD_FRAMES error on %s",
				  __func__, cif->name);
			ret = -CAN_ERROR_SETSKTOPT_CANFD;
			goto err_skt_close;
		}
	}

	/*
	 * This socket is only to transmit frames, therefore we disable
	 * reception, except for error messages
	 */
	ret = setsockopt(pdata->tx_skt, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);
	if (ret < 0) {
		log_error("%s|%s: setsockopt CAN_RAW_FILTER error (%d|%d)",
			  cif->name, __func__, ret, errno);
		ret = -CAN_ERROR_SETSKTOPT_RAW_FLT;
		goto err_skt_close;
	}

	if (cif->cfg.tx_buf_len) {
		socklen_t size_tx_buf_len_rd = sizeof(cif->cfg.tx_buf_len_rd);

		/*
		 * Try first with SO_SNDBUFFORCE which allows to exceed rmem_max limit
		 * for privileged (CAP_NET_ADMIN) processes.
		 */
		ret = setsockopt(pdata->tx_skt, SOL_SOCKET, SO_SNDBUFFORCE,
				 &cif->cfg.tx_buf_len, sizeof(cif->cfg.tx_buf_len));
		if (ret < 0) {
			log_warning("%s|%s: setsockopt SO_SNDBUFFORCE error",
				   cif->name, __func__);

			ret = setsockopt(pdata->tx_skt, SOL_SOCKET, SO_SNDBUF,
					 &cif->cfg.tx_buf_len, sizeof(cif->cfg.tx_buf_len));
			if (ret < 0) {
				log_error("%s|%s: setsockopt SO_SNDBUF error",
					  cif->name, __func__);
				ret = -CAN_ERROR_SETSKTOPT_SNDBUF;
				goto err_skt_close;
			}
		}

		ret = getsockopt(pdata->tx_skt, SOL_SOCKET, SO_SNDBUF,
				 &cif->cfg.tx_buf_len_rd, &size_tx_buf_len_rd);
		if (ret < 0) {
			log_error("%s|%s: getsockopt SO_SNDBUF error",
					  cif->name, __func__);
			ret = -CAN_ERROR_GETSKTOPT_SNDBUF;
			goto err_skt_close;
		}
	}

	if (cif->cfg.error_mask) {
		ret = setsockopt(pdata->tx_skt, SOL_CAN_RAW, CAN_RAW_ERR_FILTER,
				 &cif->cfg.error_mask,
				 sizeof(cif->cfg.error_mask));
		if (ret < 0) {
			log_error("%s: setsockopt CAN_RAW_ERR_FILTER error on %s",
				  __func__, cif->name);
			ret = -CAN_ERROR_SETSKTOPT_ERR_FLT;
			goto err_skt_close;
		}
	}

	ret = bind(pdata->tx_skt, (struct sockaddr *)&pdata->addr, sizeof(pdata->addr));
	if (ret < 0) {
		log_error("%s: socket bind error on %s", __func__, cif->name);
		ret = -CAN_ERROR_TX_SKT_BIND;
		goto err_skt_close;
	}

	/* Add the tx socket to the fd_set structure to detect errors */
	FD_ZERO(&pdata->can_fds);
	FD_SET(pdata->tx_skt, &pdata->can_fds);
	pdata->maxfd = pdata->tx_skt;

	ret = ldx_can_register_error_handler(cif, ldx_can_default_error_handler);
	if (ret < 0) {
		log_error("%s|%s: Unable to register default error handler",
			  cif->name, __func__);
		ret = -CAN_ERROR_REG_ERR_HDLR;
		goto err_skt_close;
	}

	/* Create the thread to process async events */
	if (!pdata->can_thr) {
		pdata->can_thr = malloc(sizeof(pthread_t));
		if (!pdata->can_thr) {
			log_error("%s: Unable to alloc thread memory in %s",
				  __func__, cif->name);
			ret = -CAN_ERROR_THREAD_ALLOC;
			goto err_skt_close;
		}

		pthread_attr_init(&pdata->can_thr_attr);
		pthread_attr_setschedpolicy(&pdata->can_thr_attr, SCHED_FIFO);

		ret = pthread_mutex_init(&pdata->mutex, NULL);
		if (ret) {
			log_error("%s: Unable init thread mutex %s",
				  __func__, cif->name);
			ret = -CAN_ERROR_THREAD_MUTEX_INIT;
			goto err_thr_alloc;
		}

		ret = pthread_create(pdata->can_thr, NULL, ldx_can_thr, cif);
		if (ret) {
			log_error("%s: Unable to create thread in %s",
				  __func__, cif->name);
			pthread_mutex_unlock(&pdata->mutex);
			pthread_mutex_destroy(&pdata->mutex);
			ret = -CAN_ERROR_THREAD_CREATE;
			goto err_thr_alloc;
		}
	}

	return CAN_ERROR_NONE;

err_thr_alloc:
	free(pdata->can_thr);

err_skt_close:
	close(pdata->tx_skt);

	return ret;
}

can_if_t *ldx_can_request_by_name(const char * const if_name)
{
	can_if_t *cif;
	can_priv_t *priv;

	log_debug("%s: Requesting %s interface",	__func__, if_name);
	cif = calloc(1, sizeof(can_if_t));
	if (!cif) {
		log_error("%s: Unable to allocate memory for %s node",
			  __func__, if_name);
		return NULL;
	}

	priv = calloc(1, sizeof(can_priv_t));
	if (!priv) {
		log_error("%s: Unable to allocate memory for %s node",
			  __func__, if_name);
		free(cif);
		return NULL;
	}

	strncpy(cif->name, if_name, IFNAMSIZ - 1);
	cif->name[IFNAMSIZ - 1] = '\0';
	INIT_LIST_HEAD(&priv->err_cb_list_head);
	INIT_LIST_HEAD(&priv->rx_cb_list_head);
	priv->can_tout.tv_sec = LDX_CAN_DEF_TOUT_SEC;
	priv->can_tout.tv_usec = LDX_CAN_DEF_TOUT_USEC;
	priv->run_thr = true;
	cif->_data = priv;

	return cif;
}

can_if_t *ldx_can_request(const unsigned int can_iface)
{
	char *iface;
	can_if_t *can_if;
	int ret;

	ret = asprintf(&iface, "can%d", can_iface);
	if (ret < 0) {
		log_error("%s: asprintf", __func__);
		return NULL;
	}

	can_if = ldx_can_request_by_name(iface);

	free(iface);

	return can_if;
}

int ldx_can_free(can_if_t *cif)
{
	int ret = EXIT_SUCCESS;
	can_priv_t *pdata;

	if (!cif)
		return EXIT_SUCCESS;

	pdata = cif->_data;
	pthread_mutex_lock(&pdata->mutex);
	pthread_mutex_destroy(&pdata->mutex);
	pthread_cancel(*pdata->can_thr);

	ret = ldx_can_stop(cif);
	if (ret)
		log_error("%s: can not stop iface %s", __func__, cif->name);

	close(pdata->tx_skt);
	free(pdata);
	free(cif);

	return ret;
}

int ldx_can_tx_frame(const can_if_t *cif, struct canfd_frame *frame)
{
	can_priv_t *pdata = NULL;
	int mtu = CAN_MTU;
	int ret;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	pdata = cif->_data;

	/* Set proper length and mtu for standard or fd frames */
	if (cif->cfg.canfd_enabled) {
		frame->len = can_dlc2len(CAN_LEN2DLC(frame->len));
		mtu = CANFD_MTU;
	}

	ret = write(pdata->tx_skt, frame, mtu);
	if (ret == -1) {
		if (errno == ENOBUFS || errno == EAGAIN)
			/*
			 * Nothing to log... the txqueue is full and there are
			 * no additional buffers. Tell user space to retry, as
			 * setting the socket as blocking does not seem to have
			 * any effect.
			 */
			return -CAN_ERROR_TX_RETRY_LATER;
	} else if (ret < 0) {
		log_error("%s: socket write (%d/%d) on %s", __func__, ret, errno, cif->name);
		return -CAN_ERROR_TX_SKT_WR;
	} else if (ret < mtu) {
		return -CAN_ERROR_INCOMP_FRAME;
	}

	return EXIT_SUCCESS;
}

static can_err_cb_t *find_errcb_by_function(const can_if_t *cif,
					    const ldx_can_error_cb_t cb)
{
	can_priv_t *pdata = cif->_data;
	can_err_cb_t *err_cb;

	list_for_each_entry(err_cb, &pdata->err_cb_list_head, list) {
		if (err_cb->handler == cb)
			return err_cb;
	}

	return NULL;
}

int ldx_can_register_error_handler(const can_if_t *cif, const ldx_can_error_cb_t cb)
{
	can_err_cb_t *errcb;
	can_priv_t *pdata = NULL;
	int ret;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	pdata = cif->_data;

	ret = pthread_mutex_lock(&pdata->mutex);
	if (ret) {
		log_error("%s: error mutex lock %s",
			  __func__, cif->name);
		return -CAN_ERROR_THREAD_MUTEX_LOCK;
	}

	/* Ensure the callback is not registered more than once */
	errcb = find_errcb_by_function(cif, cb);
	if (errcb) {
		log_error("%s: callback already registered on %s", __func__, cif->name);
		ret = -CAN_ERROR_ERR_CB_ALR_REG;
		goto ecb_err_unlock;
	}

	errcb = (can_err_cb_t *)malloc(sizeof(can_err_cb_t));
	if (!errcb) {
		log_error("%s: Unable to alloc memory for error callback on %s",
			  __func__, cif->name);
		ret = -CAN_ERROR_NO_MEM;
		goto ecb_err_unlock;
	}

	errcb->handler = cb;
	list_add(&(errcb->list), &(pdata->err_cb_list_head));

ecb_err_unlock:
	pthread_mutex_unlock(&pdata->mutex);

	return ret;
}

int ldx_can_unregister_error_handler(const can_if_t *cif, const ldx_can_error_cb_t cb)
{
	can_priv_t *pdata = NULL;
	can_err_cb_t *errcb;
	int ret;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	pdata = cif->_data;

	ret = pthread_mutex_lock(&pdata->mutex);
	if (ret) {
		log_error("%s: error mutex lock %s",
			  __func__, cif->name);
		ret = -CAN_ERROR_THREAD_MUTEX_LOCK;
		goto unreg_errh_ret;
	}

	/* Find the callback and remove it from the list */
	errcb = find_errcb_by_function(cif, cb);
	if (!errcb) {
		log_error("%s: callback not found on %s", __func__, cif->name);
		ret = -CAN_ERROR_ERR_CB_NOT_FOUND;
		goto unreg_errh_unlock;
	}

	list_del(&errcb->list);
	free(errcb);

unreg_errh_unlock:
	pthread_mutex_unlock(&pdata->mutex);

unreg_errh_ret:
	return ret;

}

static can_cb_t *find_rxcb_by_function(const can_if_t *cif, const ldx_can_rx_cb_t cb)
{
	can_priv_t *pdata = cif->_data;
	can_cb_t *rx_cb;

	pdata = cif->_data;
	list_for_each_entry(rx_cb, &pdata->rx_cb_list_head, list) {
		if (rx_cb->handler == cb) {
			return rx_cb;
		}
	}

	return NULL;
}

int ldx_can_register_rx_handler(can_if_t *cif, const ldx_can_rx_cb_t cb,
				struct can_filter *filters, int nfilters)
{
	can_cb_t *rxcb;
	can_priv_t *pdata = NULL;
	int ret;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	pdata = cif->_data;

	ret = pthread_mutex_lock(&pdata->mutex);
	if (ret) {
		log_error("%s: error mutex lock %s",
			  __func__, cif->name);
		return -CAN_ERROR_THREAD_MUTEX_LOCK;
	}

	/* Ensure the callback is not registered more than once */
	rxcb = find_rxcb_by_function(cif, cb);
	if (rxcb) {
		log_error("%s: callback already registered on %s", __func__, cif->name);
		ret = -CAN_ERROR_RX_CB_ALR_REG;
		goto rx_err_unlock;
	}

	rxcb = (can_cb_t *)malloc(sizeof(can_cb_t));
	if (!rxcb) {
		log_error("%s: Unable to alloc memory for rx callback on %s",
			  __func__, cif->name);
		ret = -CAN_ERROR_NO_MEM;
		goto rx_err_unlock;
	}

	/*
	 * Create the socket, set the filter condition and add it to the
	 * the fd_set variable
	 */
	rxcb->rx_skt = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (rxcb->rx_skt < 0) {
		log_error("%s: Unable to create rx socket on %s", __func__,
			  cif->name);
		ret = -CAN_ERROR_RX_SKT_CREATE;
		goto rx_err_free;
	}

	/*
	 * Set the socket as non blocking. Anyway, it does not seem to have any
	 * real effect setting it as blocking or non blocking.
	 */
	ret = fcntl(rxcb->rx_skt, F_SETFL, O_NONBLOCK);
	if (ret)
		goto rx_err_skt_close;

	if(cif->cfg.process_header) {
		/*
		 * For details, check:
		 * Documentation/networking/timestamping.txt
		 */
		int option_name, tstamp_flags;

		if (cif->cfg.hw_timestamp) {
			option_name = SO_TIMESTAMPING;
			tstamp_flags = SOF_TIMESTAMPING_SOFTWARE |
				       SOF_TIMESTAMPING_RX_SOFTWARE |
				       SOF_TIMESTAMPING_RAW_HARDWARE;
		} else {
			option_name = SO_TIMESTAMP;
			tstamp_flags = 1;
		}

		ret = setsockopt(rxcb->rx_skt, SOL_SOCKET, option_name,
				  &tstamp_flags, sizeof(tstamp_flags));
		if (ret) {
			log_info("%s: setsockopt %s not supported",
				 __func__, cif->cfg.hw_timestamp ?
				"SO_TIMESTAMPING" : "SO_TIMESTAMP");
			ret = -CAN_ERROR_SETSKTOPT_TIMESTAMP;
			goto rx_err_skt_close;
		}
	}

	if (cif->cfg.canfd_enabled) {
		int canfd_en = 1;

		ret = setsockopt(rxcb->rx_skt, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,
				 &canfd_en, sizeof(canfd_en));
		if (ret < 0) {
			log_error("%s|%s: setsockopt CAN_RAW_FD_FRAMES error",
				  cif->name, __func__);
			ret = -CAN_ERROR_SETSKTOPT_CANFD;
			goto rx_err_skt_close;
		}
	}

	if (cif->cfg.rx_buf_len) {
		socklen_t size_rx_buf_len_rd = sizeof(cif->cfg.rx_buf_len_rd);

		/*
		 * Try first with SO_RCVBUFFORCE which allows to exceed rmem_max limit
		 * for privileged (CAP_NET_ADMIN) processes.
		 */
		ret = setsockopt(rxcb->rx_skt, SOL_SOCKET, SO_RCVBUFFORCE,
				 &cif->cfg.rx_buf_len, sizeof(cif->cfg.rx_buf_len));
		if (ret < 0) {
			log_warning("%s|%s: setsockopt SO_RCVBUFFORCE error",
				   cif->name, __func__);
			ret = setsockopt(rxcb->rx_skt, SOL_SOCKET, SO_RCVBUF,
					 &cif->cfg.rx_buf_len, sizeof(cif->cfg.rx_buf_len));
			if (ret < 0) {
				log_error("%s|%s: setsockopt SO_RCVBUF error",
					  cif->name, __func__);
				ret = -CAN_ERROR_SETSKTOPT_RCVBUF;
				goto rx_err_skt_close;
			}
		}

		ret = getsockopt(rxcb->rx_skt, SOL_SOCKET, SO_RCVBUF,
				 &cif->cfg.rx_buf_len_rd, &size_rx_buf_len_rd);
		if (ret < 0) {
			log_error("%s|%s: getsockopt SO_RCVBUF error",
					  cif->name, __func__);
			ret = -CAN_ERROR_GETSKTOPT_RCVBUF;
			goto rx_err_skt_close;
		}
	}

	if (cif->cfg.error_mask) {
		ret = setsockopt(rxcb->rx_skt, SOL_CAN_RAW, CAN_RAW_ERR_FILTER,
				 &cif->cfg.error_mask,
				 sizeof(cif->cfg.error_mask));
		if (ret < 0) {
			log_error("%s: setsockopt CAN_RAW_ERR_FILTER error on %s",
				  __func__, cif->name);
			ret = -CAN_ERROR_SETSKTOPT_ERR_FLT;
			goto rx_err_skt_close;
		}
	}

	if (nfilters > 0 && filters) {
		ret = setsockopt(rxcb->rx_skt, SOL_CAN_RAW, CAN_RAW_FILTER,
				 filters, nfilters * sizeof(struct can_filter));
		if (ret) {
			log_error("%s: setsockopt CAN_RAW_FILTER error (%d) on %s",
				  __func__, ret, cif->name);
			ret = -CAN_ERROR_SETSKTOPT_RAW_FLT;
			goto rx_err_skt_close;
		}
	}

	ret = bind(rxcb->rx_skt, (struct sockaddr *)&pdata->addr, sizeof(pdata->addr));
	if (ret < 0) {
		log_error("%s: socket bind error on %s", __func__, cif->name);
		ret = -CAN_ERROR_RX_SKT_BIND;
		goto rx_err_skt_close;
	}

	FD_SET(rxcb->rx_skt, &pdata->can_fds);
	if (rxcb->rx_skt > pdata->maxfd)
		pdata->maxfd = rxcb->rx_skt;

	rxcb->handler = cb;
	list_add(&(rxcb->list), &(pdata->rx_cb_list_head));
	pthread_mutex_unlock(&pdata->mutex);

	return CAN_ERROR_NONE;

rx_err_skt_close:
	close(rxcb->rx_skt);

rx_err_free:
	free(rxcb);

rx_err_unlock:
	pthread_mutex_unlock(&pdata->mutex);

	return ret;
}

int ldx_can_unregister_rx_handler(const can_if_t *cif, const ldx_can_rx_cb_t cb)
{
	can_priv_t *pdata = NULL;
	can_cb_t *rxcb;
	int ret;

	if (!cif)
		return -CAN_ERROR_NULL_INTERFACE;

	pdata = cif->_data;
	ret = pthread_mutex_lock(&pdata->mutex);
	if (ret) {
		log_error("%s: error mutex lock %s",
			  __func__, cif->name);
		ret = -CAN_ERROR_THREAD_MUTEX_LOCK;
		goto unreg_rxh_ret;
	}

	/* Find the callback and remove it from the list */
	rxcb = find_rxcb_by_function(cif, cb);
	if (!rxcb) {
		log_error("%s: callback not found on %s", __func__, cif->name);
		ret = -CAN_ERROR_RX_CB_NOT_FOUND;
		goto unreg_rxh_unlock;
	}

	/* Remove the socket from can_fds and release the resources */
	FD_CLR(rxcb->rx_skt, &pdata->can_fds);
	close(rxcb->rx_skt);
	list_del(&rxcb->list);
	free(rxcb);

unreg_rxh_unlock:
	pthread_mutex_unlock(&pdata->mutex);

unreg_rxh_ret:
	return ret;
}
