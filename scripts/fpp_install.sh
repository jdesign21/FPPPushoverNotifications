#!/bin/bash
set -euo pipefail

: "${FPPDIR:=/opt/fpp}"

# FPP common expects LD_LIBRARY_PATH to exist. With nounset enabled (-u),
# sourcing common fails on systems where the variable is not already set.
: "${LD_LIBRARY_PATH:=}"
export LD_LIBRARY_PATH

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

# FPP 9.3 updates with a plain git pull. Ignore local executable-bit drift
# so build/install operations cannot leave the checkout dirty and block updates.
git config core.fileMode false

# callbacks.sh is committed executable in Git; do not chmod a tracked file here.
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
# During a Plugin Manager upgrade, newer FPP versions reload the plugin
# after this script returns. Do not request a competing restart in that case.
if [ "${FPP_RELOADS_PLUGIN:-0}" = "1" ]; then
    echo "FPP will reload the native plugin after the upgrade."
elif command -v setSetting >/dev/null 2>&1; then
    setSetting restartFlag 1
else
    echo "Warning: setSetting is unavailable; restart FPPD to load the plugin." >&2
fi

echo "FPP Pushover Notifications installed/updated successfully."
