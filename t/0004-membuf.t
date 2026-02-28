#!/usr/bin/env bash
#
O="$T_OUT/$(basename "$0" .t)"
mkdir -p "$O"

WIN_SRCS=""
test "$S" = "win" && WIN_SRCS="win.c wconv.c"

$CC -std=c99 -I. -It -o "$O/test_membuf" t/test_membuf.c membuf.c utils.c port.c $WIN_SRCS || exit 1

"$O/test_membuf" 2>/dev/null
