#!/usr/bin/env bash
#
. t/tap.sh
tap_setup

cat <<EOF >main.c
#include <stdio.h>
#include "sdkconfig.h"

// CONFIG_MY_OPTION

int main(int argc, char **argv)
{
    return 0;
}
EOF

touch sdkconfig.h
mkdir -p my
touch my/option.cdep

$BINARY $CC -E -M -MF main.d main.c 2>/dev/null

# Test 1: sdkconfig.h removed
tap_check ! grep sdkconfig main.d
tap_done "sdkconfig.h removed"

# Test 2: matching .cdep added
tap_check grep 'my/option.cdep' main.d
tap_done "matching .cdep added"

# Test 3: CONFIG option without .cdep file is not added
cat <<EOF >main_nocdep.c
#include <stdio.h>
#include "sdkconfig.h"

// CONFIG_MISSING_OPTION

int main(int argc, char **argv)
{
    return 0;
}
EOF

$BINARY $CC -E -M -MF main_nocdep.d main_nocdep.c 2>/dev/null
tap_check ! grep 'missing/option.cdep' main_nocdep.d
tap_done "CONFIG option without .cdep file is not added"

# Test 4: multiple CONFIG options — only existing .cdep files added
cat <<EOF >main_multi.c
#include <stdio.h>
#include "sdkconfig.h"

// CONFIG_MY_OPTION
// CONFIG_OTHER_OPTION
// CONFIG_MISSING_OPTION

int main(int argc, char **argv)
{
    return 0;
}
EOF

mkdir -p other
touch other/option.cdep

$BINARY $CC -E -M -MF main_multi.d main_multi.c 2>/dev/null
tap_check grep 'my/option.cdep' main_multi.d
tap_check grep 'other/option.cdep' main_multi.d
tap_check ! grep 'missing/option.cdep' main_multi.d
tap_done "multiple CONFIG options - only existing .cdep files added"

# Test 5: underscore-to-directory mapping (CONFIG_MY_SUB_OPT -> my/sub/opt.cdep)
cat <<EOF >main_sub.c
#include <stdio.h>
#include "sdkconfig.h"

// CONFIG_MY_SUB_OPT

int main(int argc, char **argv)
{
    return 0;
}
EOF

mkdir -p my/sub
touch my/sub/opt.cdep

$BINARY $CC -E -M -MF main_sub.d main_sub.c 2>/dev/null
tap_check grep 'my/sub/opt.cdep' main_sub.d
tap_done "underscore-to-directory mapping"

tap_result
