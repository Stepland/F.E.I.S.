#!/bin/bash
set -euo pipefail

if (( $# < 3 )); then
    echo "usage : $0 EXECUTABLE ASSETS ICON"
    exit 1
fi

EXECUTABLE="$1"
ASSETS="$2"
ICON="$3"
SCRIPT_DIR="$(dirname "${BASH_SOURCE[0]}")"
LINUXDEPLOY="$SCRIPT_DIR/linuxdeploy-x86_64.AppImage"
APP_DIR="$SCRIPT_DIR/AppDir"
TEMP_DIR="$SCRIPT_DIR/temp_dir"

if [[ ! -f "$LINUXDEPLOY" ]]; then
    wget \
        --no-clobber \
        --directory-prefix "$SCRIPT_DIR" \
        https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
fi

if [[ ! -x "$LINUXDEPLOY" ]]; then
    chmod +x "$LINUXDEPLOY"
fi

rm -rf "$APP_DIR" "$TEMP_DIR"
mkdir -p "$APP_DIR" "$TEMP_DIR"

RENAMED_EXECUTABLE="$TEMP_DIR/f.e.i.s"
cp "$EXECUTABLE" "$RENAMED_EXECUTABLE"
RENAMED_ICON="$TEMP_DIR/f.e.i.s.svg"
cp "$ICON" "$RENAMED_ICON"

mkdir -p "$APP_DIR/usr/share/metainfo"
cp "$SCRIPT_DIR/../common/f.e.i.s.appdata.xml" "$APP_DIR/usr/share/metainfo"

exec "$LINUXDEPLOY" \
    --appdir "$APP_DIR" \
    --executable "$RENAMED_EXECUTABLE" \
    --desktop-file "$SCRIPT_DIR/../common/f.e.i.s.desktop" \
    --icon-file "$RENAMED_ICON" \
    --output appimage