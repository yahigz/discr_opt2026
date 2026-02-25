from ortools.linear_solver import pywraplp
import random
import sys

random.seed(52)


def generate_solution_attempt(m, probs, costs, sets, n):
    chosen = []
    covered = set()
    
    for i in range(m):
        if probs[i] >= 1.0 - 1e-9:
            chosen.append(i)
            covered |= sets[i]
    
    fractional = []
    for i in range(m):
        if 1e-9 < probs[i] < 1.0 - 1e-9:
            rand_val = random.uniform(0, probs[i])
            fractional.append((rand_val, i))
    
    fractional.sort(reverse=True)
    
    for _, i in fractional:
        if covered == set(range(n)):
            break
        if sets[i] - covered:
            chosen.append(i)
            covered |= sets[i]
    
    total_cost = sum(costs[i] for i in chosen)
    return chosen, total_cost


def solve_set_cover(n, m, costs, sets):
    solver = pywraplp.Solver.CreateSolver('GLOP')

    indicators = [solver.NumVar(0, 1, f'indicator_{i}') for i in range(m)]

    for element in range(n):
        constraint = solver.Constraint(1, 1)
        for i in range(m):
            if element in sets[i]:
                constraint.SetCoefficient(indicators[i], 1)

    objective = solver.Objective()
    for i in range(m):
        objective.SetCoefficient(indicators[i], costs[i])
    objective.SetMinimization()

    status = solver.Solve()
    if status != pywraplp.Solver.OPTIMAL:
        print(sum(costs))
        print(' '.join(str(i + 1) for i in range(m)))
        return

    probs = [indicators[i].solution_value() for i in range(m)]
    
    best_chosen = None
    best_cost = float('inf')
    
    for _ in range(10000):
        chosen, cost = generate_solution_attempt(m, probs, costs, sets, n)
        if cost < best_cost:
            best_cost = cost
            best_chosen = chosen
    
    if best_chosen is None:
        print(sum(costs))
        print(' '.join(str(i + 1) for i in range(m)))
    else:
        print(best_cost)
        print(' '.join(str(i + 1) for i in best_chosen))

if __name__ == "__main__":
    data = sys.stdin.read().strip().splitlines()
    if not data:
        sys.exit(0)

    n, m = map(int, data[0].split())
    costs = [0] * m
    sets = []

    for i in range(m):
        parts = list(map(int, data[i + 1].split()))
        if not parts:
            costs[i] = 0
            sets.append(set())
            continue
        costs[i] = parts[0]
        sets.append(set(parts[1:]))

    solve_set_cover(n, m, costs, sets)
