# 1.1.5024 native port audit

Ported on 2026-10-16 for FountainControl 0.1.8. The full supported executable
SHA256 is `e44fbdbd469fc42a217741da8dbcdcfa967afa245af4a21d1b305110e08d456a`;
Steam build 25991404, UE 5.7.4. The archived source executable is 1.0.4963,
Steam build 25321648. The [profile](reference-build.json) records exact addresses,
typedefs and layouts. Neither executable is distributed.

The runtime remains gated to the new full hash. This was a native graph port,
not a hash-only change: all 22 hook targets and eight helpers were mapped from
old/new disassembly, call graphs, reflected types and fresh runtime dumps.
Relocation-normalized comparisons covered whole functions. PE unwind chaining
had to be followed to avoid comparing only their initial hot spans.

## Material changes

- Receiver catch-point virtual slot moved from 0x568 to 0x570. All five native
  receiver workers were inspected; goods acceptance also moved its virtual slot.
- GameState's save and spatial subsystems moved from 0xb8/0xc0 to 0xe0/0xe8.
  Clone records remain 0x200 with the same source-key, position, name and entity
  fields. The full-build ABI and output records remain compatible; internal
  SpatialBuildingState offsets changed and are left to native code.
- The save graph now reaches a direct completion function at 0x144adc760 with
  `(action, slot, user, success)`. The hook uses that ABI, including explicit
  failure cleanup. The async worker closes the native writer before queuing
  completion. Snapshot timing and atomic companion writing are preserved.
- Visual flight processing adds an entity queue-once flag. Pattern, speed,
  duration and handle fields remain at their verified offsets. Launch/refresh
  virtual slots changed inside native code; original calls remain responsible
  for those paths. Clone has a new early type-5 rejection, retained by calling
  the original. Native blueprint serialization and save flags are untouched.

The native reach function is equivalent after relocation normalization. Fresh
machine-code reports verify upright and hanging sampling, slices and transforms.
The strict oracle selects an independently hashed profile for each archived
build. The trajectory suite passed 10,080 points on each executable, including
250 repaired trajectories. Pattern constants remain 490/245 half-gravity;
loaded fountain defaults and actual rendered ring baselines were checked anew
([defaults](fountain-defaults.txt)). Getter radius is not the mesh baseline.

## Runtime evidence

[Validation](../../validation.md) records the completed live matrix and limits.
Fresh CXX/object dumps and mappings were generated with the stock installed
loader; large dumps, owned binaries, saves and full logs remain ignored locally.
The committed current PNG fixture was exported and edited through normal UI;
its metadata-free pair differs only by removal of fcRs and imports natively.
Workshop upload/update/path joins were compared after relocation normalization,
without publishing or claiming an online round trip. Current Windows operation,
non-hopper live arrivals and long-session stability remain unverified.
