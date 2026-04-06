# HW4 wrapper (TSP)

## Files

- `run.sh` - compiles `solution.cpp` and runs the checker
- `solution.cpp` - heuristic TSP solver
- `checker.py` - validates a Hamiltonian cycle and calculates points
- `config.json` - list of evaluated tests and thresholds
- `data/` - input tests
- `output/` - per-test logs created by the checker

## Expected solver output format

The solver should print:

1. Total tour length as the first number
2. Then exactly `n` vertex indices in the visit order

Example for `n = 4`:

```text
10.5
1 3 2 4
```

The checker accepts both `0..n-1` and `1..n` indexing. The tour is treated as cyclic, so the last vertex is connected back to the first one.

Checker verifies:

- exactly `n` vertices in the tour
- all vertices form a permutation
- reported length matches the actual Euclidean tour length within tolerance

## Scoring (minimization)

Smaller tour length is better. For each configured test, points are awarded if the length is at most the threshold:

- `tsp_51_1`: 3 pts for length `<= 482`, 5 pts for length `<= 430`
- `tsp_100_3`: 3 pts for length `<= 23433`, 5 pts for length `<= 20800`
- `tsp_200_2`: 3 pts for length `<= 35985`, 5 pts for length `<= 30000`
- `tsp_574_1`: 3 pts for length `<= 40000`, 5 pts for length `<= 37600`
- `tsp_1889_1`: 3 pts for length `<= 378069`, 5 pts for length `<= 323000`
- `tsp_33810_1`: 3 pts for length `<= 78478868`, 5 pts for length `<= 67700000`

## Run

```bash
chmod +x run.sh
./run.sh
```

This will compile `solution.cpp`, run `checker.py`, and write per-test logs into `output/`.
