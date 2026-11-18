#!/bin/bash
set -e
: "${FPPDIR:=/opt/fpp}"
. "${FPPDIR}/scripts/common"
PLUGIN_DIR="${MEDIADIR}/plugins/FPPPushoverNotifications"
cd "${PLUGIN_DIR}"
make "SRCDIR=${FPPDIR}/src"
