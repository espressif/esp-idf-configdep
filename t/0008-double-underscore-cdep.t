#!/usr/bin/env bash
#
# CONFIG_* is not matched when 'CONFIG_' is directly preceded by [A-Za-z0-9_].
# Collapsing of '/' runs in .cdep paths still applies for genuine options.
. t/tap.sh
tap_setup

cat <<'EOF' >main_dd.c
#include <stdio.h>
#include "sdkconfig.h"
#ifndef __CONFIG_H__
#define __CONFIG_H__
#endif
// CONFIG_MY_OPTION

int main(void)
{
	return 0;
}
EOF

cat <<'EOF' >main_embed.c
#include <stdio.h>
#include "sdkconfig.h"
#define MYCONFIG_ENABLE 1
int main(void) { return MYCONFIG_ENABLE ? 0 : 1; }
EOF

touch sdkconfig.h
mkdir -p my
touch my/option.cdep

$BINARY $CC -E -M -MF main_dd.d main_dd.c 2>/dev/null

tap_check ! grep sdkconfig main_dd.d
tap_done "sdkconfig.h removed when __CONFIG_H__-style text is present"

tap_check ! grep -Fq 'h/.cdep' main_dd.d
tap_check ! test -f h/.cdep
tap_done "identifier boundary excludes __CONFIG_H__ false option"

tap_check grep -Fq 'my/option.cdep' main_dd.d
tap_done "CONFIG_MY_OPTION still detected beside header guard text"

$BINARY $CC -E -M -MF main_embed.d main_embed.c 2>/dev/null

tap_check ! grep -Fq 'enable.cdep' main_embed.d
tap_check ! test -f enable.cdep
tap_done "identifier boundary excludes MYCONFIG_ENABLE substring"

tap_result
