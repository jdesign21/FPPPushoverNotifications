#!/bin/bash
set -e

: "${FPPDIR:=/opt/fpp}"
if [ -f "${FPPDIR}/scripts/common" ]; then
    . "${FPPDIR}/scripts/common"
    if command -v setSetting >/dev/null 2>&1; then
        setSetting restartFlag 1
    fi
fi

exit 0
