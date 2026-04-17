#!/usr/bin/env python3

import json
import math
import subprocess
import sys
from pathlib import Path


def load_config(config_path: Path):
    with config_path.open("r", encoding="utf-8") as file:
        return json.load(file)


def parse_instance(test_path: Path):
    tokens = test_path.read_text(encoding="utf-8").strip().split()
    if not tokens:
        raise ValueError("empty test file")

    it = iter(tokens)
    n = int(next(it))
    m = int(next(it))

    facilities = []
    for _ in range(n):
        setup_cost = float(next(it))
        capacity = int(float(next(it)))
        x = float(next(it))
        y = float(next(it))
        facilities.append((setup_cost, capacity, x, y))

    customers = []
    for _ in range(m):
        demand = int(float(next(it)))
        x = float(next(it))
        y = float(next(it))
        customers.append((demand, x, y))

    return n, m, facilities, customers


def run_solution(executable: Path, test_data: str, timeout_sec: int = 300):
    try:
        result = subprocess.run(
            [str(executable.resolve())],
            input=test_data,
            capture_output=True,
            text=True,
            timeout=timeout_sec,
            check=False,
        )
    except subprocess.TimeoutExpired:
        return None, None, None, "Timeout", ""
    except Exception as error:
        return None, None, None, str(error), ""

    stderr_msg = result.stderr.strip()
    if result.returncode != 0:
        message = stderr_msg or f"non-zero exit code {result.returncode}"
        return None, None, None, f"Runtime error: {message}", stderr_msg

    lines = [line.strip() for line in result.stdout.strip().splitlines() if line.strip()]
    if len(lines) < 3:
        return None, None, None, "Invalid output: expected 3 non-empty lines", stderr_msg

    first_tokens = lines[0].split()
    if not first_tokens:
        return None, None, None, "Invalid output: empty first line", stderr_msg

    try:
        reported_obj = float(first_tokens[0])
    except ValueError:
        return None, None, None, f"Invalid objective token: {first_tokens[0]}", stderr_msg

    raw_open = []
    if lines[1]:
        for token in lines[1].split():
            try:
                raw_open.append(int(token))
            except ValueError:
                return None, None, None, f"Invalid opened facility token: {token}", stderr_msg

    raw_assignment = []
    for line in lines[2:]:
        for token in line.split():
            try:
                raw_assignment.append(int(token))
            except ValueError:
                return None, None, None, f"Invalid assignment token: {token}", stderr_msg

    return reported_obj, raw_open, raw_assignment, None, stderr_msg


def normalize_assignment_1_based(raw_assignment, n, m):
    if len(raw_assignment) != m:
        return None, f"Expected {m} assignments, got {len(raw_assignment)}"

    if not all(1 <= x <= n for x in raw_assignment):
        return None, "Assignments must use 1-based indices 1..n"

    return [x - 1 for x in raw_assignment], None


def normalize_open_1_based(raw_open, n):
    if not raw_open:
        return [], None

    if not all(1 <= x <= n for x in raw_open):
        return None, "Opened facilities must use 1-based indices 1..n"

    return [x - 1 for x in raw_open], None


def evaluate_solution(n, m, facilities, customers, assignment, opened_declared):
    remaining = [cap for _, cap, _, _ in facilities]
    used = [False] * n
    objective = 0.0

    for c in range(m):
        f = assignment[c]
        if f < 0 or f >= n:
            return False, f"Invalid facility index for customer {c}: {f}", None

        demand, cx, cy = customers[c]
        remaining[f] -= demand
        if remaining[f] < 0:
            return False, f"Capacity violated at facility {f}", None

        _, _, fx, fy = facilities[f]
        objective += math.hypot(fx - cx, fy - cy)
        used[f] = True

    used_set = {i for i, v in enumerate(used) if v}
    declared_set = set(opened_declared)

    for f in declared_set:
        if f < 0 or f >= n:
            return False, f"Invalid opened facility index: {f}", None

    if used_set != declared_set:
        return False, "Opened facilities line is inconsistent with assignments", None

    for f in used_set:
        objective += facilities[f][0]

    return True, "Valid", objective


def get_thresholds(test_entry):
    thresholds = []
    for key, value in test_entry.items():
        if key == "path":
            continue
        if key.lstrip("-").isdigit():
            thresholds.append((int(key), float(value)))

    thresholds.sort(key=lambda x: x[0])
    return thresholds


def determine_points(value, thresholds):
    earned = 0
    for points, boundary in thresholds:
        if value <= boundary:
            earned = max(earned, points)
    return earned


def format_unmet_thresholds(earned_points, threshold_map):
    parts = []
    if 3 in threshold_map and earned_points < 3:
        parts.append(f"need <= {threshold_map[3]:.0f} for 3")
    if 5 in threshold_map and earned_points < 5:
        parts.append(f"need <= {threshold_map[5]:.0f} for 5")

    return f" [{', '.join(parts)}]" if parts else ""


def next_threshold_info(value, earned_points, thresholds):
    candidates = [(p, b) for p, b in thresholds if p > earned_points]
    if not candidates:
        return "-", "-"

    next_points, boundary = min(candidates, key=lambda x: x[0])
    gap = value - boundary
    if gap <= 0:
        return f"<= {boundary:.0f} ({next_points})", "0"
    return f"<= {boundary:.0f} ({next_points})", f"{gap:.0f}"


def print_results_table(results):
    headers = [
        "test",
        "objective",
        "score",
        "next threshold",
        "gap",
        "status",
    ]

    rows = []
    for result in results:
        value_str = f"{result['value']:.0f}" if result["value"] is not None else "ERROR"
        score_str = f"{result['points']}/{result['max_points']}"
        status = "PASS" if result["points"] > 0 else "FAIL"
        rows.append([
            result["test"],
            value_str,
            score_str,
            result["next_threshold"],
            result["gap_to_next"],
            status,
        ])

    widths = [len(h) for h in headers]
    for row in rows:
        for i, cell in enumerate(row):
            widths[i] = max(widths[i], len(str(cell)))

    def fmt(row):
        return " | ".join(str(cell).ljust(widths[i]) for i, cell in enumerate(row))

    sep = "-+-".join("-" * w for w in widths)
    print(fmt(headers))
    print(sep)
    for row in rows:
        print(fmt(row))


def main():
    root = Path(".")
    config_path = root / "config.json"
    data_dir = root / "data"
    executable = root / "a.out"
    output_dir = root / "output"
    output_dir.mkdir(exist_ok=True)

    if not executable.exists():
        print("ERROR: executable ./a.out not found. Compile first.", file=sys.stderr)
        return 1

    tests = load_config(config_path)

    total_points = 0
    max_total_points = 0
    results = []

    print("=" * 88)
    print("Running CFLP checker...")
    print("=" * 88)

    for position, test in enumerate(tests, start=1):
        test_name = test["path"]
        test_path = data_dir / test_name
        thresholds = get_thresholds(test)
        threshold_map = {points: boundary for points, boundary in thresholds}
        max_for_test = max((points for points, _ in thresholds), default=0)
        max_total_points += max_for_test

        print(f"\n[{position}/{len(tests)}] Testing {test_name}...", end=" ", flush=True)

        points = 0
        reported_value = None

        if not test_path.exists():
            message = "ERROR: test file not found"
            print(message)
        else:
            try:
                test_data = test_path.read_text(encoding="utf-8")
                n, m, facilities, customers = parse_instance(test_path)
            except Exception as error:
                message = f"ERROR: failed to parse test ({error})"
                print(message)
            else:
                reported_obj, raw_open, raw_assignment, error, stderr_msg = run_solution(executable, test_data)
                if stderr_msg:
                    print(f"\n[log] {test_name}: {stderr_msg}")

                if error:
                    message = f"ERROR: {error}"
                    print(message)
                else:
                    assignment, assign_err = normalize_assignment_1_based(raw_assignment, n, m)
                    opened_declared, open_err = normalize_open_1_based(raw_open, n)
                    if assign_err:
                        message = f"ERROR: {assign_err}"
                        print(message)
                    elif open_err:
                        message = f"ERROR: {open_err}"
                        print(message)
                    else:
                        valid, details, actual_obj = evaluate_solution(
                            n,
                            m,
                            facilities,
                            customers,
                            assignment,
                            opened_declared,
                        )
                        if not valid:
                            message = f"ERROR: {details}"
                            print(message)
                        else:
                            tolerance = 1e-6 * max(1.0, actual_obj)
                            if abs(reported_obj - actual_obj) > tolerance:
                                message = (
                                    f"ERROR: objective mismatch: reported {reported_obj}, actual {actual_obj}"
                                )
                                print(message)
                            else:
                                points = determine_points(actual_obj, thresholds)
                                total_points += points
                                next_thr, gap = next_threshold_info(actual_obj, points, thresholds)
                                unmet_info = format_unmet_thresholds(points, threshold_map)
                                message = (
                                    f"OK objective={actual_obj:.6f}, points={points}/{max_for_test}{unmet_info}"
                                )
                                print(message)
                                reported_value = actual_obj

        if reported_value is None:
            next_thr = "-"
            gap = "-"
        else:
            next_thr, gap = next_threshold_info(reported_value, points, thresholds)

        log_path = output_dir / f"{test_name}.log"
        log_path.write_text(message + "\n", encoding="utf-8")

        results.append(
            {
                "test": test_name,
                "value": reported_value,
                "points": points,
                "max_points": max_for_test,
                "threshold_map": threshold_map,
                "next_threshold": next_thr,
                "gap_to_next": gap,
            }
        )

    print("\n" + "=" * 88)
    print("SUMMARY")
    print("=" * 88)
    print_results_table(results)
    print("=" * 88)
    print(f"TOTAL POINTS: {total_points}/{max_total_points}")
    print("=" * 88)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
