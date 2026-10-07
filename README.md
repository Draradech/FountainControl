# FountainControl

A UE4SS mod for Alchemy Factory's upright Arcane Fountain. Version 0.1.2 is
an experimental build, tested in a separate copy of a singleplayer world.
This directory is an independent Git repository.

## Behaviour

- Per-fountain drop limit: 0–1000 cm, displayed as 0.0–10.0 m, in 0.5 m steps.
- Per-fountain horizontal limit: 0–500 cm, displayed as 0.0–5.0 m, in 0.5 m
  steps, followed by a final **Default** slider position that applies no
  extra horizontal cap. Default does not limit lower targets to the visible
  ring's radius.
- Shrink the existing range indicator when the chosen horizontal limit is
  smaller than its original rendered radius.
- Global startup INI option `RemoveCenterExclusion`, enabled by default.
  Disabling it retains the original throw model with the distance limits.
- Default fountain settings: 1000 cm drop and Default horizontal range.
- Moving preserves settings; cloning copies settings.
- Saved blueprints embed both limits inside the PNG. Share one file, including
  through Workshop; imported fountains adopt their own saved limits.
- Store settings in one JSON companion per save, keyed by persistent building
  ID and bound to the save's fingerprint. Do not alter game save contents or
  serialization flags. Missing or mismatched companions use defaults.
- Singleplayer only. Hanging fountains and other launchers are outside scope.

Distances are measured from the fountain's world emission point to the
receiver's catch point. UI uses metres; stored and native values use cm.

## Install and configure

Copy `dist/FountainControl` to `ue4ss/Mods/FountainControl` and add
`FountainControl : 1` to `ue4ss/Mods/mods.txt`. Keep both `dlls/main.dll` and
`Scripts/`; the DLL and Lua companion work together. Restart the game.

Aim at an upright fountain and press the game's Inspect key (`E` by default)
to adjust **Drop limit** and **Horizontal limit**. Escape closes the interface.
The mod adds the normal View prompt and honors the game's input mapping. Changes take effect
immediately and are captured by the next successful game save.

`config.ini` contains the startup setting:

```ini
[UprightFountains]
RemoveCenterExclusion = true
```

Set it to `false` and restart to retain the original central exclusion.
The two per-fountain limits continue to apply. Development commands are off
in the distributed configuration.

Settings live beside each save, for example `AutoSave0.fountains.json` beside
`AutoSave0.sav`. Copy the matching pair when moving saves between computers.
An absent, invalid or fingerprint-mismatched companion uses defaults.
Removing the mod leaves the native save loadable; the companion is ignored.
Saving without the mod can invalidate that slot's companion because its
fingerprint no longer matches the new `.sav`.

## Supported versions and scope

Tested with Steam build 25321648, game 1.0.4963, UE 5.7.4 and
UE4SS v3.0.1-946-g265115c0. The DLL verifies the complete executable SHA256
before installing any native hook. A different executable disables the
integration and logs `Unsupported game executable` in `FountainControl.log`.
Updates require checking addresses, layouts and behavior before adding support.

This mod handles the upright `BP_SmartCannon_C`, MovePattern 2. Ordinary
in-world cloning and cut/move preserve limits, including when a move creates
a new building ID. Saved blueprint PNGs carry limits across worlds, including
when source building IDs collide with unrelated destination buildings. Existing
blueprints without embedded settings use defaults.
Multiplayer and hanging fountains are outside scope.

The DLL hooks native range, selection, throws, visuals, save completion and
cloning. Lua uses reflection for actor discovery, companion loading and two
sliders appended to the game's existing storage panel. Polling runs on the
game thread. No Unreal installation, asset packs or save-schema changes are
needed.

See [build instructions](docs/build.md), [validation](docs/validation.md) and
[native implementation notes](docs/native.md) and [blueprint format](docs/blueprints.md). Build and investigation
milestones are committed in this repository.
