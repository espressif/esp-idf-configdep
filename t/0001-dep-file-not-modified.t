#!/usr/bin/env bash
#
. t/tap.sh
tap_setup

cat <<EOF >main.c
#include <stdio.h>

int main(int argc, char **argv)
{
    return 0;
}
EOF

# Test 1: dep file unchanged when source has no sdkconfig.h
$CC -E -M -MF main1.d main.c 2>/dev/null
$BINARY $CC -E -M -MF main2.d main.c 2>/dev/null
tap_check diff -u main1.d main2.d
tap_done "dep file unchanged when source has no sdkconfig.h"

# Test 2: compiler exit code passed through on failure
$CC -E -M -MF fail.d no_such_file.c 2>/dev/null; cc_rc=$?
$BINARY $CC -E -M -MF fail.d no_such_file.c 2>/dev/null; wrap_rc=$?
tap_check test "$cc_rc" != 0
tap_check test "$wrap_rc" = "$cc_rc"
tap_done "compiler exit code passed through on failure"

# Test 3: no -MF flag — binary exits 0, no dep file created
ls *.d >d_before 2>/dev/null || true
tap_check $BINARY $CC -E main.c
ls *.d >d_after 2>/dev/null || true
tap_check diff -u d_before d_after
tap_done "no -MF flag exits 0 with no dep file created"

tap_result
