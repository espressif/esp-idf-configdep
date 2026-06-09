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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utime.h>

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

/**
 * @brief Read @p path's timestamps (POSIX). See get_file_times() in utils.h.
 *
 * Uses stat() (whole-second precision), portable across glibc, musl and macOS
 * without feature-test-macro juggling.
 */
int get_file_times(const char *path, struct file_times *ft)
{
	struct stat st;

	if (stat(path, &st)) {
		if (errno != ENOENT)
			err_errno("stat '%s'", path);
		return -1;
	}

	ft->atime = st.st_atime;
	ft->mtime = st.st_mtime;

	return 0;
}

/**
 * @brief Apply @p ft's timestamps to @p path (POSIX). See set_file_times() in
 * utils.h. Uses utime() (whole-second precision).
 */
int set_file_times(const char *path, const struct file_times *ft)
{
	struct utimbuf ut;

	ut.actime = (time_t)ft->atime;
	ut.modtime = (time_t)ft->mtime;

	if (utime(path, &ut)) {
		err_errno("utime '%s'", path);
		return -1;
	}

	return 0;
}
