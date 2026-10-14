#!/usr/bin/env bash
# Linux replacement for scripts/Package-Android-Preview.ps1.
#
# Builds the arm64 Android runtime from prepared Patch Pipeline payloads and
# packages a sideloadable APK. The upstream packaging script is PowerShell with
# windows-x86_64 NDK paths compiled in; this performs the same staging, the
# same safety checks and the same Gradle assembly on a Linux host.
#
# Run scripts/Prepare-DKR-Runtime-Linux.sh first to produce the payloads.
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GENERATED_ROOT="${DKR_GENERATED_ROOT:-$PROJECT_ROOT/build/generated}"
BUILD_DIR="${DKR_ANDROID_BUILD_DIR:-$PROJECT_ROOT/build/android-arm64}"
STAGE_DIR="${DKR_ANDROID_STAGE_DIR:-$PROJECT_ROOT/build/android-stage}"
OUTPUT_DIR="${DKR_ANDROID_OUTPUT_DIR:-$PROJECT_ROOT/dist}"
NDK="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"
API_LEVEL=28
JOBS="${CMAKE_BUILD_PARALLEL_LEVEL:-}"
VARIANT=release
CLEAN_STAGE=0

usage() {
  cat <<EOF
Usage: $(basename "$0") [options]
  --debug          Assemble the debuggable variant instead of release
  --jobs N         Parallel jobs (default: host CPU count)
  --clean-stage    Remove an existing staging directory first
  --output DIR     APK output directory (default: dist)
Environment:
  ANDROID_NDK_HOME  NDK r29+ (required)
  DKR_GENERATED_ROOT, DKR_ANDROID_BUILD_DIR, DKR_ANDROID_STAGE_DIR
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
  --debug) VARIANT=debug ;;
  --clean-stage) CLEAN_STAGE=1 ;;
  --jobs)
    [[ $# -ge 2 && -n "$2" ]] || { echo "$1 requires a value." >&2; exit 1; }
    JOBS="$2"; shift ;;
  --output)
    [[ $# -ge 2 && -n "$2" ]] || { echo "$1 requires a path." >&2; exit 1; }
    OUTPUT_DIR="$2"; shift ;;
  -h | --help) usage; exit 0 ;;
  *) echo "Unknown option: $1" >&2; usage >&2; exit 1 ;;
  esac
  shift
done

fail() { echo "[ERROR] $*" >&2; exit 1; }
step() { echo; echo "==> $*"; }

[[ -n "$JOBS" ]] || JOBS="$(nproc)"
[[ -n "$NDK" && -f "$NDK/source.properties" ]] || fail "Set ANDROID_NDK_HOME to an NDK r29+ installation."
TOOLCHAIN="$NDK/build/cmake/android.toolchain.cmake"
[[ -f "$TOOLCHAIN" ]] || fail "NDK toolchain file is missing: $TOOLCHAIN"
LLVM_BIN="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin"
[[ -x "$LLVM_BIN/llvm-strip" && -x "$LLVM_BIN/llvm-readelf" ]] || fail "NDK host binaries are missing from $LLVM_BIN"

SDL2_SOURCE="$PROJECT_ROOT/extern/sdl2"
[[ -f "$SDL2_SOURCE/CMakeLists.txt" ]] || fail "SDL2 source is missing. Run the dependency preparation."
for revision in v77 v80; do
  [[ -f "$GENERATED_ROOT/$revision/RecompiledFuncs/funcs.h" ]] ||
    fail "Missing $revision payload. Run scripts/Prepare-DKR-Runtime-Linux.sh."
done
VERSION="$(tr -d '\r\n' <"$PROJECT_ROOT/VERSION")"

step "Host file_to_c"
# RT64 generates shader blobs with a tool that must run on the BUILD host, so
# it is built natively and handed to the Android configure as a cache entry.
HOST_TOOLS="$PROJECT_ROOT/build/runtime-tools/file_to_c"
cmake -S "$PROJECT_ROOT/extern/rt64/src/tools/file_to_c" -B "$HOST_TOOLS" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "$HOST_TOOLS" --parallel "$JOBS" >/dev/null
FILE_TO_C="$HOST_TOOLS/file_to_c"
[[ -x "$FILE_TO_C" ]] || FILE_TO_C="$(find "$HOST_TOOLS" -name file_to_c -type f -perm -u+x | head -1)"
[[ -x "$FILE_TO_C" ]] || fail "file_to_c was not built."
echo "[OK] $FILE_TO_C"

step "Configure arm64 runtime"
cmake -S "$PROJECT_ROOT/runtime-recomp" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM="android-$API_LEVEL" \
  -DANDROID_STL=c++_shared \
  -DCMAKE_BUILD_TYPE=Release \
  -DDKRPORT_ROOT="$PROJECT_ROOT" \
  -DDKR_RELEASE_VERSION="$VERSION" \
  -DDKR_RUNTIME_BUILD_GENERATED=ON \
  -DDKR_RUNTIME_BUILD_RT64=ON \
  -DDKR_ANDROID_SDL2_SOURCE="$SDL2_SOURCE" \
  -DDKR_HOST_FILE_TO_C="$FILE_TO_C" \
  -DDKR_GENERATED_SOURCE_V77="$GENERATED_ROOT/v77/RecompiledFuncs" \
  -DDKR_GENERATED_SOURCE_V80="$GENERATED_ROOT/v80/RecompiledFuncs"

step "Build DKRPortGame and DKRLegacyModWorker"
cmake --build "$BUILD_DIR" --parallel "$JOBS" --target DKRPortGame DKRLegacyModWorker

step "Stage native libraries"
# Refuse to package any private qualification switch, matching the PowerShell
# packaging guards.
for flag in DKR_ANDROID_RENDER_QUALIFICATION DKR_WATER_QUALIFICATION DKR_TASK_QUALIFICATION; do
  if grep -qE "^${flag}:BOOL=(ON|1|TRUE|YES)$" "$BUILD_DIR/CMakeCache.txt" 2>/dev/null; then
    fail "Refusing to package private qualification build: $flag is enabled."
  fi
done
if grep -qa -e 'DKR_WATER_TEST_MAP' -e '\[perf\]\[private-water-preview\]' "$BUILD_DIR/libDKR-R.so"; then
  fail "Android library still contains private water qualification; rebuild with it disabled."
fi

if [[ "$CLEAN_STAGE" -eq 1 && -e "$STAGE_DIR" ]]; then rm -rf "$STAGE_DIR"; fi
[[ ! -e "$STAGE_DIR" ]] || fail "Choose a fresh staging directory, or pass --clean-stage: $STAGE_DIR"
NATIVE="$STAGE_DIR/jniLibs/arm64-v8a"
ASSETS="$STAGE_DIR/assets"
mkdir -p "$NATIVE" "$ASSETS" "$OUTPUT_DIR"

for library in \
  "$BUILD_DIR/libDKR-R.so" \
  "$BUILD_DIR/sdl2/libSDL2.so" \
  "$BUILD_DIR/bin/Release/libexec/dkr-r/libDKR-R-ModWorker.so" \
  "$NDK/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so"; do
  [[ -f "$library" ]] || fail "Missing native library: $library"
  "$LLVM_BIN/llvm-readelf" -h "$library" | grep -q AArch64 || fail "Wrong native architecture: $library"
  cp "$library" "$NATIVE/"
  "$LLVM_BIN/llvm-strip" --strip-debug "$NATIVE/$(basename "$library")" || fail "Native symbol stripping failed."
  echo "[OK] $(basename "$library")"
done

cp -r "$BUILD_DIR/assets" "$ASSETS/runtime"
NOTICES="$ASSETS/notices"
mkdir -p "$NOTICES"
for file in LICENSE.md THIRD_PARTY.md runtime-recomp/COPYING-NOTICE.md; do
  cp "$PROJECT_ROOT/$file" "$NOTICES/"
done
[[ ! -f "$PROJECT_ROOT/docs/PUBLIC-$VERSION-NOTES.txt" ]] ||
  cp "$PROJECT_ROOT/docs/PUBLIC-$VERSION-NOTES.txt" "$NOTICES/"
[[ ! -f "$PROJECT_ROOT/packaging/RELEASE-README.md" ]] ||
  cp "$PROJECT_ROOT/packaging/RELEASE-README.md" "$NOTICES/README.md"
cp -r "$PROJECT_ROOT/packaging/licenses" "$NOTICES/licenses"
cp "$SDL2_SOURCE/LICENSE.txt" "$NOTICES/SDL2-LICENSE.txt"
while IFS='|' read -r source name; do
  [[ -f "$PROJECT_ROOT/$source" ]] && cp "$PROJECT_ROOT/$source" "$NOTICES/$name"
done <<'NOTICE_LIST'
patches/enhanced-water/LICENSE-WAVE-RACE.txt|WAVE-RACE-WATER-LICENSE.txt
extern/rt64/LICENSE|RT64-LICENSE.txt
extern/rt64/src/contrib/imgui/LICENSE.txt|Dear-ImGui-LICENSE.txt
extern/n64-modern-runtime/COPYING|N64ModernRuntime-COPYING.txt
extern/n64-modern-runtime/N64Recomp/LICENSE|N64Recomp-LICENSE.txt
extern/libdatachannel/LICENSE|LIBDATACHANNEL-LICENSE.txt
extern/mbedtls/LICENSE|MBEDTLS-LICENSE.txt
extern/libdatachannel/deps/libjuice/LICENSE|LIBJUICE-LICENSE.txt
extern/libdatachannel/deps/usrsctp/LICENSE.md|USRSCTP-LICENSE.txt
extern/libdatachannel/deps/json/LICENSE.MIT|NLOHMANN-JSON-LICENSE.txt
extern/libdatachannel/deps/plog/LICENSE|PLOG-LICENSE.txt
NOTICE_LIST

step "Verify no game data is staged"
# Same contract as the PowerShell packager: no ROMs, saves or N64 images, by
# extension or by header magic.
python3 - "$STAGE_DIR" <<'PY'
import pathlib, sys
blocked_suffixes = {".z64", ".v64", ".n64", ".eep", ".mpk", ".sra", ".fla", ".o2r", ".otr"}
blocked_magic = {bytes.fromhex("80371240"), bytes.fromhex("37804012"), bytes.fromhex("40123780")}
for path in pathlib.Path(sys.argv[1]).rglob("*"):
    if not path.is_file():
        continue
    if path.suffix.lower() in blocked_suffixes:
        raise SystemExit(f"Game data in stage: {path.name}")
    with path.open("rb") as handle:
        if handle.read(4) in blocked_magic:
            raise SystemExit(f"ROM header found in staging: {path.name}")
print("[OK] staging contains no game data")
PY

step "Assemble APK"
task=":app:assembleRelease"
[[ "$VARIANT" == release ]] || task=":app:assembleDebug"
gradle -p "$PROJECT_ROOT/packaging/android" \
  "-PdkrSdlSource=$SDL2_SOURCE" "-PdkrNativeStage=$STAGE_DIR" \
  :app:clean "$task" --no-daemon || fail "Android APK assembly failed."

suffix=""
[[ "$VARIANT" == release ]] || suffix="-debug"
OUTPUT="$OUTPUT_DIR/DKR-R-$VERSION-Android-arm64-pixel10$suffix.apk"
[[ ! -e "$OUTPUT" ]] || fail "Refusing to replace an existing APK: $OUTPUT"
cp "$PROJECT_ROOT/packaging/android/app/build/outputs/apk/$VARIANT/app-$VARIANT.apk" "$OUTPUT"

echo
echo "[OK] $OUTPUT"
sha256sum "$OUTPUT"
# Report the signing identity: a changed certificate means Android will refuse
# to install this build over the previous one.
APKSIGNER="$(ls "$ANDROID_HOME"/build-tools/*/apksigner 2>/dev/null | head -1)"
if [[ -x "$APKSIGNER" ]]; then
  "$APKSIGNER" verify --print-certs "$OUTPUT" 2>/dev/null |
    grep -i "certificate SHA-256 digest" || true
fi
