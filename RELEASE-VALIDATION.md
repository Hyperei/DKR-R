# Release validation

## 12 October 2026 — 1.0.6 Windows Track Select brief-stretch correction

The follow-up recording shows preview images, arrows and captions briefly
widening independently of their wooden frames. The widened per-draw scissors
also entered RT64's aggregate framebuffer extent, which its rectangle renderer
uses to infer native aspect. That can disable the native-aspect correction
without changing the separately projected wooden frame.

The authored bridge now remembers only the native screen images observed by
Track Select's UI/preview markers. Before full-sync submits the producer-owned
workload, it confines those images' aggregate horizontal extent to the original
320-pixel framebuffer. Individual draw/projection scissors and color/depth
write extents retain their wider coverage. The bounded identities reset with
each task; no GPU wait, extra cinematic, shared guest-state change or dependency
source edit is introduced. Existing checked guest/replay exports are reused.

- Windows Release suite: 140/140 passed (246.29 seconds).
- Real RT64 framebuffer/rectangle conversion regression: 9,680 signed image,
  glyph and arrow-sized placements across five aspects and four clipping unions;
  widened per-draw scissors and offscreen targets remain unchanged.
- Signed drawing/window/identity-budget regression: 9,154 checks passed.
- Both owned revision dispatches: 58 production checks passed. Native callback
  and guest-helper audit hashes remain unchanged.
- Version 1.0.6 and 57 patch pins validated; repository asset scan passed.
- Exact-package importer, controller and Controller Pak checks passed. All 58
  ZIP files were scanned; the executable matches the tested build and all seven
  custom-background assets match source. No ROM, save, crash dump or private
  fixture is included. Source export scan passed (1,283 files).

Windows test ZIP: `G:/DiddykongWorkFolder/release-106-20261011/track-select-regression-0024/release/DKR-R-1.0.6-Windows-x64.zip`.
SHA-256: `f6f2314312bb6328f0f59b8fc134962ee12260760d5b746906da9d34dbabb758`.

The brief stretching still needs a hands-on visual retest, including rapid
scrolling, wide-aspect edges and confirmation zoom. No gameplay window was
opened. This Windows-only refresh supersedes the preceding test ZIP; other
platforms remain deferred pending the user's test. Nothing was pushed to GitHub.

## 12 October 2026 — 1.0.6 Windows Track Select widescreen refresh

The horizontal-scroll recording showed preview contents being clipped at the
original 4:3 boundaries while the wooden frames remained visible. Track Select
now has a scoped signed drawing path for images, glyphs and arrows. The live
preview uses its original frame bounds intersected with the current renderer
window; confirmation zoom and frame proportions remain unchanged.

Both reviewed US revisions prepare five columns by three rows in a separate
240-byte guest stack table. The original nine-entry global is never extended
or overwritten. Stock unlock/placeholder/arrow logic is retained, custom rows
use the same bounded table, and no additional cinematic scenes are loaded.
Local aspect/resize choices stay outside shared rollback memory. Overlong
in-game track captions are abbreviated to protect the fixed drawing heaps;
installed mod names and launcher details retain their full text.

Golden Balloon and Wave Race now appear alongside the other compact technology
credit rows. Long labels wrap clear of the fixed-width buttons, narrow layouts
stack, and all original acknowledgements and links are retained. The shared
About implementation applies this to the launcher and overlay.

- Fresh normal, fixture and isolated owned outputs generated through the checked
  Patch Pipeline for both revisions; no protected generated input or submodule edited.
- Windows Release suite: 140/140 passed. The signed-draw/window tests passed
  9,126 checks; actual generated retail table/rectangle functions passed 42,123
  checks per revision, including unlocked Future Fun Land and 512 custom tracks.
- Track Select adapter/emitter, bounded command execution, marker policy and
  mixed local-aspect memory regressions passed. Both owned revisions passed
  all 58 production dispatch checks. The generator audit passed 49/49 checks.
- Existing real-GPU water/lava tests passed on DirectX 12 and Vulkan. These are
  shader regressions, not a claim of visual gameplay or frame-rate qualification.
- Version 1.0.6 and 57 pinned patch files validated; repository asset scan passed.
- Exact-package importer/capability, SDL3 controller, Controller Pak and two
  SDL2/SDL3 ownership round trips passed. The ZIP has 58 files; its executable
  matches the tested build and all seven custom background files match source.
  No ROM, save, crash dump or private fixture is bundled. Final source export
  passed its archive scan (1,283 files).

Windows test ZIP: `G:/DiddykongWorkFolder/release-106-20261011/track-select-widescreen-20261012/release/DKR-R-1.0.6-Windows-x64.zip`.
SHA-256: `cc64c158c8c758e3efba14047f5fa39640c56b5eb8906b3ee16121acdcb70c21`.

This is a Windows-only test refresh. Wider-screen gameplay/resize/rapid-scroll
and launch/return checks still need the user's hands-on test. No gameplay window
was opened. Previous Linux/Steam Deck/Android 1.0.6 binaries do not contain this
refresh; use this exact build on every online peer until the other platforms
are rebuilt from the same source and qualified. Nothing was pushed to GitHub.

## 11 October 2026 — 1.0.6 Windows Track Select correction

The supplied recording showed the new background flashing over the entire
menu, then falling back to the stock Future Fun Land artwork. Source tracing
confirmed that `thread3_main` calls the background through `&gCurrDisplayList`
before `menu_loop` updates `sMenuCurrDisplayList`. The earlier custom bridge
incorrectly wrote through that stale menu pointer.

The correction retains the actual callback argument from entry to exit and
appends into that current list. The common, bounds-checked command emitter is
now covered by regressions for stale global pointers, material/quad/UI ordering,
two adjacent rows, consecutive caller buffers and invalid tails. Transient draw
ownership is excluded from checkpoints and cleared on restore. Images, row
cycling, headings and local-only widescreen cover remain unchanged.

Fresh audited Patch Pipeline exports for both US revisions were regenerated
read-only into new directories. All 44 files per revision are byte-identical
to the previous exports, so those checked guest outputs are reused. The native
owned-contract identity includes the corrected adapter and bridge sources.
No protected generated source or submodule was edited.

- Windows Release runtime suite: 136/136 passed, including 1,126 native Track
  Select adapter/emitter checks and the stock/custom rollback-preview regressions.
- Both owned revision dispatches passed all 58 production checks. The fresh
  generator audit suite passed 49/49; generated exports were verified as above.
- Existing water/lava real-GPU checks passed on DirectX 12 and Vulkan. The
  enhanced-water and independent lava-lighting changes are retained unchanged.
- Version 1.0.6 and 57 pinned patch files validated; source asset scan passed.

- Exact-package importer/capability, SDL3 controller, Controller Pak and two
  SDL2/SDL3 ownership round trips passed. The ZIP's executable and all seven
  custom background files match the checked build/source. Package scans exclude
  ROMs, saves and private fixtures.

Windows correction: `track-background-fix-2233/release/DKR-R-1.0.6-Windows-x64.zip`
SHA-256: `0c590f0f777feaeaf2f09d1f80480a26f9edf70751eb2c596a539923628a773d`

Gameplay/visual confirmation remains user-led; other platforms stay on hold.

## 11 October 2026 — earlier Windows test refresh (superseded)

This refresh follows the first local 1.0.6 candidate below. Other platform
rebuilds are on hold until the Windows changes have been tested by the user.
No gameplay window is opened as part of this build's automated checks.

- Skip intro resets the logo-owned music lock, delay and fade before the
  normal title-screen constructor queues its music. Both US revisions use
  the same reviewed startup policy; ordinary and online startup are unchanged.
- Custom Track Select rows use all six supplied backgrounds in a repeating
  cycle, with a distinct CUSTOM TRACKS heading for each row. Leaving the
  custom rows restores the final vanilla world's scrolling background.
  The owned rollback path applies viewport cover only in disposable draw state.
- Enhanced lava adds analytically shaded rough hot-rock islands and ridges
  using its existing noise samples. This is material relief, not tessellated
  geometry, changed collision or an additional ray pass.
- RTX lava scene lighting has a separately saved on/off setting. Performance,
  Balanced and High use quarter-, half- and full-resolution lighting fields;
  Balanced is the default. The game image stays at its chosen resolution.
  Full-precision depth and depth-aware reconstruction avoid smearing across
  silhouettes or treating distant receivers as sky. The existing aggregate
  scratch budget also accounts for pooled lighting images; allocation failure
  retains the enhanced lava surface without the optional light pass.

- Windows Release runtime suite: 136/136 passed.
- Production owned revision dispatch: 58 checks passed across both US revisions.
- Fresh owned generator suite: 49/49 passed. Build tooling: 31 covered, with
  seven Windows platform-specific skips. Both startup guest revisions passed.
- DirectX 12 and Vulkan real-GPU checks passed. Each lighting quality passed
  near and distant depth receiver readback at 1x and 4x MSAA, alongside actual
  1440p scratch budgeting, prepared-buffer ownership, reuse and recovery.
  The sampled lava material retained bright molten coverage, white-hot cores,
  spatial variation and bounded continuous animation. These synthetic checks
  do not establish full-game frame rates or visual gameplay correctness.
- The custom menu adapter passed 1,105 checks. All six bundled images match
  the supplied originals byte-for-byte; their proxies match the actual RT64
  texture hasher. Source asset scan: 2,057 files inspected; release metadata
  validated 1.0.6 and 57 pinned patch files. Protected source trees are unchanged.

- Exact-package ModWorker/import capability, SDL3 controller, Controller Pak
  and two SDL2/SDL3 ownership round trips passed. The ZIP contains all six
  backgrounds plus their renderer manifest. Its executable matches the checked
  Release binary. No gameplay, diagnostic fixture, ROM or save is bundled.

Windows refresh: `windows-refresh/release/DKR-R-1.0.6-Windows-x64.zip`
SHA-256: `59b7cd3528d9f8d799bfae9223e57061612a1131b1fad49438e27a060b1b282d`

This does not claim a measured RTX 2080 Super frame rate, full-game visual
qualification or finished real-WAN rollback qualification.

## 11 October 2026 — earlier 1.0.6 candidate (superseded)

Runtime source: `9d0e86a`. GitHub main through `d089593` is included alongside
the preserved local work. Integration was conflict-free; RT64, N64Recomp,
N64ModernRuntime, RecompiledFuncs and RecompiledPatches were not edited.
Dependency changes are composed into isolated build stages by the authored
Patch Pipeline. Track Lab and the contributor's Blender changes are retained.

- Skip intro keeps the Start menu's original opening sequence. Its optional
  Character select destination requests the normal retail title-screen exit;
  online startup and direct Track Lab launches remain unchanged.
- Lava lighting uses bounded ray-tested diffuse illumination with intensity,
  pulse strength, pulse speed and range controls. Static geometry occludes it;
  dynamic racers are not in its acceleration structure. This is not a full
  multi-bounce path tracer or a gameplay performance measurement.
- Windows Release suite: 136/136 passed. Linux x86_64: 137/137 passed.
- Production owned revision dispatch: 58 checks passed on Windows and Linux
  x86_64, covering both US revision payloads.
- Real GPU water/lava checks passed on Windows DirectX 12 and Vulkan, and
  Linux Vulkan under WSL. Lava receiver readback, occlusion, smooth pulse and
  intensity-off passed at 1x and 4x MSAA. These are headless renderer checks,
  not full-game visual or frame-rate qualification.
- Build tooling: 30 tests covered on each desktop platform. Windows had seven
  platform-specific skips; Linux passed all 30. Startup policy/guest and owned
  generation checks passed. Four isolated HLE staging cases passed, including
  fail-closed rejection of a partially prepared guard.
- All 30 plain Blender addon suites were covered. ROM extraction was checked
  separately with the required NumPy runtime; six Blender-background suites
  were not run because Blender is unavailable on this host.
- Release metadata validated 1.0.6 and 57 pinned patch hashes. The final runtime
  source export passed the 1,268-file game-data scan. No ROMs, saves or private
  replay fixtures are shipped.
- Android package: application ID unchanged, version code `1060001`,
  non-debuggable, signature verified, AArch64 libraries and APK entries
  16 KiB-aligned. Native payload checks passed. The existing debug signing
  identity is retained for in-place upgrades; this is not a newly provisioned
  production/Play Store signing key. Do not uninstall just to update.
- Linux ARM64 qualification and packaging were not completed for this
  candidate. Further cross-platform rebuilds are deferred at the user's request.

Earlier candidate artifact SHA-256 values (not the Windows refresh):

| Artifact | SHA-256 |
|---|---|
| DKR-R-1.0.6-Windows-x64.zip | `eb2a607e253737eb219563fc089b3363bac4fdfb84d1707cda74e8a42db9cf90` |
| DKR-R-1.0.6-Linux-x86_64.AppImage | `563afd538ea4a64a7c73bdfe0a0be8106011c2486adfac1d6747db765b5e65b4` |
| DKR-R-1.0.6-SteamDeck-x86_64.AppImage | `563afd538ea4a64a7c73bdfe0a0be8106011c2486adfac1d6747db765b5e65b4` |
| DKR-R-1.0.6-Android-arm64.apk | `b298fafd3fdf6816cce0a7285cadd696ac0f139a7db37989dc043286944643fe` |
| track_lab.zip | `fb915375dcf19cbd06cecb9b31123f858f9bdba493e4677794e7866bf7bcae51` |

Steam Deck uses the Linux x86_64 payload. Physical Android, Linux ARM64 and
SteamOS checks, complete visual gameplay and real WAN matches remain user-led.
No autonomous gameplay windows were opened. Experimental Rollback remains
experimental. No macOS/iOS binary, GitHub push, release tag or publication is
implied by this local build and integration. The contributor's existing iOS
validation below is preserved, not claimed as new 1.0.6 device qualification.

## 11 October 2026 — iOS sideload validation

Validated on an iPhone 18 Pro Max running iOS 27, built with Xcode 27 and
the iOS 27 SDK for an iOS 18.0 deployment target, from both prepared US
revision payloads. The IPA was re-signed with an Apple Development identity
and installed with `devicectl`.

- `bash scripts/build-ios.sh` rebuilt and packaged the device app from a
  clean dependency checkout; all patches applied from pristine sources.
- The app launched, found a v1.0 ROM copied into Documents and reached
  gameplay with touch controls. After Control Center the game and overlay
  stayed landscape at full resolution; logged SDL and ImGui sizes held at
  956x440 points and 3x scale throughout.
- All 13 IPA packaging tests and the build tooling tests passed; release
  metadata validated 57 pinned patch hashes; the asset scan passed.

Artifact: `dist/DKR-R-1.0.5-iOS-arm64.ipa`
SHA-256: `e7e53940828208f664142cf3a8c62a6b46d2d003c8e82c06a961f06ee55bbc8b`

Audio, audio interruptions, simultaneous touch comfort, controller switching
and iPad layouts remain unverified. No release was published.

## 10 October 2026 — 1.0.5 Public Build

The current desktop delivery combines the preserved local work with PR #49
head `3c5e28ae8d6eb6465c3dc5e5fb128a85a14c6180` and PR #50 head
`dba2b742bed706cb9bab8ffbf2e901cff2cf1dbd`. Validated runtime source:
`6bbc2e373ffa3f0f1cdfd14b7c08e20a6a5d750b`. The subsequent documentation-only
refresh does not change that runtime or its recorded package hashes.

- Windows Release runtime suite: 129/129 passed.
- Linux Release runtime suite: 129/129 passed.
- Owned revision dispatch: 58 production checks passed on each platform.
- Build tooling: 18 tests per platform; Linux passed all, Windows had three
  Bash-only skips.
- Release metadata: 1.0.5 and 48 pinned patch hashes validated.
- Packaged SDL3/import workers, online import capability and Controller Pak
  checks passed; two SDL2/SDL3 ownership round trips passed on each platform.
- Track Lab retained the byte-identical addon that passed 35 suites, including
  six Blender-background suites.
- Source export asset scan: 1,189 files passed. Protected dependency/generated
  trees remained unchanged from the preservation checkpoint.

Package SHA-256 values:

| Artifact | SHA-256 |
|---|---|
| DKR-R-1.0.5-Windows-x64.zip | `461aaaee0dc3369a84e59b5ed4c37328b3d2038515eff12e0de72049ca9c4f00` |
| DKR-R-1.0.5-Linux-x86_64.AppImage | `2cc723ed4a56fbebad788e81ccdad316533be412f5493451eb962e7f1682a2ff` |
| track_lab.zip | `4acb705d63c1d92ef8051aecf63acaed70db5c0cdd2bb9b4499660aa51df3100` |

These are the runtime packages produced before the documentation refresh;
they have not been repackaged by editing this record. Their staged VALIDATION
and SHA256SUMS records identify the exact delivery.
Compilation/headless/package checks are not visual gameplay or WAN testing.
Those checks remain user-led. Experimental Rollback remains experimental.
This delivery does not contain new Android, ARM64 or macOS binaries; macOS
preparation syntax/checks are not a native Mac build. No GitHub publication is
implied by local PR integration.

See [current guides](docs/README.md) and [release history](CHANGELOG.md).

### Documentation refresh — 10 October 2026

The current guides, release history and packaged readme templates were updated
against the source above. This was a documentation-only pass, not another
runtime build or gameplay test.

- 44 current/reference Markdown files checked; all 297 local link targets exist.
  Historical machine-specific paths and line numbers remain recorded snapshots.
- All 76 dated reports/plans retain their original text beneath the new
  historical notices. The original legacy-test milestones are retained separately.
- Generator example arguments checked against the scripts' help; normal/owned
  payloads and the explicit RSP input are covered in the desktop build guide.
- Release metadata passed for 1.0.5 with 48 patch files verified.
- Repository asset scan passed with 1,979 files inspected.
- Whitespace checks passed. No non-document files, protected dependency/generated
  trees or exact licence texts were changed.

Existing binaries and their bundled readmes have not been replaced. The updated
packaging templates apply when the next packages are built.

## Historical results

The dated entries below retain their original target, paths, counts and limits.
They do not describe or qualify the current Public Build.

## 26 September 2026 — Beta 12 water performance rebuild

See [water performance implementation and qualification](docs/WATER-PERFORMANCE-IMPLEMENTATION-20260926.md)
for the measured water draw/matching bottleneck, water-only scroll-wrap correction,
83/83 project tests on both Windows and Linux x64, visible startup/preview checks,
and remaining physical-device/full-route checks. New Windows, Linux x64 AppImage
and Android packages are available. Linux ARM rebuilding remains deferred at the
user's request to leave its broken Docker environment alone.

Current development target: **v1.0.5 Beta 12**, based on main revision `ab141cb`
plus the existing ARM/mobile changes and the optimisation pass documented in
[Beta 12 qualification](docs/BETA12-OPTIMISATION.md). See
[Legacy mod compatibility coverage](docs/LEGACY-MOD-COMPATIBILITY-BETA7.md) and
[Offline mod usage and limits](docs/LEGACY-MODS-BETA.md).
Beta 11 build/package results are recorded with its artifacts; the historical
results below do not qualify that build.

See [Beta 10 regression investigation and qualification](docs/BETA10-REGRESSION-VALIDATION.md)
for the confirmed crash, exact patch scope and remaining visual checks.

Earlier development packages: see
[Optional mipmaps and loading-feedback validation](docs/GENERATED-MIPMAP-VALIDATION.md) and
[Online stability changes and qualification limits](docs/ONLINE-STABILITY.md).
The historical 1.0.4 results below do not qualify the current beta.

## v1.0.5 Beta 3 finish presentation rebuild — 8 September 2026

See [Finish presentation changes and qualification](docs/FINISH-PRESENTATION-VALIDATION.md).
This rebuild delays the post-race aspect switch until the first contracted
wooden-frame task has been submitted, and replaces task-wide finish-camera
interpolation disabling with per-camera shot continuity. Its exact final test
results and package hashes are in
`dist/playtest-v1.0.5-beta.3-finish-presentation-20260908/BUILD-MANIFEST.json`.
The preceding Beta 3 output is preserved. Native visual acceptance is pending;
online protocol 47 and all prior protected systems remain unchanged.

## v1.0.5 Beta 3 boss introductions and client input — 8 September 2026

- Windows Release: 68/68 project suites passed (178.23 seconds).
- Linux Release under Ubuntu 24.04/WSL: 68/68 passed (87.95 seconds).
- An expanded 120-case deterministic transport-clock matrix passed separately
  on both platforms after the full-suite run. It covers 2/3/4 peers, UDP and
  Quick Join commit-history policies, Rollback and Lockstep, 0/20/60/100/150/200 ms
  base RTTs, asymmetric jitter/loss, manual 1/2 and automatic settings, send
  blockage and a client scheduling pause. This tests production protocol/input
  logic, not actual WAN/SDK bandwidth or renderer timing.
- Six held button edges per client were delivered without duplication in each
  case; maximum observed scripted sample-to-consumption delay stayed below
  900 ms including the injected pause. Low manual delay still reduces throughput
  on high RTT paths; use the explicit automatic-delay option for those routes.
- Boss classification reads the active ROM's own redirect table through the
  existing generated payload entrypoint. Normal-race topology, controller slot
  ownership and the baseline/epoch/Arm/Go sequence are preserved.
- Gameplay protocol is 47; all racers require this build. Friend/profile/save
  protocols and dependency patches are unchanged. All 44 patch hashes validate.
- Rollback: `dist/playtest-v1.0.5-beta.2-friend-delivery-20260907`, copied and
  hash-checked with a source snapshot in `build/beta3-baseline-20260908`.
- No game/launcher window was opened for visual testing. Real boss fly-ins,
  both Adventure lead players, normal races after bosses, native Steam Deck,
  rotating physical hosts and distant Internet sessions remain user acceptance
  checks. Beta 3 is not promoted to known-good before that feedback.
- Package checks and artifact hashes are recorded in the Beta 3 output folder's
  build manifest. No GitHub publication is authorized by this packaging pass.

## v1.0.5 Beta 1 split-screen presentation — 7 September 2026

- Windows and Linux Release builds: 64/64 project suites passed on each platform.
- Three-player native Windows validation confirmed corrected world/HUD proportions,
  outward item and position-counter placement, and restored animated-number transparency.
- Scope, evidence, rollback location and remaining platform/player-count qualification
  are recorded in [Split-screen validation](docs/SPLIT-SCREEN-VALIDATION.md).
- No online simulation, controller routing, shadow, dependency-source or launcher
  implementation changes are included in this presentation pass.

## v1.0.5 Beta 1 boundary recovery / four-player — 7 September 2026

- Gameplay protocol 46 requires this rebuild on every connected machine.
- Normal races/minigames allow 2–4 players; JOINTVENTURE Adventure stays 2-player.
- Existing rollback packages and a pre-edit source snapshot are retained in
  `build/rollback-pre-online-gaps-four-player-20260907`; the prior candidate is
  not promoted to known-good solely on automated test results.
- Final-source suite, packaging and native results are recorded in
  `dist/online-boundary-recovery-four-player-v1.0.5-beta.1-20260907/VALIDATION.md`.
- Native chained-cutscene, four-device, Steam Deck and adverse-WAN qualification
  must not be inferred from the protocol regression suite. See the package's
  explicit outstanding gates before promoting this beta to a release.

## v1.0.5 Beta 1 online recovery qualification — 6 September 2026

- Cutscene-correction Windows Release build: 62/62 project suites passed (88.87 seconds).
- Cutscene-correction Linux Release build: 62/62 project suites passed (14.20 seconds),
  Ubuntu 24.04 under WSL. This is not Steam Deck gameplay qualification.
- Gameplay protocol 45 requires matching online-recovery-cutscenes builds on both peers.
- The preceding protocol-44 recovery package failed native consecutive-cutscene
  validation and is withheld. The corrected native balloon-plus-key retest is
  pending; protocol/policy tests are not a replacement for that acceptance gate.
- Regression coverage, protected systems and outstanding native/WAN acceptance
  gates are listed in `docs/ONLINE-STABILITY.md`.
- Output is staged separately from the previous cancellation rebuild; its
  packages and a pre-edit source snapshot remain available for rollback.
- Packaging and native test results are recorded in the output folder's
  `VALIDATION.md`; this section does not substitute historical release results
  for current-package tests.

## DKR-R 1.0.4 historical release validation

Validated on 3 September 2026 from `C:\DKRPort`.

## Reproducibility and repository safety

- `VERSION`, Windows resources and Linux AppStream metadata resolve to `1.0.4`.
- N64ModernRuntime, N64Recomp, RT64 and the matching DKR decomp are locked to
  exact commits in `dependencies.lock.json`.
- Release metadata validation verified all 42 dependency-patch SHA-256 values
  declared by `patches/manifest.json` against the pinned dependency revisions.
- The repository asset scan passed across 1,117 files.
- Final documentation contains no development-stage roadmap language or
  research-project references.
- ROMs, saves, Controller Pak files, local configuration, logs and build caches
  are excluded from source and binary releases.

## Windows x64

- Native Visual Studio 2022 Release build completed.
- All 55 DKR-R regression tests passed, including the two-map gameplay handoff,
  single-viewport Adventure boss policy, dual-control Adventure race policy and
  conflicting-scene rejection cases. The regular-race policy also verifies the
  retail six-racer topology and both four-racer challenge variants without
  weakening the boss or shared-hub gates. Accessory-policy coverage verifies
  that an enabled virtual Mem Pak takes storage-probe priority, a disabled Mem
  Pak falls through to the retail accessory probe, and disabling Rumble Pak
  clears the game-visible rumble mask.
- The built and staged executables passed the virtual Controller Pak round-trip
  and backup-recovery self-test. The staged SDL3 input host and two live
  SDL2-to-SDL3-to-SDL2 switching round trips also passed.
- The Launcher and Overlay share the same `Enable Mem Pak` and `Enable Rumble
  Pak` controls under Controls > Device; both settings retain their existing
  persistent `memory_pak` and `rumble` configuration keys.
- The 40-file staged package and ZIP passed prohibited-extension and N64-header
  scans.
- Executable file and product versions report `1.0.4`.

Deliverable:

```text
dist/DKR-R-1.0.4-Windows-x64.zip
SHA-256 93E3569E13DF6C225B7DDE99D486F98C29A4E1DE9A655F8E778E8C4A44722A25
```

## Linux x86_64

- Native Ubuntu 24.04/GCC 13 Release build completed with RT64/Vulkan.
- All 55 DKR-R regression tests passed, including the same Mem Pak priority and
  Rumble Pak enablement policy exercised by the Windows build.
- The native runtime and packaged AppImage passed the virtual Controller Pak
  round-trip and backup-recovery self-test. The private SDL3 input host and two
  live input-backend switching round trips also passed.
- AppImage staging passed a 189-file prohibited-data scan before SquashFS
  creation.

Deliverable:

```text
dist/DKR-R-1.0.4-Linux-x86_64.AppImage
SHA-256 53AB95E4B067109DA477F256BBF7ADEC33BBB57DBF3CD9318E5C15EDC14DDDF4
```

## Source archive

The deterministic source package passed a 353-file archive scan for unsafe
paths, prohibited game-data extensions and N64 ROM headers:

```text
dist/DKR-R-1.0.4-Source.zip
```

## Scope

These checks are non-interactive release gates. The confirmed gameplay,
graphics, interpolation and audio baseline remains the user-visible acceptance
test for DKR-R 1.0.4.
