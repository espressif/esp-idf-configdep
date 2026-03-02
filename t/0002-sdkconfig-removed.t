#!/usr/bin/env bash
#
. t/tap.sh
tap_setup

cat <<EOF >main.c
#include <stdio.h>
#include "sdkconfig.h"

int main(int argc, char **argv)
{
    return 0;
}
EOF

touch sdkconfig.h

# Test 1: sdkconfig.h present in original deps
$CC -E -M -MF main1.d main.c 2>/dev/null
tap_check grep sdkconfig main1.d
tap_done "sdkconfig.h present in original deps"

# Test 2: sdkconfig.h removed after wrapper
$BINARY $CC -E -M -MF main2.d main.c 2>/dev/null
tap_check ! grep sdkconfig main2.d
tap_done "sdkconfig.h removed after wrapper"

# Test 3: other dependencies preserved when sdkconfig.h removed
tap_check grep 'stdio.h' main2.d
tap_done "other dependencies preserved when sdkconfig.h removed"

# Test 4: sdkconfig.h with directory prefix removed
mkdir -p builddir
cp sdkconfig.h builddir/sdkconfig.h

cat <<EOF >main_dir.c
#include <stdio.h>
#include "builddir/sdkconfig.h"

int main(int argc, char **argv)
{
    return 0;
}
EOF

$BINARY $CC -E -M -MF main_dir.d main_dir.c 2>/dev/null
tap_check ! grep sdkconfig main_dir.d
tap_done "sdkconfig.h with directory prefix removed"

tap_result
