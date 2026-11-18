#!/bin/bash
set -e
: "${FPPDIR:=/opt/fpp}"
. "${FPPDIR}/scripts/common"
PLUGIN_DIR="${MEDIADIR}/plugins/FPPPushoverNotifications"
cd "${PLUGIN_DIR}"
chmod +x callbacks.sh
make "SRCDIR=${FPPDIR}/src"
