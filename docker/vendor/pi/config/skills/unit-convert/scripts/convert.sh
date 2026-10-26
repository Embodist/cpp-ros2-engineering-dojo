#!/usr/bin/env bash
# unit conversion helper for the unit-convert skill
set -euo pipefail

value="$1"; from="$2"; to="$3"

case "$from:$to" in
  c:f) awk -v v="$value" 'BEGIN{printf "%.2f", v*9/5+32}' ;;
  f:c) awk -v v="$value" 'BEGIN{printf "%.2f", (v-32)*5/9}' ;;
  km:mi) awk -v v="$value" 'BEGIN{printf "%.2f", v*0.621371}' ;;
  mi:km) awk -v v="$value" 'BEGIN{printf "%.2f", v*1.60934}' ;;
  kg:lb) awk -v v="$value" 'BEGIN{printf "%.2f", v*2.20462}' ;;
  lb:kg) awk -v v="$value" 'BEGIN{printf "%.2f", v*0.453592}' ;;
  *) echo "ERROR: unsupported conversion $from -> $to"; exit 1 ;;
esac
echo
