#!/usr/bin/env python3
"""Package only the staged mod, excluding development saves, logs and PDBs."""
from pathlib import Path
import hashlib
import zipfile

root=Path(__file__).resolve().parents[1]
stage=root/'dist/FountainControl'
assert (stage/'dlls/main.dll').is_file(),'Run tools/build.py first'
assert (stage/'LICENSE').read_bytes()==(root/'LICENSE').read_bytes(),'Stage the current LICENSE before packaging'
archive=root/'dist/FountainControl-0.1.7.zip'
with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as output:
    for path in sorted(stage.rglob('*')):
        if path.is_file():
            info=zipfile.ZipInfo(str(path.relative_to(stage.parent)),date_time=(2026,10,7,0,0,0))
            info.compress_type=zipfile.ZIP_DEFLATED
            info.external_attr=0o644<<16
            output.writestr(info,path.read_bytes())
digest=hashlib.sha256(archive.read_bytes()).hexdigest()
archive.with_suffix('.zip.sha256').write_text(digest+'  '+archive.name+'\n')
print(archive)
print('SHA256',digest)
