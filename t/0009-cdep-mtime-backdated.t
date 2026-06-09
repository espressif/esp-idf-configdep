#!/usr/bin/env bash
#
# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
#
# SPDX-License-Identifier: Apache-2.0
#
# A freshly created .cdep stub must be backdated to auto.conf's mtime so it is
# never newer than the object files of the current build (which are compiled
# after auto.conf is written). Otherwise the stub would force one unnecessary
# rebuild on the next ninja run.
. t/tap.sh
tap_setup

cat <<EOF >main.c
#include <stdio.h>
#include "sdkconfig.h"

// CONFIG_BACKDATE_OPT

int main(int argc, char **argv)
{
    return 0;
}
EOF

touch sdkconfig.h

# auto.conf is written during the config phase, before any compilation.
touch auto.conf
# Distinct whole second, then a stand-in object file compiled "after" auto.conf.
sleep 1
touch reference.o

$BINARY $CC -E -M -MF main.d main.c 2>/dev/null

# Stub is created and listed.
tap_check test -f backdate/opt.cdep
tap_check grep 'backdate/opt.cdep' main.d
tap_done "missing .cdep created with auto.conf present"

# Core property: the stub must NOT be newer than the object compiled this build.
tap_check ! test backdate/opt.cdep -nt reference.o
tap_done "created .cdep is not newer than the build object"

# It is backdated to auto.conf's mtime (floored to whole seconds, so it may be
# marginally older, but must never be newer than auto.conf).
tap_check ! test backdate/opt.cdep -nt auto.conf
tap_done "created .cdep is not newer than auto.conf"

# Fallback: with no auto.conf the stub is still created (previous behaviour).
cat <<EOF >main_noac.c
#include <stdio.h>
#include "sdkconfig.h"

// CONFIG_NOAUTOCONF_OPT

int main(int argc, char **argv)
{
    return 0;
}
EOF

rm -f auto.conf
$BINARY $CC -E -M -MF main_noac.d main_noac.c 2>/dev/null
tap_check test -f noautoconf/opt.cdep
tap_check grep 'noautoconf/opt.cdep' main_noac.d
tap_done "missing .cdep still created when auto.conf is absent"

tap_result
