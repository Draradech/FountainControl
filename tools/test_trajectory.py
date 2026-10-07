#!/usr/bin/env python3
"""Compare C++ solver with the game's original machine code and physical bounds."""
from pathlib import Path
import ctypes
import math
import random
import subprocess
import sys

root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root.parent/'research'))
from verify_fountain_range import NativePredicate
build=root/'build'
build.mkdir(exist_ok=True)
subprocess.run(['clang++','-std=c++20','-O2','-shared','-fPIC','-I',str(root/'src'),str(root/'tests/trajectory_bridge.cpp'),'-o',str(build/'trajectory.so')],check=True)
fn=ctypes.CDLL(str(build/'trajectory.so')).flight
fn.argtypes=[ctypes.c_double]*5+[ctypes.c_int,ctypes.POINTER(ctypes.c_double)]
native=NativePredicate()
rng=random.Random(3669570)
points=[(rng.uniform(-1400,1400),rng.uniform(-1400,1400),rng.uniform(-1100,400)) for _ in range(10000)]
points += [(r,0,z) for r in (0,1e-12,1,50,500,510.204081632653,1335,1336) for z in (-1000.01,-1000,-500,-1,0,1,100,249.999,250,250.001)]
out=(ctypes.c_double*3)()
repaired=0
for x,y,z in points:
    vanilla=native.accepts(x,y,z)
    assert bool(fn(x,y,z,1000,-1,0,out))==vanilla,(x,y,z,'vanilla mismatch')
    accepted=bool(fn(x,y,z,1000,-1,1,out))
    assert not vanilla or accepted,(x,y,z,'lost vanilla target')
    r=math.hypot(x,y)
    expected=z>=-1000 and z<250 and r<=250*((700+math.sqrt(700**2-1960*z))/980)**2
    assert accepted==expected,(x,y,z,'envelope mismatch')
    if accepted:
        t,v,new=out
        repaired += bool(new)
        assert t>0 and -1e-7<=v<=700+1e-7 and v-980*t<1e-7
        assert 2*r/t**2<=500+1e-7
        assert abs(v*t-490*t*t-z)<1e-7
    for drop,horizontal in ((0,0),(500,250),(1000,500)):
        limited=bool(fn(x,y,z,drop,horizontal,1,out))
        assert limited==(accepted and z>=-drop and r<=horizontal)
print(f'{len(points)} points: native agreement, envelope, trajectory and distance caps passed; {repaired} repaired trajectories')
