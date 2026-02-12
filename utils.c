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
