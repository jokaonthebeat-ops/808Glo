#!/usr/bin/env python3
"""Compare two probe runs: did the bank actually get more distinct and punchier?

Reports the numbers that matter perceptually, not a single aggregate score - an
aggregate can improve while the worst collisions stay exactly where they were.
"""
import json, sys
sys.path.insert(0, 'tools')
from analyze_probe import normalise, distance

def stats(path):
    rows = json.load(open(path))
    vec, _ = normalise(rows)
    nearest = []
    for i in range(len(rows)):
        d = min(distance(vec[i], vec[j]) for j in range(len(rows)) if j != i)
        nearest.append((d, rows[i]['name'], rows[i]['category']))
    nearest.sort()
    ds = [d for d, _, _ in nearest]
    n = len(ds)
    return {
        'rows': rows, 'nearest': nearest,
        'min': ds[0], 'p10': ds[n // 10], 'median': ds[n // 2], 'p90': ds[(9 * n) // 10],
        'clones': sum(1 for d in ds if d < 0.35),
        'knock': sorted(float(r['knockDb']) for r in rows),
        'peak': sorted(float(r['peakDb']) for r in rows),
    }

a, b = stats(sys.argv[1]), stats(sys.argv[2])
print(f"{'metric':<34}{'BEFORE':>12}{'AFTER':>12}{'':>4}")
def line(label, x, y, better_high=True, fmt='{:.3f}'):
    arrow = '' if abs(y - x) < 1e-9 else ('  better' if ((y > x) == better_high) else '  WORSE')
    print(f"{label:<34}{fmt.format(x):>12}{fmt.format(y):>12}{arrow}")
line('nearest-neighbour  min', a['min'], b['min'])
line('nearest-neighbour  p10', a['p10'], b['p10'])
line('nearest-neighbour  median', a['median'], b['median'])
line('nearest-neighbour  p90', a['p90'], b['p90'])
line('presets with a near-clone', a['clones'], b['clones'], False, '{:.0f}')
k1, k2 = a['knock'], b['knock']
line('knockDb median', k1[len(k1)//2], k2[len(k2)//2], True, '{:.2f}')
line('knockDb p10 (weakest attacks)', k1[len(k1)//10], k2[len(k2)//10], True, '{:.2f}')
p1, p2 = a['peak'], b['peak']
line('peak dBFS median', p1[len(p1)//2], p2[len(p2)//2], True, '{:.2f}')
line('peak dBFS max', p1[-1], p2[-1], False, '{:.2f}')
print("\nworst 8 remaining collisions:")
for d, name, cat in b['nearest'][:8]:
    print(f"  {d:5.3f}  {name:24s} [{cat}]")
