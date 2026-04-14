#!/usr/bin/env bash
#
# Scanned CONFIG_* suffixes with '__' map to .cdep paths without empty
# path components (Kconfig names are not expected to contain '__').
. t/tap.sh
tap_setup

cat <<'EOF' >main_dd.c
#include <stdio.h>
#include "sdkconfig.h"
#ifndef __CONFIG_H__
#define __CONFIG_H__
#endif

int main(void)
{
	return 0;
}
EOF

touch sdkconfig.h

$BINARY $CC -E -M -MF main_dd.d main_dd.c 2>/dev/null

tap_check ! grep sdkconfig main_dd.d
tap_done "sdkconfig.h removed when __CONFIG_H__-style text is present"

tap_check grep -Fq 'h/.cdep' main_dd.d
tap_check test -f h/.cdep
tap_done "double-underscore suffix yields touchable h/.cdep path"

tap_result
