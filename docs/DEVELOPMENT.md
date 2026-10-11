# DKR-R development guide

Keep the changes focused and reproducible. Project-owned runtime code lives in
`runtime-recomp/src/game`, tests in `runtime-recomp/tests`, release assets in
`assets`/`packaging`, automation in `scripts` and Track Lab in
`tools/blender`.

## First contribution: tests without a ROM

Policy tests exercise project-owned rules, state ownership and build tooling
without generated game functions or a GPU renderer. You need Git, Python 3,
CMake 3.24+, a C++20 compiler and Ninja (or Visual Studio 2022 on Windows).
From the repository root, prepare the pinned source dependencies once:

```sh
python3 scripts/bootstrap_dependencies.py --only n64-modern-runtime --only rt64 --only gekkonet --only monocypher --only libdatachannel --only mbedtls --only sdl3
python3 scripts/apply_dependency_patches.py
cd runtime-recomp
cmake --preset policy
cmake --build --preset policy
ctest --preset policy
```

On Windows, use `python` and the `policy-windows` preset for all three CMake/CTest
commands. Run the Ninja preset from a shell with a configured compiler. The
presets write to separate `build/policy` and `build/policy-windows` directories.
They use Release mode with test assertions retained. For a focused change,
append `-R <test-name>` to `ctest --preset policy`; use `ctest --preset policy -N`
to list available tests. These checks do not compile the complete game or
validate gameplay; follow [Building](BUILDING.md) for that path.

## Where a change belongs

| Change | Owner | Verification |
|---|---|---|
| Game-independent rules and state transitions | `runtime-recomp/src/game/*_policy.hpp` and focused tests | ROM-free policy preset |
| Runtime, UI and renderer integration | `runtime-recomp/src/game` and platform adapters | Focused tests, full target build, relevant runtime checks |
| Dependency behaviour | `patches/<dependency>` and `patches/manifest.json` | Patch tooling tests and affected target build |
| Guest game code | Revision policies and composed fragments | Regenerate both revisions and run guest/runtime tests |
| Setup and packaging | `scripts`, platform CMake modules, `packaging` | Hermetic tooling tests and a real target package |

Keep platform APIs behind their adapter boundary. Extract shared behaviour
when multiple callers need it; a file split alone does not reduce coupling.
Generated files and dependency checkouts are outputs of their owners above.

## Protected inputs

Never hand-edit submodules/dependency checkouts, `RecompiledFuncs` or
`RecompiledPatches`. Game changes go through versioned policies and composed
fragments; dependency changes go through checksum-pinned patches.
Use fresh generated output directories, not manual repairs to generated C.

Revision policies map named functions and checked offsets. Both normal and
owned outputs must contain the reviewed hooks. A policy change can require
regenerating all four CPU payloads, not simply relinking the executable.
[Building](BUILDING.md) explains the current flow.

## Behaviour and ownership

Accurate is the presentation reference. Modern must not accidentally speed up
simulation/audio or change race outcomes through interpolation.
Explicit reviewed mod behaviour is bound to the custom identity and compatibility
manifest; it must not leak into stock characters/courses.

Replay-safe state belongs to the checkpoint owner. Renderer resources, device
handles, audible output, filesystem changes and external effects need separate
lifetimes/confirmation. Do not add an untracked mutable sidecar to rollback.
Local camera/scenery/audio settings may require a presentation implementation,
not a private mutation of shared simulation.

## Checks before a release

- Add focused policy/ownership tests for changed behaviour.
- Run the complete configured DKR CTest suite, revision dispatch and required
  packaged worker/runtime self-tests.
- Validate dependency patch hashes and release metadata.
- Scan source and staged packages for game assets, saves, secrets and local data.
- Package Windows and Linux AppImage; preserve earlier artifacts.
- Record real hardware, gameplay and multiplayer checks separately from headless
  results. A passing unit suite is not a WAN/GPU certification.

Do not weaken a failing gate just to produce a package.
Respect the owner's testing preferences; when visual/gameplay testing is
user-led, record that outstanding acceptance rather than claiming it was run.

## Documentation belongs with the change

Before handing over a feature or fix:

1. Update the relevant current guide and actual menu names/defaults.
2. Update the changelog and any packaged readme affected.
3. State platform gates, limits and unfinished parts next to the feature.
4. Retain historical investigations with a clear historical/proposal status.
5. Check local links, example arguments and release metadata references.
6. Keep contributor attribution and licence texts intact.

Use direct, practical British English. Explain the user action first, then
technical detail where it helps. Don't describe experimental features as
flawless, “infinite”, or qualified on hardware nobody tested.

The [documentation index](README.md) is the starting point; technical formats
remain the reference for encoders, not casual prose to simplify at the expense
of field accuracy.
