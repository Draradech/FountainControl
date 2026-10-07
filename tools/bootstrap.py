#!/usr/bin/env python3
"""Fetch the pinned public dependencies for a Linux/MSVC cross-build."""
import argparse
import hashlib
from pathlib import Path
import subprocess
import tarfile
import urllib.request

root=Path(__file__).resolve().parents[1]
cache=root/'.tools'
commit='6eb3d9bc3346ba01f7191612b5812e0e47a3bbe1'

def run(*args):
    subprocess.run([str(a) for a in args],check=True,cwd=root)

def archive(name,url,digest,directory):
    path=cache/name
    if not path.exists():
        print('Downloading',name,flush=True)
        temporary=path.with_suffix('.partial')
        urllib.request.urlretrieve(url,temporary)
        temporary.replace(path)
    if hashlib.sha256(path.read_bytes()).hexdigest()!=digest:
        raise RuntimeError('Checksum mismatch: '+str(path))
    if not (cache/directory).exists():
        with tarfile.open(path) as tar:
            tar.extractall(cache,filter='data')

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--accept-microsoft-license',action='store_true',help='Accept the CRT/SDK license displayed by xwin')
    a=p.parse_args()
    cache.mkdir(exist_ok=True)
    ue=cache/'ue4ss'
    if not ue.exists():
        run('git','clone','--filter=blob:none','https://github.com/UE4SS-RE/RE-UE4SS.git',ue)
        run('git','-C',ue,'checkout',commit)
    if subprocess.check_output(['git','-C',str(ue),'rev-parse','HEAD'],text=True).strip()!=commit:
        raise RuntimeError('UE4SS checkout must match '+commit)
    archive('fmt.tar.gz','https://codeload.github.com/fmtlib/fmt/tar.gz/refs/tags/10.2.1',
            '1250e4cc58bf06ee631567523f48848dc4596133e163f02615c97f78bab6c811','fmt-10.2.1')
    archive('minhook.tar.gz','https://codeload.github.com/TsudaKageyu/minhook/tar.gz/refs/tags/v1.3.4',
            '1aebeae4ca898330c507860acc2fca2eb335fe446a3a2b8444c3bf8b2660a14e','minhook-1.3.4')
    archive('xwin.tar.gz','https://github.com/Jake-Shadle/xwin/releases/download/0.10.0/xwin-0.10.0-x86_64-unknown-linux-musl.tar.gz',
            'd870eb4b2f390878af6da1ccd3cf321d22fcb72720984853b4be732ae597fc88','xwin-0.10.0-x86_64-unknown-linux-musl')
    if not (cache/'sdk/crt/include').exists():
        if not a.accept_microsoft_license:
            p.error('Review the Microsoft CRT/SDK license and rerun with --accept-microsoft-license, or provide .tools/sdk yourself')
        run(cache/'xwin-0.10.0-x86_64-unknown-linux-musl/xwin','--accept-license',
            '--crt-version','14.44','--sdk-version','10.0.26100',
            '--cache-dir',cache/'.xwin-cache','splat','--output',cache/'sdk')
    print('Public dependencies ready. Clang, llvm-dlltool and lld-link 18 must be installed separately.')

if __name__=='__main__':
    main()
