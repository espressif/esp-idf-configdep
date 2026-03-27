/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file wconv.h
 * @brief UTF-8 <-> wide-char string conversion helpers (Windows).
 *
 * Thin wrappers around MultiByteToWideChar / WideCharToMultiByte that
 * operate on membuf descriptors.  Each function comes in two flavours:
 *  - allocating (grows the destination membuf when it is too small), and
 *  - noalloc   (fails immediately if the buffer is insufficient).
 */
#ifndef WCONV_H
#define WCONV_H

#include <wchar.h>

struct membuf;

size_t mbs_to_wcs(const char *mbs, struct membuf *wcs);
size_t mbs_to_wcs_noalloc(const char *mbs, struct membuf *wcs);
size_t wcs_to_mbs(const wchar_t *wcs, struct membuf *mbs);
size_t wcs_to_mbs_noalloc(const wchar_t *wcs, struct membuf *mbs);

#endif /* WCONV_H */
