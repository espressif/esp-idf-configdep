/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Unit tests for membuf.c — outputs TAP (Test Anything Protocol).
 *
 * Compile: $CC -std=c99 -I. -It -o test_membuf t/test_membuf.c membuf.c utils.c
 * port.c Run:     ./test_membuf
 */

#include "membuf.h"
#include "tap.h"

int main(void)
{
	/* ---- membuf_init ---- */
	{
		char data[] = "hello";
		struct membuf mb;
		tap_check(membuf_init(&mb, data, 5) == 0);
		tap_check(membuf_buf(&mb) == data);
		tap_check(membuf_size(&mb) == 5);
		tap_check(!membuf_is_allocated(&mb));
		tap_done("membuf_init wraps pointer and size");
	}
	{
		struct membuf mb;
		tap_check(membuf_init(&mb, NULL, MEMBUF_MAX_SIZE + 1) == -1);
		tap_done("membuf_init rejects oversized buffer");
	}

	/* ---- membuf_init_str ---- */
	{
		char str1[] = "hello world";
		struct membuf mb1;
		tap_check(membuf_init_str(&mb1, str1) == 0);
		tap_check(membuf_size(&mb1) == 11);

		char str2[] = "";
		struct membuf mb2;
		tap_check(membuf_init_str(&mb2, str2) == 0);
		tap_check(membuf_size(&mb2) == 0);
		tap_done("membuf_init_str wraps C strings");
	}

	/* ---- membuf_init_alloc ---- */
	{
		struct membuf mb;
		tap_check(membuf_init_alloc(&mb, 100) == 0);
		tap_check(membuf_buf(&mb) != NULL);
		tap_check(membuf_size(&mb) == 100);
		tap_check(membuf_is_allocated(&mb));
		membuf_free(&mb);
		tap_done("membuf_init_alloc allocates heap buffer");
	}
	{
		struct membuf mb;
		tap_check(membuf_init_alloc(&mb, MEMBUF_MAX_SIZE + 1) == -1);
		tap_done("membuf_init_alloc rejects size overflow");
	}

	/* ---- membuf_grow ---- */
	{
		char data[64];
		struct membuf mb;
		membuf_init(&mb, data, 64);
		tap_check(membuf_grow(&mb, 32) == 0);
		tap_check(membuf_buf(&mb) == data);
		tap_check(membuf_size(&mb) == 64);
		tap_check(!membuf_is_allocated(&mb));
		tap_done("membuf_grow is no-op when sufficient");
	}
	{
		char data[16];
		struct membuf mb;
		membuf_init(&mb, data, 16);
		tap_check(membuf_grow(&mb, 128) == 0);
		tap_check(membuf_size(&mb) == 128);
		tap_check(membuf_is_allocated(&mb));
		membuf_free(&mb);
		tap_done("membuf_grow allocates larger buffer");
	}

	/* ---- membuf_realloc ---- */
	{
		struct membuf mb;
		membuf_init_alloc(&mb, 64);
		void *orig = membuf_buf(&mb);
		tap_check(membuf_realloc(&mb, 32) == 0);
		tap_check(membuf_size(&mb) == 64);
		tap_check(membuf_buf(&mb) == orig);
		membuf_free(&mb);
		tap_done("membuf_realloc is no-op when sufficient");
	}
	{
		struct membuf mb;
		membuf_init_alloc(&mb, 16);
		memcpy(membuf_buf(&mb), "preserved data!", 15);
		tap_check(membuf_realloc(&mb, 64) == 0);
		tap_check(memcmp(membuf_buf(&mb), "preserved data!", 15) == 0);
		tap_check(membuf_size(&mb) == 64);
		membuf_free(&mb);
		tap_done("membuf_realloc preserves content");
	}
	{
		struct membuf mb;
		membuf_init_alloc(&mb, 16);
		tap_check(membuf_realloc(&mb, MEMBUF_MAX_SIZE + 1) == -1);
		membuf_free(&mb);
		tap_done("membuf_realloc rejects size overflow");
	}

	/* ---- membuf_free ---- */
	{
		struct membuf mb;
		membuf_init_alloc(&mb, 64);
		membuf_free(&mb);
		tap_check(membuf_buf(&mb) == NULL);
		tap_check(membuf_size(&mb) == 0);
		tap_check(!membuf_is_allocated(&mb));

		char data[16];
		struct membuf mb2;
		membuf_init(&mb2, data, 16);
		membuf_free(&mb2);
		tap_check(membuf_buf(&mb2) == NULL);
		tap_check(membuf_size(&mb2) == 0);
		tap_done("membuf_free resets buffer state");
	}

	/* ---- membuf_empty ---- */
	{
		DEFINE_MEMBUF_EMPTY(mb_null);
		tap_check(membuf_empty(&mb_null));

		char data[1];
		struct membuf mb_zero;
		membuf_init(&mb_zero, data, 0);
		tap_check(membuf_empty(&mb_zero));

		struct membuf mb_valid;
		membuf_init(&mb_valid, data, 1);
		tap_check(!membuf_empty(&mb_valid));
		tap_done("membuf_empty detects empty and non-empty");
	}

	/* ---- membuf_lower ---- */
	{
		char data1[] = "Hello WORLD 123";
		DEFINE_MEMBUF(mb1, data1, 15);
		membuf_lower(&mb1);
		tap_check(memcmp(data1, "hello world 123", 15) == 0);

		char data2[] = "!@#456";
		DEFINE_MEMBUF(mb2, data2, 6);
		membuf_lower(&mb2);
		tap_check(memcmp(data2, "!@#456", 6) == 0);
		tap_done("membuf_lower converts ASCII to lowercase");
	}

	/* ---- membuf_replace ---- */
	{
		char data1[] = "a-b-c-d";
		DEFINE_MEMBUF(mb1, data1, 7);
		membuf_replace(&mb1, '-', '_');
		tap_check(memcmp(data1, "a_b_c_d", 7) == 0);

		char data2[] = "abcd";
		DEFINE_MEMBUF(mb2, data2, 4);
		membuf_replace(&mb2, 'x', 'y');
		tap_check(memcmp(data2, "abcd", 4) == 0);
		tap_done("membuf_replace replaces characters");
	}

	/* ---- membuf_chr ---- */
	{
		char data[] = "abcdef";
		DEFINE_MEMBUF(mb, data, 6);
		tap_check(membuf_chr(&mb, 'd') == data + 3);
		tap_check(membuf_chr(&mb, 'z') == NULL);
		tap_done("membuf_chr finds first occurrence");
	}

	/* ---- membuf_rchr ---- */
	{
		char data[] = "abcabc";
		DEFINE_MEMBUF(mb, data, 6);
		tap_check(membuf_rchr(&mb, 'a') == data + 3);
		tap_check(membuf_rchr(&mb, 'z') == NULL);
		tap_done("membuf_rchr finds last occurrence");
	}

	/* ---- membuf_endswith ---- */
	{
		char data1[] = "hello.txt";
		DEFINE_MEMBUF(mb1, data1, 9);
		tap_check(membuf_endswith(&mb1, ".txt") == data1 + 5);
		tap_check(membuf_endswith(&mb1, ".csv") == NULL);

		char data2[] = "hi";
		DEFINE_MEMBUF(mb2, data2, 2);
		tap_check(membuf_endswith(&mb2, "longer") == NULL);

		char data3[] = "exact";
		DEFINE_MEMBUF(mb3, data3, 5);
		tap_check(membuf_endswith(&mb3, "exact") == data3);
		tap_done("membuf_endswith checks suffix correctly");
	}

	/* ---- membuf_cmp ---- */
	{
		DEFINE_MEMBUF_STR(a, "abc");
		DEFINE_MEMBUF_STR(b, "abc");
		tap_check(membuf_cmp(&a, &b) == 0);

		DEFINE_MEMBUF_STR(c, "ab");
		tap_check(membuf_cmp(&c, &b) < 0);
		tap_check(membuf_cmp(&b, &c) > 0);

		DEFINE_MEMBUF_STR(d, "abd");
		tap_check(membuf_cmp(&a, &d) < 0);
		tap_done("membuf_cmp compares buffers correctly");
	}

	/* ---- membuf_fread ---- */
	{
		FILE *fp = tmpfile();
		tap_check(fp != NULL);
		if (fp) {
			fputs("file content", fp);
			rewind(fp);
			DEFINE_MEMBUF_EMPTY(mb);
			ssize_t n = membuf_fread(&mb, fp);
			tap_check(n == 12);
			tap_check(memcmp(membuf_buf(&mb), "file content", 12) ==
				  0);
			membuf_free(&mb);
			fclose(fp);
		}
		tap_done("membuf_fread reads entire file");
	}
	{
		FILE *fp = tmpfile();
		tap_check(fp != NULL);
		if (fp) {
			DEFINE_MEMBUF_EMPTY(mb);
			tap_check(membuf_fread(&mb, fp) == 0);
			membuf_free(&mb);
			fclose(fp);
		}
		tap_done("membuf_fread handles empty file");
	}

	/* ---- membuf_fwrite ---- */
	{
		char src[] = "output data";
		DEFINE_MEMBUF(mb, src, 11);
		FILE *fp = tmpfile();
		tap_check(fp != NULL);
		if (fp) {
			tap_check(membuf_fwrite(&mb, fp) == 11);
			rewind(fp);
			char buf[16];
			size_t got = fread(buf, 1, 11, fp);
			tap_check(got == 11);
			tap_check(memcmp(buf, "output data", 11) == 0);
			fclose(fp);
		}
		tap_done("membuf_fwrite writes and verifies content");
	}

	/* ---- membuf_cat ---- */
	{
		DEFINE_MEMBUF_STR(a, "Hello");
		DEFINE_MEMBUF_STR(b, " ");
		DEFINE_MEMBUF_STR(c, "World");
		DEFINE_MEMBUF_EMPTY(dst);
		ssize_t n = membuf_cat(&dst, &a, &b, &c);
		tap_check(n == 11);
		tap_check(memcmp(membuf_buf(&dst), "Hello World", 11) == 0);
		tap_check(*((char *)membuf_buf(&dst) + 11) == '\0');
		membuf_free(&dst);
		tap_done("membuf_cat concatenates buffers with NUL terminator");
	}
	{
		DEFINE_MEMBUF_STR(a, "hello");
		DEFINE_MEMBUF_EMPTY(empty);
		DEFINE_MEMBUF_EMPTY(dst);
		ssize_t n = membuf_cat(&dst, &a, &empty);
		tap_check(n == 5);
		tap_check(memcmp(membuf_buf(&dst), "hello", 5) == 0);
		membuf_free(&dst);
		tap_done("membuf_cat handles empty source");
	}

	/* ---- Macros ---- */
	{
		DEFINE_MEMBUF_STR(mb_str, "test");
		tap_check(membuf_size(&mb_str) == 4);

		DEFINE_MEMBUF_EMPTY(mb_empty);
		tap_check(membuf_buf(&mb_empty) == NULL);
		tap_check(membuf_size(&mb_empty) == 0);

		char data[] = "abcd";
		DEFINE_MEMBUF(mb, data, 4);
		tap_check(membuf_end(&mb) == data + 4);
		tap_done("DEFINE_MEMBUF macros initialize correctly");
	}
	{
		struct membuf mb;
		mb.buf = NULL;
		mb.size = 0;
		tap_check(!membuf_is_allocated(&mb));
		membuf_set_allocated(&mb);
		tap_check(membuf_is_allocated(&mb));
		membuf_clear_allocated(&mb);
		tap_check(!membuf_is_allocated(&mb));
		tap_done("allocation flag set/clear round-trip");
	}

	return tap_result();
}
