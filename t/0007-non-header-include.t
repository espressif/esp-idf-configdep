#!/usr/bin/env bash
#
# CONFIG_* only in a non-.h include (.inc) must still add the matching .cdep.
. t/tap.sh
tap_setup

mkdir -p extra
touch extra/only.cdep

cat <<EOF >extra.inc
/* CONFIG_EXTRA_ONLY referenced only in this .inc file */
EOF

cat <<EOF >main_inc.c
#include <stdio.h>
#include "sdkconfig.h"
#include "extra.inc"

int main(void)
{
    return 0;
}
EOF

touch sdkconfig.h

$BINARY $CC -E -I. -M -MF main_inc.d main_inc.c 2>/dev/null

tap_check grep 'extra/only.cdep' main_inc.d
tap_done ".inc dependency scanned for CONFIG_*"

tap_result
