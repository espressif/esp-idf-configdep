#!/usr/bin/env bash
#
O="$T_OUT/$(basename "$0" .t)"
mkdir -p "$O"
pushd "$O" || exit

echo 1..2

cat << EOF >main.c
#include <stdio.h>
#include "sdkconfig.h"

int main(int argc, char **argv)
{
    return 0;
}
EOF

touch sdkconfig.h

$CC -E -M -MF main1.d main.c

if grep sdkconfig main1.d; then
    echo "ok 1"
else
    echo "not ok 1"
fi

$BINARY $CC -E -M -MF main2.d main.c

if ! grep sdkconfig main2.d; then
    echo "ok 2"
else
    echo "not ok 2"
fi
