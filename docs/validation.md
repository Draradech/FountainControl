# Validation through 0.1.1

Validated on 2026-10-07 with Steam build 25321648, Alchemy Factory 1.0.4963,
UE 5.7.4, UE4SS v3.0.1-946-g265115c0 and Proton Experimental. Integration
work used a disposable `FountainControl_Dev` copy of the world. The original
world's save files were not edited.

## Completed checks

| Check | Result |
|---|---|
| Native geometry oracle | 10,080 sampled points agreed with the game's actual reach predicate when exclusion removal was disabled. |
| Repaired trajectories | Outer envelope, velocity, acceleration, endpoint and limit checks passed, including 250 repaired trajectories. |
| Lua companion handling | Valid, missing, malformed, stale fingerprint, incorrect schema/data, numeric bounds, step size, building IDs and native clone adoption passed host tests. |
| Native save completion | Matching companions were written after successful saves; Python independently verified their SHA256 against the `.sav`. AutoSave0/1/2 rotation and wrap were exercised. |
| Restart and reload | Per-fountain limits and the smaller ring survived restart and loading the matching companion. |
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
Cross-world imported blueprints, multiplayer and hanging fountains were not
validated. Hanging fountains and other launchers are outside the intended
scope.

Fingerprint rejection after a changed save was tested in the host suite.
Loading without the mod was tested in game; overwriting that slot without the
mod and then re-enabling it was not separately exercised.

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
