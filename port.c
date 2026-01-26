/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdarg.h>
#include <stdio.h>

int (*fprintf_raw)(FILE *, const char *, ...) = fprintf;
int (*vfprintf_raw)(FILE *, const char *, va_list) = vfprintf;
