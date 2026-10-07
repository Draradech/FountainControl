# FountainControl

A UE4SS mod for Alchemy Factory's upright Arcane Fountain. Development is
in progress. This directory is an independent Git repository.

## Intended behaviour

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
- Store settings in one JSON companion per save, keyed by persistent building
  ID and bound to the save's fingerprint. Do not alter game save contents or
  serialization flags. Missing or mismatched companions use defaults.
- Singleplayer only. Hanging fountains and other launchers are outside scope.

Distances are measured from the fountain's world emission point to the
receiver's catch point. UI uses metres; stored and native values use cm.

## Development approach

Build a UE4SS C++ DLL for native hooks, with a Lua companion where reflection
and existing widgets make integration simpler. Keep investigation facilities
in the mod so successful probes can become production integration. Native
hooks must verify the executable and target instructions before installation.

The parent project's `research/` directory documents the verified native
geometry and addresses for Steam build 25321648, UE 5.7.4, game 1.0.4963,
UE4SS v3.0.1-946-g265115c0.

## Required validation

Native trajectory and eligibility agreement (including vertical throws),
real delivery at boundaries, slider and indicator behaviour, save/load,
building movement/cloning, autosave slot rotation, stale companions, and
unmodified game loading. Build and probe progress is tracked in Git commits.
