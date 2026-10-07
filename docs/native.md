# Native integration, 1.0.4963

All addresses below are preferred-image virtual addresses (base 0x140000000).
The DLL rebases them against the loaded image. A full executable fingerprint
gates all hooks; these addresses are not signatures for other builds.

SHA256: `66618d029654d125723c95e8e62919f267973ad881e24abff3b80e0b355a52f8`.

| Integration | Address |
|---|---|
| ScriptInteractionComponent primary interaction | 0x144a24200 |
| Fountain reach predicate | 0x144a1e1d0 |
| Compatible receiver selection | 0x144a19500 |
| Fountain product launch | 0x144a073d0 |
| Product visual interpolation | 0x1449fae30 |
| Hopper receive worker | 0x144a0d760 |
| Cauldron, goods, hopper, raw material, trash receive | 0x144a14120, 0x144a14380, 0x144a145f0, 0x144a14790, 0x144a149c0 |
| Native save begin / completion | 0x144acd490 / 0x144aaae90 |
| Reload notification | 0x144acb850 |
| SpatialCloneState clone / building creation | 0x144ac84c0 / 0x144ad34d0 |

## Geometry and throws

Let r be horizontal emitter-to-catch-point distance and z the catch point's
height relative to the emitter, all in cm. Vanilla uses t = sqrt(2r/500),
v = z/t + 490t, requires 0 <= v <= 700 and a descending arrival
(v - 980t < 0), and rejects z < -1000. The central exclusion follows from
those speed/arrival constraints. Its inner radius at a given height is
approximately |z|/1.96, rather than a constant cylinder.

With exclusion removal enabled, the solver retains accepted vanilla flights
and selects a longer duration where required to fill the centre. Gravity
remains 980 cm/s²; vertical speed stays within 0–700 cm/s and horizontal
acceleration never exceeds 500 cm/s². The original outer envelope is retained:
z < 250 and r <= 250 * ((700 + sqrt(700² - 1960z))/980)², subject to the
original maximum drop and the chosen extra caps.

A thread-local emission context identifies the fountain in the receiver
hooks. Repaired flights carry a private marker only in queued throw arguments.
The visual hook first creates the interpolation with the duration, then sets
the existing visual entity to ordinary MovePattern 2 with the repaired vertical
speed and duration. The hopper's separate arrival timer also receives that
duration. No marker or mod setting is added to saved actor data.

The existing receiver selector retains native item compatibility and rotation
among targets, with the extra reach check applied. Changing settings invalidates
the native nearby-target cache.

## Saves and building identity

Native save begin snapshots live registered fountains by SettleKey. The native
completion callback commits the companion only when SaveGameToSlot actually
succeeds, after the `.sav` writer closes. It computes SHA256, writes schema 1
JSON to a temporary file and replaces the companion atomically. Native
serialization and SaveGame property flags are untouched.

SettleKeys persist across save/load and game sessions for surviving buildings
within a world. Lua validates the companion schema and each record's data
bounds. A mismatched or unavailable save fingerprint logs a warning in
`FountainControl.log` and the UE4SS console, then still loads valid records by
SettleKey. This recovers settings after sessions with the mod disabled;
unrecorded keys use defaults. Reload notifications supply a generation counter
so a reused GameState address still triggers restoration. Limits are in cm;
Default horizontal range is represented by JSON null (runtime -1).

This recovery applies only to save companions. Executable fingerprint gating
and embedded blueprint metadata fingerprint validation remain strict.

Cloning captures the source limits and blueprint class and transfers them only
to a confirmed new actor of the same class. Cut/move can delete the source
before cloning; a per-world remembered-key map preserves its settings until
reload. Deleted keys are excluded from save snapshots once their facilities
leave the live registration map.

## Reflection and UI

Lua registers only settled upright fountains with MovePattern 2. It adds a
runtime ScriptInteractionComponent advertising Inspect, using the game's
normal input mapping and focus/range checks. The native primary interaction
hook queues only registered fountain owners; other script interactions retain
their original delegate dispatch. Lua consumes the request on the game thread
and opens StorageInspectWidget in the normal GameMenu layer. GameLayer bypasses
the native furniture layout and must not be used.

For a fountain, the storage widget's original children are collapsed and a
single replacement body uses the bank portal's complete Slider WidgetStyle
and frame brush from its cooked WidgetTree template. The normal fountain title
brush is reused. The panel is 600 UI units wide, centered in the left interaction
area, and original storage children are restored on exit. No bank widget is
opened to obtain its assets. Integer slider positions are polled on the game
thread. Runtime interaction components have no save fields and are recreated
when fountains are discovered after load. The indicator's
original world scale and local mesh bounds define the rendered baseline.
Its XY scale uses min(1, cap / baseline radius); Default restores the baseline.
Building identity distinguishes a new mesh at a recycled UObject address.

Use UE4SS's `LoopInGameThreadWithDelay` for the actor, widget and development
mailbox timers. The earlier LoopAsync → ExecuteInGameThread scheduling caused
a callback-registry abort during load; the pinned UE4SS implementation accesses
the hook coroutine before acquiring its execution mutex on that path.

Development-only probes can read guarded memory, compare native reach,
inject one item through the real hopper receive path, and invoke a source-based
clone. The throw receiver's context is a native worker-parameter structure,
not the GameState actor. The clone method belongs to SpatialCloneState,
whose typed outer is the player controller.

## Portable blueprints

See [blueprint format and hooks](blueprints.md). Six additional hooks capture
settings on export and restore them on import. Imported settings match copied
building records by source key, name and relative position, then apply only
to confirmed upright MovePattern 2 actors. Empty import sessions prevent
cross-world source-ID collisions from inheriting local settings. Saved
blueprint edits retain their own metadata instead of using the current world
selection. The native save format and property flags remain unchanged.

## Ghost range preview in 0.1.3

The preview bridge reads the controller's serial-checked CurrentState weak
reference at `+0xaf0`. It accepts only `BP_SpatialCloneState_C` in placement
mode (`+0x148`), and reads its current building records at `+0x2d8`.
Each record's native entity reference at `+0x1d8` resolves its ghost actor
through `0x144a5b040`, which checks the weak object's serial and validity.
The CurrentState weak resolver is `0x141540ca0`. These reads remain protected
by the existing executable fingerprint; no additional hooks were added.

Only unsettled upright fountain actors are returned. The shared source lookup
used by actual construction supplies remembered copy/move settings or imported
blueprint settings. A metadata-free import supplies Default, even when its
source IDs collide with current-world fountains. Ghosts are not matched by
position or their shared zero SettleKey, and are never registered as facilities
or added to save companions.

A 100 ms game-thread loop displays each ghost's existing range indicator and
applies the same horizontal scale as a settled fountain. It maintains a separate
baseline cache, preserving Z scale, and restores and hides departing ghost
indicators before reuse. Rotation and translation continue through the native
placement system. This previews the existing horizontal ring, not a volume
representing the drop limit or the trajectory envelope at every height.

Menus and loading can have no active controller state. The reflected
GetCurrentStateName method assumes one exists and must not be called by this
poller; the checked weak resolver returns an empty preview instead.
