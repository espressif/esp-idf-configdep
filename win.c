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

int exec_process(char **argv)
{
#define CMDL_WC_CNT (1024 * 10)

	static wchar_t cmdl_buf[CMDL_WC_CNT];
	DEFINE_MEMBUF(cmdl, cmdl_buf, CMDL_WC_CNT * sizeof(wchar_t));

	wchar_t *c = GetCommandLineW();

	int quotes = 0;
	while (*c && (*c != L' ' || quotes)) {
		if (*c == L'"')
			quotes = !quotes;
		c++;
	}

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
		err("GetExitCodeProcess failed (%lu)", cmdl, GetLastError());
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

size_t __mbs_to_wcs(const char *mbs, struct membuf *wcs, int alloc)
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
		err_raw("MultiByteToWideChar size failed (%lu)", rv);
		return 0;
	}

	if (membuf_grow(wcs, rv))
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

size_t mbs_to_wcs(const char *mbs, struct membuf *wcs)
{
	return __mbs_to_wcs(mbs, wcs, 1);
}

size_t mbs_to_wcs_noalloc(const char *mbs, struct membuf *wcs)
{
	return __mbs_to_wcs(mbs, wcs, 0);
}

size_t __wcs_to_mbs(const wchar_t *wcs, struct membuf *mbs, int alloc)
{
	int rv;
	DWORD err;

	rv = WideCharToMultiByte(CP_UTF8, 0, wcs, -1, membuf_buf(mbs),
				 membuf_size(mbs), NULL, NULL);
	if (rv)
		return rv;

	if (!alloc) {
		err = GetLastError();
		err_raw("MultiByteToWideChar failed (%lu)", err);
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

size_t wcs_to_mbs(const wchar_t *wcs, struct membuf *mbs)
{
	return __wcs_to_mbs(wcs, mbs, 1);
}

size_t wcs_to_mbs_noalloc(const wchar_t *wcs, struct membuf *mbs)
{
	return __wcs_to_mbs(wcs, mbs, 0);
}

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
	n = vsnprintf(membuf_buf(&msg), membuf_size(&msg), fmt, args);
	va_end(args_copy);
	if (n >= membuf_size(&msg)) {
		if (membuf_grow(&msg, n + 1))
			goto err;

		va_copy(args_copy, args);
		n = vsnprintf(membuf_buf(&msg), membuf_size(&msg), fmt, args);
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

int fprintf_w(FILE *stream, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int rv = vfprintf_w(stream, fmt, args);
	va_end(args);
	return rv;
}

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

int wmain(int argc, wchar_t **wargv)
{
#define ARGS_BUF_SIZE (1024 * 10 * 4)
#define ARGV_BUF_SIZE (1024)

	static char args_buf[ARGS_BUF_SIZE];
	DEFINE_MEMBUF(args_mb, args_buf, ARGS_BUF_SIZE);
	char *args;

	static char *argv_buf[ARGV_BUF_SIZE];
	DEFINE_MEMBUF(argv_mb, argv_buf, ARGS_BUF_SIZE);
	char **argv;

	int rv = EXIT_FAILURE;

	if (membuf_grow(&argv_mb, argc + 1)) {
		err_errno("argv malloc failed");
		goto err;
	}

	if (membuf_grow(&args_mb, wcslen(GetCommandLineW()) * 4 + argc)) {
		err("args malloc failed");
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
