/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file port.h
 * @brief Platform abstraction layer for POSIX / Windows differences.
 *
 * On Windows (_WIN32):
 *  - Provides a ssize_t typedef (not part of the Windows CRT).
 *  - Overrides fprintf, printf, vfprintf, fopen, and access with UTF-8-aware
 *    wrappers (implemented in win.c) so that file paths containing non-ASCII
 *    characters work correctly.
 *  - Keeps pointers to the original C-library fprintf/vfprintf as
 *    fprintf_raw / vfprintf_raw for use in contexts where calling the
 *    overridden versions would cause re-entrancy (e.g. inside the
 *    UTF-8 conversion helpers themselves).
 *  - Defines EOL as "\r\n" for Windows-native dependency files.
 *
 * On POSIX:
 *  - Defines EOL as "\n".
 *  - Includes <unistd.h> for access() / F_OK.
 */
#ifndef _PORT_H_
#define _PORT_H_

#ifdef _WIN32

typedef ptrdiff_t ssize_t;

/* Forward-declare main so that wmain (the Windows wide-char entry point
 * implemented in win.c) can call it after converting arguments to UTF-8. */
int main(int, char **);

/* UTF-8-aware replacements for standard I/O and filesystem functions. */
int fprintf_w(FILE *, const char *, ...);
int vfprintf_w(FILE *, const char *, va_list);
FILE *fopen_w(const char *, const char *);
int access_w(const char *, int);

/**
 * Raw (non-overridden) fprintf and vfprintf function pointers.
 * Used by err_raw / err_errno_raw to avoid re-entrancy during
 * UTF-8 <-> wide-char conversions on Windows.
 */
extern int (*fprintf_raw)(FILE *, const char *, ...);
extern int (*vfprintf_raw)(FILE *, const char *, va_list);

/* Override standard I/O functions with UTF-8-aware versions. */
#define fprintf fprintf_w
#define printf(...) fprintf_w(stdout, __VA_ARGS__)
#define vfprintf vfprintf_w
#define vprintf(fmt, args) vfprintf_w(stdout, fmt, args)

#define F_OK 0
#define access access_w

#define fopen fopen_w

/** End-of-line sequence for dependency files (CRLF on Windows). */
#define EOL "\r\n"

#else // _WIN32

/** End-of-line sequence for dependency files (LF on POSIX). */
#define EOL "\n"

#include <unistd.h>

#endif // _WIN32

#endif // _PORT_H_
