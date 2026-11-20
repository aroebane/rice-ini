#!/bin/sh
# check-exports.sh LIB: every exported symbol starts with riceini_ or RICEINI_,
# and the soname is librice-ini.so.0.
set -eu
[ $# -eq 1 ] || { echo "usage: check-exports.sh LIB" >&2; exit 2; }
stray=$(nm -D --defined-only "$1" | awk '{ print $NF }' |
  grep -Ev '^(riceini_|RICEINI_|__odr_asan\.|_init$|_fini$|_edata$|_end$|__bss_start$)' || true)
if [ -n "$stray" ]; then
  echo "unexpected exports:" >&2
  printf '%s\n' "$stray" >&2
  exit 1
fi
readelf -d "$1" | grep -q 'SONAME.*librice-ini\.so\.0' || { echo "wrong soname" >&2; exit 1; }
