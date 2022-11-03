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

#ifndef PROCESS_H_
#define PROCESS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <sys/types.h>

/**
 * ldx_process_execute_cmd() - Execute the provided command and gets the response.
 *
 * @cmd:	Command to execute.
 * @resp: 	Buffer for the response.
 * @timeout:	Number of seconds to wait for the command execution.
 *
 * Response may contain an error string or the result of the command. It must
 * be freed.
 *
 * Return: 0 on success, any other value otherwise.
 */
int ldx_process_execute_cmd(const char *cmd, char **resp, int timeout);

/**
 * _ldx_process_exec_fd() - Executes the provided command.
 *
 * @infd:	File descriptor for the process input.
 * @outfd:	File descriptor for the process output.
 * @errfd:	File descriptor for the process error.
 * @cmd:	Command to execute.
 * @args:	Command parameters.
 *
 * Return: The process identifier.
 */
pid_t _ldx_process_exec_fd(int infd, int outfd, int errfd, const char *cmd, ...);

/**
 * ldx_process_exec_fd() - Executes the provided command.
 *
 * @infd:	File descriptor for the process input.
 * @outfd:	File descriptor for the process output.
 * @errfd:	File descriptor for the process error.
 * @cmd:	Command to execute.
 * @args:	Command parameters.
 *
 * Return: The process identifier.
 */
#define ldx_process_exec_fd(tout, ...) _ldx_process_exec_fd(tout, __VA_ARGS__, NULL)

/**
 * ldx_process_wait() - Waits for the provided pid.
 *
 * @pid:	Identifier of the process to wait for.
 *
 * Return: The exit value of the process or the number of the signal that
 * 	   terminates the process.
 */
int ldx_process_wait(pid_t pid);

#ifdef __cplusplus
}
#endif

#endif /* PROCESS_H_ */
