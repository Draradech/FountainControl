# Validation through 0.1.5

Checks through 0.1.4 were validated on 2026-10-07 with Steam build 25321648, Alchemy Factory 1.0.4963,
UE 5.7.4, UE4SS v3.0.1-946-g265115c0 and Proton Experimental. Integration
work used disposable `FountainControl_Dev`, `FountainControl_Blueprint_Dev`
and `FountainControl_Recovery_Dev`
copies of the world. The original
world's save files were not edited.

The clean installation and interface check in 0.1.5 used the supplied UE4SS
1161 archive with its stock settings, as described below.

## Completed checks

| Check | Result |
|---|---|
| Native geometry oracle | 10,080 sampled points agreed with the game's actual reach predicate when exclusion removal was disabled. |
| Repaired trajectories | Outer envelope, velocity, acceleration, endpoint and limit checks passed, including 250 repaired trajectories. |
| Lua companion handling | Valid, missing, malformed, stale fingerprint, incorrect schema/data, numeric bounds, step size, building IDs and native clone adoption passed host tests. |
| Native save completion | Matching companions were written after successful saves; Python independently verified their SHA256 against the `.sav`. AutoSave0/1/2 rotation and wrap were exercised. |
| Restart and reload | Per-fountain limits and the smaller ring survived restart and loading the matching companion. |
| Stale save recovery in 0.1.4 | A real save with FountainControl disabled changed the checksum. After restarting with the mod enabled, all 29 fountain SettleKeys were unchanged; key 73598 recovered 350 cm drop, 150 cm horizontal and a 150 cm ring. The warning appeared in FountainControl.log. |
| Ordinary clone | The native clone/build path copied 350 cm drop and 150 cm horizontal limits to a new building ID. |
| Cut/move | Actual selection, cut and placement through the game UI preserved those limits after the source was destroyed and a new building ID created. |
| Range indicator | Zero, 50 and 250 cm caps produced the corresponding radius; 500 cm and Default restored the original ring when it was already smaller than the cap. |
| Interface in 0.1.1 | A fountain advertised View and the normal Inspect key (`E`) opened its controls. Escape closed it. Mouse input selected Default; 3.5 m / 1.5 m restored correctly. The bank portal’s complete slider textures/style and native frame were reused in the left, vertically centered interaction area. Ordinary storage layout restoration was checked. |
| Mod-free load | A save made with the mod loaded into normal gameplay after disabling FountainControl and restarting. UE4SS itself remained installed. Repeated after adding runtime Inspect components in 0.1.1. |

The indicator uses each actor's actual mesh bounds and original scale. Two
observed rendered baselines were approximately 480.204 and 479.724 cm; the
getter returned approximately 510.204 cm. The mod does not equate those values.

## Controlled live delivery matrix

All 14 cases completed successfully through the actual hopper receive worker,
product visual system and delayed inventory arrival. Accepted cases added one
SilverCoin to the hopper and emptied the corresponding pending throw entry;
rejected cases added none.

| Case | Expected |
|---|---|
| Directly below, z = -100 cm | Accept |
| Same XY and height | Accept |
| Directly above, z = +100 cm | Accept |
| Inside the former exclusion, r = 50 cm, z = 0 | Accept |
| Drop cap 100 cm, z = -100 cm | Accept |
| Drop cap 100 cm, z = -100.01 cm | Reject |
| Drop cap zero, z = -1 cm | Reject |
| Horizontal cap 250 cm, r = 250 cm | Accept |
| Horizontal cap 250 cm, r = 250.01 cm | Reject |
| Horizontal cap 500 cm, r = 500 cm | Accept |
| Default horizontal, r = 800 cm, z = -500 cm | Accept |
| Horizontal cap zero, directly below | Accept |
| Original drop boundary, z = -1000 cm | Accept |
| Beyond original drop, z = -1000.01 cm | Reject |

`tools/game_tests/delivery.lua` temporarily positions a real receiver at the
specified catch point and injects a single item into its native receive path.
It restores the receiver and source settings afterward. This exercises flight
and arrival behavior, including the repaired vertical case, but bypasses the
upstream belt, fountain launch and receiver selection. It is not a complete
automated production-chain test.

## Remaining coverage limits

The native range oracle and controlled live deliveries provide detailed
geometry coverage. Live arrival testing covered hoppers; cauldron, goods,
raw-material and trash receive hooks still need equivalent in-game scenarios.
Multiplayer was not validated. Hanging fountains and other launchers are outside the intended
scope.

Save checksum mismatches now permit recovery in 0.1.4. The host suite checks
repeated changed checksums and an unavailable checksum, warning logging,
unchanged validation of malformed records and defaults for unrecorded keys.
Saving a disposable slot with FountainControl disabled and then restarting
with it enabled was exercised in game. This does not establish SettleKey
stability across future game updates that migrate building identity.

This is an experimental, executable-pinned build rather than a claim of broad
compatibility across game updates or long play sessions. The earlier UE4SS
async callback abort was addressed by moving all polling onto the game thread;
subsequent restarts and integration sessions used that corrected scheduling.

## Interface correction in 0.1.1

The 0.1.0 interface check opened a widget through a development command. It
missed the lack of an ordinary fountain interaction, and used GameLayer instead
of the game’s GameMenu furniture layer. Version 0.1.1 adds a runtime Inspect
component, opens the proper layer through actual keyboard input, and replaces
the generic sliders and plain background with the bank portal’s native style.
The geometry and persistence results above remain applicable; their logic did
not change.

## Blueprint support in 0.1.2

- Host tests passed with AddressSanitizer and UndefinedBehaviorSanitizer:
  metadata round trips, schema/hash/count/index/bounds checks, PNG insertion,
  replacement, CRC corruption, duplicate chunks and truncated/malformed input.
- A two-fountain selection was saved through the real blueprint interface.
  Independent Python parsing verified one `fcRs` chunk, its CRC32 and the
  SHA256 of the unchanged PNG/native payload excluding that chunk.
- Import into a second disposable world restored 700/300 and 350/150 cm limits
  despite deliberately conflicting source IDs set to 100/50 cm. Actual native
  cost-free construction assigned new IDs and adopted both settings. A separate
  restart and ordinary mouse placement through the game UI repeated the same
  collision test and restored both limits.
- A rotated placement restored the expected setting on its successful building;
  the other building was blocked by the available floor/geometry. This is
  partial rotation coverage, not a successful two-building rotated placement.
- Editing the saved blueprint's title and description preserved both entries,
  refreshed the fingerprint and retained exactly one metadata chunk.
- With FountainControl's DLL and scripts disabled, the same PNG entered native
  placement mode and an ordinary mouse click constructed both fountains.
  UE4SS and a temporary Lua-only inspection mailbox remained installed.
- Workshop's native upload path was inspected and passes the existing PNG as
  content without re-encoding. An online publish/download was not tested.

`tools/game_tests/blueprint.lua` exercises actual cost-free construction only
in disposable development worlds. Its input PNG contains the two expected
settings. The per-save persistence and UI code are unchanged by this release.

## Ghost previews in 0.1.3

- Native menu state safely returned an empty preview before world loading.
- Ordinary bulk copy and the actual middle-mouse quick-copy action previewed
  a fountain's 150 cm horizontal cap. Scale and visibility remained correct
  over 100 samples at 10 ms intervals across native preview updates.
- Actual `T` cut/move removed the source actor while its ghost retained the
  150 cm cap through the remembered source settings.
- Importing the two-fountain PNG displayed separate 150 and 300 cm rings.
  Native rotation retained both radii and visibility. A further 100 samples
  at 10 ms intervals found both rings stable across native preview updates.
- The same native PNG without mod metadata displayed both original rings
  (about 480.204 cm), despite conflicting destination source IDs.
- A zero cap collapsed the ghost ring. 500 cm and Default restored its native
  radius. Cancellation cleared preview context and the separate baseline cache.
- No zero-ID fountain entered the Lua settings or native facility/save maps.
- Host Lua tests cover independent zero-ID ghosts, repeated updates without
  cumulative shrinkage, zero/Default transitions, cancellation and reuse.

`tools/game_tests/preview.lua` reads active ghosts in disposable development
worlds and checks their actual mesh radii, visibility and absent save settings.
An initial prototype called the game's state-name getter before a state
existed and crashed at the menu. The final bridge uses serial-checked weak
resolution and was restarted and validated at the menu and in gameplay.

## Clean installation with UE4SS 1161 in 0.1.5

Followed the README's installation steps on 2026-10-07, starting with a Win64
directory containing no UE4SS installation:

- Extracted the supplied standard `UE4SS_v3.0.1-1161-g6eb3d9bc.zip` beside the
  game executable, then extracted the FountainControl ZIP and copied its
  folder into `ue4ss/Mods/`.
- Left `UE4SS-settings.ini`, `mods.txt` and `mods.json` byte-for-byte identical
  to the archive originals. No custom signatures were installed; the engine
  version overrides stayed empty. The existing Proton `dwmapi` DLL override
  was used.
- UE4SS logged engine version 5.7 and Shipping configuration autodetection.
  Both the DLL and Lua mod started through the included `enabled.txt`.
  FountainControl verified the game executable and enabled its native hooks.
- Loaded `FountainControl_Recovery_Dev/SaveGame0` through the normal game menus.
  A fountain advertised View, and pressing E opened the styled controls.
  Mouse input set 3.5 m drop and 1.5 m horizontal; the ring shrank, and both
  values remained after Escape and reopening with E. No development commands
  or helper mods were used.
- Closed the game and verified all 13 original-world save hashes were unchanged.

The supplied archive's SHA256 is
`938dc9901e6452cad3120280681c93a00ae12df2b259668815a150e221a35563`.
Its DLL startup banner reports `v3.0.1 Beta #0 - Git SHA #5418fa4e`; the
installed DLL was verified byte-for-byte against the archive. The archive
filename is retained above to identify the tested download precisely.

This check covers clean installation and normal fountain interaction on the
new loader. The full geometry, delivery and blueprint matrices above were
performed on the earlier UE4SS build.
