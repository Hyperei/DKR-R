#!/usr/bin/env bash
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
skip_deps=0
case "${1:-}" in
'') ;;
--skip-deps) skip_deps=1 ;;
-h | --help)
  echo 'Usage: ./Setup-macOS.sh [--skip-deps] (install prerequisites, or only check them)'
  exit 0
  ;;
*)
  echo "Unknown option: $1" >&2
  exit 1
  ;;
esac
[[ $# -le 1 ]] || {
  echo 'Too many arguments.' >&2
  exit 1
}
source "$project_root/scripts/macos-host-tools.sh"
dkr_macos_host_tools "$skip_deps"
echo 'macOS prerequisites passed (same checks as Build preparation). Run ./Build-macOS.sh --rom-v77 "/path/to/US 1.0.z64" --rom-v80 "/path/to/US Rev A.z64".'
