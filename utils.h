/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file utils.h
 * @brief Common utility declarations: error reporting and process execution.
 *
 * Provides printf-style error macros that automatically include the source
 * file, line number, and function name. Two variants exist:
 *  - err() / err_errno()           -- use the (potentially overridden)
 *                                     fprintf/vfprintf, suitable for normal
 *                                     operation on all platforms.
 *  - err_raw() / err_errno_raw()   -- use the original C-library fprintf
 *                                     and vfprintf directly (Windows only;
 *                                     avoids re-entrant UTF-8 conversion
 *                                     inside the conversion helpers).
 */
#ifndef UTILS_H
#define UTILS_H

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port.h"

/**
 * @brief Execute the compiler command passed in @p argv (platform-specific).
 *
 * On POSIX this forks + execvp; on Windows it uses CreateProcessW.
 * argv[0] is the wrapper itself; the actual command starts at argv[1].
 *
 * @param argv  Full argument vector (argv[0] = wrapper, argv[1..] = command).
 * @return Exit code of the child process, or EXIT_FAILURE on error.
 */
int exec_process(char **argv);

/**
 * @brief Core error-reporting function (not called directly -- use macros).
 *
 * Formats a diagnostic message to stderr with location information.
 * If @p add_errno is non-zero, the current errno string is appended.
 *
 * @param file       Source file name (__FILE__).
 * @param line       Source line number (__LINE__).
 * @param func       Function name (__func__).
 * @param add_errno  If non-zero, append strerror(errno) to the message.
 * @param _f         fprintf-compatible function pointer for formatted output.
 * @param _vf        vfprintf-compatible function pointer for va_list output.
 * @param fmt        printf-style format string.
 */
void err_msg(char *file, int line, const char *func, int add_errno,
	     int (*_f)(FILE *, const char *, ...),
	     int (*_vf)(FILE *, const char *, va_list), char *fmt, ...);

/** Log an error message with source location (no errno). */
#define err(...)                                                               \
	err_msg(__FILE__, __LINE__, __func__, 0, fprintf, vfprintf, __VA_ARGS__)

/** Log an error message with source location and errno description. */
#define err_errno(...)                                                         \
	err_msg(__FILE__, __LINE__, __func__, 1, fprintf, vfprintf, __VA_ARGS__)

/** Log an error using raw (non-overridden) fprintf (Windows-safe). */
#define err_raw(...)                                                           \
	err_msg(__FILE__, __LINE__, __func__, 0, fprintf_raw, vfprintf_raw,    \
		__VA_ARGS__)

/** Log an error with errno using raw fprintf (Windows-safe). */
#define err_errno_raw(...)                                                     \
	err_msg(__FILE__, __LINE__, __func__, 1, fprintf_raw, vfprintf_raw,    \
		__VA_ARGS__)

/**
 * @brief Create an empty file at @p path, creating parent directories as
 * needed.
 *
 * @param path  File path to create (must be a modifiable copy).
 * @return 0 on success, -1 on error.
 */
int touch_file(char *path);

/**
 * @brief Whole-second file timestamps (access and modification time).
 *
 * Wide enough to hold both a POSIX @c time_t and the 64-bit Windows
 * @c __time64_t without truncation.
 */
struct file_times {
	long long atime; /**< Access time, seconds since the epoch. */
	long long mtime; /**< Modification time, seconds since the epoch. */
};

/**
 * @brief Read the access and modification times of @p path into @p ft.
 *
 * A missing @p path reports @c ENOENT via errno without logging, since callers
 * may treat its absence as a non-fatal, best-effort condition; other errors
 * are logged.
 *
 * @param path  File to read the timestamps of.
 * @param ft    Out: timestamps of @p path on success.
 * @return 0 on success, -1 on error.
 */
int get_file_times(const char *path, struct file_times *ft);

/**
 * @brief Apply the access and modification times in @p ft to @p path.
 *
 * Used to backdate a freshly created .cdep stub to auto.conf's mtime so the
 * stub is never newer than the object files compiled in the same build,
 * avoiding a one-shot spurious rebuild on the next ninja run. Whole-second
 * precision is sufficient: auto.conf is written before any compilation, so
 * its (floored) mtime is always <= every object mtime.
 *
 * auto.conf's timestamps are read once (get_file_times) and applied to every
 * stub created in a build, so it is stat'd a single time per invocation.
 *
 * @param path  File whose timestamps are to be set.
 * @param ft    Timestamps to apply.
 * @return 0 on success, -1 on error.
 */
int set_file_times(const char *path, const struct file_times *ft);

#endif /* UTILS_H */
