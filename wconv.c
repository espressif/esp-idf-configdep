/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file wconv.c
 * @brief UTF-8 <-> wide-char string conversion helpers (Windows).
 *
 * Implements the conversion routines declared in wconv.h using the Win32
 * MultiByteToWideChar / WideCharToMultiByte APIs with membuf descriptors.
 */
#include <stringapiset.h>
#include <wchar.h>
#include <windows.h>
#include <winerror.h>

#include "membuf.h"
#include "port.h"
#include "utils.h"

/**
 * @brief Convert a UTF-8 multibyte string to a wide-char string.
 *
 * If the destination membuf is too small and @p alloc is set, the buffer is
 * grown to fit. If @p alloc is 0 and the buffer is insufficient, an error
 * is reported immediately.
 *
 * @param mbs    NUL-terminated UTF-8 source string.
 * @param wcs    Destination membuf for the wide-char result.
 * @param alloc  If non-zero, allow the membuf to be grown on demand.
 * @return Number of bytes written (including NUL), or 0 on error.
 */
static size_t mbs_to_wcs_impl(const char *mbs, struct membuf *wcs, int alloc)
{
	int rv;
	DWORD err;

	rv = MultiByteToWideChar(CP_UTF8, 0, mbs, -1, membuf_buf(wcs),
				 membuf_size(wcs) / sizeof(wchar_t));
	if (rv)
		return rv * sizeof(wchar_t);

	if (!alloc) {
		err = GetLastError();
		err_raw("MultiByteToWideChar failed (%lu)", err);
		return 0;
	}

	err = GetLastError();
	if (err != ERROR_INSUFFICIENT_BUFFER) {
		err_raw("MultiByteToWideChar failed (%lu)", err);
		return 0;
	}

	rv = MultiByteToWideChar(CP_UTF8, 0, mbs, -1, NULL, 0);
	if (!rv) {
		err_raw("MultiByteToWideChar size failed (%lu)",
			GetLastError());
		return 0;
	}

	if (membuf_grow(wcs, rv * sizeof(wchar_t)))
		return 0;

	rv = MultiByteToWideChar(CP_UTF8, 0, mbs, -1, membuf_buf(wcs), rv);
	if (!rv) {
		err = GetLastError();
		err_raw("MultiByteToWideChar for malloc buffer failed (%lu)",
			err);
		return 0;
	}

	return rv * sizeof(wchar_t);
}

/** Convert UTF-8 to wide-char, growing the membuf if needed. */
size_t mbs_to_wcs(const char *mbs, struct membuf *wcs)
{
	return mbs_to_wcs_impl(mbs, wcs, 1);
}

/** Convert UTF-8 to wide-char without allocating (fail if buffer too small). */
size_t mbs_to_wcs_noalloc(const char *mbs, struct membuf *wcs)
{
	return mbs_to_wcs_impl(mbs, wcs, 0);
}

/**
 * @brief Convert a wide-char string to a UTF-8 multibyte string.
 *
 * Mirror of mbs_to_wcs_impl in the opposite direction.
 *
 * @param wcs    NUL-terminated wide-char source string.
 * @param mbs    Destination membuf for the UTF-8 result.
 * @param alloc  If non-zero, allow the membuf to be grown on demand.
 * @return Number of bytes written (including NUL), or 0 on error.
 */
static size_t wcs_to_mbs_impl(const wchar_t *wcs, struct membuf *mbs, int alloc)
{
	int rv;
	DWORD err;

	rv = WideCharToMultiByte(CP_UTF8, 0, wcs, -1, membuf_buf(mbs),
				 membuf_size(mbs), NULL, NULL);
	if (rv)
		return rv;

	if (!alloc) {
		err = GetLastError();
		err("WideCharToMultiByte failed (%lu)", err);
		return 0;
	}

	err = GetLastError();
	if (err != ERROR_INSUFFICIENT_BUFFER) {
		err("WideCharToMultiByte failed (%lu)", err);
		return 0;
	}

	rv = WideCharToMultiByte(CP_UTF8, 0, wcs, -1, NULL, 0, NULL, NULL);
	if (!rv) {
		err = GetLastError();
		err("WideCharToMultiByte size failed (%lu)", err);
		return 0;
	}

	if (membuf_grow(mbs, rv))
		return 0;

	rv = WideCharToMultiByte(CP_UTF8, 0, wcs, -1, membuf_buf(mbs), rv, NULL,
				 NULL);
	if (!rv) {
		err = GetLastError();
		err("WideCharToMultiByte for malloc buffer failed (%lu)", err);
		return 0;
	}

	return rv;
}

/** Convert wide-char to UTF-8, growing the membuf if needed. */
size_t wcs_to_mbs(const wchar_t *wcs, struct membuf *mbs)
{
	return wcs_to_mbs_impl(wcs, mbs, 1);
}

/** Convert wide-char to UTF-8 without allocating (fail if buffer too small). */
size_t wcs_to_mbs_noalloc(const wchar_t *wcs, struct membuf *mbs)
{
	return wcs_to_mbs_impl(wcs, mbs, 0);
}
