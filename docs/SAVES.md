# Saves and portable mode

Your offline Adventure belongs to you. Online play uses a separate save owner;
joining a host must not replace your personal progress.

## Where data lives

| Mode | Data root |
|---|---|
| Windows normal profile | `%APPDATA%\DKRPort` |
| Linux normal profile | `$XDG_CONFIG_HOME/dkr-port` or `~/.config/dkr-port` |
| Portable desktop mode | `dkr-runtime-data` beside the application |
| Android | The app's private data directory; use the export controls |

In **Play**, the portable-mode checkbox creates/removes `portable.txt` beside
the launched application. It takes effect on the next application launch.
A command-line config override takes priority. Switching mode does not copy or
move saves, mods or settings, so a different profile can look empty without
anything having been deleted. Unwritable or unsafe marker paths report an error
instead of changing unrelated files.

## Offline, modded and online progress

Ordinary offline Adventure uses `saves/dkr.us.v77.bin`; the two supported
revisions use the compatible EEPROM format. Ordinary Controller Paks are
`controller-pak-1.mpk` through `controller-pak-4.mpk`.

An offline legacy mod set uses `saves/mods/<mod-set fingerprint>/`.
Its first Adventure starts from a copy of offline progress, or a valid blank
save. Existing progress for that same set is retained. Changing the set can
select a different modded save; disabling it restores the ordinary route.
Modded Controller Paks are separate.

The host's online Adventure uses `saves/online/host/`. A client's received
Adventure uses `saves/online/sessions/<match-id>/`.
The host chooses to copy single-player, start fresh or continue the previous
online session. Copying reads offline progress; it does not replace it.
Clients receive only the separate online save.

Online Paks also stay in online storage. Legacy backends use `paks/` beside
the session Adventure; Experimental Rollback uses
`experimental-controller-paks.bin`. Persistence is confirmed-only in rollback.

One launch holds its save route until the runtime and save producers retire.
A disconnect cannot reroute writes into offline progress, and a route/lock
failure refuses the launch rather than falling back to another writable save.

## Save Manager

- **Offline:** manage personal Adventure and ordinary Controller Paks, including
  import/export, backups and checksum-aware Save Builder actions.
- **Online:** inspect/export separately stored session progress.
- **Modded:** inspect/export isolated mod-set progress.
- **Recovery:** inspect surviving saves, backups/history and known older paths.

Stored-save scans are bounded and read-only. **Refresh Stored Saves** checks
again after files change. Restoring a valid Adventure to offline storage is an
explicit confirmed action with a safety backup; it is never automatic.
Experimental four-port Pak images are export-only in this view, not ordinary
single-port Pak imports.

Offline edits, imports, repair and restoration are locked while a game or lobby
owns the profile. Leave the lobby before changing personal progress.
Destructive confirmations identify the target. Keep an external backup before
updates or recovery work.

## If progress appears missing

1. Check whether portable mode or a config override selected a different root.
2. Check the Offline, Online and Modded scopes before restoring anything.
3. Refresh Recovery and export a surviving candidate without altering it.
4. Validate the candidate, then deliberately restore to the intended offline
   target if that is what you want.

A scan cannot recreate progress when no valid copy survives. Do not delete a
profile to diagnose a missing save. Automatic `.bak` and bounded content
history are separate from manual Save Manager backups.
See [troubleshooting](TROUBLESHOOTING.md) for reports.
