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

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include "_log.h"
#include "process.h"

#define concat_va_list(arg) __extension__({		\
	__typeof__(arg) *_l;				\
	va_list _ap;					\
	int _n;						\
							\
	_n = 0;						\
	va_start(_ap, arg);				\
	do { ++_n;					\
	} while (va_arg(_ap, __typeof__(arg)));		\
	va_end(_ap);					\
	_l = alloca((_n + 1) * sizeof(*_l));		\
							\
	_n = 0;						\
	_l[0] = arg;					\
	va_start(_ap, arg);				\
	do { _l[++_n] = va_arg(_ap, __typeof__(arg));	\
	} while (_l[_n]);				\
	va_end(_ap);					\
	_l;						\
})

int ldx_process_execute_cmd(const char *cmd, char **resp, int timeout)
{
	FILE *f_cmd, *f_out;
	int bytes_available, rc;
	pid_t pid;
	char *timeout_str = NULL;

	if (cmd == NULL || strlen(cmd) == 0) {
		log_error("%s: Invalid command", __func__);
		return -1;
	}

	if (timeout < 0) {
		log_error("%s: Invalid timeout '%d'", __func__, timeout);
		return -1;
	}

	f_cmd = tmpfile();
	if (!f_cmd) {
		if (asprintf(resp, "Unable to execute '%s' (cmd tmpfile)", cmd) < 0)
			log_error("%s: Unable to execute '%s' (cmd tmpfile): %s, %d",
				  __func__, cmd, strerror(errno), errno);
		return -1;
	}

	if (write(fileno(f_cmd), cmd, strlen(cmd)) < 0
		|| write(fileno(f_cmd), "\n", 1) < 1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			fclose(f_cmd);
			if (asprintf(resp, "Unable to execute '%s'", cmd)  < 0)
				log_error("%s: Unable to execute'%s': %s, %d",
					  __func__, cmd, strerror(errno), errno);
			return -1;
		}
	}

	f_out = tmpfile();
	if (!f_out) {
		fclose(f_cmd);
		if (asprintf(resp, "Unable to execute '%s' (out tmpfile)", cmd) < 0)
			log_error("%s: Unable to execute '%s' (out tmpfile): %s, %d",
				  __func__, cmd, strerror(errno), errno);
		return -1;
	}

	fseek(f_cmd, 0, SEEK_SET);
	fseek(f_out, 0, SEEK_SET);

	if (asprintf(&timeout_str, "%d", timeout) < 0)
		timeout_str = NULL;

	pid = ldx_process_exec_fd(fileno(f_cmd), fileno(f_out), fileno(f_out),
				  "timeout", timeout_str != NULL ? timeout_str : "0",
				  "/bin/sh");
	free(timeout_str);

	rc = ldx_process_wait(pid);
	if (rc != 0)
		log_debug("%s: Failed to execute '%s' (%d)", __func__, cmd, rc);

	fseek(f_out, 0, SEEK_SET);
	ioctl(fileno(f_out), FIONREAD, &bytes_available);
	if (bytes_available > 0) {
		int read_rc = -1;

		*resp = calloc(bytes_available + 1, sizeof(char));
		read_rc = read(fileno(f_out), *resp, bytes_available);
		if (read_rc < 0) {
			rc = -1;
			log_debug("%s: Failed to get response for %s (%d)",
				  __func__, *resp, read_rc);
		}
	}

	fclose(f_cmd);
	fclose(f_out);

	if (rc && !*resp) {
		if (asprintf(resp, "Unable to execute '%s'", cmd) < 0)
			log_error("%s: Unable to execute '%s': %s, %d", cmd,
				  __func__, strerror(errno), errno);
	}

	return rc;
}

pid_t _ldx_process_exec_fd(int infd, int outfd, int errfd, const char *cmd, ...)
{
	const char **cmd_list = NULL;
	struct sigaction sa = { .sa_handler = SIG_DFL };
	pid_t pid;
	va_list argp;

	va_start(argp, cmd);
	cmd_list = concat_va_list(cmd);

	pid = fork();
	if (pid != 0)
		return pid;

	if (infd < 0)
		infd = open("/dev/null", O_RDONLY);
	fcntl(infd, F_SETFD, fcntl(infd, F_GETFD) & ~FD_CLOEXEC);
	if (infd != STDIN_FILENO) {
		dup2(infd, STDIN_FILENO);
		if (infd != outfd && infd != errfd)
			close(infd);
	}

	if (outfd < 0)
		outfd = open("/dev/null", O_WRONLY);
	fcntl(outfd, F_SETFD, fcntl(outfd, F_GETFD) & ~FD_CLOEXEC);
	if (outfd != STDOUT_FILENO) {
		dup2(outfd, STDOUT_FILENO);
		if (outfd != errfd)
			close(outfd);
	}

	if (errfd < 0)
		errfd = open("/dev/null", O_WRONLY);
	fcntl(errfd, F_SETFD, fcntl(errfd, F_GETFD) & ~FD_CLOEXEC);
	if (errfd != STDERR_FILENO) {
		dup2(errfd, STDERR_FILENO);
		close(errfd);
	}

	sigaction(SIGPIPE, &sa, NULL);

	execvp(cmd_list[0], (char * const *)cmd_list);

	_exit(1);
}

int ldx_process_wait(pid_t pid)
{
	int status;
	int ret;

	if (pid <= 0)
		return -256;

	do {
		ret = waitpid(pid, &status, 0);
	} while (ret < 0 && errno == EINTR);
	if (ret != pid)
		return -257;

	if (WIFEXITED(status))
		return WEXITSTATUS(status);
	else if (WIFSIGNALED(status))
		return -WTERMSIG(status);
	else
		return -258;
}
