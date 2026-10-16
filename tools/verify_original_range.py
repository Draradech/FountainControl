"""Execute the game's native reach predicate offline; never runs or edits the game.

Only the world-transform helper and failure-message FName constructor are stubbed.
The distance arithmetic, pattern dispatch, speed limits and arrival test are the
original x64 instructions from the installed executable.
"""
import json
import math
import random
import struct
from pathlib import Path

import argparse
from native_predicate import NativePredicate


def equation(x, y, z):
    r = math.hypot(x, y)
    if r == 0 or z < -1000:
        return False
    t = math.sqrt(2*r / 500)
    v = z/t + 490*t
    return 0 <= v <= 700 and v - 980*t < 0


def horizontal_interval(z):
    if z < -1000 or z >= 250:
        return None
    outer_t = (700 + math.sqrt(700**2 - 1960*z)) / 980
    return abs(z) / 1.96, 250*outer_t**2


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('runtime/research/fountain-range-verification.json'))
    args = parser.parse_args()
    native = NativePredicate()
    rng = random.Random(3669570)
    points = [(rng.uniform(-1300, 1300), rng.uniform(-1300, 1300), rng.uniform(-1100, 400)) for _ in range(10000)]
    mismatches = []
    for p in points:
        actual = native.accepts(*p)
        if actual != equation(*p):
            mismatches.append((p, actual))
    cases = [(0, 0, 0), (510, 0, 0), (511, 0, 0), (700, 0, -300),
             (50, 0, 100), (52, 0, 100), (200, 0, 200), (128, 0, 251),
             (800, 0, -1000), (800, 0, -1000.01),
             (1335, 0, -1000), (1336, 0, -1000)]
    report = {'native_entry': hex(native.image.entry), 'seed': 3669570,
              'random_points': len(points), 'equation_mismatches': mismatches,
              'cases': [{'target_relative_to_emitter': p, 'accepted': native.accepts(*p)} for p in cases],
              'max_distance_ignored': [native.accepts(510, 0, 0, max_distance=d) for d in (1, 500, 10000)],
              'rotation_invariance': [native.accepts(400*math.cos(a), 400*math.sin(a), 50) for a in (0, .4, 1.7, 3.14)],
              'translation_invariance': native.accepts(1400, 2000, 3050, emitter=(1000, 2000, 3000)),
              'horizontal_slices_cm': [{'height': z, 'radius_interval': horizontal_interval(z)} for z in (-1000, -400, -100, 0, 50, 100, 200, 249, 250)]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))
    assert not mismatches
    assert all(report['max_distance_ignored'])
    assert all(report['rotation_invariance'])
    assert report['translation_invariance']
