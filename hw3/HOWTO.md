# HW3 wrapper (graph coloring)

## Files

- `run.sh` - compiles selected solver and runs checker
- `solution.cpp` - baseline coloring solver
- `solution_rlf.cpp` - alternative coloring solver with RLF-style heuristic
- `fcns.cpp` - FCNS-based solver with tunable heuristics
- `checker.py` - validates coloring and calculates points
- `config.json` - list of evaluated tests and thresholds
- `data/` - input tests
- `output/` - per-test logs created by checker

## Expected solver output format

For each test, your solver should print:

1. First integer: number of colors used (`k`)
2. Then exactly `n` integers: color of each vertex

Example for `n = 4`:

```text
3
1 2 3 1
```

This means:

- reported `k = 3`
- vertex colors: `[1, 2, 3, 1]`

Checker verifies:

- exactly `n` assigned colors
- non-negative integer color labels
- proper coloring (`color[u] != color[v]` for each edge)
- reported `k` equals number of distinct colors in assignment

Notes:

- both one-line and two-line output layouts are accepted (parser reads all integers)
- color labels may start from `0` or `1`; only proper coloring and number of distinct colors matter

## Scoring (minimization)

Smaller number of colors is better.
For each configured test, points are awarded if `k <= threshold`:

- `gc_50_3`: 3 pts for `k <= 8`, 5 pts for `k <= 6`
- `gc_70_7`: 3 pts for `k <= 20`, 5 pts for `k <= 17`
- `gc_100_5`: 3 pts for `k <= 21`, 5 pts for `k <= 16`
- `gc_250_9`: 3 pts for `k <= 95`, 5 pts for `k <= 78`
- `gc_500_1`: 3 pts for `k <= 18`, 5 pts for `k <= 16`
- `gc_1000_5`: 3 pts for `k <= 124`, 5 pts for `k <= 100`

## Run

```bash
chmod +x run.sh
./run.sh
```

Optional mode arguments:

```bash
./run.sh cpp
./run.sh rlf
./run.sh fcns
```

To test the RLF-based solver:

```bash
./run.sh rlf
```

To test the FCNS solver:

```bash
./run.sh fcns
```

FCNS also accepts a UVERTEX heuristic argument that is forwarded to the solver:

```bash
./run.sh fcns brelaz
./run.sh fcns nonsingleton
```

Notes:

- `brelaz` uses the Brélaz-style UVERTEX rule.
- `nonsingleton` uses the weaker nonsingleton UVERTEX rule.
- If no second argument is provided, the solver defaults to `brelaz`.