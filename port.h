#ifndef _PORT_H_
#define _PORT_H_

#ifdef _WIN32

typedef ptrdiff_t ssize_t;

int main(int, char **);
int fprintf_w(FILE *, const char *, ...);
int vfprintf_w(FILE *, const char *, va_list);
FILE *fopen_w(const char *, const char *);
int access_w(const char *, int);

extern int (*fprintf_raw)(FILE *, const char *, ...);
extern int (*vfprintf_raw)(FILE *, const char *, va_list);

#define fprintf fprintf_w
#define printf(...) fprintf_w(stdout, __VA_ARGS__)
#define vfprintf vfprintf_w
#define vprintf(fmt, args) vfprintf_w(stdout, fmt, args)

#define F_OK 0
#define access access_w

#define fopen fopen_w

#define access access_w

#define EOL "\r\n"

#else // _WIN32

#define EOL "\n"

#include <unistd.h>

#endif // _WIN32

#endif // _PORT_H_
