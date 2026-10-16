# Evidence inventory and regeneration

The repository is the handoff. Nothing under `../research`, `../dumps`, the old
parent staging folder, or an ignored `runtime/` directory is required to build
or understand it. This page identifies the small preserved artifacts and how
to recreate the large/environment-specific ones.

## Included artifacts

| Location | What it preserves |
|---|---|
| [research/1.1.5024/reference-build.json](research/1.1.5024/reference-build.json) | Current executable, hook, ABI and layout profile; accompanied by port audit, fresh native oracle reports and live defaults. |
| [research/1.0.4963/reference-build.json](research/1.0.4963/reference-build.json) | Exact game/loader identities; 22 native hooks, helpers/research anchors, typedefs and raw layouts. Historical data, not mod configuration. |
| [research/1.0.4963/individual/](research/1.0.4963/individual/) | Original placed upright actor, ThrowIndicator and SmartCannonFacility LiveView JSON dumps. Addresses, object names and FName indices belong to that old session. |
| [fountain-range-verification.json](research/1.0.4963/fountain-range-verification.json) | Original native upright oracle results: 10,000 points, slices, translation/rotation, max-distance ignored. |
| [hanging-fountain-range-verification.json](research/1.0.4963/hanging-fountain-range-verification.json) | Native pattern-3 verification and boundary slices. |
| [hanging-fountain-runtime.txt](research/1.0.4963/hanging-fountain-runtime.txt) | Actual upright/hanging loaded component-template defaults and getter returns. Defaults, not placed hanging-instance overrides. |
| [move-pattern-fallback-verification.json](research/1.0.4963/move-pattern-fallback-verification.json) | Unsupported mode behavior and reference getter values; not evidence of coherent extra modes. |
| [tests/fixtures/blueprints](../tests/fixtures/blueprints/README.md) | Real exported native PNG pairs from 4963 and 5024, with/without settings, plus canonical/file hashes and source-key provenance. No full player world needed. |
| [images/](images/) | End-user range graph and settings screenshot. Graph source is tools/plot_range.py; screenshot came from the real game. |

These were selected from parent-project research and local validation artifacts.
Earlier design notes proposed a native saved field, strict stale-save rejection,
0.5 m steps and an old companion path; all are superseded. Their relevant
findings/decisions are consolidated in maintaining/native/blueprint docs rather
than copying obsolete proposals as current instructions. Full historical logs
are replaced by the candid test record in validation.md; logs are not runtime
requirements.

## Native binary and disassembly (large, build-specific)

Use your owned shipping EXE from the Win64 directory. Preserve old binaries
locally before Steam updates; hash and record the Steam appmanifest/build ID.
There is no committed executable or assumption that Steam can later supply an
old build. If the supported EXE has already been overwritten and no local
backup exists, the old machine-code oracle cannot be rerun from just this repo;
the recorded reports and instructions remain available.

With the supported EXE and analysis dependencies:

```sh
.tools/venv/bin/pip install -r tools/analysis-requirements.txt
.tools/venv/bin/python tools/dump_reference.py
.tools/venv/bin/python tools/verify_original_range.py
.tools/venv/bin/python tools/verify_hanging_range.py
```

Outputs go to ignored `runtime/research/`. `dump_reference.py --game <path>`
strictly checks the chosen profile's SHA before disassembling functions.
Original/hanging verifiers use `FOUNTAINCONTROL_GAME_EXE` for an archived path
and independently enforce the same known hash. Recreated JSON should match the
archived reports (their historical `radius_cubes` columns use 25 cm per cube).
`test_trajectory.py` separately verifies the mod solver.

The disassembly output replaces parent `fountain-range-evidence.asm`,
`move-pattern-visual-evidence.asm` and Workshop trace material: it includes range,
speed, visual dispatcher, upload/update/path join and all mod entry points. Extra
static inspection can use `pe_inspect.py disasm`, `xrefs`, `fields`, `constants`
with explicit `--game`. Callsite addresses resolve to their containing `.pdata`
function; don't detour the callsite just because it is listed as an anchor.

To recreate the fallback reach observations, run native predicates with
patterns -1/0/1/2/3/4/99 and parameters min=0, max=700, speed=500:

```sh
.tools/venv/bin/python - <<'PY'
import sys
sys.path.insert(0, 'tools')
from native_predicate import NativePredicate
for mode in (-1, 0, 1, 2, 3, 4, 99):
    print(mode, NativePredicate(pattern=mode).accepts(5000, 0, 300))
PY
```

Getter columns can be rederived by disassembling `0x144a1cc80` on the reference
EXE. For live checking, query a disposable placed indicator, temporarily changing
only MovePattern with its original field values recorded and restored in the
same game-thread command. Do not change a loaded Blueprint template globally or
try unsupported modes in ordinary player worlds. Default upright/hanging getter
results can be logged without mutations via `tools/game_tests/templates.lua`.

## Full reflection dumps and mappings (large)

Use the installed stock loader GUI temporarily; normal mod use needs no GUI.
In `ue4ss/UE4SS-settings.ini`, set `[Debug] GuiConsoleEnabled=1`, restart and
press **Ctrl+O** (the configured ToggleGuiKey defaults to O, with Ctrl required).
GUI visibility/debug options may differ with a later loader, so inspect its
actual settings. Restore them afterward. ConsoleEnabled and GuiConsoleVisible
are optional for stdout/window startup and are not prerequisites for FountainControl.

- GUI Console **Dump Objects & Properties** creates `UE4SS_ObjectDump.txt`.
  `DumpAllObjects()` through the development mailbox is an alternative. The
  ObjectDumperOutputDirectory setting can redirect its output.
- GUI Dumpers **Dump CXX Headers** creates `CXXHeaderDump/` under the program's
  working directory. Search the reflected classes listed in porting.md; at
  least retain BeltTD, upright/hanging BP, player controller, clone state and
  bank/storage widgets for the build you are studying.
- In LiveView, select the **placed actor**, not Default__ class objects or
  `_GEN_VARIABLE` templates; follow its component references and use its JSON
  dump/export action. Output is `IndividualObjectDumps/<object>.json` under the
  program working directory. Search `BP_SmartCannon_C`, verify settled/key and
  actor position, then inspect ThrowIndicator/SmartCannonFacility separately.
- GUI Dumpers **Generate .usmap file** (UnrealMappingsDumper/OutTheShade) produces
  the version's mappings. Include Blueprint-generated types when inspecting
  cooked Blueprint defaults. Loading all assets first can expose unloaded types
  but consumes substantial memory; use a separate disposable inspection session
  and restart afterward. A .jmap is optional.

The game working directory and loader output settings determine output paths;
check the log and search the game/ue4ss folders rather than assuming the parent
project's `dumps/`. Move results to ignored `runtime/reference/<build>/dumps/`.
A .usmap maps serialized property schemas for tools such as FModel to decode
unversioned cooked assets. It is not native source code, debug symbols or a
native hook signature. Regenerate it after game updates. Point FModel at that
build's `Content/Paks` `.pak/.utoc/.ucas` and matching mappings, then inspect the
paths in porting.md. No Unreal Editor or 3D content toolchain is necessary.

## Loader, build dependencies and game fixtures

The end-user README links the moving UE4SS experimental release page. To reproduce
the tested loader, obtain the **standard** archive identified in reference-build.json
and verify its recorded SHA. At handoff it was locally available in
`~/Downloads/UE4SS_v3.0.1-1161-g6eb3d9bc.zip`. Filename and checksum are the
identity; its DLL banner uses a different Git SHA. Keep archives outside Git.
If the moving release no longer offers that exact build, use a preserved archive
or treat the newer loader as a separate compatibility test, not the same evidence.

Bootstrap recreates compiler-side dependencies from pinned upstream sources
and Microsoft CRT/SDK download steps; see build.md and the script for hashes.
The parent Python environment is unnecessary. Optional analysis/plot requirements
are now in this repo. `build/`, `dist/`, `.tools/`, `runtime/` remain ignored.

Desktop automation needs no parent scripts or server: testing.md records the
optional local X11 CLI and standard-package substitutes if it is absent.

Full historical test saves were multi-megabyte copies of a player world; don't
commit them. [testing.md](testing.md) describes building a fresh Sandbox world,
choosing actual keys/catchers, exporting a new collision-test PNG and rerunning
all integration checks. The included PNGs can test portable import immediately;
a new same-world export is needed for deliberate key collisions. Back up and
hash current saves before live work; old world titles and manifests are stale.

Collect new persistent FountainControl.log, probe.log, UE4SS.log, fixture file
hashes/metadata and UI screenshots under ignored runtime/validation/<date>/.
Steam cloud evidence is in Steam's logs/cloud_log.txt. Preserve enough selected
results in validation.md to make claims reviewable; don't publish personal
account IDs, cloud URLs or complete unrelated player data.

## Images

Regenerate the range graph from code:

```sh
.tools/venv/bin/pip install -r tools/plot-requirements.txt
.tools/venv/bin/python tools/plot_range.py
```

It writes tracked `docs/images/fountain-range.png`. The graph intentionally
shows the native outer envelope and the corrected flat cap, with emitter height
above the ground rather than cube-scale/limit overlay annotations. It is a
vertical cross-section rotated around the emission axis, not the range ring.
Do not close the native exclusion's lower edge with a spurious line or move the
corrected top stroke by half its width. Review the rendered PNG after changes.
The settings screenshot can be recreated by opening a fountain via real E
Inspect at the native left/vertically centered position and capturing the panel.
