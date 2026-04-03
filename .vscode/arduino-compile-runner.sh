#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PY="$SCRIPT_DIR/arduino_compile_helper.py"

if [ ! -f "$PY" ]; then
  echo "Error: helper not found at $PY" >&2
  exit 2
fi

if [ ! -x "$PY" ]; then
  chmod +x "$PY" 2>/dev/null || true
fi

# Allow overriding workspace path
export WORKSPACE_FOLDER="${WORKSPACE_FOLDER:-$PWD}"

# Run the helper with an explicit Python interpreter to avoid executing
# the helper via its shebang on filesystems mounted noexec (pCloud/FUSE).
PY_CMD="$(command -v python3 || command -v python || true)"
if [ -z "$PY_CMD" ]; then
  echo "Error: python3 or python not found in PATH" >&2
  exit 3
fi

exec "$PY_CMD" "$PY" "$@"
