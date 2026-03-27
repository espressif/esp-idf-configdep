#!/usr/bin/env bash
#
# CONFIG_* only in an included header must still add the matching .cdep.
. t/tap.sh
tap_setup

mkdir -p inc my
touch my/option.cdep

cat <<EOF >inc/hdr.h
#ifndef HDR_H
#define HDR_H
/* CONFIG_MY_OPTION appears only here, not in the .c file */
#endif
EOF

cat <<EOF >main_hdr.c
#include <stdio.h>
#include "sdkconfig.h"
#include "inc/hdr.h"

int main(void)
{
    return 0;
}
EOF

touch sdkconfig.h

$BINARY $CC -E -I. -M -MF main_hdr.d main_hdr.c 2>/dev/null

tap_check ! grep sdkconfig main_hdr.d
tap_done "sdkconfig.h removed when CONFIG_* is only in a header"

tap_check grep 'my/option.cdep' main_hdr.d
tap_done "matching .cdep added from header-only CONFIG_* reference"

tap_result
