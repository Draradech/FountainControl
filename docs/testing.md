# Reproducing tests and live research

Host checks require no parent project and do not launch or alter the game.
Live probes do alter a test world. This page replaces the old assumption that
fountain 28916 and a particular development save already exist on the machine.
Historical results and their limits remain in [validation.md](validation.md).

## Host checks

From the repo root, after setting up the environment in [build.md](build.md):

```sh
python3 tools/test_state.py
clang++ -std=c++20 -O2 -fsanitize=address,undefined -I src tests/blueprint.cpp -o /tmp/fountain-blueprint-test
/tmp/fountain-blueprint-test
python3 tools/inspect_blueprint.py --check-fixtures
.tools/venv/bin/pip install -r tools/test-requirements.txt
.tools/venv/bin/python tools/test_trajectory.py
```

The state suite uses host Lua 5.4 and mocks Unreal/native bridge APIs. It tests
companion handling, checksum recovery, normalization, clone adoption and ghost
lifecycle, not real Slate or native delivery. The C++ sanitizer suite checks
metadata and synthetic PNG corruption cases. The fixture check independently
reads actual exported PNGs. The trajectory suite executes original game machine
code in Unicorn and checks 10,080 points against the C++ solver, including
bounds and repaired endpoints. It strictly requires the supported EXE hash.

For an archived EXE or a non-default Steam library, set
`FOUNTAINCONTROL_GAME_EXE=/absolute/path/AlchemyFactory-Win64-Shipping.exe` for
the trajectory/original/hanging verification tools. Other tools use explicit
`--game`, `--ue4ss`, `--ue4ss-dir` or `--mod-dir` options. The environment variable
selects a file; it does not disable native comparison's compatibility check.

## Local paths and installation

| Item | Linux/Proton default | Windows equivalent |
|---|---|---|
| Win64 game | `~/.local/share/Steam/steamapps/common/Alchemy Factory/AlchemyFactory/Binaries/Win64` | Steam library's same relative game path |
| Mod | `<Win64>/ue4ss/Mods/FountainControl` | Same relative path |
| Saved root | `~/.local/share/Steam/steamapps/compatdata/3669570/pfx/drive_c/users/steamuser/AppData/Local/AlchemyFactory/Saved` | `%LOCALAPPDATA%\AlchemyFactory\Saved` |
| Native worlds | `<Saved>/SaveGames/<world>/` | Same relative path |
| Blueprint exports | `<Saved>/Blueprints/` | Same relative path; query the subsystem for the actual directory |
| Companion | `<world>/FountainControl/<slot>.sav` (JSON) | Same relative path |

Steam libraries/Proton user names can differ; discover the actual directories.
The original user's current world names are not test prerequisites. Do not use
Continue, silently load the main world, or assume an old hash manifest protects
today's saves. Before live work with the game closed, back up existing SaveGames
and Blueprints to ignored `runtime/backups/<timestamp>/`; record a SHA256 map of
all existing files with relative paths. Afterward compare those original files
and restore only if needed with the user's intent understood. Native saves are
private and should not be committed. Newly created disposable worlds/blueprints
are expected additional files; report them separately.

Install the tested stock UE4SS archive using README. Build with matching imports,
close the game, then deploy:

```sh
.tools/venv/bin/python tools/build.py
python3 tools/deploy.py
```

The deploy script preserves installed INI. Temporarily enable development commands:

```ini
[Development]
EnableCommands = 1
```

Restart, check `FountainControl.log` for executable verification/all hooks enabled,
and check `probe.log` for mailbox startup. At the end restore the previous INI,
remove leftover `commands.lua`/`commands.tmp` and restart with commands disabled.
Never carry this debugging option into distributed default configuration.

## Create a new disposable fixture

Use the game's ordinary New Game/Sandbox UI to create a world whose title matches
`^FountainControl_.*Dev$`, for example **FountainControl_Handoff_Dev**. Leave
multiplayer off. The game may append `_` and ten timestamp digits; the mutation
tests also accept that exact suffix. They reject unrelated world names. Provide a large
clear supported floor area, two upright Arcane Fountains, an isolated hopper
receiver with room in its inventory, and enough materials (or Sandbox mode).
Keep ordinary production chains away from the test receiver so incoming items
don't interfere with exact counts. A simple belt-fed fountain/receiver chain
is additionally needed for the ordinary gameplay check.

Save a native `SaveGame0` before experimenting. Discover actual fountain IDs and
receiver names rather than expecting historical IDs:

```sh
python3 tools/probe.py tools/game_tests/inventory.lua
python3 tools/probe.py tools/game_tests/templates.lua
```

The second probe logs real numeric getter return values; LiveView's
`call_succeeded: true` is not the value. Component templates are defaults;
query the placed actor's fields separately for instance overrides. If templates
aren't loaded, open the build menu or place the relevant building and retry.

Scripts sent by probe.py are executed on the **game thread**. A one-off readiness query:

```sh
python3 tools/probe.py <<'LUA'
local name,remove,ready=FCNativeInfo()
FCDev.log(name..'; remove='..tostring(remove)..'; ready='..tostring(ready))
LUA
```

`probe.py` returns at its own DONE marker. A script that schedules a loop has
only **started**, not passed, at that point. Monitor subsequent completion/failure
lines in installed `probe.log`. A timeout can leave a pending command for later
execution: inspect/remove it before reopening or switching worlds. Native pointer
addresses and FName comparison indices are session-local; re-enumerate after
reload/restart. Linux host paths for in-game `dofile` need the Proton `Z:/...`
form; alternatively copy a script into the mod and use `FCDev.root`.

## Desktop automation and repeatable aiming

The original Linux sessions used the local X11 CLI at
`~/Software/comp_use_tool/computer`. It is optional and not part of the mod.
Its `README.md`/`AGENT.md` describe the current executable; start with `doctor`
and `windows`, select the Alchemy Factory window, `activate <id>`, then
`screenshot --window <id> --output <path>`. Click coordinates are screen
coordinates; add screenshot response x/y to a point measured in the window
image. Window IDs and positions must be rediscovered after restarts. Do not
reuse old screenshot coordinates. The tool requires X11, Python, libX11,
libXtst/libXcomposite and tkinter for clipboard support; no server/MCP bridge
was needed. There was no configured upstream remote to fetch this local tool.

On another X11 machine, `xdotool` for input/window enumeration and `scrot` for
screenshots are sufficient replacements (Ubuntu packages with those names):
`xdotool search --name 'Alchemy Factory'`, `xdotool windowactivate <id>`, and
`scrot -u runtime/game.png`. Use an available platform desktop tool on Windows
or inspect manually. Tiny instantaneous key pulses can be missed by the game's
input polling. A held press worked in earlier checks; for example:

```sh
xdotool keydown e
sleep 0.2
xdotool keyup e
```

Always inspect a new screenshot before/after an action. For deterministic aiming
in a disposable world, a mailbox command can position the **player**, while the
actual key press still proves the real interaction:

```lua
local pc=FindFirstOf('BeltTDPlayerController')
local actor -- Resolve the chosen settled fountain from its current SettleKey.
for _,a in pairs(FountainControl.actors) do
    if a:GetSettleKey()==FCDeliveryFountainKey then actor=a end
end
assert(actor)
local p=actor:K2_GetActorLocation()
pc.Pawn:K2_SetActorLocation({X=p.X,Y=p.Y+200,Z=p.Z+88},false,{},true)
pc:SetControlRotation({Pitch=-30,Yaw=-90,Roll=0})
```

The reflected Pawn property avoids a getter-name collision encountered during
research. These offsets aim at the reference upright model; verify the real
focus/View prompt rather than treating them as an assertion. Optional
`FindFirstOf('TimeDateSubsystem'):SetTimePaused(true)` pauses the day clock,
**not all native item/visual updates**; restore false afterward.

For an already-created disposable world, the pinned player controller's
`CreateSession(false, slot, false, false, worldTitle, sandbox, '', false)` was
used to load a slot from the menu. The full reflected signature is
`CreateSession(bNewGame, GameName, bNetSession, bUseLAN, worldTitle, bSandbox,
roomPassword, bIsOpenRoom)`. Supply the actual disposable title/slot and matching
Sandbox mode; never substitute a player-world name or call it blindly during
an active world. Ordinary New Game/Load UI is still the fixture creation path.

## Controlled delivery matrix

Choose an isolated receiver from the inventory output. Set the actual source
key and optionally its exact catcher `GetFullName`, then send the script body
in the same mailbox request. For example, create an ignored local driver:

```lua
FCDeliveryFountainKey = 12345 -- Replace with the fresh inventory's key.
FCDeliveryReceiverName = 'HopperCatcherComponent /Game/...' -- Exact observed name.
dofile(FCDev.root..'/delivery-test.lua')
```

Copy `tools/game_tests/delivery.lua` into the installed mod as
`delivery-test.lua`, then `python3 tools/probe.py runtime/delivery-driver.lua`.
Alternatively prepend the two assignments to the file passed through stdin.
Wait for **DELIVERY COMPLETE: 14 cases** and check there is no DELIVERY FAILED.
Remove the installed helper after testing. The test moves the real receiver,
injects SilverCoin through the native hopper path and checks delayed inventory
arrival; it restores location/source settings on success and caught failure.
Received coins remain in the disposable inventory. Native crashes cannot run
Lua cleanup, so discard/reload the fixture rather than trusting its final state.

This deliberately bypasses upstream belts, fountain launch and receiver selection.
Also run an ordinary belt-fed chain delivering into the former exclusion, a
capped-out receiver, and a vanilla-valid receiver. Repeat with exclusion removal
false after restarting; vanilla eligibility must match, while caps still apply.
Hanging fountains and unrelated launchers/interactions should remain unchanged.

## Portable blueprint and ghost checks

Two real PNGs are included in [tests/fixtures/blueprints](../tests/fixtures/blueprints/README.md).
Copy them, unchanged, to the actual native Blueprint directory. The settings
PNG contains two fountains with 700/300 and 350/150 cm; the other is its exact
native payload without `fcRs`. Inspect with:

```sh
python3 tools/inspect_blueprint.py tests/fixtures/blueprints/fountains-with-settings.png
```

To run actual construction, choose a genuinely clear supported placement in
world cm; set `FCBlueprintTestFile='fountains-with-settings.png'` and
`FCBlueprintTestPosition={x,y,z,quarterTurn}` (0–3), then execute
`tools/game_tests/blueprint.lua` through the mailbox as for delivery. The native
probe's final `true` argument **bypasses costs and really builds objects**; it
is not a preview mode. Both fountains must fit. Repeated tests construct more
objects; pick new space or reload a disposable slot.

Included PNG source IDs are 28916/73598, but a fresh world usually has different
keys. Importing it proves portability, not automatically collision handling.
For a genuine collision test: set two fresh-world fountains to 700/300 and
350/150, record their keys, export exactly that selection through the normal
blueprint UI, then change those originals to 100/50. Import the just-exported
PNG into the same disposable world or a closed-game copied world preserving
those IDs. Set `FCBlueprintConflictKeys={keyA,keyB}` and
`FCBlueprintRequireCollisions=true` before the blueprint test. It asserts that
two real collisions were exercised; a test with none logs zero rather than
claiming coverage. Export order may differ; the test accepts either order of
the two settings. The collision setup changes original settings in the disposable
world; it does not restore them.

Repeat ordinary mouse placement (not only development construction), rotation
with room for both buildings, saved blueprint title/description editing and
export inspection. Test a metadata-free import defaults despite colliding keys.
Disable **both DLL and Lua** and restart; `enabled.txt` removal alone may be
insufficient if other mod-loader config enables it. Moving the entire mod folder
out of Mods is simplest. Import/place the same PNG normally, and load a native
save made with the mod. UE4SS may remain installed. No native save changes should
be required to make either work.

While real copy/cut/blueprint placement ghosts are active, run
`tools/game_tests/preview.lua`; optionally set `FCPreviewExpected={150,300}`
(or `{-1,-1}` for the vanilla PNG) first. Check ordinary middle-mouse quick copy,
bulk copy, native cut/move, rotation, repeated native updates, zero/500/Default
transitions, cancellation/reuse and an empty menu state. Use the game's current
bindings rather than assuming an old key map; recorded tests used middle mouse,
F copy and T cut. For repeated sampling, schedule with
`LoopInGameThreadWithDelay` and watch its own final success/error marker.
Never poll `GetCurrentStateName` at menus. Ghosts must stay unsettled, shared
key zero must never enter settings/native save maps, and each ring must keep its
own baseline. The preview test's 470–490 cm native radius assumption is specific
to the reference game art and must be remeasured after an update.

## UI, persistence and cloud acceptance

Use actual **E** Inspect and mouse input; opening a widget by command alone
missed the missing interaction in the first prototype. Confirm left/vertical
layout, native bank styling, Escape/focus restoration and ordinary storage UI
afterward. Select adjacent 0.1 m positions, 10.0 m maximum drop, 5.0 m maximum
numbered horizontal and the following Default position. Compare stored cm and
ring radius, not just labels.

Save normally; independently parse the nested companion's schema, IDs and
values and compute SHA256 of its **parent native save**. Check replacement,
no old-path output and no remaining `.sav.tmp`. Exercise AutoSave0/1/2 rotation,
restart and reload (including reused GameState address). Save once with the mod
removed, restart/re-enable, confirm surviving keys restore valid limits and the
checksum warning is in persistent FountainControl.log. New/unrecorded keys get
defaults. Tests of invalid JSON/schema/bounds can use host fixtures or disposable
companions; never corrupt player saves.

Exit cleanly, wait for Steam sync and inspect Steam's `logs/cloud_log.txt` for
both the native save and `FountainControl/<slot>.sav`. A new disposable world can
sync too; cleanup is a separate deliberate action. An upload is not proof of a
second-machine download. Record OS, loader, EXE hash, fixture creation, coverage
and evidence path in validation notes, then restore config/check original-file
hashes. Do not include personal cloud URLs/account data in public logs.

## Useful bridge calls

Pointers below are integer `GetAddress()` values, not UE4SS object wrappers.
Native probes are development-only; check readiness before use. Definitions in
`src/main.cpp`/`src/native.cpp` are authoritative.

| Call | Result / purpose |
|---|---|
| `FCNativeInfo()` | Version string, remove-exclusion boolean, ready boolean. |
| `FCNativeRoot()` | Mod root UTF-8 path, development boolean. |
| `FCNativeRead(address,length)` | Guarded bytes or nil, up to 16 KiB; development only. |
| `FCProbeReach(facility,dx,dy,dz)` | Vanilla, modified, emitter X/Y/Z, flight time/vertical speed (or -1), repaired flag. Delta in cm, not world point. |
| `FCProbeCatch(catcher)` | Real catch world X/Y/Z. |
| `FCFlightStats()` | Repaired receive, visual and hopper event counts. |
| `FCProbeDeliver(facility,catcher,saveSubsystem,visualSystem,itemFNameIndex)` | One real native item enqueue; boolean. |
| `FCProbeClone(spatialSubsystem,cloneState,sourceKey,x,y,z)` | Real source-based construction; boolean. State is not player controller. |
| `FCProbeBlueprintPlace(cloneState,x,y,z,rotation,ignoreCost)` | Count of actual successful constructions. |
| `FCGetGhostPreviews(playerController)` | Map ghost actor address → horizontal cap; checked native weak/entity resolution. |
| `FCGetNativeLimits(key)` | Found boolean, drop, horizontal. |
| `FCFingerprint(path)` | File SHA256 or empty string. |
| `FCLog(message)` | Persistent FountainControl.log. |
| `FCLoadGeneration()` | Reload counter. |

Internal calls include `FCSetFacility`, `FCRemoveFacility`, `FCResetFacilities`,
`FCSetSaveRoot`, class-FName setters and interaction queue registration/consumption.
Use `FountainControl.set(actor,drop,horizontal)` for normalized live settings,
`get(actor)` for clone-aware reads, `scan()` for discovery and `preview()` for
ring updates. Native pointer injection with the wrong type/ABI can crash even
inside xpcall; development functions are not an external stable modding API.

For ordinary production tests, a Smart Hopper needs its native item filter set
to the supplied product and free capacity. A basic hopper is not a smart target.
A portal-to-belt-to-fountain chain exercises native supply, selection and launch;
the isolated receive probe above does not replace that check.

On the local X11 computer tool, instantaneous click/key pulses can be missed.
A stationary drag with `--steps 1 --duration .1` provides a held UI click.
Captured mouse placement needs a held button without motion events; repeated
stationary motion events can rotate the camera or repeat placement. Always
activate the game immediately before input.
