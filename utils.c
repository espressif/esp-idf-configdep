#include "utils.h"

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
