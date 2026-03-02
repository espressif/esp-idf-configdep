# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
#
# SPDX-License-Identifier: Apache-2.0

# Minimal TAP (Test Anything Protocol) helpers for bash integration tests.
#
# Mirrors the C helpers in t/tap.h. The test plan (1..N) is printed at
# the end by tap_result, so there is no need to maintain a count manually.
#
# Usage:
#   . t/tap.sh
#
#   tap_setup
#
#   tap_check diff -u expected actual
#   tap_done "files match"
#
#   tap_check ! grep sdkconfig main.d
#   tap_done "sdkconfig removed"
#
#   tap_result

tap_test_num=0
tap_test_pass=1
tap_failures=0

# tap_check [!] <cmd> [args...]
#
# Run a command; mark current test as failed if it exits non-zero.
# Leading "!" inverts the check (expect failure).
# On failure, print a diagnostic with the command, its output, and
# the caller's source location.
tap_check () {
	local output invert=0 status desc

	desc="$*"

	if [ "$1" = "!" ]; then
		invert=1
		shift
	fi

	output=$("$@" 2>&1)
	status=$?

	if [ $invert -eq 1 ]; then
		if [ $status -eq 0 ]; then
			status=1
		else
			status=0
		fi
	fi

	if [ $status -ne 0 ]; then
		echo "# check failed: $desc (${BASH_SOURCE[1]}:${BASH_LINENO[0]})"
		if test -n "$output"; then
			printf '%s\n' "$output" | sed 's/^/# /'
		fi
		tap_test_pass=0
	fi
}

# tap_done <description>
#
# Finalize a test point. Prints "ok N - desc" or "not ok N - desc".
tap_done () {
	tap_test_num=$((tap_test_num + 1))
	if test "$tap_test_pass" = "1"; then
		echo "ok $tap_test_num - $1"
	else
		echo "not ok $tap_test_num - $1"
		tap_failures=$((tap_failures + 1))
	fi
	tap_test_pass=1
}

# tap_result
#
# Print the TAP plan (1..N) and exit with the failure count.
tap_result () {
	echo "1..$tap_test_num"
	return $tap_failures
}

# tap_setup
#
# Common boilerplate: set O to the per-test output directory, create it,
# and cd into it.
tap_setup () {
	O="$T_OUT/$(basename "$0" .t)"
	mkdir -p "$O"
	cd "$O" || exit 1
}
