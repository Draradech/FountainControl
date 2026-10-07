#!/usr/bin/env python3
"""Install the staged mod for local development; preserve the user's config."""
from pathlib import Path
import argparse
import shutil

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--ue4ss-dir', type=Path, default=Path.home() / '.local/share/Steam/steamapps/common/Alchemy Factory/AlchemyFactory/Binaries/Win64/ue4ss')
args = p.parse_args()
source = root / 'dist/FountainControl'
target = args.ue4ss_dir / 'Mods/FountainControl'
for folder in ('dlls', 'Scripts'):
    shutil.copytree(source / folder, target / folder, dirs_exist_ok=True)
if not (target / 'config.ini').exists():
    shutil.copy2(source / 'config.ini', target / 'config.ini')
mods = args.ue4ss_dir / 'Mods/mods.txt'
text = mods.read_text()
if not any(line.split(':')[0].strip() == 'FountainControl' for line in text.splitlines()):
    text = 'FountainControl : 1\n' + text
    mods.write_text(text)
print(target)
