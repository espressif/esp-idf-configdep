/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file port.c
 * @brief Stores raw C-library fprintf/vfprintf function pointers that are
 *        not affected by port.h macro overrides.
 *
 * This file intentionally does not include port.h, so the initializers
 * capture the real C-library function addresses rather than any macro-based
 * wrappers. On Windows, these are used by err_raw() / err_errno_raw() to
 * print error messages without going through the UTF-8 conversion layer.
 */
#include <stdarg.h>
#include <stdio.h>

/** Original C-library fprintf, captured without port.h macro overrides. */
int (*fprintf_raw)(FILE *, const char *, ...) = fprintf;

/** Original C-library vfprintf, captured without port.h macro overrides. */
int (*vfprintf_raw)(FILE *, const char *, va_list) = vfprintf;
