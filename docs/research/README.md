# Archived fountain research

All reports under `1.0.4963/` refer to the executable fingerprint in
[reference-build.json](1.0.4963/reference-build.json). They establish native
geometry/defaults, not general compatibility or actual delivery to every
receiver type. Current implementation and decisions are in
[maintaining.md](../maintaining.md), [native.md](../native.md) and source.

## Why the original centre is excluded

For upright pattern 2, let r be XY emitter-to-catch distance and z its signed
height, in cm. Horizontal motion has acceleration 500, so t=sqrt(2r/500).
The native required vertical velocity is v=z/t+490t. It accepts only
0<=v<=700, descending arrival v-980t<0, and z>=-1000. At r=0 it divides by
zero and does not accept the vertical axis. Near the axis, below-emitter points
require negative initial velocity, or above-emitter points arrive while still
ascending. Those constraints form the central hole; it is not an arbitrary
radius setting or a constant cylinder.

Equivalent vertical cross-section constraints (rounding aside):

```text
r > 0; z >= -1000
z >= -1.96*r
z <   1.96*r
z <= 700*sqrt(r/250) - 1.96*r
```

For -1000<=z<250, the inner radius is |z|/1.96 and outer radius is
250*((700+sqrt(700²-1960z))/980)². Rotate the section around Z to obtain the
volume. The getter at emission height is 2*500*700²/980²=510.2040816 cm,
not the earlier coincidental 2*500²/980 expression. SmartCannonMaxDistance
(500 by default) is not read by this predicate; the included report checks
1/500/10000 without changing sampled acceptance. This does not prove complete
absence of every other possible consumer of that property.

FountainControl retains valid native t/v, otherwise increases t so the flight
fills the hole while staying in the outer envelope. Near its top the retained
envelope has a flat disk just below z=250 cm (the exact z=250 plane is rejected).
A conventional projectile model with a bound on total launch speed would give
a parabola and shrink some native reach, so the user rejected that redesign.
See src/trajectory.hpp for endpoint/duration handling, including same-point
and directly vertical throws; the graph depicts eligibility, not every path.

## MovePattern meanings

These labels are inferred from native code; the reflected property is int32,
not a documented enum. It is unrelated to EBeltMotionType.

| Value | Geometry and animation |
|---:|---|
| 0 | No getter/indicator model (radius zero); straight visual interpolation, also fallback for unknown modes. |
| 1 | Constant horizontal speed, solve vertical launch speed, gravity 980. Specific launcher Blueprint assignments were not queried; cannon/catapult was a user hypothesis. |
| 2 | Constant horizontal acceleration, solve vertical launch speed, gravity 980. Upright fountain. |
| 3 | Fixed initial vertical-motion parameter; solve horizontal acceleration, gravity 490 in product animation. Hanging fountain. |

Modes 1/2 constrain initial vertical velocity by MinVerticalSpeed/MaxVerticalSpeed
and require descending arrival, with the 1000 cm general drop cutoff. In mode 3,
HorizontalSpeed means vertical velocity and min/max constrain horizontal
acceleration. Changing the integer changes the interpretation of all fields.
Unsupported -1/0/4/99 modes fall through the shared reach predicate after its
general cutoff and accept (5000,0,300) with the tested defaults, but getter=0
and fallback straight animation. They are not coherent additional range models.
CalculateEmitPosition only implements mode 1; it returns a zero vector for other
modes, including supported 2/3. Use the facility world-emitter transform instead.

## Hanging fountain (outside mod scope)

Actual loaded templates, preserved in hanging-fountain-runtime.txt:

| Property | Upright | Hanging |
|---|---:|---:|
| Indicator pattern | 2 | 3 |
| Indicator min/max | 0 / 700 | 0 / 800 |
| Indicator HorizontalSpeed | 500 | -100 |
| Getter radius (cm) | 510.2040816 | 366.2279030 |
| Facility local emitter Z (cm) | +60 | -55 |
| Facility HorizontalSpeed | 500 | 360 |
| Facility SmartCannonMaxDistance | 500 | 500 |
| Facility UseDrop | false | true |

For hanging defaults let d=-z>0 be drop. Native reach requires 0<d<=750,
t=(-100+sqrt(10000+980d))/490, and 0<=2r/t²<=800. Each slice is a solid disk
from the centre to r=400t²; no central exclusion exists. Equal height/above
are rejected. At d=750 its outer radius is about 970.218 cm. The hanging getter
uses a fixed reference drop of **320 cm**, giving 366.227903 cm, not maximum
radius. Pattern 3 visual height subtracts 245t²; patterns 1/2 subtract 490t².
Facility HorizontalSpeed=360 is not the indicator parameter used by this reach
branch. No hanging-instance overrides or feature modifications were required.

## Reproduction

Use the owned exact EXE and tools described in [artifacts.md](../artifacts.md).
Original/hanging verifier scripts now use the fingerprint-gated shared native
oracle; both reproduce the historical 10,000-point reports without the parent
Python environment. Annotated disassembly, including visual branch and save
property gates, can be regenerated with dump_reference.py rather than committing
copied machine-code listings. Fresh template/instance JSON exports and .usmap
creation are also documented there. Old JSON pointers and names are breadcrumbs,
not addresses for a current live process.
