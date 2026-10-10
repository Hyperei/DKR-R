# Building DKR-R 1.0.5

Build from the pinned sources and authored Patch Pipeline. Don't patch RT64,
N64Recomp, N64ModernRuntime or generated functions by hand to make a build pass.

Windows and Linux releases go together, and Linux must include an AppImage.
The [platform guide](PLATFORMS.md) separates public packages from preview ports.

## Tools and preparation

Windows needs Visual Studio 2022 Desktop development with C++, Git, Python 3,
PowerShell and WSL2/Ubuntu for the preparation workflow.
Linux needs CMake 3.24+, Ninja, a compatible C++ toolchain, Vulkan/display
development libraries and the AppImage packaging tools.
Use the locked dependency revisions, not arbitrary installed substitutes.

Supply your own clean US v1.0/v77 and US Rev A/v1.1/v80 ROMs locally.
A complete release needs both native CPU payloads and the matching Rare RSP
output. Ordinary players only need the ROM they want to play.

`Build-DKR-Runtime.cmd` invokes `scripts/Prepare-DKR-Runtime.ps1`.
It prepares the decomp ELF, pinned dependencies and generated/probe inputs.
It is not, by itself, the complete mod-enabled/owned-rollback Public Build.
`Diagnose-DKR-Recompile.cmd` is a diagnostic regeneration entry point; it
does not replace composing both production policies.

Use fresh build/output directories. Keep ROMs, generated output and local
profiles out of source control.

## Compose both normal payloads

The versioned policies are only part of the policy. Production legacy scene,
track/character menu, presentation and behaviour fragments must be composed
before N64Recomp runs:

```text
python scripts/generate_legacy_menu_qualification.py --characters --character-menu
    --v77-build <prepared-v77-ELF-and-ROM-folder>
    --v80-build <prepared-v80-ELF-and-ROM-folder>
    --recompiler <N64Recomp-executable> --output <fresh-pipeline-output>
```

The displayed arguments are one command; join the lines using your shell's
continuation syntax. Paths in angle brackets are placeholders.
Use the resulting `generated-v77` and `generated-v80` directories separately.
CMake checks required hooks and rejects v80 pointed at the protected v77 output.

Revision translation uses named functions and checked offsets, not a global
address delta. If translating the base v80 policy, retain and validate its
revision-specific entries; don't overwrite them with a blind translation.

## Generate the owned rollback payloads

The current Windows/Linux Public Build also needs two isolated full-scene owned
outputs. A normal payload or the old single-track probe is not sufficient.

Run `scripts/generate_replay_probe.py --help` for the input contract.
It reads composed generated CPU inputs and the reviewed `recomp.h`, then emits
new isolated output; it does not edit the input tree. For each revision use:

```text
python scripts/generate_replay_probe.py <normal-generated-directory>
    <pinned-recomp.h> <fresh-owned-output-directory> --revision <77-or-80>
    --scene-cuts --audio-services --input-services --authored-cpu
    --isolate-symbols --full-scenes --mod-services --inline-memory
```

Despite the historical “probe/test” names, these checked full-scene outputs
supply the integrated backend. Native callback signatures, header hashes,
service ownership and the manifest are mandatory. Don't loosen a failed check
or edit generated output to admit a new closure.

## Configure a complete desktop build

Example Windows configuration, with placeholder paths replaced:

```powershell
cmake -S runtime-recomp -B build/public-windows -G "Visual Studio 17 2022" -A x64 `
  '-DDKR_RELEASE_VERSION=1.0.5' `
  -DDKR_RUNTIME_BUILD_RT64=ON -DDKR_RUNTIME_BUILD_SDL3_INPUT_HOST=ON `
  '-DDKR_GENERATED_SOURCE_V77=<normal-v77>' '-DDKR_GENERATED_SOURCE_V80=<normal-v80>' `
  '-DDKR_GENERATED_RSP_SOURCE=<prepared-RSP-output>' `
  -DDKR_EXPERIMENTAL_RACE_TEST=ON `
  '-DDKR_EXPERIMENTAL_OWNED_GENERATED=<owned-v77>' `
  '-DDKR_EXPERIMENTAL_OWNED_GENERATED_V80=<owned-v80>' `
  -DDKR_LEGACY_QUALIFICATION=OFF -DDKR_REPLAY_QUALIFICATION=OFF `
  -DDKR_EXPERIMENTAL_RENDER_QUALIFICATION=OFF
cmake --build build/public-windows --config Release --parallel
ctest --test-dir build/public-windows -C Release --output-on-failure -R '^DKR'
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/Package-Windows.ps1 -BuildDirectory build/public-windows
```

Quote whole `-D...` arguments when paths contain spaces. PowerShell arguments
containing dotted version values must also be quoted.

On Linux use Ninja and a fresh build directory, with the same reviewed payloads:

```bash
cmake -S runtime-recomp -B build/public-linux -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DDKR_RELEASE_VERSION=1.0.5 \
  -DDKR_RUNTIME_BUILD_RT64=ON -DDKR_RUNTIME_BUILD_SDL3_INPUT_HOST=ON \
  '-DDKR_GENERATED_SOURCE_V77=<normal-v77>' \
  '-DDKR_GENERATED_SOURCE_V80=<normal-v80>' \
  '-DDKR_GENERATED_RSP_SOURCE=<prepared-RSP-output>' \
  -DDKR_EXPERIMENTAL_RACE_TEST=ON \
  '-DDKR_EXPERIMENTAL_OWNED_GENERATED=<owned-v77>' \
  '-DDKR_EXPERIMENTAL_OWNED_GENERATED_V80=<owned-v80>' \
  -DDKR_LEGACY_QUALIFICATION=OFF -DDKR_REPLAY_QUALIFICATION=OFF \
  -DDKR_EXPERIMENTAL_RENDER_QUALIFICATION=OFF
cmake --build build/public-linux --parallel
ctest --test-dir build/public-linux --output-on-failure -R '^DKR'
DKR_LINUX_BUILD_DIR="$PWD/build/public-linux" ./scripts/Package-Linux-AppImage.sh
```

`VERSION` supplies the package version. Default outputs are
`dist/DKR-R-1.0.5-Windows-x64.zip` and
`dist/DKR-R-1.0.5-Linux-x86_64.AppImage`.
Keep prior outputs; the AppImage packager expects a fresh output/AppDir.
Do not skip runtime checks for a public package.

The simpler `Build-Windows.cmd`/`Build-Linux.sh` wrappers consume prepared
normal sources. Supply `-GeneratedSource`/`-Revision80GeneratedSource`
on the Windows script, or `DKR_LINUX_GENERATED_SOURCE`/
`DKR_LINUX_V80_GENERATED_SOURCE` on Linux.
They do not configure the extra owned outputs automatically; don't describe
that basic build as reproduction of the full experimental Public Build.

## macOS

Run `./Setup-macOS.sh` to install and check prerequisites, or
`./Setup-macOS.sh --skip-deps` to check without installing. Setup and Build's
preparation share the same checks: full Xcode with Metal, GNU Make 4+ (`gmake`),
CMake, Ninja, Python, SDL2, PCRE2, and a `sha1sum` supporting `-c --quiet`
(Homebrew `coreutils`). The verifier is checked before ROM extraction.

Build and either preparation script hold a checkout lock for their entire run.
A second invocation in the same checkout fails immediately; nested preparation
inherits the lock. Locks release automatically when processes exit, including
failed builds. Separate checkouts must also use separate `extern/`, generated
payloads, runtime tools, and build output directories; do not point overrides at
another active checkout's resources.

macOS packages get increasing `CFBundleVersion` values, including repeated
packages within one minute. `CFBundleShortVersionString` remains major.minor.patch.
Set `DKR_MAC_BUNDLE_VERSION=N[.N[.N]]` for an explicit release/CI build number
(up to 4/2/2 digits); keep explicit values increasing for the same bundle ID.
Packaging checks the version metadata copied beside the executable after linking,
so a new package label cannot silently rename a build of a different version.

With Xcode setup and Homebrew installed:

```bash
./Build-macOS.sh --rom-v77 "/path/to/US 1.0.z64" --rom-v80 "/path/to/US Rev A.z64"
./Run-macOS.sh
```

The script prepares pinned checkouts and both normal payloads. Add `--package`
for an ad-hoc-signed app ZIP, not a notarised public release. Bundled Homebrew
libraries inherit the host's macOS deployment requirements.

Setup recovery checks staged Python/native tools, and preparation uses isolated
revision trees. Do not edit live dependencies or delete protected directories
to recover a stage. See [macOS kit notes](../packaging/MACOS-BUILD-README.md).
The integrated experimental backend is gated out on Apple; use a supported
legacy backend rather than expecting Auto to provide an unavailable engine.

## Linux ARM64 and Android

Use the project-owned ARM/Android toolchains and packaging adapters, with
separate generated/build directories. The older [ARM recipes](ARM-PLATFORM-PREVIEW.md)
document the initial cross-build setup; their preview version and test counts
are historical, not current release certification.

Android needs the correct NDK/SDK, ARM64 SDL2 input, shared C++ runtime and
native alignment requirements. Don't reuse a desktop SDL3 worker as an Android
input backend. No APK or cross-compiled AppImage certifies a physical GPU.

## Release checks

Run metadata and asset checks:

```text
python scripts/validate_release_metadata.py
python scripts/scan_for_game_assets.py
```

Keep Controller Pak, SDL3/import-worker, online import-capability and owned
revision tests in the release process. Package scans must reject ROMs, extracted
assets, saves, logs, local config and signing material.
Ship the whole package and notices, not just the game executable.

Record source identity, configuration, package hashes, test results and
outstanding device/gameplay checks in [release validation](../RELEASE-VALIDATION.md).
