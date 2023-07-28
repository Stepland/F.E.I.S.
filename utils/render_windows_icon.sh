#!/bin/bash

if (( $# < 1 )); then
    echo "usage : $0 SVG_FILE"
    exit 1
fi

ICON_PATH="$(echo $1 | sed 's/[.]svg/.ico/')"

convert \
    -density 300 \
    -define icon:auto-resize=256,128,96,64,48,32,16 \
    -background none \
    "$1" "$ICON_PATH"