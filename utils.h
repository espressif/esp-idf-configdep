#ifndef _UTILS_H_
#define _UTILS_H_

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port.h"

int exec_process(char **argv);
void err_msg(char *file, int line, const char *func, int add_errno,
	     int (*_f)(FILE *, const char *, ...),
	     int (*_vf)(FILE *, const char *, va_list), char *fmt, ...);

#define err(...)                                                               \
	err_msg(__FILE__, __LINE__, __func__, 0, fprintf, vfprintf, __VA_ARGS__)
#define err_errno(...)                                                         \
	err_msg(__FILE__, __LINE__, __func__, 1, fprintf, vfprintf, __VA_ARGS__)
#define err_raw(...)                                                           \
	err_msg(__FILE__, __LINE__, __func__, 0, fprintf_raw, vfprintf_raw,    \
		__VA_ARGS__)
#define err_errno_raw(...)                                                     \
	err_msg(__FILE__, __LINE__, __func__, 1, fprintf_raw, vfprintf_raw,    \
		__VA_ARGS__)

#endif /* _UTILS_H_ */
