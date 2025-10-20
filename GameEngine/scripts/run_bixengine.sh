#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Launcher script used by JetBrains Rider to execute the BixEngine sample.
# This script ensures the target is built via xmake and then executes it.
# -----------------------------------------------------------------------------

set -euo pipefail

# Resolve repository root relative to this script.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
ENGINE_ROOT="${REPO_ROOT}/BixEngine"

cd "${ENGINE_ROOT}"

if ! command -v xmake >/dev/null 2>&1; then
    echo "[run_bixengine] Error: xmake is not available in PATH." >&2
    echo "Install xmake from https://xmake.io or ensure it is added to PATH." >&2
    exit 1
fi

# Rebuild the engine to make sure binaries are up to date.
xmake build BixEngine

# Forward any arguments supplied from the run configuration to the executable.
if [ $# -gt 0 ]; then
    xmake run BixEngine -- "$@"
else
    xmake run BixEngine
fi
