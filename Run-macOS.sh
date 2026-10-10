#!/usr/bin/env bash
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${DKR_MAC_BUILD_DIR:-${project_root}/build/dkr-runtime-macos}"
executable="${build_dir}/bin/Release/DKR-R"
[[ -x "${executable}" ]] || {
  echo 'DKR-R is missing. Run ./Build-macOS.sh --help for build instructions.' >&2
  exit 1
}
exec "${executable}" "$@"
