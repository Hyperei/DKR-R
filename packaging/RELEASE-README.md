# DKR-R 1.0.5 Public Build

There's been a lot added! This release includes custom tracks and characters,
Track Lab integration, texture-pack priorities, online mod sharing, enhanced
water and the updated launcher/overlay.

The current Public Build delivery is Windows x64 and Linux x64 AppImage.
Android/ARM previews and macOS source support have separate qualification.
Experimental Rollback and hardware ray tracing remain experimental.

## Start playing

Keep the complete package together, including libexec workers and notices.
Start DKR-R.exe on Windows or the AppImage on Linux, then add your own clean
Diddy Kong Racing US v1.0 or US Rev A/v1.1 ROM in Play.
.z64, .v64 and .n64 byte orders are detected from their headers.
Both revisions run in the same application.

Accurate keeps the original 4:3/30 FPS presentation. Modern enables widescreen,
interpolated presentation and optional enhancements.
Escape, F1 or controller Back/View opens settings; Alt+Enter/F11 toggles fullscreen.

## Mods, textures and online

Mods / Hacks imports legacy xdelta/ZIP patches and DKR-R track packages.
Custom tracks keep the originals and use wooden-frame Track Select previews.
Four custom actors share the original Player Select stage; L/R browses the
remaining enabled roster. Reviewed behaviour adapters are not unrestricted
ROM-code execution.

Textures imports RT64/Rice packs and manages CRT filters. Multiple packs can
be active; lower priority numbers win texture clashes.

DKR-R Online uses Quick Join codes or friend invitations. The host chooses
the session mods; clients consent, reuse matching local data, receive only
missing payloads and can keep verified downloads inactive for offline use.
No ROM is transferred. Everyone needs a compatible complete build and the
same original revision.

Auto selects Experimental Rollback. Host Prediction and Strict Input Sync
remain available. Experimental play follows the normal menus, but has stated
local scenery/audio restrictions and ongoing real-world qualification.
The optional Modern camera/F7 tuning panel is for offline local races.

## Saves and support

Back up saves before updating. Offline, modded and online save routes are
separate. Save Manager includes Offline, Online, Modded and Recovery views.
Portable mode in Play takes effect next application launch; it does not move
existing data. A different profile can look empty without deleting the old one.

About DKR-R → Support summary exports support information and opens diagnostic
folders. Android previews also offer startup-log export. Runtime logs retain
history beyond runtime.log/runtime-previous.log; review them before sharing.

No ROMs, personal saves or sample mods are supplied. Verify the official
release SHA-256 before running a downloaded package.

## Guides and credits

Current guides: https://github.com/ThatGuyMcd/DKR-R/tree/main/docs
Release history: https://github.com/ThatGuyMcd/DKR-R/blob/main/CHANGELOG.md
Source and updates: https://github.com/ThatGuyMcd/DKR-R

PR #49 contributes macOS build/platform support and ultrawide sky fixes.
PR #50 contributes Modern camera and Track Lab improvements.
POOTERMAN created the application icon; sr.gu leads the community HDR pack.
Golden Balloon informed HUD grouping; Wave Race 64 Recomp informed enhanced water.
See the bundled notices and THIRD_PARTY.md in the source for exact licensing.
