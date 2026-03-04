#!/usr/bin/env python3

import json
import subprocess
import sys
from pathlib import Path


def load_config(config_path: Path):
    with config_path.open("r", encoding="utf-8") as file:
        return json.load(file)


def parse_test(test_path: Path):
    lines = test_path.read_text(encoding="utf-8").strip().splitlines()
    if not lines:
        raise ValueError("empty test file")

    first = lines[0].split()
    if len(first) != 2:
        raise ValueError("invalid header: expected 'n capacity'")

    n = int(first[0])
    capacity = int(first[1])

    if len(lines) < n + 1:
        raise ValueError(f"invalid number of lines: expected at least {n + 1}")

    items = []
    for i in range(1, n + 1):
        parts = lines[i].split()
        if len(parts) != 2:
            raise ValueError(f"invalid item format at line {i + 1}")
        value = int(parts[0])
        weight = int(parts[1])
        items.append((value, weight))

    return n, capacity, items


def run_solution(executable: Path, test_data: str):
    try:
        result = subprocess.run(
            [str(executable.resolve())],
            input=test_data,
            capture_output=True,
            text=True,
            timeout=120,
            check=False,
        )
    except subprocess.TimeoutExpired:
        return None, None, "Timeout", ""
    except Exception as error:
        return None, None, str(error), ""

    stderr_msg = result.stderr.strip()
    if result.returncode != 0:
        message = stderr_msg or f"non-zero exit code {result.returncode}"
        return None, None, f"Runtime error: {message}", stderr_msg

    tokens = result.stdout.strip().split()
    if not tokens:
        return None, None, "Empty output", stderr_msg

    try:
        reported_value = int(tokens[0])
    except ValueError:
        return None, None, f"Invalid objective value: {tokens[0]}", stderr_msg

    chosen = []
    for token in tokens[1:]:
        try:
            chosen.append(int(token))
        except ValueError:
            return None, None, f"Invalid item index token: {token}", stderr_msg

    return reported_value, chosen, None, stderr_msg


def validate_solution(n, capacity, items, reported_value, chosen_indices):
    if len(set(chosen_indices)) != len(chosen_indices):
        return False, "Duplicate item indices in output"

    candidates = []

    candidates.append(("1-based", lambda x: x - 1))
    candidates.append(("0-based", lambda x: x))

    best_error = "No valid index convention matched"
    for convention, mapper in candidates:
        real_indices = []
        ok = True
        for index in chosen_indices:
            mapped = mapper(index)
            if mapped < 0 or mapped >= n:
                ok = False
                break
            real_indices.append(mapped)

        if not ok:
            continue

        total_weight = sum(items[idx][1] for idx in real_indices)
        total_value = sum(items[idx][0] for idx in real_indices)

        if total_weight > capacity:
            best_error = (
                f"Overweight ({convention}): {total_weight} > capacity {capacity}"
            )
            continue

        if total_value != reported_value:
            best_error = (
                f"Value mismatch ({convention}): reported {reported_value}, real {total_value}"
            )
            continue

        return True, f"Valid ({convention})", total_value, total_weight

    return False, best_error, None, None


def get_thresholds(test_entry):
    thresholds = []
    for key, value in test_entry.items():
        if key == "path":
            continue
        if key.lstrip("-").isdigit():
            thresholds.append((int(key), int(value)))

    thresholds.sort(key=lambda x: x[0])
    return thresholds


def determine_points(reported_value, thresholds):
    earned = 0
    for points, boundary in thresholds:
        if reported_value >= boundary:
            earned = max(earned, points)
    return earned


def format_unmet_thresholds(earned_points, threshold_map):
    parts = []
    if 3 in threshold_map and earned_points < 3:
        parts.append(f"need 3: {threshold_map[3]}")
    if 5 in threshold_map and earned_points < 5:
        parts.append(f"need 5: {threshold_map[5]}")

    return f" [{', '.join(parts)}]" if parts else ""


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

    print("=" * 72)
    print("Running knapsack checker...")
    print("=" * 72)

    for position, test in enumerate(tests, start=1):
        test_name = test["path"]
        test_path = data_dir / test_name
        thresholds = get_thresholds(test)
        threshold_map = {points: boundary for points, boundary in thresholds}
        max_for_test = max((points for points, _ in thresholds), default=0)
        max_total_points += max_for_test

        print(f"\n[{position}/{len(tests)}] Testing {test_name}...", end=" ", flush=True)

        if not test_path.exists():
            message = "ERROR: test file not found"
            points = 0
            reported_value = None
            print(message)
        else:
            try:
                test_data = test_path.read_text(encoding="utf-8")
                n, capacity, items = parse_test(test_path)
            except Exception as error:
                message = f"ERROR: failed to parse test ({error})"
                points = 0
                reported_value = None
                print(message)
            else:
                reported_value, chosen, error, stderr_msg = run_solution(executable, test_data)
                if stderr_msg:
                    print(f"\n[log] {test_name}: {stderr_msg}")

                if error:
                    message = f"ERROR: {error}"
                    points = 0
                    reported_value = None
                    print(message)
                else:
                    valid, details, real_value, real_weight = validate_solution(
                        n, capacity, items, reported_value, chosen
                    )

                    if not valid:
                        message = f"ERROR: {details}"
                        points = 0
                        print(message)
                    else:
                        points = determine_points(reported_value, thresholds)
                        total_points += points
                        unmet_info = format_unmet_thresholds(points, threshold_map)
                        message = (
                            f"OK value={real_value}, weight={real_weight}, points={points}/{max_for_test}{unmet_info}"
                        )
                        print(message)

        log_path = output_dir / f"{test_name}.log"
        log_path.write_text(message + "\n", encoding="utf-8")

        results.append(
            {
                "test": test_name,
                "value": reported_value,
                "points": points,
                "max_points": max_for_test,
                "threshold_map": threshold_map,
            }
        )

    print("\n" + "=" * 72)
    print("SUMMARY")
    print("=" * 72)
    for result in results:
        status = "✓" if result["points"] > 0 else "✗"
        value_str = str(result["value"]) if result["value"] is not None else "ERROR"
        unmet_info = format_unmet_thresholds(result["points"], result["threshold_map"])
        print(
            f"{status} {result['test']:20} | Value: {value_str:10} | "
            f"Points: {result['points']}/{result['max_points']}{unmet_info}"
        )

    print("=" * 72)
    print(f"TOTAL POINTS: {total_points}/{max_total_points}")
    print("=" * 72)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
