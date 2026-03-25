/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file utils.c
 * @brief Common utility implementations shared across all platforms.
 */
#include "utils.h"

/**
 * @brief Format and print a diagnostic message to stderr.
 *
 * Output format:  file:line: in func: <user message>[: strerror (errno)]\n
 *
 * The fprintf/vfprintf function pointers allow the caller to choose between
 * the platform-overridden versions (which handle UTF-8 on Windows consoles)
 * and the raw C-library versions (used inside the Windows conversion helpers
 * to avoid re-entrancy).
 */
void err_msg(char *file, int line, const char *func, int add_errno,
	     int (*_f)(FILE *, const char *, ...),
	     int (*_vf)(FILE *, const char *, va_list), char *fmt, ...)
{
	va_list ap;

	_f(stderr, "%s:%d: in %s: ", file, line, func);

	va_start(ap, fmt);
	_vf(stderr, fmt, ap);
	va_end(ap);

	if (add_errno)
		_f(stderr, ": %s (%d)\n", strerror(errno), errno);
	else
		_f(stderr, "\n");
}

/**
 * @brief Create parent directories for @p path, then create an empty file.
 *
 * The path buffer is temporarily modified (separators replaced with NUL)
 * during directory creation, then restored.
 */
int touch_file(char *path)
{
	FILE *fp;
	char *s;

	for (s = path; *s; s++) {
		if (*s != '/')
			continue;
		if (s == path)
			continue;
		*s = '\0';
		(void)mkdir(path, 0755);
		*s = '/';
	}

	fp = fopen(path, "wb");
	if (!fp) {
		err_errno("cannot create '%s'", path);
		return -1;
	}
	if (fclose(fp)) {
		err_errno("cannot close '%s'", path);
		return -1;
	}
	return 0;
}
