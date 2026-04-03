#!/usr/bin/env bash
# arduino-cli-wrapper.sh — wrapper to locate and run Arduino CLI from VS Code
#
# Purpose:
#  - Prefer using the `VSCODE_ARDUINO_CLI` environment variable when set.
#  - Search for a bundled Arduino CLI inside the VS Code extensions directories.
#  - Fall back to the system `arduino-cli` if available in PATH.
#  - Print a helpful message and exit non-zero if no CLI is found.
#
# Usage:
#  - ./arduino-cli-wrapper.sh compile --fqbn ... path/to/sketch
#  - Export `VSCODE_ARDUINO_CLI=/path/to/arduino-cli` to force a specific binary.
#
# Notes:
#  - This script defaults to `linux-x64` under `platform_dir`; change it for other OSes.
#  - It is intended to be called by VS Code tasks or manually from shell.
set -euo pipefail

if [[ -n "${VSCODE_ARDUINO_CLI:-}" && -x "${VSCODE_ARDUINO_CLI}" ]]; then
  exec "${VSCODE_ARDUINO_CLI}" "$@"
fi

resolve_bundled_cli() {
  local extension_root="$1"
  local platform_dir="$2"
  local pattern="${extension_root}/vscode-arduino.vscode-arduino-community-*-${platform_dir}"
  local candidates=()

  shopt -s nullglob
  candidates=(${pattern})
  shopt -u nullglob

  if [[ ${#candidates[@]} -eq 0 ]]; then
    return 1
  fi

  local newest
  newest=$(printf '%s\n' "${candidates[@]}" | sort -V | tail -n 1)
  local cli="${newest}/assets/platform/${platform_dir}/arduino-cli/arduino-cli.app"

  if [[ -x "${cli}" ]]; then
    printf '%s\n' "${cli}"
    return 0
  fi

  return 1
}

platform_dir="linux-x64"
bundled_cli=""

if [[ -n "${HOME:-}" ]]; then
  bundled_cli=$(resolve_bundled_cli "${HOME}/.vscode/extensions" "${platform_dir}" || true)
  if [[ -z "${bundled_cli}" ]]; then
    bundled_cli=$(resolve_bundled_cli "${HOME}/.vscode-insiders/extensions" "${platform_dir}" || true)
  fi
fi

if [[ -n "${bundled_cli}" ]]; then
  exec "${bundled_cli}" "$@"
fi

if command -v arduino-cli >/dev/null 2>&1; then
  exec arduino-cli "$@"
fi

echo "No Arduino CLI found. Install vscode-arduino-community extension or set VSCODE_ARDUINO_CLI." >&2
exit 1
