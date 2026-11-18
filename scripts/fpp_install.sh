#!/bin/bash
set -euo pipefail

: "${FPPDIR:=/opt/fpp}"

if [ ! -f "${FPPDIR}/scripts/common" ]; then
    echo "FPP common script not found at ${FPPDIR}/scripts/common" >&2
    exit 1
fi

# Load FPP paths and settings helpers.
. "${FPPDIR}/scripts/common"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${FPPDIR}/src"

if [ ! -f "${BUILD_DIR}/Plugin.h" ]; then
    echo "FPP plugin headers not found in ${BUILD_DIR}" >&2
    exit 1
fi

if [ ! -f "${PLUGIN_DIR}/callbacks.sh" ]; then
    echo "Required callbacks.sh is missing from ${PLUGIN_DIR}" >&2
    exit 1
fi

cd "${PLUGIN_DIR}"

# FPP discovers and loads the C++ library through callbacks.sh.
chmod 755 callbacks.sh
CALLBACK_TYPES="$(./callbacks.sh --list)"
if [ "${CALLBACK_TYPES}" != "c++" ]; then
    echo "callbacks.sh did not register the C++ plugin (returned: ${CALLBACK_TYPES})" >&2
    exit 1
fi

# Always rebuild during install/update so the library matches this FPP build.
make "SRCDIR=${BUILD_DIR}" clean
make "SRCDIR=${BUILD_DIR}" all

LIBRARY="${PLUGIN_DIR}/libFPPPushoverNotifications.${SHLIB_EXT:-so}"
if [ ! -f "${LIBRARY}" ]; then
    # FPP's build makefiles define SHLIB_EXT; this fallback gives a clear
    # diagnostic if the expected output was not produced.
    LIBRARY="${PLUGIN_DIR}/libFPPPushoverNotifications.so"
fi

if [ ! -f "${LIBRARY}" ]; then
    echo "Native plugin library was not produced. Expected libFPPPushoverNotifications.so." >&2
    exit 1
fi

chmod 755 "${LIBRARY}"

# Ask FPP to reload its plugin list and load the newly built library.
if command -v setSetting >/dev/null 2>&1; then
    setSetting restartFlag 1
else
    echo "Warning: setSetting is unavailable; restart FPPD to load the plugin." >&2
fi

echo "FPP Pushover Notifications installed/updated successfully."
