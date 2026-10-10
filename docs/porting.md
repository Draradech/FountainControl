# Updating to a different game or UE4SS version

A Steam update can change native addresses, struct layouts, calling conventions,
Blueprint defaults and UI assets independently. FountainControl is intentionally
pinned to one executable; a successful UE4SS startup or similar version number
is not proof that its native hooks are compatible.

## Preserve and identify both builds

Before updating, preserve the owned old shipping EXE, Steam appmanifest,
UE4SS archive and DLL, configuration, and a disposable native save + settings
companion + blueprint export. Keep these in ignored `runtime/reference/<build>/`
or another local archive. Record SHA256 for every binary. Do not commit game
binaries, full player saves or downloaded SDK/loader archives. See
[artifacts.md](artifacts.md) for locations and regeneration.

Start a separate branch for the port. Record new game display version, Steam
build ID from `steamapps/appmanifest_3669570.acf`, EXE SHA, image base and UE4SS
archive/DLL/SDK identities. If the new EXE SHA exactly matches the old one,
native-code relocation is unnecessary; a loader-only update still needs ABI,
startup, UI, save and blueprint checks. If it differs, leave the old mod gated
while collecting evidence. Development mailbox/reflection inspection can run
with `EnableCommands=1` even while native readiness is false; there will be no
`FountainControl` state/UI table in that case. Don't call native probes then.

## Recreate reflection and asset evidence

Use the new build's UE4SS dumps and mappings; don't decode new cooked assets
with the old .usmap. [artifacts.md](artifacts.md) gives exact dump controls.
Place an upright fountain and a hanging fountain in a disposable Sandbox world.
Record their actor/component dumps and query live defaults/return values.

Check at least:

- `BP_SmartCannon_C`, `BP_SmartCannonCeiling_C`, ThrowIndicator and
  SmartCannonFacility names, types, offsets and owning actor transforms;
- MovePattern, speed parameters, UseDrop, EmitPosition, getter return and real
  rendered mesh bounds/scales (the getter is not the ring's baseline);
- `GameSaveLoadSubsystem`, `BeltTDPlayerController`, `SpatialCloneState`,
  clone building/state records, native receiver and product visual layouts;
- `ScriptInteractionComponent`, Inspect info registration, HUD interaction
  routing, `WBP_StoragePanel_C` fields/children and activation lifecycle;
- cooked bank slider WidgetTree template and its parent chain/frame brush;
- native save slot path, world/slot naming and actual companion cloud transport.

Important cooked paths currently include:

```text
/Game/Blueprints/Buildings/BP_SmartCannon
/Game/Blueprints/Buildings/BP_SmartCannonCeiling
/Game/Blueprints/WBP/FurnitureInfo/WBP_BankPanel
/Game/Blueprints/WBP/FurnitureInfo/WBP_BankPanel.WBP_BankPanel_C:WidgetTree.Sld_SupplyLimit
/Game/DataTables/DT_Buildings
```

CXX dumps describe reflected types, not all native C++ implementation. For
unreflected worker records and helper code, follow disassembly. Shared Blueprint
VM function pointers are not unique native implementations. A large dump is a
search index: begin with these exact classes/methods, then their callers.

## Re-discover the native graph

Install the optional analysis requirements into the repo's own environment:

```sh
.tools/venv/bin/pip install -r tools/analysis-requirements.txt
.tools/venv/bin/python tools/dump_reference.py --game /path/to/old/AlchemyFactory-Win64-Shipping.exe
.tools/venv/bin/python tools/pe_inspect.py --game /path/to/new/AlchemyFactory-Win64-Shipping.exe fields 0x2a0 0x2c0 --low 0x140000000 --high 0x150000000
```

The first command recreates old annotated disassembly with a **strict old-build
fingerprint**. The second only suggests candidates from old field displacements;
changed offsets may make it miss everything. `--low`/`--high` refer to preferred
VAs, and the tool's defaults restrict the old game-code region. Use new PE
section bounds instead of assuming the example upper bound covers every build.
Other useful modes are `disasm <VA> [--length <bytes>]`, `xrefs <VA>` and
`constants <VA>...`. Default disassembly uses `.pdata` function ranges; a missing
range falls back to 256 bytes. Raw E8/E9 xrefs can be false instruction-boundary
hits and omit indirect calls. Field scans only search disp32 encoding, so small
disp8 fields need direct instruction review. RIP-relative f64 annotations are
interpretations of arbitrary bytes, not confirmed constant types.

Load old and new EXEs into Ghidra/IDA if useful. Find functions through their
registration strings, reflected UFunction native pointers/wrappers, constructor
field assignments, distinctive constants, and call graph relationships. Confirm
candidates by instructions and caller/callee behavior; never apply one guessed
address delta across all hooks. The archived profile supplies **22 hook entries**,
eight helpers, extra research anchors, typedefs and layouts to audit:

| Group | Confirmation required |
|---|---|
| Geometry | Reach predicate, EmitPosition transform, speed checker, horizontal radius getter, target refresh and incremental registration. Decode all pattern branches and drop/arrival constraints again. |
| Selection and launch | Selection retains item compatibility and rotates among cached targets; retry/count and cache-invalidating fields still mean what they did. Launch supplies the facility emission context. |
| Five receiver paths | Cauldron, goods, hopper, raw-material and trash functions all have the expected signatures and receiver/catch-point vtable slot. Repair marker cannot leak into saved state or other launchers. |
| Visual/arrival | Visual creation ABI, FTransform precision, visual entity handle/stride/pattern/duration/velocity, hopper array entry/timer and native queued worker structures. Both animation and delivery duration must agree. |
| Interaction | Primary ScriptInteraction dispatch and owner field; non-fountain interactions must pass through untouched. |
| Persistence | Actual save begin and completion result, writer closing order, slot FString conventions, reload generation. Hooking a reflected notification alone can miss successful writes or run too early. |
| Clone/build | State is SpatialCloneState, not player controller. Clone result/cost flag, build output success and actual new actor, record stride and identity, cut source deletion and typed outer. |
| Blueprint | Both enter paths, common PNG read success/out record layout, pack/edit path, clear selection and synchronous SaveArrayToFile. Confirm borrowed TArrayView64 rather than owning allocation, PNG tail magic and record order. |
| Ghosts | Checked CurrentState weak reference, placement flag, entity-ref resolver, serial/lifetime checks, clone records and class FName comparison. Avoid unsafe state-name getter at menu. |
| Development probes | Product/config packet, game/subsystem pointers, synthetic receive parameters and clone packet layout. A broken probe is not evidence that the feature is broken, nor safe to run. |

`reference-build.json` records all raw offsets used at handoff. Distinguish
**different contexts with similar offsets**: the probe's receive parameter block
stores a visual-system pointer at +0x98, while a queued hopper worker stores
flight speed/duration at +0x98. Vec is three doubles (24 bytes). FNames stored
in records are 64-bit process-local values; class comparison uses the comparison
index. FString count includes UTF-16 NUL. Windows x64 function signatures and
by-value aggregate/out-parameter conventions matter as much as addresses.

The clone development packet is 0x218 bytes, copied from native source info
and record fields. Blueprint placement copies a 0x200 record plus 0x14 selection
parameters. Keep its individual copy map in the profile/source; don't construct
new packet layouts from reflected field names alone. The general `field<T>`
helper is an unchecked raw dereference. VirtualQuery in FCNativeRead guards a
memory region, not semantic type correctness. Native faults bypass Lua pcall.

## Update the implementation and independent oracle

Once each entry's evidence is sound, update addresses, offsets, helper calls,
ABIs and Blueprint/UI paths in `src/native.cpp` and scripts on the port branch.
Only then permit the **exact newly measured executable hash** in initialization.
Keep all-or-nothing MinHook setup and readiness gating. Update the startup
version text as well as README; retain the old snapshot in its own directory
and create a new dated profile with provenance.

Port `tools/native_predicate.py` independently: its pinned hash, ENTRY,
TRANSFORM/FNAME stubs, synthetic facility/indicator offsets and memory needs
are game-specific too. `FOUNTAINCONTROL_GAME_EXE=/path/to/exe` selects an archived
file but never disables hashing. If the new code calls extra helpers, understand
what they do before adding stubs; stubbing the eligibility calculation would
make the comparison circular. Unexpected memory access or instruction-count
failure should fail the test, not be forced to success.

Re-run randomized and boundary native comparison **with exclusion removal off**;
rederive constants if Blueprint defaults/native arithmetic changed. Then test
that all accepted vanilla samples remain accepted with removal on inside caps,
plus trajectories, max speeds/accelerations, endpoints and repaired paths.
Do not retain hard-coded 980/500/700/1000 just because old tests expect them.
`verify_original_range.py` and `verify_hanging_range.py` are archived-build
reproduction tools: adapt their expected equations/reports explicitly for a new
build rather than silently overwriting old evidence. Unknown MovePattern values
are fallback behavior, not useful extra fountain modes.

For a loader change, review the new official CppUserModBase layout, virtuals,
LuaMadeSimple exports and startup callbacks. Pin matching SDK sources, generate
imports from the installed DLL, preserve DATA exports, and keep release MSVC
CRT/STL ABI. The include-light header substitution must still be equivalent.
Check all bridge registrations/callback signatures and game-thread loop APIs.
Do not bring back the historical custom loader signatures/engine overrides
without evidence that the new loader actually needs them.

## Prove compatibility before claiming it

Follow the fresh-world matrix in [testing.md](testing.md), and add scenarios
for changed native paths. At minimum: stock loader install; menu/loading with
no controller state; real E UI and storage restoration; original/corrected
geometry and actual production chain; both setting bounds/Default; save,
autosave rotation, restart, mod-off save/recovery warning; copy, cut/move,
blueprint export/import/edit/mod-free placement; ghost stability/cancellation;
non-fountain passthrough. Re-measure ring bounds instead of carrying the old
preview test's 470–490 cm baseline assertion to new art. Exercise the changed
receiver types, not just the known hopper fixture.

Verify on real Windows when available and accurately label Proton results.
Check Steam cloud logs for the new companion path; second-machine download is
additional coverage, not established by an upload. A native save must still
load without FountainControl. Never add SaveGame flags to work around identity
or persistence issues. If game migrations change SettleKeys, research mapping
and ask about the resulting behavior instead of silently applying unrelated IDs.

Keep the PNG metadata schema stable unless its data model actually changes.
Inspect a freshly exported native PNG, CRC, strict canonical hash and unchanged
tail using `inspect_blueprint.py`; check old portable fixtures still import.
If the new Workshop upload code still passes the original content file without
re-encoding, document the inspection as such. Do not claim an online round trip
without doing one.

Finally update supported-build documentation and validation evidence, run the
host checks and rebuild/package from the current tree. Bump the mod version in
all listed locations, retain the license/notices and production configuration,
and record the ZIP hash. Commit the port and evidence. Release/tag/push remain
separate user-requested actions.
