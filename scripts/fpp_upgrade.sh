#!/bin/bash
set -euo pipefail

# FPP calls this after pulling new repository code. Reuse the same verified
# build/install steps as a fresh install, while fpp_install.sh respects
# FPP_RELOADS_PLUGIN when Plugin Manager will reload fppd itself.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/fpp_install.sh"
