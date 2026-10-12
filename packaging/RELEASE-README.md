# DKR-R 1.0.6

There's been a lot added! This release includes custom tracks and characters,
Track Lab integration, texture-pack priorities, online mod sharing, enhanced
water and the updated launcher/overlay.

Packages cover Windows x64, Linux x86_64/ARM64 AppImages, Steam Deck and
Android ARM64. Steam Deck uses the Linux x86_64 payload. ARM, Android and
SteamOS still need physical-device qualification; Apple binaries are not supplied.
Experimental Rollback and hardware ray tracing remain experimental.

In Play, Skip intro logos offers Start menu or Character select. Start menu
keeps its original first sequence. The setting does not change online startup.
RTX lava lighting adds optional pulsating light to nearby geometry, with
its own on/off checkbox, Performance/Balanced/High quality and intensity,
pulse strength/speed and range settings. Balanced uses a half-resolution
lighting field; Performance uses quarter resolution. Neither changes the
game's selected image resolution. The molten surface has rough hot-rock
islands and shaded ridges, and stays enhanced when scene lighting is off.
Unsupported devices keep the emissive lava surface. This is ray-tested diffuse lighting, not a full
multi-bounce path tracer; dynamic racers are not hardware occluders.

Android build 1060001 keeps the previous application ID and signing identity
to preserve upgrades. Its release Java wrapper is not debuggable; it retains
the existing development signing key, not a newly provisioned production key.

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
Each custom row has its own header and cycles through six custom backgrounds.
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
