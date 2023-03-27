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

#ifndef CAN_H_
#define CAN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/can/raw.h>
#include <linux/can/netlink.h>
#include <net/if.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>

#define NLMSG_TAIL(nmsg) \
        ((struct rtattr *)(((void *) (nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

#define LDX_CAN_DEF_TOUT_SEC	0
#define LDX_CAN_DEF_TOUT_USEC	0

#define LDX_CAN_INVALID_BITRATE		0
#define LDX_CAN_INVALID_RESTART_MS	0
#define LDX_CAN_UNCONFIGURED_MASK 0

/**
 * Callback functions
 */
typedef void (*ldx_can_rx_cb_t)(struct canfd_frame *frame, struct timeval *tv);
typedef void (*ldx_can_error_cb_t)(int error, void *data);

/**
 * can_if_cfg_t - CAN interface configuration type.
 *
 * @nl_cmd_verify:		Enable the command verification.
 * @canfd_enabled:		Enable the flexible data rate.
 * @process_header:		Enable the processing of the frame headers.
 * @hw_timestamp:		Enable the hardware timestamp.
 * @rx_buf_len:			Length of the reception buffer.
 * @tx_buf_len:			Length of the transmission buffer.
 * @rx_buf_len_rd:		Length of the reception buffer socket.
 * @tx_buf_len_rd:		Length of the transmission buffer socket.
 * @bitrate:			Bitrate of the CAN interface.
 * @restart_ms:			Restart time in ms.
 * @error_mask:		 	Struct with the CAN error mask.
 * @bit_timing:			Struct with the CAN bittiming values.
 * @ctrl_mode:			Struct with the CAN control mode values.
 */
typedef struct can_if_cfg {
	bool			nl_cmd_verify;
	bool			canfd_enabled;
	bool			process_header;
	bool			hw_timestamp;
	int			rx_buf_len;
	int			tx_buf_len;
	int			rx_buf_len_rd;
	int			tx_buf_len_rd;
	uint32_t		bitrate;
	uint32_t		dbitrate;
	uint32_t		restart_ms;
	can_err_mask_t		error_mask;
	struct can_bittiming	bit_timing;
	struct can_bittiming	dbit_timing;
	struct can_ctrlmode	ctrl_mode;
} can_if_cfg_t;

typedef struct can_if {
	char			name[IFNAMSIZ];
	can_if_cfg_t		cfg;
	uint32_t		dropped_frames;
	void			*_data;
} can_if_t;

/* Error values for the CAN interface */
enum {
	CAN_ERROR_NONE = 0,

	/* General syscall errors */
	CAN_ERROR_NULL_INTERFACE,
	CAN_ERROR_IFR_IDX,
	CAN_ERROR_NO_MEM,

	/* Netlink errors */
	CAN_ERROR_NL_GET_STATE,
	CAN_ERROR_NL_START,
	CAN_ERROR_NL_STOP,

	CAN_ERROR_NL_STATE_MISSMATCH,
	CAN_ERROR_NL_BITRATE,
	CAN_ERROR_NL_RESTART,
	CAN_ERROR_NL_SET_RESTART_MS,
	CAN_ERROR_NL_GET_RESTART_MS,
	CAN_ERROR_NL_RSTMS_MISSMATCH,
	CAN_ERROR_NL_SET_CTRL_MODE,
	CAN_ERROR_NL_GET_CTRL_MODE,
	CAN_ERROR_NL_CTRL_MISSMATCH,
	CAN_ERROR_NL_GET_DEV_STATS,
	CAN_ERROR_NL_BT_MISSMATCH,
	CAN_ERROR_NL_SET_BIT_TIMING,
	CAN_ERROR_NL_GET_BIT_TIMING,
	CAN_ERROR_NL_GET_BIT_ERR_CNT,
	CAN_ERROR_NL_BR_MISSMATCH,

	/* Socket communication errors */
	CAN_ERROR_TX_SKT_CREATE,
	CAN_ERROR_TX_SKT_WR,
	CAN_ERROR_TX_SKT_BIND,
	CAN_ERROR_TX_RETRY_LATER,
	CAN_ERROR_INCOMP_FRAME,
	CAN_ERROR_NETWORK_DOWN,

	CAN_ERROR_RX_SKT_CREATE,
	CAN_ERROR_RX_SKT_BIND,

	CAN_ERROR_SETSKTOPT_RAW_FLT,
	CAN_ERROR_SETSKTOPT_ERR_FLT,
	CAN_ERROR_SETSKTOPT_CANFD,
	CAN_ERROR_SETSKTOPT_TIMESTAMP,
	CAN_ERROR_SETSKTOPT_SNDBUF,
	CAN_ERROR_GETSKTOPT_SNDBUF,
	CAN_ERROR_SETSKTOPT_RCVBUF,
	CAN_ERROR_GETSKTOPT_RCVBUF,

	CAN_ERROR_SIOCGIFMTU,
	CAN_ERROR_NOT_CANFD,
	CAN_ERROR_THREAD_CREATE,
	CAN_ERROR_THREAD_ALLOC,
	CAN_ERROR_THREAD_MUTEX_INIT,
	CAN_ERROR_THREAD_MUTEX_LOCK,

	CAN_ERROR_REG_ERR_HDLR,
	CAN_ERROR_DROPPED_FRAMES,

	/* Handlers */
	CAN_ERROR_RX_CB_NOT_FOUND,
	CAN_ERROR_RX_CB_ALR_REG,
	CAN_ERROR_ERR_CB_NOT_FOUND,
	CAN_ERROR_ERR_CB_ALR_REG,

	__CAN_ERR_LAST
};

#define CAN_ERROR_MAX	(__CAN_ERR_LAST - 1)

/**
 * ldx_can_get_state() - retrieve the device state
 *
 * @cif:	The pointer to the CAN device to get the state from.
 * @state:	Pointer where the state will be stored.
 *
 * Get the state from the specified device. Possible states are defined
 * in can/netlink.h.
 *
 * Return: CAN_ERR_NONE on success, error code otherwise.
 */
int ldx_can_get_state(const can_if_t *cif, enum can_state *state);

/**
 * ldx_can_get_dev_stats() - retrieve the device stats
 *
 * @cif:	The pointer to the CAN device to get the stats from.
 * @cds:	Pointer to the device stats struct.
 *
 * Get the can_device_stats from the specified interface.
 *
 * Return: CAN_ERR_NONE on success, error code otherwise.
 */
int ldx_can_get_dev_stats(const can_if_t *cif, struct can_device_stats *cds);

/**
 * ldx_can_get_bit_error_counter() - retrieve the bit error counter
 *
 * @cif:	The pointer to the CAN interface to get the error counter from.
 * @bc:	Pointer to the device bit error counter struct.
 *
 * Get the bit error counter information from the specified interface.
 *
 * Return: CAN_ERR_NONE on success, error code otherwise.
 */
int ldx_can_get_bit_error_counter(const can_if_t *cif, struct can_berr_counter *bc);

/**
 * ldx_can_stop() - stop the specified CAN interface
 *
 * @cif:	The pointer to the CAN interface to stop.
 *
 * This function is similar to ifconfig can0 down. Changes the interface
 * state to down.
 *
 * Return: CAN_ERR_NONE on success, error code otherwise.
 */
int ldx_can_stop(const can_if_t *cif);

/**
 * ldx_can_restart() - restart the specified CAN interface
 *
 * @cif:	The pointer to the CAN interface to restart.
 *
 * This function allows to restart the CAN interface from a BUS_OFF state.
 * Invoking the function from a different state will fail.
 *
 * Return: CAN_ERR_NONE on success, error code otherwise.
 */
int ldx_can_restart(const can_if_t *cif);

/**
 * ldx_can_set_bit_timing() - set the bit timing of the specified interface
 *
 * @cif:	The pointer to the CAN interface to configure.
 * @bt:		Pointer to the can_bittiming structure with the configuration.
 *
 * This function sets the bit timing configuration of the specified CAN
 * interface. Normally, this is not required as the CAN driver computes the
 * proper bit timing information for the selected bitrate.
 *
 * Return: CAN_ERR_NONE on success, error code otherwise.
 */
int ldx_can_set_bit_timing(can_if_t *cif, struct can_bittiming *bt);

/**
 * ldx_can_set_data_bit_timing() - set the data bit timing of the specified interface
 *
 * @cif:	The pointer to the CAN interface to configure.
 * @dbt:	Pointer to the can_bittiming structure with the configuration.
 *
 * This function sets the data bit timing configuration of the specified CAN
 * interface.
 *
 * Return: CAN_ERR_NONE on success, error code otherwise.
 */
int ldx_can_set_data_bit_timing(can_if_t *cif, struct can_bittiming *dbt);

/**
 * ldx_can_request() - request a CAN interface by index
 *
 * @can_iface:	The CAN interface index.
 *
 * This function returns a can_if_t pointer based on the index.
 * Memory for the struct is obtained with 'malloc' and must be freed with
 * 'ldx_can_free()'.
 *
 * Return: A pointer to can_if_t on success, NULL on error.
 */
can_if_t *ldx_can_request(unsigned int can_iface);

/**
 * ldx_can_request_by_name() - request a CAN interface by name
 *
 * @can_iface:	The CAN interface name.
 *
 * This function returns a can_if_t pointer based on the name interface.
 * Memory for the struct is obtained with 'malloc' and must be freed with
 * 'ldx_can_free()'.
 *
 * Return: A pointer to can_if_t on success, NULL on error.
 */
can_if_t *ldx_can_request_by_name(const char * const can_iface);

/**
 * ldx_can_free() - Free a previously requested CAN interface
 *
 * @cif:	A pointer to the requested CAN to free.
 *
 * Return: EXIT_SUCCESS on success, error code otherwise.
 */
int ldx_can_free(can_if_t *cif);

/**
 * ldx_can_set_defconfig() - Configure default parameters for CAN interface
 *
 * @cfg:	A pointer to the configuration variable (can_if_cfg_t).
 *
 * These are the set of default parameters:
 *    * nl_cmd_verify: Enabled
 *    * canfd_enabled: Disabled
 *    * process_header: Enabled
 *    * hw_timestamp: Disabled
 *    * error_mask: CAN_ERR_TX_TIMEOUT | CAN_ERR_CRTL | CAN_ERR_BUSOFF |
 *    			    CAN_ERR_BUSERROR | CAN_ERR_RESTARTED;
 */
void ldx_can_set_defconfig(can_if_cfg_t *cfg);

/**
 * ldx_can_init() - Initialize the selected CAN interface with the given
 *                     configuration
 *
 * @cif:	A pointer to the requested CAN to configure.
 * @cfg:	A pointer to the configuration variable (can_if_cfg_t).
 *
 * Return: CAN_ERR_NONE on success, error code otherwise.
 */
int ldx_can_init(can_if_t *cif, can_if_cfg_t *cfg);

/**
 * ldx_can_set_bitrate() - Set the bitrate in the CAN interface
 *
 * @cif:	A pointer to the requested CAN to configure.
 * @bitrate:	The bitrate to configure.
 *
 * Return: CAN_ERR_NONE on success, error code otherwise.
 */
int ldx_can_set_bitrate(can_if_t *cif, uint32_t bitrate);

/**
 * ldx_can_set_data_bitrate() - Set the data bitrate in the CAN interface
 *
 * @cif:	A pointer to the requested CAN to configure.
 * @dbitrate:	The data bitrate to configure.
 *
 * Return: CAN_ERR_NONE on success, error code otherwise.
 */
int ldx_can_set_data_bitrate(can_if_t *cif, uint32_t dbitrate);

/**
 * ldx_can_set_ctrlmode() - Set the control mode in the CAN interface
 *
 * @cif:	A pointer to the requested CAN to configure.
 * @cm:	The control mode to set (can_controlmode).
 *
 * Return: CAN_ERR_NONE on success, error code otherwise.
 */
int ldx_can_set_ctrlmode(can_if_t *cif, struct can_ctrlmode *cm);

/**
 * ldx_can_set_restart_ms() - Set the timeout to restart the
 * 				communication on error.
 *
 * @cif:	A pointer to the requested CAN to configure.
 * @restart_ms:	Timeout in ms to configure.
 *
 * Return: CAN_ERR_NONE on success, error code otherwise.
 */
int ldx_can_set_restart_ms(can_if_t *cif, uint32_t restart_ms);

/**
 * ldx_can_start() - Start the CAN interface for communication
 *
 * @cif:	A pointer to the requested CAN to start.
 *
 * Return: EXIT_SUCCESS on success, error code otherwise.
 */
int ldx_can_start(const can_if_t *cif);

/**
 * ldx_can_tx_frame() - Send a frame through the CAN interface
 *
 * @cif:	A pointer to the requested CAN to send the frame.
 * @frame:	The frame to send through the CAN interface (struct canfd_frame)
 *
 * Return: EXIT_SUCCESS on success, error code otherwise.
 */
int ldx_can_tx_frame(const can_if_t *cif, struct canfd_frame *frame);

/**
 * ldx_can_register_rx_handler() - Start frame reception on the given CAN
 *
 * @cif:	A pointer to the requested CAN to start the reception.
 * @cb:		Callback to execute each time a frame is received.
 * @filters:	A set of filters to filter the reception of frames.
 * @nfilters:	The number of filters contained in the filters variable.
 *
 * This function waits for interrupts to occur on the CAN asynchronously.
 * It is a non-blocking function that registers the provided callback to be
 * executed when a frame is received. After that it continues waiting for
 * new frames.
 *
 * To stop listening for interrupts use 'ldx_can_unregister_rx_handler()'.
 *
 * Return: CAN_ERR_NONE on success, error code otherwise.
 */
int ldx_can_register_rx_handler(can_if_t *cif, const ldx_can_rx_cb_t cb,
				struct can_filter *filters, int nfilters);

/**
 * ldx_can_unregister_rx_handler() - Remove the interrupt frame reception on the given CAN
 *
 * @cif:	A pointer to a requested CAN to stop an frame reception handler.
 * @cb:		Callback to be removed.
 *
 * This function stops the previously set interrupt handler on a CAN using
 * 'ldx_can_register_rx_handler()'.
 *
 * Return: EXIT_SUCCESS on success, error code otherwise.
 */
int ldx_can_unregister_rx_handler(const can_if_t *cif, const ldx_can_rx_cb_t cb);

/**
 * ldx_can_register_error_handler() - Start an error handler on the given CAN
 *
 * @cif:	A pointer to the requested CAN to start the reception.
 * @cb:		Callback to execute each time an error occurs on the CAN interface.
 *
 * This function waits for errors on the CAN interface asynchronously.
 * It is a non-blocking function that registers the provided callback to be
 * executed when an error happens in the interface. After that it continues waiting for
 * new errors.
 *
 * To stop listening for errors use 'ldx_can_unregister_error_handler()'.
 *
 * Return: EXIT_SUCCESS on success, error code otherwise.
 */
int ldx_can_register_error_handler(const can_if_t *cif, const ldx_can_error_cb_t cb);

/**
 * ldx_can_unregister_error_handler() - Remove the error handler on the given CAN
 *
 * @cif:	A pointer to the requested CAN to remove the handler.
 * @cb:		Callback to be removed
 *
 * This function stops the previously set interrupt handler on a CAN using
 * 'ldx_can_register_error_handler()'.
 *
 * Return: EXIT_SUCCESS on success, error code otherwise.
 */
int ldx_can_unregister_error_handler(const can_if_t *cif, const ldx_can_error_cb_t cb);

/**
 * ldx_can_strerror() - return the string describing the error
 *
 * @error:	Error number to get the description string.
 *
 * Return: the pointer to the string that describes the error or NULL
 *         if it was not found.
 */
const char * ldx_can_strerror(int error);

/**
 * ldx_can_is_extid_frame() - verify if the frame has extended id
 *
 * @frame:	Frame to check if it is a extended frame
 *
 * Return: True on extended id, false otherwise
 */
static inline bool ldx_can_is_extid_frame(const struct canfd_frame *frame)
{
	return frame->can_id & CAN_EFF_FLAG ? true : false;
}

/**
 * ldx_can_get_id() - return the id of a CAN frame
 *
 * @frame:	Frame to get the ID.
 *
 * Return: The id of the provided frame.
 */
static inline uint32_t ldx_can_get_id(const struct canfd_frame *frame)
{
	if (ldx_can_is_extid_frame(frame))
		return frame->can_id & CAN_EFF_MASK;
	else
		return frame->can_id & CAN_SFF_MASK;
}

#ifdef __cplusplus
}
#endif

#endif /* CAN_H_ */
