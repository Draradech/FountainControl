# Cross-building on Linux

The mod uses the installed UE4SS DLL's import table and the matching public
headers, with the Microsoft C++ ABI and release dynamic CRT. No Unreal Editor
or private Unreal source is required. Do not substitute MinGW libstdc++ for
MSVC's standard library: UE4SS passes standard-library types across this ABI.

Current development setup:

- Ubuntu Clang 18 and llvm-dlltool 18.
- LLD 18 extracted locally from Ubuntu's lld-18 package.
- xwin 0.10.0, MSVC 14.44 CRT and Windows SDK 10.0.26100 in `.tools/sdk`.
- UE4SS public sources at commit `265115c0` in `.tools/ue4ss`.
- fmt 10.2.1 headers in `.tools/fmt-10.2.1`.
- Python pefile (parent project's research virtualenv).

Run `../research/.venv/bin/python tools/build.py` from this repository.
Output is `dist/FountainControl`, suitable for the UE4SS `Mods` directory.

The build generates an include-light copy of the official CppUserModBase
header: it removes the unused GUI dependency and spells the GUI callback's
function-pointer alias directly. Class members, virtual order, signatures
and import declarations are unchanged. The original dependency otherwise
recursively pulls in the private Unreal sources through the debug GUI.
