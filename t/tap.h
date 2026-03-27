/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Minimal TAP (Test Anything Protocol) helpers for C unit tests.
 *
 * The test plan is printed automatically at the end by tap_result(),
 * so there is no need to maintain a test count manually.
 *
 * Usage:
 *   #include "tap.h"
 *
 *   int main(void) {
 *       tap_check(1 + 1 == 2);
 *       tap_check(strlen("hi") == 2);
 *       tap_done("arithmetic and strings work");
 *
 *       tap_check(some_func() == 0);
 *       tap_done("some_func succeeds");
 *
 *       return tap_result();
 *   }
 */
#ifndef TAP_H
#define TAP_H

#include <stdio.h>

static int tap_test_num;
static int tap_test_pass = 1;
static int tap_failures;

#define tap_check(cond)                                                        \
	do {                                                                   \
		if (!(cond)) {                                                 \
			printf("# check failed: %s (%s:%d)\n", #cond,          \
			       __FILE__, __LINE__);                            \
			tap_test_pass = 0;                                     \
		}                                                              \
	} while (0)

#define tap_done(desc)                                                         \
	do {                                                                   \
		tap_test_num++;                                                \
		printf("%s %d - %s\n", tap_test_pass ? "ok" : "not ok",        \
		       tap_test_num, (desc));                                  \
		if (!tap_test_pass)                                            \
			tap_failures++;                                        \
		tap_test_pass = 1;                                             \
	} while (0)

#define tap_result() (printf("1..%d\n", tap_test_num), tap_failures)

#endif /* TAP_H */
