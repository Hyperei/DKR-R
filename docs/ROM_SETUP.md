# ROM setup

Add your own original ROM in **Play → Add a ROM…**. The **Game ROM** dropdown
keeps the installed choices and selects the native revision automatically.

DKR-R's game engines support these clean US revisions after byte-order
normalisation:

| Original ROM | SHA-1 |
|---|---|
| US v1.0/v77 | `0cb115d8716dbbc2922fda38e533b9fe63bb9670` |
| US Rev A/v1.1/v80 | `6d96743d46f8c0cd0edb0ec5600b003c89b93755` |

`.z64`, `.v64` and `.n64` byte orders are recognised from the header, not
the extension. Swapped inputs get a verified private big-endian cache;
the original file is never modified or distributed.

Both CPU payloads run inside the same application, frontend and runtime.
Players never choose a separate engine executable. Validation happens before
registration and renderer/audio startup.

Import a legacy patch through Mods / Hacks, not by selecting its patched ROM
as the game. Xdelta decoding requires the exact original source revision, even
when prepared content can later target both supported engines.
PAL/Japanese native engines are not included.

For development, keep each revision's ELF/ROM/generated files in separate
local build directories and use [Building](BUILDING.md).
For rejection/recovery, see [troubleshooting](TROUBLESHOOTING.md).
