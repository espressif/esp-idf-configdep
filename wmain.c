/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file wmain.c
 * @brief Windows wide-char entry point.
 *
 * Provides wmain() which converts the wide-char argument vector to UTF-8
 * and delegates to main() (defined in configdep.c).  Separated from win.c
 * so that the extern main declaration does not leak into translation units
 * that define their own main (e.g. unit tests including membuf.h).
 *
 * Linked with -municode so that the CRT calls wmain() instead of main().
 */
#include <processenv.h>
#include <wchar.h>

#include "membuf.h"
#include "wconv.h"

extern int main(int argc, char **argv);

/**
 * @brief Windows wide-char entry point.
 *
 * Converts the wide-char argument vector to UTF-8 strings and delegates
 * to main(). This is the true entry point on Windows (linked with
 * -municode); main() is defined in configdep.c.
 */
int wmain(int argc, wchar_t **wargv)
{
#define ARGS_BUF_SIZE (1024 * 10 * 4)
#define ARGV_BUF_SIZE (1024)

	static char args_buf[ARGS_BUF_SIZE];
	DEFINE_MEMBUF(args_mb, args_buf, ARGS_BUF_SIZE);
	char *args;

	static char *argv_buf[ARGV_BUF_SIZE];
	DEFINE_MEMBUF(argv_mb, argv_buf, ARGV_BUF_SIZE * sizeof(char *));
	char **argv;

	int rv = EXIT_FAILURE;

	if (membuf_grow(&argv_mb, (argc + 1) * sizeof(char *))) {
		err_errno("argv malloc failed");
		goto err;
	}

	if (membuf_grow(&args_mb, wcslen(GetCommandLineW()) * 4 + argc)) {
		err_errno("args malloc failed");
		goto err;
	}

	args = membuf_buf(&args_mb);
	argv = membuf_buf(&argv_mb);

	size_t remaining = membuf_size(&args_mb);
	size_t offset = 0;
	size_t cnt = 0;
	DEFINE_MEMBUF_EMPTY(arg_mb);
	for (int i = 0; i < argc; i++) {
		if (membuf_init(&arg_mb, args + offset, remaining)) {
			err("arg buffer initialization failed");
			goto err;
		}

		cnt = wcs_to_mbs_noalloc(wargv[i], &arg_mb);
		if (!cnt) {
			err("args conversion failed");
			goto err;
		}

		argv[i] = membuf_buf(&arg_mb);
		offset += cnt;
		remaining -= cnt;
	}

	argv[argc] = NULL;

	rv = main(argc, argv);
err:
	membuf_free(&args_mb);
	membuf_free(&argv_mb);
	return rv;

#undef ARGS_BUF_SIZE
#undef ARGV_BUF_SIZE
}
