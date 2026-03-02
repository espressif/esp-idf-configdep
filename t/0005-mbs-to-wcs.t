#!/usr/bin/env bash
#
if test "$S" != "win"; then
	echo "1..0 # SKIP windows only"
	exit 0
fi

O="$T_OUT/$(basename "$0" .t)"
mkdir -p "$O"

$CC -std=c99 -I. -It -o "$O/test_mbs_to_wcs" t/test_mbs_to_wcs.c membuf.c utils.c port.c wconv.c win.c || exit 1

"$O/test_mbs_to_wcs" 2>/dev/null
