# Cross-building on Linux

The mod uses the installed UE4SS DLL's import table and the matching public
headers, with the Microsoft C++ ABI and release dynamic CRT. No Unreal Editor
or private Unreal source is required. Do not substitute MinGW libstdc++ for
MSVC's standard library: UE4SS passes standard-library types across this ABI.

Install Python 3.12+, Git, Clang 18, llvm-dlltool 18 and lld-link 18. On Ubuntu
24.04 the compiler packages are `clang-18 llvm-18 lld-18`; ensure `clang` and
`clang++` resolve to version 18. Then run from this repository:

```sh
python3 -m venv .tools/venv
.tools/venv/bin/pip install -r tools/requirements.txt
.tools/venv/bin/python tools/bootstrap.py --accept-microsoft-license
.tools/venv/bin/python tools/build.py
```

Review Microsoft's CRT/SDK license before using the acceptance flag. Bootstrap
fetches xwin 0.10.0, fmt 10.2.1 and MinHook 1.3.4 with pinned SHA256 checksums,
and UE4SS at commit `6eb3d9bc3346ba01f7191612b5812e0e47a3bbe1`. It obtains MSVC
14.44 and Windows SDK 10.0.26100 through xwin. Dependencies remain in `.tools/`.

`build.py` defaults to the installed game's `ue4ss/UE4SS.dll`; override with
`--ue4ss /path/to/UE4SS.dll`. It generates an import library from that DLL,
preserving DATA exports, and outputs `dist/FountainControl`. An extracted
Ubuntu LLD in `.tools/llvm/usr/lib/llvm-18/bin/lld-link` is a local fallback
when `lld-link-18` is not on PATH.

The build generates an include-light copy of the official CppUserModBase
header: it removes the unused GUI dependency and spells the GUI callback's
function-pointer alias directly. Class members, virtual order, signatures
and import declarations are unchanged. The original dependency otherwise
recursively pulls in the private Unreal sources through the debug GUI.

## Checks

```sh
python3 tools/test_state.py
clang++ -std=c++20 -O2 -fsanitize=address,undefined -I src tests/blueprint.cpp -o /tmp/fountain-blueprint-test
/tmp/fountain-blueprint-test
python3 tools/inspect_blueprint.py --check-fixtures
.tools/venv/bin/pip install -r tools/test-requirements.txt
.tools/venv/bin/python tools/test_trajectory.py
```

The blueprint test checks portable metadata and PNG parsing with host sanitizers.
The Lua suite also checks ghost indicator lifecycle and isolation from saved settings.
The state test requires the host Lua 5.4 shared library (`liblua5.4-0` on
Ubuntu). The trajectory test executes the installed, supported game's native
reach predicate in Unicorn and compares it with the C++ solver. It reads and
fingerprints the executable; it does not run or modify the game. Both tests
run independently of the parent research repository.

## Local integration probes

`python3 tools/deploy.py` copies the staged DLL and scripts to the local game,
preserving the installed INI. Restart after changing the DLL.

Development commands are disabled by default. To enable them temporarily,
add `[Development]` / `EnableCommands = 1` to the installed `config.ini` and
restart. `tools/probe.py` sends Lua through an atomic file mailbox; the mod
executes it on the game thread and returns nonzero for Lua failures.
`tools/game_tests/delivery.lua` requires a disposable `FountainControl_*Dev`
world and an explicitly selected `FCDeliveryFountainKey` from that world.
It injects items into a real hopper worker and checks arrival, inventory counts and boundaries. Completion appears later
in `probe.log` as `DELIVERY COMPLETE` or `DELIVERY FAILED`.

The INI enables arbitrary local Lua execution and development native calls;
leave it disabled for normal play. There is no network listener.

See [maintaining.md](maintaining.md) for the complete handoff,
[testing.md](testing.md) for creating fresh live fixtures, and
[porting.md](porting.md) for updating native integration. Optional analysis and
plot dependencies are in `tools/analysis-requirements.txt` and
`tools/plot-requirements.txt`; the parent research environment is not required.
