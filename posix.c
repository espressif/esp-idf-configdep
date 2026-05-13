/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file posix.c
 * @brief POSIX implementation of process execution.
 *
 * Provides exec_process() for POSIX systems using fork()/execvp()/waitpid().
 */
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "utils.h"

/**
 * @brief Fork and exec the compiler command, then wait for it to finish.
 *
 * argv[0] is the configdep wrapper itself; the actual compiler command
 * and its arguments start at argv[1]. The child process replaces itself
 * with execvp(argv[1], &argv[1]).
 *
 * @param argv  Full argument vector (argv[0] = wrapper).
 * @return Exit code of the child process, signal number if killed,
 *         0 if status is indeterminate, or EXIT_FAILURE on fork/wait error.
 */
int exec_process(char **argv)
{
	pid_t pid;
	int status;

	pid = fork();
	if (pid == -1) {
		err_errno("fork");
		return EXIT_FAILURE;
	}

	/* Child: replace this process image with the real compiler. */
	if (pid == 0) {
		if (execvp(argv[1], &argv[1]) == -1) {
			err_errno("execvp: '%s'", argv[1]);
			exit(EXIT_FAILURE);
		}
	}

	/* Parent: wait for the child to terminate. */
	if (waitpid(pid, &status, 0) == -1) {
		err_errno("waitpid: '%s'", argv[1]);
		return EXIT_FAILURE;
	}

	if (WIFEXITED(status))
		return WEXITSTATUS(status);

	if (WIFSIGNALED(status))
		return WTERMSIG(status);

	return 0;
}
