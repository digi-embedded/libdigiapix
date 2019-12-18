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

#ifndef PRIVATE__CAN_H_
#define PRIVATE__CAN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/select.h>

/*
 * Define _UAPI_CAN_NETLINK_H to avoid 'libsocketcan.h' including
 * the libsocketcan specific 'can_netlink.h' and use the standard
 * 'linux/can/netlink.h' header
 */
#define _UAPI_CAN_NETLINK_H
#include <libsocketcan.h>
#include "_list.h"

/**
 * can_cb - Data required in the CAN rx callback
 *
 * @list:			A list that contains CAN interfaces.
 * @function:		Function to be executed in the callback.
 * @rx_skt:			Reception socket for incoming frames.
 * @droped_frames:	Dropped frames.
 */
typedef struct can_cb {
	struct list_head	list;
	ldx_can_rx_cb_t		handler;
	int			rx_skt;
} can_cb_t;

/**
 * can_err_cb	CAN callback on error
 *
 * @list:		A list that contains CAN interfaces.
 * @function:	Function to be executed in the callback.
 */
typedef struct can_err_cb {
	struct list_head	list;
	ldx_can_error_cb_t	handler;
} can_err_cb_t;

/**
 * can_priv_t - Internal data type used by the library
 *
 * @ifr:		Ifreq structure used by the socket layer.
 * @addr:		Sockaddr_can structure used by the socket layer.
 * @tx_skt:		Transmission socket.
 * @mtu:		Maximun transmit unit for the CAN interface.
 * @maxdlen:	Maximun length of the data to transmit.
 * @maxfd:		Maximun flexible data rate.
 * @can_fds:	CAN Flexible data rate.
 * @can_tout:	CAN timeval.
 * @can_thr:		Working thread used by the library.
 * @can_thr_attr:	Working thread attribute structure.
 * @mux:			Mutex lock thread.
 * @run_thr:		Variable to check if the thread is running.
 * @rx_cb_list_head:	Linked list head for rx callbacks.
 * @err_cb_list_head:	Linked list head for error callbacks.
 */
typedef struct {
	struct ifreq		ifr;
	struct sockaddr_can	addr;
	int			tx_skt;
	uint32_t		mtu;
	uint32_t		maxdlen;

	int			maxfd;
	fd_set			can_fds;
	struct timeval		can_tout;

	pthread_t		*can_thr;
	pthread_attr_t		can_thr_attr;
	pthread_mutex_t		mutex;
	bool			run_thr;

	struct list_head	rx_cb_list_head;
	struct list_head	err_cb_list_head;

} can_priv_t;

#ifdef __cplusplus
}
#endif

#endif /* PRIVATE__CAN_H_ */
