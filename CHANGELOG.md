# DKR-R changelog

This is the player-facing release history. Current instructions live in the
[documentation index](docs/README.md); dated investigations retain the findings
and limitations of the build they describe.

## Unreleased

- Play now has a **Skip intro** tickbox! Go straight to the normal Start Game
  title screen when launching offline. The choice is remembered; online startup
  and Track Lab's direct-track launch stay as they were.

## 1.0.5 Public Build — 10 October 2026

There's been a lot added since the earlier releases! This update brings the
current local work together with contributor PRs
[#49](https://github.com/ThatGuyMcd/DKR-R/pull/49) and
[#50](https://github.com/ThatGuyMcd/DKR-R/pull/50).
These contributions are integrated in this source branch; this entry does not
claim the branch has already been pushed or the PRs closed on GitHub.

### Launcher and controls

- Shared launcher/overlay styling, more consistent spacing, readable text and
  backgrounds, and better use of available width.
- ROM selection restored in Play, portable-mode control and startup window mode
  following the selected graphics setting.
- Textures separated from Mods / Hacks. Mod libraries use searchable/filterable
  cards, with details, hide/restore, removal and duplicate review.
- Bulk re-preparation of retained legacy patch sources in Help & imports.
- SDL2/SDL3 controller ownership, hot-plug and discovery fixes, including work
  to avoid periodic controller-discovery stalls.
- Android's separate port adds a mobile menu, finger scrolling and editable
  N64-style touch-control groups. Hardware performance remains under review.

### Tracks and characters

- Additive legacy tracks with wooden-frame previews beneath the original worlds.
  Fresh Track Select entry starts at Dino Domain.
- Four visible custom actors on the original Player Select stage, with L/R
  browsing that skips characters displayed in other custom slots.
- A much larger enabled character roster, bounded by presentation identities
  and memory budgets rather than the previous two/16-character limits.
- Custom AI selections in eligible races and scoped custom portraits/voices
  wherever the mod actually supplies replacements.
- Reviewed native behaviour adapters for racer parameters, object-header data
  and known executable changes. Unknown code still requires a dedicated adapter.
- Track Lab geometry, textures, minimaps, music, waterfalls and HD pack workflows.
  Contributor texture cleanup uses recoverable multi-file replacement.

### Online play

- Auto defaults to genuine Experimental Rollback; Host Prediction and Strict
  Input Sync remain selectable in the same launcher.
- Experimental play follows the normal game boot/menu flow and supports the
  admitted US v1.0/v1.1 payloads, rather than forcing a single test track.
- Checkpoint, input scheduling, scene transitions, Track Select, finish/boss
  handling, resource lifetime and disconnect-return fixes.
- Host-managed mods with client consent, local content reuse, missing-payload
  transfer, host/client progress and offline retention of verified downloads.
- Separate online EEPROM/Pak ownership and confirmed persistence, protecting
  personal offline progress.
- Some simulation-affecting visual settings and independent sound-category
  mixing remain restricted in Experimental Rollback. It is still experimental.

### Presentation and performance

- Grouped one/two-player HUD placement without horizontally stretching artwork.
  The accepted three/four-player HUD remains separate; the HUD editor is disabled.
- Finish interpolation and the timing of the return to 4:3 during frame zoom-out.
- Camera near-plane/clipping work and ultrawide sky-motion corrections.
- Water draw, matching and scrolling-wrap improvements.
- Optional enhanced water, screen-space reflections and experimental hardware
  ray queries, with aspect-ratio, budget, lifetime and performance fixes.
- Bounded renderer workers, owned rendering snapshots, pacing and allocation
  work; fixes for audio dropouts and unsafe custom-model lifetime.
- Optional offline Modern camera with an F7 tuning panel, contributed in PR #50.
- macOS source/build support and setup/lifecycle improvements from PR #49.
- Runtime log history, more useful diagnostics and package/worker checks.

### Delivery and limitations

This delivery contains Windows x64, Linux x64 AppImage and the Track Lab addon.
It does not replace the separate Android/ARM previews or claim a newly tested
macOS release. Automated checks cover compilation, policies, runtime ownership
and package contents; real gameplay, WAN routes and hardware still need testing.
See [release validation](RELEASE-VALIDATION.md) for the recorded results.

## Earlier releases

The previous README recorded these highlights. They are retained as release
history, not as current instructions or a guarantee that every device was tested.

### 1.0.4

The earlier documented Windows/Linux release included both supported US ROM
revisions, Accurate/Modern presentation, controller profiles, gyro, audio/EQ,
Save Manager, Magic Codes, texture packs, CRT filters and support tools.
Its detailed qualification remains in [release validation](RELEASE-VALIDATION.md).

### 1.0.2

Stability and performance fixes.

### 1.0.1

- Shadow/vehicle-prop, texture-alignment and widescreen black-bar fixes.
- Extended controller compatibility, SDL2/SDL3 and native Steam Deck gyro.
- Launcher/overlay improvements and per-vehicle control inversion.
- Two-player online play and shared two-player Adventure routing.
- Quick Join, authenticated friend invitations and synchronised startup.
- Performance work and an audio-overflow crash correction.

### Original release

Accurate 4:3/30 FPS and optional Modern widescreen/interpolated presentation,
local multiplayer controls, audio settings, saves, Controller Paks and renderer
enhancements formed the starting point. The current guides describe how those
features work now.
