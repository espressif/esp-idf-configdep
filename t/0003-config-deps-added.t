#!/usr/bin/env bash
#
O="$T_OUT/$(basename "$0" .t)"
mkdir -p "$O"
pushd "$O" || exit

echo 1..2

cat << EOF >main.c
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
touch my/option.h

$BINARY $CC -E -M -MF main.d main.c

if ! grep sdkconfig main.d; then
    echo "ok 1"
else
    echo "not ok 1"
fi

if grep 'my/option.h' main.d; then
    echo "ok 2"
else
    echo "not ok 2"
fi
