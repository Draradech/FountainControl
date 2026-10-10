# Maintainer handoff

This repository contains the implementation and the evidence needed to continue
FountainControl without the parent AlchemyModding project or chat history.
This handoff was assembled on 2026-10-10 for mod 0.1.7. Use the current source
and Git history for subsequent changes; dated evidence describes the build
actually tested, not compatibility with whatever Steam installs next.

## Start here

1. Read this page and the [end-user README](../README.md).
2. Follow [build.md](build.md) for a standalone build and host checks.
3. Use [testing.md](testing.md) to make fresh disposable integration fixtures.
4. For a game update, follow [porting.md](porting.md), using the exact old build
   snapshot in [reference-build.json](research/1.0.4963/reference-build.json).
5. [artifacts.md](artifacts.md) inventories small included evidence and gives
   creation steps for binaries, dumps, mappings, saves and logs not committed.

Existing detailed documentation remains useful: [native.md](native.md) explains
runtime integration, [blueprints.md](blueprints.md) the PNG format and hooks,
and [validation.md](validation.md) distinguishes completed checks from limits.
Source is authoritative for behavior; the reference JSON is an archival lookup
and is not loaded by the mod.

## Current state and release workflow

The supported executable is Alchemy Factory **1.0.4963 / Steam build 25321648**,
UE **5.7.4**, Win64, Steam app **3669570**. Its SHA256 is
`66618d029654d125723c95e8e62919f267973ad881e24abff3b80e0b355a52f8`.
The tested stock loader archive is **UE4SS_v3.0.1-1161-g6eb3d9bc.zip**; the
SDK is pinned separately in bootstrap. See the reference JSON for checksums
and the archive/banner discrepancy. No custom UE4SS signatures, engine override
or loader settings are needed on this tested combination. The older parent
project's ForcePull/946 staging instructions are obsolete for this setup.

0.1.7 includes the range correction, styled Inspect UI, per-save persistence,
clone/move transfer, embedded blueprint settings, ghost previews, 0.1 m steps,
cloud-synced companion location and MIT license. The user reported a successful
Windows test and final integration pass; automated development primarily used
Linux/Proton. [validation.md](validation.md) records which versions and loader
builds received each check. No further feature implementation was pending when
this handoff was written. Equivalent live arrivals for non-hopper receivers,
long sessions and a Workshop online round trip remain coverage limits.

The user's publishing plan is GitHub first (create the tag in GitHub's release
process), then Nexus with a GitHub link; README serves as release documentation
and GitHub issues the bug route. The configured remote was
`git@github.com:Draradech/FountainControl.git`. This handoff does not establish
whether a release has since been published: inspect current Git/remote state
when release work is requested. Do not publish as part of ordinary development.

Keep the MIT license and notices for fmt, MinHook and the bundled JSON library.
Keep README's accurate AI disclosure: Codex researched, implemented, documented
and ran controlled tests; draradech specified requirements, supplied measurements,
made decisions and reviewed/tested behavior and UI. Online Workshop testing was
explicitly accepted as optional: single-PNG transport was the core requirement.

## Agreed behavior and rejected alternatives

| Subject | Required behavior / reason |
|---|---|
| Scope | Only settled `BP_SmartCannon_C` with indicator `MovePattern=2`. Hanging fountains and other launchers retain native behavior. Singleplayer only. |
| Defaults | Drop 1000 cm (10 m); horizontal Default (`-1` in memory, `null` in companion JSON). Existing and new fountains without settings use these. |
| Bounds | Drop 0–1000 cm. Numbered horizontal 0–500 cm; anything above 500 means Default. Default is the last slider position and uses native reach, not a numerical 500 cm cap. |
| Steps and display | Store cm, display m, 10 cm / **0.1 m** steps. Old 50 cm / 0.5 m proposals are superseded. |
| Exclusion | `[UprightFountains] RemoveCenterExclusion=true` by default. Global startup INI option, no per-fountain or dynamic UI required. Sliders work when false too. |
| Reach | Preserve originally reachable destinations within chosen caps. Extend into the excluded centre while retaining the native outer envelope. Do not replace this with a total-launch-speed parabola: that proposal would lose native targets. The flat top in the graph is intentional. |
| Coordinates | Emitter to receiver catch point, not either actor origin. Upright default emitter is local Z=60 cm; catch points can have their own offsets. 1 floor tile=400 cm, 1 building cube=25 cm; player eye measurement was about 170 cm. |
| Ring | Cap the rendered XY radius at its own measured baseline; preserve Z and restore baseline for Default. The native getter (~510.204 cm) differs from the visible mesh (~480 cm). Neither is the full 3D range. |
| Saves | Never alter native actor serialization, property flags or native save structure. JSON companion `<world>/FountainControl/<slot>.sav`, atomic `.sav.tmp`, schema 1. The `.sav` suffix intentionally matches Steam cloud rules. No migration/fallback for unreleased older paths. |
| Identity recovery | Surviving SettleKeys persisted across tested sessions, including mod-off saves. A changed/unavailable native-save SHA logs a warning and still restores valid records by key. Missing/new keys use defaults. Stability through future game migrations is not guaranteed. |
| Integrity | Only save-companion fingerprint matching is advisory. Executable and PNG metadata hashes remain strict. Schema, index, count and numeric validation remain strict. |
| Copy and move | Copy duplicates settings. Move preserves them even when cut destroys the original actor before placement. Remembered keys are world-local, cleared on reload, and are not themselves save snapshots. |
| Blueprints | A single game-exported PNG carries settings; no companion file. Settings index native building records, not destination IDs. Metadata-free imports must not borrow settings from colliding local source IDs. Native import without the mod must keep working. |
| Ghosts | Preview each ghost's cap using native entity references. Shared zero SettleKey is not identity; ghosts never enter saved/registered facility maps. Separate baseline cache prevents compounded shrinkage and pooled-object errors. |
| UI | Normal E Inspect interaction; native GameMenu furniture layer, left half and vertically centered. Reuse bank portal slider style and native frame, without opening a bank widget. Restore ordinary storage layout after closing. |

`SmartCannonMaxDistance` was investigated as a possible settings store and
rejected: it is not read by the native reach predicate and is **not saved by
default**. Making it SaveGame would change serialization. At the pinned build,
its registration descriptor at `0x147a92fe0` has flags `0x0010000000010015`,
without `CPF_SaveGame=0x01000000`; its ctor assignment at `0x1449a790f` is 500.
The serializer/load property gates are recorded in the reference profile.
Do not revive the earlier field-encoding or SaveGame-flag proposal.

## Source map and data flow

| File | Responsibility |
|---|---|
| `src/main.cpp` | UE4SS C++ mod entry, startup INI, exported Lua bridge registration, development guard and persistent log. |
| `src/native.cpp`, `src/native.hpp` | Executable gate, MinHook detours, raw layouts/ABI, live maps, receive/visual repairs, save/clone/blueprint integration and native probes. |
| `src/trajectory.hpp` | Pure upright flight solver. Native-valid flights are returned unchanged; otherwise duration is extended inside the retained envelope. |
| `src/blueprint.hpp` | Portable PNG extraction/insertion, CRC and metadata text validation. Native tail stays opaque. |
| `Scripts/main.lua` | Initializes class FNames and starts state/UI only when native hooks are ready. Development mailbox may still start on an unsupported build. |
| `Scripts/state.lua` | Settled-actor discovery, limits, companion reader, native map registration, clone adoption and settled/ghost ring scaling. |
| `Scripts/ui.lua` | Runtime interaction component, native widget template/style access, two sliders, focus/layout restoration and polling. |
| `Scripts/dev.lua` | Explicitly enabled, game-thread Lua mailbox; no network listener. |
| `tools/bootstrap.py`, `build.py` | Pinned dependencies, Microsoft-ABI cross-build, UE4SS import library, staging. |
| `tools/deploy.py`, `package.py` | Local binary/script installation preserving INI; ZIP from staged files. |
| `tools/native_predicate.py`, `test_trajectory.py` | Offline original-machine-code oracle and C++ solver comparison. |
| `tools/pe_inspect.py`, `dump_reference.py` | Read-only PE disassembly, field/xref candidates and reference evidence regeneration. |
| `tools/game_tests/`, `tests/` | Live probes and host state/preview/PNG/trajectory checks. |
| `tools/plot_range.py` | Code-generated README graph; regenerate instead of raster editing. |

Lua scans settled fountains every second and registers their native facility
pointers/limits. Slider changes update maps immediately and invalidate the
native nearby-target cache. Hooks retain receiver compatibility and native
selection behavior while checking the modified reach. A thread-local emission
context carries the source through receive hooks; repaired flight marker
`0x4643` exists only in queued arguments. Visual interpolation is converted back
to ordinary pattern 2 and the hopper timer gets the same flight duration.

Native save begin captures live registered keys. Successful asynchronous save
completion writes the companion only after the real save closes. Lua restores
it when GameState identity or native reload generation changes. Native clone
adoption happens before a later Lua scan, so `FC.get` also queries native limits.
Blueprint import sessions deliberately distinguish “no metadata” from “not an
import”; otherwise a cross-world key collision could inherit local settings.

## Development and packaging traps

Run tools from the repo root. A new checkout needs only this repo, dependency
access and the owned game/UE4SS files; no `../research/.venv` is required.
`build.py` stages current source, README, docs, config, enabled.txt and licenses.
`deploy.py` only copies DLL/Scripts/enabled.txt and preserves existing INI; it
does not synchronize installed documentation. `package.py` packages the stage,
not fresh source. **Rebuild/restage before packaging after doc edits**, or the
ZIP will contain old docs. Full research/test fixtures are maintainer material;
the build stages docs, not `tests/` or `tools/`.

Version text currently appears twice in `src/main.cpp`, in `tools/package.py`
and in README. Update these together for an actual release. Keep format schema
numbers independent of the mod version. Check ZIP layout/notices/config and
record its SHA256. No tag/push is necessary to prepare a local ZIP.

Use exported LuaMadeSimple methods: some inline table templates call Lua C
symbols not exported by this loader, causing link failures. The build's reduced
CppUserModBase header removes only unused GUI includes; do not change ABI layout,
virtual order or switch to MinGW's standard-library ABI.

All actor/widget/mailbox loops use `LoopInGameThreadWithDelay`. An earlier
`LoopAsync` → `ExecuteInGameThread` path caused a callback-registry abort during
load. An earlier `GetCurrentStateName` menu poll crashed when no state existed;
use the serial-checked weak resolver. Lua `pcall` cannot make a wrong native
address, raw layout or function signature safe.

## What to record on the next change

Commit implementation/evidence at meaningful milestones. Record game/loader
identities, what changed, actual test results and known gaps. Preserve a reference
for each supported executable rather than relabeling old evidence as new. Keep
end-user setup accurate and short. Research about the ledger/day-end screen in
the original parent project was a separate interest, not part of this mod.
