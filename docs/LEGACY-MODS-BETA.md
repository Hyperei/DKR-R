# Custom tracks and characters — Beta 15 Playtest 12

## Import and play

1. Import your legally obtained original Game Pak in **Play**.
2. Open **Mods / Hacks → Import mods → ROM patches → Choose patch file**.
3. Choose an `.xdelta` file or a ZIP containing patches. The separate importer
   identifies tracks and characters automatically. A progress modal remains
   visible during inspection and preparation; Cancel stops the active job.
4. Read the result. Supported content appears in **My mods**, under **Tracks**
   (marked **Legacy**) or **Characters**. Imports do not enable themselves
   automatically.
5. Enable the content you want and start an offline game normally. Import both
   original US revisions before preparing tracks if you want to switch between
   v 1.0 and v 1.1. The library shows which variants have been prepared.

Custom courses appear below the original worlds in Track Select, using the
original wooden-frame previews and selection flow. Fresh Track Select entry
still starts at Dino Domain. Enabled custom characters join the original
animated selection stage; they do not replace their stock donors.

## Managing the library

Texture Packs and CRT Overlays now live in the **Textures** sidebar page, in
both the launcher and overlay. Mods / Hacks opens with its four sections
closed - **My mods**, **Magic Codes**, **Track Lab** and **Help & imports** -
and shows one at a time.

**Help & imports → Prepare all source imports again** rechecks retained patches
for all installed legacy content, including hidden entries. It shows progress,
supports cancellation and reports individual failures without aborting the
remaining sources. Existing selections and saves stay unchanged. New reviewed
editions are installed inactive; choose those editions in My mods when ready.
The October 10 follow-up adds exact rebuilt-code profiles for the eighteen
previously partial character packages and a scoped random-texture animation
adapter for eight of them. Unknown code is not granted arbitrary execution.
The sidebar sequence after Controls is **Save Manager → Textures → Mods / Hacks
→ DKR-R Online → About DKR-R**. Navigation and visible buttons use the same order.
**My mods** shows one full-name card per mod. Legacy courses (marked
**Legacy**) share the **Tracks** list with installed `.dkrmap` tracks (marked
**DKR**); custom characters have their own **Characters** list. Search (a
controller opens the on-screen keyboard), **Status** and **Sort by** sit above
the cards, and **Filters** adds track format, compatibility, source pack and
Visibility. A course's Game Pak variants share one card, and its switch turns
it on or off for the next launch.

**Details** provides activation, hide/restore, managed location, source import
details and removal. Hidden mods are inactive; restoring never activates them.
**Help & imports** lists every import with **Prepare this import again**, and
can refresh the library or turn all legacy tracks or characters off.
Up to 16 custom characters can be active, regardless of search filters. Four
appear on the original stage: two on the lower lawn and two on the upper shelf.
With more than four enabled, highlight a custom character and use **L / R** to
browse the rest. Other players' highlighted/confirmed choices stay visible.
Changes to installed content are locked while a game/lobby or import owns it.

Removing a card removes that content's prepared variants, not the other tracks
or characters from its source pack. Original ROMs, ZIPs, xdeltas, retained
source-import material and all saves remain untouched. Reimporting or explicitly
re-preparing its source can reinstall removed content. Import dates for older
entries may be unknown. Managed sizes exclude retained source-import material.

Removal stages and validates surviving entries before publishing a recovery
journal. Cancellation is honoured before commit; after commit, the transaction
finishes (or is recovered on the next library refresh). Do not manually alter
prepared receipts or removal journals.

## Clear beta limits

- **Host-managed online mods.** The host's enabled legacy tracks, characters
  and Track Lab courses are frozen when the lobby is created. Clients review
  the required files and explicitly consent before downloading, reconstruction
  and lobby admission. Received content lives in a separate online cache and
  never replaces offline activation, ROMs or saves. Every racer needs this
  build and the lobby's original Game Pak revision; patch reconstruction may
  also require the original revision named by an xdelta. No ROM is shared.
  Host Prediction and Strict Input Sync retain their existing flow. Experimental
  Rollback owns and checkpoints the native mod state as well as guest memory;
  its existing US v1.0 game-engine restriction remains. Real multiplayer visual
  and gameplay qualification of this new feature is still a playtester task.
- **16 enabled additions, four visible at a time.** More can be stored. Stage
  replacement is transactional and bounded by the native model budget; if a
  replacement cannot load, the previous choice remains intact.
- **Exact patch source required for decoding, portable assets afterwards.** An
  xdelta cannot be decoded using the wrong source bytes. Import its required
  original Game Pak once. Preparation creates track variants for both imported
  US revisions, including the v 1.1 2D-texture ID shift. Prepared characters own
  complete asset graphs and are validated against either native revision.
  If you import another Game Pak later, prepare/reimport the track patch again.
  PAL/Japanese game-engine support is not added by this asset adapter.
- This is a validated asset adapter, **not arbitrary ROM-hack execution**.
  Changed executable code is not run. Unsupported behaviour, dependencies or
  layouts can be rejected even when a patch works in an emulator. Rebuilt code
  is excluded rather than installed. Music instruments are compared by resolved
  content so exporter repacking does not require replacing the live sound bank.
- Enabled course assets have a 256 MiB aggregate preparation budget. Disable
  some tracks if the launch check reports that budget exceeded.
- Normal race models and native selection animations are supported for the
  reviewed character families. Custom selection actors now use the same shared
  animation clock as stock actors and share the selection camera's frontal
  plane, without changing stock actor transforms.
- Supplied custom portraits are used for the committed human racer's post-race,
  time-trial result, finishing-order, multiplayer result and trophy/ranking
  portraits. This changes portrait resources, not the established HUD layout.
  The scoped challenge/Adventure HUD and trophy portrait hooks also use the
  selected human's private assets. Saved ghost identity is not remapped. Mods
  that retain the donor's face pixels intentionally retain that portrait; the
  runtime log distinguishes supplied faces from that fallback.
- Selection/cancel/confirm sounds and all eight positive/eight negative race
  voice variants plus the horn use isolated character sound banks. Original
  sound selection, repeat suppression and spatial mixing remain in charge.
  A mod that supplies no replacement for a cue still sounds like its donor;
  arbitrary hacked-code audio behaviour and replacement global music are not
  implemented. Custom AI selections use the same scoped portrait/voice routing;
  stock selections retain their original resources.
- Custom record/ghost identity is not a new, independently named record system
  yet. Retail carrier slots can share records inside a modded save. Treat custom
  time-trial records/ghosts as experimental, not verified comparable records.

## Save protection

The complete enabled mod set and its original Game Pak determine an isolated
save folder: `saves/mods/<mod-set fingerprint>/` in your DKR-R profile.

On first launch of a mod set, its Adventure EEPROM is copied byte-for-byte from
the normal single-player save, or a checksum-valid blank save is created if
there is no original. Existing modded progress is never overwritten when the
same set is launched again. Changing the enabled set creates a different save
namespace. Controller Paks begin separately in that folder's `paks/` directory;
normal Paks are never copied into or replaced by it.

Disabling all custom mods restores the normal single-player path on the next
game launch. Online save routing remains unchanged. The existing Save Manager
continues to manage normal saves; modded-save management is not added there.

## Recovery and previous imports

Enable/disable operations are unavailable during gameplay, while preparing a
launch, or in a lobby. Prepared assets are revalidated before guest startup.
Preparation errors stay in a launcher modal instead of entering a partial game.
Runtime asset faults stop the guest and return to the launcher with an error.

**Import Details / Previous Imports** retains compatibility notes and offers
**Prepare This Import Again** for earlier review-only imports. Reimporting the
same patch deduplicates content and does not reset activation. Each library has
**Disable All In This Library** as a recovery action; this changes enablement
only and retains installed files and all saves.

The character-presentation rebuild reconstructs race sound banks from the
retained, verified source review when preparing a launch. Existing imports do
not need to be reimported, and their logical IDs and modded save paths remain
unchanged. Keep the managed `reviews` data: incomplete or altered source audio
is rejected before launch instead of playing unverified samples.

The packages include neither ROMs nor the sample mods. Use only patches you
have permission to use and distribute. No patch is uploaded to a service.

See [Beta 7 compatibility coverage](LEGACY-MOD-COMPATIBILITY-BETA7.md) for the
supplied seven-file test corpus, author-specific limitations and the difference
between automated resource validation and visual/gameplay acceptance.

## Build boundary

The Playtest 11 AI rebuild adds enabled custom characters to the offline AI
pool alongside unlocked originals. Human identities are reserved and each AI
draw is distinct. Native racer counts and inherited statistics are unchanged.
Bosses, hub worlds, time trials/ghosts and demos do not receive custom AI
identities. Playtest 12 also uses the frozen host-managed character pool for
eligible online races; clients' offline selections are never consulted.
L/R browsing stays in the current stage slot and skips the
characters already displayed in the other three slots.

`scripts/legacy_character_ai_policy.py` pins both original race-construction
bodies and checks the topology, participant-index and primary-spawn ABI. Its
four hooks are composed into the versioned policy and production generated
payloads through the Patch Pipeline; no generated function is hand-edited.

Normal builds include the checked scene, track-menu, character-resource and
character-menu fragments for both revisions. Generate these with
`scripts/generate_legacy_menu_qualification.py --characters --character-menu`
(the historical script name does not enable the private recipe).
The composition step includes `legacy_character_presentation_policy.py`, which
pins both retail ELFs and checks hook ownership and instruction signatures for
the local result-portrait and race-sound adapters.
`DKR_LEGACY_QUALIFICATION` must be **OFF** for packaging. The package guards and
required-hook checks remain enabled. No submodule or generated C source is
hand-edited.

## Online preparation reuse

The Playtest 14 synchronization rebuild checks matching installed offline mods
after consent, regardless of their local enablement or visibility. Patch hashes,
prepared asset identities and the track's ROM revision must match the frozen
host selection. Reuse populates a separate online transaction; it never copies
host enablement over the offline catalogues or writes offline saves. Track Lab
courses can also supply matching canonical package bytes locally.

Completed per-payload preparation survives failure of a larger library job.
Fresh imports share verified ROM data and a single patch reconstruction between
review, track and character preparation. Bounded transfer bursts and reordering
buffers reduce network stalls; batched disk flushes retain mandatory final
length/hash verification. Progress continues during blocking local preparation
without granting admission. See
[synchronization rebuild notes](PLAYTEST14-MOD-SYNC-PERFORMANCE-20261006.txt)
for measurements, installation instructions and qualification limits.
# Shared-source online preparation (6 October 2026)

Online host/client profiles now reference exact locally verified catalogue
entries or independently completed importer caches. They do not copy complete
multi-patch prepared/review groups into every payload cache or aggregate profile.
The session index is local metadata, not wire-authorized filesystem access.
Every root is checked against the host manifest, and runtime preparation still
verifies selected records, native compatibility, portraits, voices and banks.

Offline activation/visibility/save data is never copied from the host. Keeping
new downloaded content deduplicates whole groups, publishes complete inactive
copies, and skips already installed source trees. Track Lab supports direct
installed-course reuse too. Stale source indexes are rebuilt without replacing
existing offline files; corrupt content cannot satisfy admission.

Runtime preparation retains a pristine verified world template. Fresh worlds
use independent asset buses, mutable cache owners, resident generations and
replay authority, with deterministic matching virtual addresses. One bounded
in-process immutable cache accelerates unchanged retries/rematches; it is keyed
by canonical original-ROM SHA and full manifest identity, never timestamps.
