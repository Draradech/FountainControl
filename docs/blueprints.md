# Blueprint companions

FountainControl packs per-fountain drop and horizontal limits alongside local
blueprints. A blueprint named `example.png` gets an `example.png.fountains`
companion in the same directory. Copy both files when sharing or moving it.
The native PNG and its embedded building records are unchanged, so it remains
usable without FountainControl. Existing PNGs without companions use defaults.

The companion is a small, versioned text file: magic and schema version, the
PNG's SHA256, building count and entry count, then rows of zero-based building
index, drop in cm and horizontal limit in cm (`-1` means Default). It contains
no object pointers or destination-world building IDs. Missing, malformed or
stale companions are ignored. Limits must satisfy the same bounds and 50 cm
steps as the interface. Writes use a temporary file and atomic replacement
after the native PNG writer succeeds.

At import, entries attach to that blueprint's building records. Placement
copies them to the newly assigned building IDs, where ordinary save companions
take over. Empty import sessions also suppress ordinary clone lookup by source
ID, preventing collisions with unrelated buildings in the destination world.
Leaving the clone selection or reloading a world clears that import context.
Preview placement does not register live fountain settings.

The hooks cover `PackToBlueprint` (including the ByIndex wrapper), both native
EnterBlueprintMode paths, the common PNG reader, selection clearing, and the
final file writer. They are protected by the existing complete executable
fingerprint. Workshop publishing is not covered: the game's uploader may send
only the PNG, so the companion must be supplied separately.

Native addresses for the pinned executable:

| Path | Preferred VA |
|---|---|
| PackToBlueprint implementation | `0x144ac6000` |
| Common blueprint PNG reader | `0x144aca1e0` |
| EnterBlueprintMode by index | `0x144ab6d90` |
| EnterBlueprintModePath | `0x144ab7180` |
| Clear clone selection | `0x144ab3ce0` |
| SaveArrayToFile | `0x1412efd40` |

`USpatialCloneState + 0x2d8` contains the current `FCloneBuildingState`.
Its building records have stride `0x200`; source key, relative position and
building name are at `0x1ac`, `0x1b8` and `0x1d0`. These form the runtime
identity when a copied command is placed or a loaded blueprint is repacked.
Serialized companions use list indices because FName values vary by process.
