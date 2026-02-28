/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file win.c
 * @brief Windows-specific implementations.
 *
 * This file provides:
 *  - exec_process()   -- child process creation via CreateProcessW.
 *  - UTF-8-aware wrappers for fprintf, vfprintf, fopen, and access
 *    (overriding the standard CRT functions via macros in port.h).
 *
 * All file-system and console operations go through wide-char Win32 APIs
 * so that paths containing non-ASCII characters are handled correctly.
 */
#include <errhandlingapi.h>
#include <fcntl.h>
#include <io.h>
#include <processenv.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stringapiset.h>
#include <wchar.h>
#include <windows.h>
#include <winerror.h>

#include "membuf.h"
#include "port.h"
#include "utils.h"
#include "wconv.h"

/**
 * @brief Execute the compiler command as a child process (Windows).
 *
 * Retrieves the original wide-char command line via GetCommandLineW(),
 * strips the wrapper's own executable name, and passes the remainder to
 * CreateProcessW. Waits for the child to terminate and returns its exit code.
 *
 * @param argv  Unused on Windows (command line obtained from the OS).
 * @return Exit code of the child process, or EXIT_FAILURE on error.
 */
int exec_process(char **argv)
{
#define CMDL_WC_CNT (1024 * 10)

	static wchar_t cmdl_buf[CMDL_WC_CNT];
	DEFINE_MEMBUF(cmdl, cmdl_buf, CMDL_WC_CNT * sizeof(wchar_t));

	/* Get the full command line and skip past the wrapper executable name.
	 * Handles quoted paths (e.g. "C:\Program Files\..."). */
	wchar_t *c = GetCommandLineW();

	int quotes = 0;
	while (*c && (*c != L' ' || quotes)) {
		if (*c == L'"')
			quotes = !quotes;
		c++;
	}

	/* Skip whitespace between the wrapper name and the actual command. */
	while (*c == L' ')
		c++;

	if (membuf_grow(&cmdl, (wcslen(c) + 1) * sizeof(wchar_t)))
		goto err;

	wcscpy(membuf_buf(&cmdl), c);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	DWORD exitCode;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcessW(NULL, membuf_buf(&cmdl), NULL, NULL, FALSE, 0, NULL,
			    NULL, &si, &pi)) {
		err("CreateProcess failed (%lu)", GetLastError());
		goto err;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
		err("GetExitCodeProcess failed (%lu)", GetLastError());
		exitCode = EXIT_FAILURE;
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	membuf_free(&cmdl);

	return exitCode;
err:
	membuf_free(&cmdl);
	return EXIT_FAILURE;

#undef CMDL_WC_CNT
}

/**
 * @brief UTF-8-aware vfprintf replacement for Windows.
 *
 * Formats the message into a UTF-8 buffer with vsnprintf, then either:
 *  - writes raw bytes if the stream is redirected to a file/pipe, or
 *  - converts to wide-char and uses WriteConsoleW for true console output
 *    (preserving Unicode characters).
 */
int vfprintf_w(FILE *stream, const char *fmt, va_list args)
{
#define MSG_SIZE (1024 * 2)
#define WMSG_WC_CNT (1024 * 2)

	static char msg_buf[MSG_SIZE];
	DEFINE_MEMBUF(msg, msg_buf, MSG_SIZE);

	static wchar_t wmsg_buf[WMSG_WC_CNT];
	DEFINE_MEMBUF(wmsg, wmsg_buf, WMSG_WC_CNT * sizeof(wchar_t));

	int rv = -1;
	int n;

	va_list args_copy;

	HANDLE h = (HANDLE)_get_osfhandle(_fileno(stream));
	DWORD dwMode;

	va_copy(args_copy, args);
	n = vsnprintf(membuf_buf(&msg), membuf_size(&msg), fmt, args_copy);
	va_end(args_copy);
	if (n >= membuf_size(&msg)) {
		if (membuf_grow(&msg, n + 1))
			goto err;

		va_copy(args_copy, args);
		n = vsnprintf(membuf_buf(&msg), membuf_size(&msg), fmt,
			      args_copy);
		va_end(args_copy);
	}

	if (!GetConsoleMode(h, &dwMode)) {
		rv = fwrite(membuf_buf(&msg), 1, n, stream);
		if (rv != n)
			goto err;
	} else {
		size_t wmsg_size = mbs_to_wcs(membuf_buf(&msg), &wmsg);
		if (!wmsg_size)
			goto err;

		if (!WriteConsoleW(h, membuf_buf(&wmsg),
				   wmsg_size / sizeof(wchar_t) - 1, NULL, NULL))
			goto err;
	}

	rv = n;
err:
	membuf_free(&msg);
	membuf_free(&wmsg);
	return rv;

#undef MSG_SIZE
#undef WMSG_WC_CNT
}

/** UTF-8-aware fprintf replacement. Delegates to vfprintf_w(). */
int fprintf_w(FILE *stream, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int rv = vfprintf_w(stream, fmt, args);
	va_end(args);
	return rv;
}

/**
 * @brief UTF-8-aware fopen replacement for Windows.
 *
 * Converts the UTF-8 filename and mode string to wide-char and calls
 * _wfopen, which correctly handles non-ASCII paths.
 */
FILE *fopen_w(const char *fn, const char *mode)
{
#define FN_WC_CNT (1024 * 10)
#define MODE_WC_CNT 16

	static wchar_t wfn_buf[FN_WC_CNT];
	DEFINE_MEMBUF(wfn, wfn_buf, FN_WC_CNT * sizeof(wchar_t));

	wchar_t wmode_buf[MODE_WC_CNT];
	DEFINE_MEMBUF(wmode, wmode_buf, MODE_WC_CNT * sizeof(wchar_t));

	FILE *fp = NULL;

	if (!mbs_to_wcs(fn, &wfn)) {
		errno = ENOMEM;
		goto err;
	}

	if (!mbs_to_wcs(mode, &wmode)) {
		errno = ENOMEM;
		goto err;
	}

	fp = _wfopen(membuf_buf(&wfn), membuf_buf(&wmode));
err:
	membuf_free(&wfn);
	membuf_free(&wmode);
	return fp;

#undef FN_WC_CNT
#undef MODE_WC_CNT
}

/**
 * @brief UTF-8-aware access() replacement for Windows.
 *
 * Converts the UTF-8 path to wide-char and calls _waccess.
 */
int access_w(const char *fn, int mode)
{
#define FN_WC_CNT (1024 * 10)

	static wchar_t wfn_buf[FN_WC_CNT];
	DEFINE_MEMBUF(wfn, wfn_buf, FN_WC_CNT * sizeof(wchar_t));

	int rv = -1;

	if (!mbs_to_wcs(fn, &wfn)) {
		errno = ENOMEM;
		goto err;
	}

	rv = _waccess(membuf_buf(&wfn), mode);
err:
	membuf_free(&wfn);

	return rv;

#undef FN_WC_CNT
}
