# macOS builds

Use [Building DKR-R](../docs/BUILDING.md#macos) for setup, playing and the optional
`--package` flag. A source kit is not a claim of a newly qualified macOS release.

The kit contains DKR-R sources and the dependency lock file. The first build
requires internet access to fetch the pinned dependency checkouts.

Preparation checks staged Python imports and native asset/CRC/compiler tools.
Existing incomplete stages can be repaired without deleting protected source.
The integrated Experimental Rollback option is gated out on Apple; select an
available legacy online backend. See [platform support](../docs/PLATFORMS.md).
