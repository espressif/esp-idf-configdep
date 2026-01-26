#include <stdarg.h>
#include <stdio.h>

int (*fprintf_raw)(FILE *, const char *, ...) = fprintf;
int (*vfprintf_raw)(FILE *, const char *, va_list) = vfprintf;
