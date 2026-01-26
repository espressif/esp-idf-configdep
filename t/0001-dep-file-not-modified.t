#!/usr/bin/env bash
#
O="$T_OUT/$(basename "$0" .t)"
mkdir -p "$O"
pushd "$O" || exit

echo 1..1

cat << EOF >main.c
#include <stdio.h>

int main(int argc, char **argv)
{
    return 0;
}
EOF

$CC -E -M -MF main1.d main.c
$BINARY $CC -E -M -MF main2.d main.c

if diff -u main1.d main2.d; then
    echo "ok 1"
else
    echo "not ok 1"
fi
