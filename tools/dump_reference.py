#!/usr/bin/env python3
"""Recreate disassembly evidence from a fingerprint-matched reference executable."""
import argparse
import contextlib
import hashlib
import json
from pathlib import Path

from pe_inspect import GAME, Image

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game', type=Path, default=GAME)
parser.add_argument('--profile', type=Path, default=root / 'docs/research/1.1.5024/reference-build.json')
parser.add_argument('--output', type=Path, default=root / 'runtime/research/reference-disassembly.txt')
args = parser.parse_args()
profile = json.loads(args.profile.read_text())
with args.game.open('rb') as executable:
    digest = hashlib.file_digest(executable, 'sha256').hexdigest()
if digest != profile['executable_sha256']:
    parser.error('Executable does not match the reference profile; do not use old addresses on a new build')
image = Image(args.game)
args.output.parent.mkdir(parents=True, exist_ok=True)
with args.output.open('w') as output, contextlib.redirect_stdout(output):
    print(f'Executable SHA256: {digest}; image base: {image.base:#x}')
    for category in ('hooks', 'helpers', 'research_anchors'):
        for name, address in profile[category].items():
            print(f'\n{category}: {name} @ {address}')
            image.disassemble(int(address, 0))
print(args.output)
