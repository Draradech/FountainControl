# Single-file blueprint settings

FountainControl embeds per-fountain drop and horizontal limits directly inside
saved blueprint PNGs. Copy or share the PNG normally; no companion is required.
The native image, building records and footer are preserved. The PNG still
loads and constructs buildings with FountainControl disabled. Blueprints
without valid embedded settings use the mod's defaults when imported.

## Format

One private ancillary `fcRs` PNG chunk is inserted immediately before `IEND`.
The chunk follows the [PNG chunk naming and CRC rules](https://www.w3.org/TR/png-3/#5ChunkNamingConventions):
ancillary, private, reserved bit clear, safe to copy. Existing `fcRs` chunks are
removed before writing a replacement. All other chunks and the game's opaque
trailing building payload remain byte-for-byte intact.

The UTF-8 text payload has magic `FountainControlBlueprint`, schema version 1,
a SHA256 fingerprint, building count and entry count, then rows of zero-based
building index, drop in cm and horizontal limit in cm (`-1` means Default).
The fingerprint covers the entire PNG and native tail with `fcRs` chunks
excluded. It binds settings to that exact image and building payload without
hashing itself. No object pointers or destination-world IDs are serialized.

Missing chunks, duplicate chunks, incorrect CRCs, stale fingerprints and
malformed or unsupported payloads are ignored. Metadata is limited to 1 MiB;
loaded PNGs to 128 MiB. Counts, indices, uniqueness, numeric bounds and 10 cm
steps are validated before applying anything. Image editors that strip unknown
chunks or change the PNG invalidate the settings, and may also remove the
game's native building data; share the game's exported file directly.

## Import, edits and Workshop

At import, entries attach to the loaded building records. Placement copies
them to newly assigned building IDs, where ordinary per-save companions take
over. Empty import sessions suppress clone lookup by source ID, preventing
collisions with unrelated destination buildings. Leaving the clone selection
or reloading a world clears import context. Both normal and cost-free native
construction transfer settings. The clone function's last argument bypasses
costs; it is not a preview flag.

Editing a saved blueprint loads and preserves that file's embedded settings
before replacing its PNG. Repacking an imported selection uses its imported
settings. The hooks cover PackToBlueprint (including the ByIndex wrapper),
both EnterBlueprintMode paths, the common PNG reader, selection clearing and
the final synchronous file writer. All remain executable-fingerprint gated.

The native Workshop upload implementation was traced on the pinned build:
`UploadBlueprint` reaches `0x144b034c0`, then the update implementation at
`0x144ae9880`. It joins the existing directory and PNG filename using
`0x144ab4d30` and passes that file as content to the Steam UGC interface; it
reads blueprint descriptions separately and does not re-encode the PNG.
Embedding the settings in the content file therefore supports its normal
Workshop transport. This is a conclusion from native code inspection; no
online Workshop item was published or round-trip tested. Steam's
[UGC content API](https://partner.steamgames.com/doc/api/ISteamUGC#SetItemContent)
is distinct from its preview-image API; the full blueprint content file is
the relevant carrier.

## Native layout

| Path | Preferred VA |
|---|---|
| PackToBlueprint implementation | `0x144ac6000` |
| Common blueprint PNG reader | `0x144aca1e0` |
| EnterBlueprintMode by index | `0x144ab6d90` |
| EnterBlueprintModePath | `0x144ab7180` |
| Clear clone selection | `0x144ab3ce0` |
| SaveArrayToFile | `0x1412efd40` |

`USpatialCloneState + 0x2d8` contains the current `FCloneBuildingState`.
Building records have stride `0x200`; source key, relative position and
building name are at `0x1ac`, `0x1b8` and `0x1d0`. These form the runtime
identity when a copied command is placed or a loaded blueprint is repacked.
Serialized settings use list indices because FName values vary by process.
SaveArrayToFile accepts a borrowed TArrayView64; the hook supplies a temporary
buffer for its synchronous write without changing Unreal's owning array.
