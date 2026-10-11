#!/usr/bin/env bash
# Shared prerequisite contract for Setup and runtime preparation.
dkr_macos_host_tools() {
  local skip_deps="$1" make_version probe
  [[ "$(uname -s)" == Darwin ]] || {
    echo 'macOS host tools require macOS.' >&2
    return 1
  }
  xcodebuild -version >/dev/null 2>&1 || {
    echo 'Install Xcode and select it with sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer' >&2
    return 1
  }
  xcodebuild -checkFirstLaunchStatus >/dev/null 2>&1 || {
    echo 'Run sudo xcodebuild -runFirstLaunch' >&2
    return 1
  }
  if [[ "$skip_deps" -eq 0 ]]; then
    command -v brew >/dev/null || {
      echo 'Install Homebrew: https://brew.sh/' >&2
      return 1
    }
    HOMEBREW_NO_AUTO_UPDATE=1 HOMEBREW_NO_INSTALL_UPGRADE=1 brew install --quiet cmake ninja make pcre2 python sdl2 coreutils || return 1
  fi
  local tool
  for tool in git python3 cmake ninja c++ shasum sha1sum pcre2-config sdl2-config; do
    command -v "$tool" >/dev/null || {
      echo "Missing $tool. Run ./Setup-macOS.sh (sha1sum is supplied by brew install coreutils)." >&2
      return 1
    }
  done
  GMAKE="$(command -v gmake || true)"
  if [[ -z "$GMAKE" ]] && command -v brew >/dev/null; then
    GMAKE="$(brew --prefix make)/bin/gmake"
  fi
  [[ -x "$GMAKE" ]] || {
    echo 'GNU Make 4 or newer is required: brew install make' >&2
    return 1
  }
  make_version="$("$GMAKE" --version)"
  [[ "$make_version" =~ GNU\ Make\ ([0-9]+) ]] && [[ "${BASH_REMATCH[1]}" -ge 4 ]] || {
    echo 'GNU Make 4 or newer is required: brew install make' >&2
    return 1
  }
  # Exercise the exact verifier interface before any ROM extraction.
  probe="$(mktemp -d)" || return 1
  printf abc >"$probe/input"
  printf 'a9993e364706816aba3e25717850c26c9cd0d89d  input\n' >"$probe/checksums"
  if ! (cd "$probe" && sha1sum -c --quiet checksums); then
    rm -rf -- "$probe"
    echo 'sha1sum must support -c --quiet. Install coreutils and put its bin directory on PATH.' >&2
    return 1
  fi
  rm -rf -- "$probe"
  if ! xcrun metal -v >/dev/null 2>&1; then
    if [[ "$skip_deps" -eq 1 ]]; then
      echo 'Install Metal: xcodebuild -downloadComponent MetalToolchain' >&2
      return 1
    fi
    xcodebuild -downloadComponent MetalToolchain || return 1
  fi
}
