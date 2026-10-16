"""Verify the hanging range using runtime-read Blueprint defaults and native code."""
import json
import math
import random
from pathlib import Path

import argparse
from native_predicate import NativePredicate


def radius_for_drop(drop):
    t = (-100 + math.sqrt(10000 + 980*drop))/490
    return 400*t*t


def equation(x, y, z):
    drop = -z
    if not 0 < drop <= 750:
        return False
    t = (-100 + math.sqrt(10000 + 980*drop))/490
    required = 2*math.hypot(x, y)/(t*t)
    return 0 <= required <= 800


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('runtime/research/hanging-fountain-range-verification.json'))
    args = parser.parse_args()
    native = NativePredicate(pattern=3, min_speed=0, max_speed=800, speed=-100)
    rng = random.Random(3669573)
    points = [(rng.uniform(-1100, 1100), rng.uniform(-1100, 1100), rng.uniform(-800, 100)) for _ in range(10000)]
    mismatches = []
    for p in points:
        actual = native.accepts(*p)
        if actual != equation(*p):
            mismatches.append((p, actual))
    slices = []
    for drop in (25, 50, 100, 200, 320, 400, 750):
        radius = radius_for_drop(drop)
        slices.append({'drop': drop, 'radius': radius, 'radius_cubes': radius/25,
                       'centre_accepted': native.accepts(0, 0, -drop),
                       'inside_accepted': native.accepts(radius-.01, 0, -drop),
                       'outside_accepted': native.accepts(radius+.01, 0, -drop)})
    report = {'native_entry': hex(native.image.entry), 'blueprint': 'BP_SmartCannonCeiling',
              'defaults_source': 'Live component defaults for '+native.image.version,
              'pattern': 3, 'min_speed': 0, 'max_speed': 800, 'speed': -100,
              'random_points': len(points), 'seed': 3669573, 'equation_mismatches': mismatches,
              'slices': slices, 'equal_height_accepted': native.accepts(0,0,0),
              'above_accepted': native.accepts(0,0,1), 'beyond_drop_cap_accepted': native.accepts(0,0,-750.01),
              'max_distance_ignored': [native.accepts(400,0,-400,max_distance=d) for d in (1,500,10000)]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))
    assert not mismatches
    assert all(s['centre_accepted'] and s['inside_accepted'] and not s['outside_accepted'] for s in slices)
    assert not report['equal_height_accepted'] and not report['above_accepted'] and not report['beyond_drop_cap_accepted']
    assert all(report['max_distance_ignored'])
