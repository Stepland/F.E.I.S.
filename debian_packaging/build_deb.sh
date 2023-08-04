#!/bin/bash
set -euxo pipefail

SCRIPT_DIR="$(dirname "${BASH_SOURCE[0]}")"

if (( $# < 3 )); then
    echo "usage : $0 EXECUTABLE ASSETS ICON "
    exit 1
fi

EXECUTABLE="$1"
ASSETS="$2"
ICON="$3"
PACKAGE_DIR="${SCRIPT_DIR}/package"

rm -rf "${PACKAGE_DIR}"
mkdir -p "${PACKAGE_DIR}"
cp "${SCRIPT_DIR}/f.e.i.s-control" "${SCRIPT_DIR}/f.e.i.s.desktop" "${SCRIPT_DIR}/postinst" "${SCRIPT_DIR}/postrm" "${PACKAGE_DIR}"
cp "${EXECUTABLE}" "${PACKAGE_DIR}/f.e.i.s"
cp "${ICON}" "${PACKAGE_DIR}/f.e.i.s.svg"
tar --create --gzip --owner root --group root --mode="u=rwX,g=rwX,o=rX" --file "${PACKAGE_DIR}/assets.tar.gz" "${ASSETS}"
cd "${PACKAGE_DIR}"
equivs-build f.e.i.s-control