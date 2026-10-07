#!/usr/bin/env python3
"""Send Lua from stdin/file to the explicitly enabled local development mailbox."""
import argparse
from pathlib import Path
import sys
import time
import uuid

p = argparse.ArgumentParser()
p.add_argument('file', nargs='?', type=Path)
p.add_argument('--timeout', type=float, default=15)
p.add_argument('--mod-dir', type=Path, default=Path.home() / '.local/share/Steam/steamapps/common/Alchemy Factory/AlchemyFactory/Binaries/Win64/ue4ss/Mods/FountainControl')
a = p.parse_args()
code = a.file.read_text() if a.file else sys.stdin.read()
mail = a.mod_dir / 'commands.lua'
if mail.exists():
    sys.exit('A command is already pending')
marker = uuid.uuid4().hex
log = a.mod_dir / 'probe.log'
offset = log.stat().st_size if log.exists() else 0
wrapped = f'local ok,err=xpcall(function()\n{code}\nend,debug.traceback); if not ok then FCDev.log("ERROR:{marker}"); FCDev.log(err) end; FCDev.log("DONE:{marker}")\n'
tmp = mail.with_suffix('.tmp')
tmp.write_text(wrapped)
tmp.replace(mail)
deadline = time.monotonic() + a.timeout
while time.monotonic() < deadline:
    if log.exists():
        with log.open('rb') as f:
            f.seek(offset)
            text = f.read().decode(errors='replace')
        if f'DONE:{marker}' in text:
            print(text.split(f'DONE:{marker}')[0].rstrip())
            sys.exit(1 if f'ERROR:{marker}' in text else 0)
    time.sleep(.1)
sys.exit('No completion received; is the game running with development commands enabled?')
