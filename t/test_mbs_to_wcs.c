/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Unit tests for mbs_to_wcs / wcs_to_mbs — outputs TAP (Test Anything
 * Protocol).
 *
 * Compile: $CC -std=c99 -I. -It -o test_mbs_to_wcs t/test_mbs_to_wcs.c
 *          membuf.c utils.c port.c wconv.c
 * Run:     ./test_mbs_to_wcs
 */

#include <wchar.h>

#include "membuf.h"
#include "tap.h"
#include "wconv.h"

int main(void)
{
	/* ---- mbs_to_wcs: ASCII ---- */
	{
		wchar_t buf[32];
		DEFINE_MEMBUF(wcs, buf, sizeof(buf));
		size_t n = mbs_to_wcs("hello", &wcs);
		tap_check(n == 6 * sizeof(wchar_t));
		tap_check(wcscmp(membuf_buf(&wcs), L"hello") == 0);
		tap_done("mbs_to_wcs converts ASCII string");
	}

	/* ---- mbs_to_wcs: empty string ---- */
	{
		wchar_t buf[8];
		DEFINE_MEMBUF(wcs, buf, sizeof(buf));
		size_t n = mbs_to_wcs("", &wcs);
		tap_check(n == 1 * sizeof(wchar_t));
		tap_check(wcscmp(membuf_buf(&wcs), L"") == 0);
		tap_done("mbs_to_wcs converts empty string");
	}

	/* ---- mbs_to_wcs: UTF-8 multibyte ---- */
	{
		/* "café" = 63 61 66 c3 a9 (5 bytes UTF-8, 4 wide chars + NUL)
		 */
		wchar_t buf[32];
		DEFINE_MEMBUF(wcs, buf, sizeof(buf));
		size_t n = mbs_to_wcs("caf\xc3\xa9", &wcs);
		tap_check(n == 5 * sizeof(wchar_t));
		tap_check(wcscmp(membuf_buf(&wcs), L"caf\x00e9") == 0);
		tap_done("mbs_to_wcs converts UTF-8 multibyte");
	}

	/* ---- mbs_to_wcs: grows buffer when too small ---- */
	{
		DEFINE_MEMBUF_EMPTY(wcs);
		size_t n = mbs_to_wcs("grow me", &wcs);
		tap_check(n == 8 * sizeof(wchar_t));
		tap_check(membuf_is_allocated(&wcs));
		tap_check(wcscmp(membuf_buf(&wcs), L"grow me") == 0);
		membuf_free(&wcs);
		tap_done("mbs_to_wcs grows empty buffer");
	}
	{
		wchar_t tiny[2];
		DEFINE_MEMBUF(wcs, tiny, sizeof(tiny));
		size_t n = mbs_to_wcs("longer string", &wcs);
		tap_check(n == 14 * sizeof(wchar_t));
		tap_check(membuf_is_allocated(&wcs));
		tap_check(wcscmp(membuf_buf(&wcs), L"longer string") == 0);
		membuf_free(&wcs);
		tap_done("mbs_to_wcs grows insufficient buffer");
	}

	/* ---- mbs_to_wcs_noalloc: sufficient buffer ---- */
	{
		wchar_t buf[32];
		DEFINE_MEMBUF(wcs, buf, sizeof(buf));
		size_t n = mbs_to_wcs_noalloc("test", &wcs);
		tap_check(n == 5 * sizeof(wchar_t));
		tap_check(wcscmp(membuf_buf(&wcs), L"test") == 0);
		tap_check(!membuf_is_allocated(&wcs));
		tap_done("mbs_to_wcs_noalloc converts with sufficient buffer");
	}

	/* ---- mbs_to_wcs_noalloc: insufficient buffer ---- */
	{
		wchar_t tiny[2];
		DEFINE_MEMBUF(wcs, tiny, sizeof(tiny));
		size_t n = mbs_to_wcs_noalloc("longer string", &wcs);
		tap_check(n == 0);
		tap_check(!membuf_is_allocated(&wcs));
		tap_done("mbs_to_wcs_noalloc fails on insufficient buffer");
	}

	/* ---- wcs_to_mbs: ASCII ---- */
	{
		char buf[32];
		DEFINE_MEMBUF(mbs, buf, sizeof(buf));
		size_t n = wcs_to_mbs(L"hello", &mbs);
		tap_check(n == 6);
		tap_check(strcmp(membuf_buf(&mbs), "hello") == 0);
		tap_done("wcs_to_mbs converts ASCII wide string");
	}

	/* ---- wcs_to_mbs: empty string ---- */
	{
		char buf[8];
		DEFINE_MEMBUF(mbs, buf, sizeof(buf));
		size_t n = wcs_to_mbs(L"", &mbs);
		tap_check(n == 1);
		tap_check(strcmp(membuf_buf(&mbs), "") == 0);
		tap_done("wcs_to_mbs converts empty wide string");
	}

	/* ---- wcs_to_mbs: UTF-8 multibyte ---- */
	{
		char buf[32];
		DEFINE_MEMBUF(mbs, buf, sizeof(buf));
		size_t n = wcs_to_mbs(L"caf\x00e9", &mbs);
		tap_check(n == 6);
		tap_check(strcmp(membuf_buf(&mbs), "caf\xc3\xa9") == 0);
		tap_done("wcs_to_mbs converts to UTF-8 multibyte");
	}

	/* ---- wcs_to_mbs: grows buffer ---- */
	{
		DEFINE_MEMBUF_EMPTY(mbs);
		size_t n = wcs_to_mbs(L"grow me", &mbs);
		tap_check(n == 8);
		tap_check(membuf_is_allocated(&mbs));
		tap_check(strcmp(membuf_buf(&mbs), "grow me") == 0);
		membuf_free(&mbs);
		tap_done("wcs_to_mbs grows empty buffer");
	}

	/* ---- wcs_to_mbs_noalloc: sufficient buffer ---- */
	{
		char buf[32];
		DEFINE_MEMBUF(mbs, buf, sizeof(buf));
		size_t n = wcs_to_mbs_noalloc(L"test", &mbs);
		tap_check(n == 5);
		tap_check(strcmp(membuf_buf(&mbs), "test") == 0);
		tap_check(!membuf_is_allocated(&mbs));
		tap_done("wcs_to_mbs_noalloc converts with sufficient buffer");
	}

	/* ---- wcs_to_mbs_noalloc: insufficient buffer ---- */
	{
		char tiny[2];
		DEFINE_MEMBUF(mbs, tiny, sizeof(tiny));
		size_t n = wcs_to_mbs_noalloc(L"longer string", &mbs);
		tap_check(n == 0);
		tap_check(!membuf_is_allocated(&mbs));
		tap_done("wcs_to_mbs_noalloc fails on insufficient buffer");
	}

	/* ---- round-trip: mbs -> wcs -> mbs ---- */
	{
		const char *original = "caf\xc3\xa9 \xc3\xa0 la cr\xc3\xa8me";

		wchar_t wbuf[64];
		DEFINE_MEMBUF(wcs, wbuf, sizeof(wbuf));
		size_t wn = mbs_to_wcs(original, &wcs);
		tap_check(wn > 0);

		char mbuf[64];
		DEFINE_MEMBUF(mbs, mbuf, sizeof(mbuf));
		size_t mn = wcs_to_mbs(membuf_buf(&wcs), &mbs);
		tap_check(mn > 0);
		tap_check(strcmp(membuf_buf(&mbs), original) == 0);
		tap_done("round-trip mbs->wcs->mbs preserves UTF-8");
	}

	return tap_result();
}
