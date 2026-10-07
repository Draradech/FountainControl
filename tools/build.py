#!/usr/bin/env python3
"""Cross-build against the installed UE4SS ABI without building Unreal itself.

Requires Clang 18, llvm-dlltool 18, LLD 18, xwin CRT/SDK and the pinned UE4SS
public sources in .tools. Bootstrap instructions are in docs/build.md.
"""
from pathlib import Path
import argparse
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / '.tools'
UE = TOOLS / 'ue4ss'
SDK = TOOLS / 'sdk'


def run(*args):
    subprocess.run([str(a) for a in args], check=True, cwd=ROOT)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--ue4ss', type=Path, default=Path.home() / '.local/share/Steam/steamapps/common/Alchemy Factory/AlchemyFactory/Binaries/Win64/ue4ss/UE4SS.dll')
    args = parser.parse_args()
    import pefile
    linker = shutil.which('lld-link-18') or str(TOOLS / 'llvm/usr/lib/llvm-18/bin/lld-link')
    build = ROOT / 'build'
    build.mkdir(exist_ok=True)
    inc = build / 'include/Mod'
    inc.mkdir(parents=True, exist_ok=True)
    # Preserve the upstream class layout, every virtual and exported declaration.
    # Its GUI include recursively requires the private Unreal submodule even
    # though this mod never renders a UE4SS GUI tab. Replace only that include
    # and its function-pointer alias with equivalent forward declarations.
    header = (UE / 'UE4SS/include/Mod/CppUserModBase.hpp').read_text()
    header = header.replace('#include <GUI/GUITab.hpp>',
        'namespace RC { class CppUserModBase; namespace GUI { class GUITab; } }')
    header = header.replace('GUI::GUITab::RenderFunctionType', 'void (*)(CppUserModBase*)')
    (inc / 'CppUserModBase.hpp').write_text(header)
    pe = pefile.PE(str(args.ue4ss))
    definitions = ['LIBRARY UE4SS.dll', 'EXPORTS']
    for export in pe.DIRECTORY_ENTRY_EXPORT.symbols:
        if export.name:
            section = pe.get_section_by_rva(export.address)
            data = '' if section.Characteristics & 0x20 else ' DATA'
            definitions.append('"' + export.name.decode() + '"' + data)
    (build / 'UE4SS.def').write_text('\n'.join(definitions) + '\n')
    run('/usr/lib/llvm-18/bin/llvm-dlltool', '-m', 'i386:x86-64',
        '-d', build / 'UE4SS.def', '-l', build / 'UE4SS.lib')
    includes = [build / 'include', ROOT / 'src', UE / 'UE4SS/include',
                TOOLS / 'fmt-10.2.1/include', TOOLS / 'minhook-1.3.4/include']
    includes += sorted((UE / 'deps/first').glob('*/include'))
    system = [SDK / 'crt/include'] + [SDK / 'sdk/include' / x for x in ('ucrt', 'shared', 'um', 'winrt')]
    options = ['--target=x86_64-pc-windows-msvc', '-std=c++20', '-O2', '-g',
               '-fms-compatibility-version=19.40', '-D_DLL', '-D_MT',
               '-D_ITERATOR_DEBUG_LEVEL=0', '-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH',
               '-DWIN32_LEAN_AND_MEAN', '-DNOMINMAX', '-Wno-ignored-attributes']
    for include in includes:
        options += ['-I', str(include)]
    for include in system:
        options += ['-isystem', str(include)]
    objects = []
    for source in [*(TOOLS / 'minhook-1.3.4/src').glob('*.c'), TOOLS / 'minhook-1.3.4/src/hde/hde64.c']:
        obj = build / (source.stem + '.obj')
        c_options = [x for x in options if x != '-std=c++20']
        run('clang', *c_options, '-c', source, '-o', obj)
        objects.append(obj)
    for source in sorted((ROOT / 'src').glob('*.cpp')):
        obj = build / (source.stem + '.obj')
        run('clang++', *options, '-c', source, '-o', obj)
        objects.append(obj)
    out = ROOT / 'dist/FountainControl/dlls'
    out.mkdir(parents=True, exist_ok=True)
    run(linker, '/dll', '/debug',
        '/out:' + str(out / 'main.dll'), '/pdb:' + str(build / 'main.pdb'),
        '/implib:' + str(build / 'main.lib'),
        '/libpath:' + str(SDK / 'crt/lib/x86_64'),
        '/libpath:' + str(SDK / 'sdk/lib/um/x86_64'),
        '/libpath:' + str(SDK / 'sdk/lib/ucrt/x86_64'),
        *objects, build / 'UE4SS.lib', 'msvcrt.lib', 'vcruntime.lib', 'ucrt.lib',
        'msvcprt.lib', 'kernel32.lib', 'bcrypt.lib')
    (out / 'main.lib').unlink(missing_ok=True)
    shutil.copy2(ROOT / 'config.ini', out.parent / 'config.ini')
    shutil.copy2(ROOT / 'enabled.txt', out.parent / 'enabled.txt')
    shutil.copytree(ROOT / 'Scripts', out.parent / 'Scripts', dirs_exist_ok=True)
    shutil.copytree(ROOT / 'LICENSES', out.parent / 'LICENSES', dirs_exist_ok=True)
    shutil.copytree(ROOT / 'docs', out.parent / 'docs', dirs_exist_ok=True)
    shutil.copy2(ROOT / 'README.md', out.parent / 'README.md')
    print(out / 'main.dll')


if __name__ == '__main__':
    main()
