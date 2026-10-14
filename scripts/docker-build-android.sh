#!/usr/bin/env bash
# Run Build-Android.sh inside the pinned toolchain container.
#
# The signing keystore is mounted from the host. Gradle's debug signing config
# reads ~/.android/debug.keystore, and a container without that mount mints a
# fresh key on every run - each APK then has a different signing identity and
# Android refuses to install one over another with "App not installed".
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="${DKR_ANDROID_IMAGE:-dkr-android-build:latest}"
KEYSTORE_DIR="${DKR_ANDROID_KEYSTORE_DIR:-$HOME/dev/dkr-android-keystore}"
CONTAINER="${DKR_ANDROID_CONTAINER:-dkr-android-build-run}"

if [[ ! -f "$KEYSTORE_DIR/debug.keystore" ]]; then
  cat >&2 <<EOF
No keystore at $KEYSTORE_DIR/debug.keystore

Create one before building, or every build gets a different signing identity:

  mkdir -p "$KEYSTORE_DIR"
  docker run --rm -v "$KEYSTORE_DIR:/ks" $IMAGE \\
    keytool -genkeypair -v -keystore /ks/debug.keystore \\
      -storepass android -keypass android -alias androiddebugkey \\
      -keyalg RSA -keysize 2048 -validity 10950 \\
      -dname "C=US, O=Android, CN=Android Debug"
EOF
  exit 1
fi

docker rm -f "$CONTAINER" >/dev/null 2>&1 || true
# The validation layer lives outside the repo: it is a 27 MB third-party
# binary, and only diagnostic builds need it.
LAYER_MOUNT=()
LAYER_ENV=()
if [[ -n "${DKR_ANDROID_VALIDATION_LAYER:-}" ]]; then
  [[ -f "$DKR_ANDROID_VALIDATION_LAYER" ]] ||
    { echo "No validation layer at $DKR_ANDROID_VALIDATION_LAYER" >&2; exit 1; }
  LAYER_MOUNT=(-v "$DKR_ANDROID_VALIDATION_LAYER:/opt/vvl/libVkLayer_khronos_validation.so:ro")
  LAYER_ENV=(-e DKR_ANDROID_VALIDATION_LAYER=/opt/vvl/libVkLayer_khronos_validation.so)
fi

exec docker run --rm --name "$CONTAINER" \
  -v "$PROJECT_ROOT:/work/dkr-r" \
  -v "$KEYSTORE_DIR:/home/builder/.android" \
  "${LAYER_MOUNT[@]}" "${LAYER_ENV[@]}" \
  -w /work/dkr-r \
  "$IMAGE" \
  ./Build-Android.sh "$@"
