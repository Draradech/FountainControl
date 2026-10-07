#!/usr/bin/env python3
"""Render the README's original and corrected upright fountain range comparison.

Documentation tool only; requires NumPy and Matplotlib. Geometry comes from
src/trajectory.hpp and the verified native predicate described in docs/native.md.
"""
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

root = Path(__file__).resolve().parents[1]
plt.rcParams.update({'font.family': 'DejaVu Sans', 'font.size': 11})
fig, axes = plt.subplots(1, 2, figsize=(13, 6.8), sharex=True, sharey=True)
# Height and radius in metres; the native solver uses cm. The outer boundary
# is shared. Vanilla also requires r >= |z|/1.96 (strict above the emitter).
z = np.linspace(-1000, 250, 2400)
outer = 250 * ((700 + np.sqrt(700**2 - 1960*z)) / 980)**2 / 100
inner = np.abs(z) / 1.96 / 100
for ax, original, color, title in zip(
        axes, (True, False), ('#73b5e4', '#77c4ac'),
        ('Original game: central exclusion', 'FountainControl: exclusion removed')):
    y = z / 100
    if original:
        ax.fill_betweenx(y, inner, outer, color=color, alpha=.85)
        ax.fill_betweenx(y, -outer, -inner, color=color, alpha=.85)
        ax.plot(inner, y, color='#42647d', linewidth=1.2)
        ax.plot(-inner, y, color='#42647d', linewidth=1.2)
    else:
        ax.fill_betweenx(y, -outer, outer, color=color, alpha=.85)
        ax.hlines(y[-1], -outer[-1], outer[-1], color='#42647d', linewidth=1.2)
    ax.plot(outer, y, color='#42647d', linewidth=1.2)
    ax.plot(-outer, y, color='#42647d', linewidth=1.2)
    ax.hlines(-10, -outer[0], outer[0], color='#42647d', linewidth=1.2)
    ax.axhline(0, color='#60717c', linestyle='--', linewidth=.8)
    ax.axvline(0, color='#60717c', linestyle='--', linewidth=.8)
    ax.scatter([0], [0], color='#ce533c', s=35, zorder=5)
    ax.set(xlim=(-14,14), ylim=(-10.8,3.2), title=title,
           xlabel='Horizontal offset from emitter (m)')
    ax.set_xticks(np.arange(-12,13,4))
    ax.set_aspect('equal', adjustable='box')
    ax.grid(alpha=.18)
axes[0].set_ylabel('Receiver height relative to emitter (m)')
fig.suptitle('Upright Arcane Fountain: original and corrected range', fontsize=17, y=.97)
fig.text(.5,.095,'Shaded areas show reachable receiver catch points in a vertical cross-section.\n'
         'Rotate each section around its vertical axis to obtain the full range volume.',
         ha='center',fontsize=10)
fig.text(.5,.035,'Red point = fountain emitter (0.60 m above ground)',
         ha='center',fontsize=10)
fig.subplots_adjust(left=.075,right=.985,top=.88,bottom=.20,wspace=.13)
out = root/'docs/images/fountain-range.png'
fig.savefig(out, dpi=160, facecolor='white')
print(out)
