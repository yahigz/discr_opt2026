#!/usr/bin/env python3

import json
import math
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

	n = int(lines[0].split()[0])
	if len(lines) < n + 1:
		raise ValueError(f"invalid number of lines: expected at least {n + 1}")

	points = []
	for i in range(1, n + 1):
		parts = lines[i].split()
		if len(parts) != 2:
			raise ValueError(f"invalid point format at line {i + 1}")
		x = float(parts[0])
		y = float(parts[1])
		points.append((x, y))

	return n, points


def run_solution(executable: Path, test_data: str):
	try:
		result = subprocess.run(
			[str(executable.resolve())],
			input=test_data,
			capture_output=True,
			text=True,
			timeout=1200,
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
		reported_length = float(tokens[0])
	except ValueError:
		return None, None, f"Invalid tour length: {tokens[0]}", stderr_msg

	order = []
	for token in tokens[1:]:
		try:
			order.append(int(token))
		except ValueError:
			return None, None, f"Invalid vertex token: {token}", stderr_msg

	return reported_length, order, None, stderr_msg


def normalize_order(n, order):
	if len(order) != n:
		return None, f"Expected {n} vertices in the tour, got {len(order)}"

	if sorted(order) == list(range(n)):
		return order, None

	if sorted(order) == list(range(1, n + 1)):
		return [vertex - 1 for vertex in order], None

	return None, "Tour must be a permutation of 0..n-1 or 1..n"


def tour_length(points, order):
	total = 0.0
	n = len(order)
	for i in range(n):
		x1, y1 = points[order[i]]
		x2, y2 = points[order[(i + 1) % n]]
		total += math.hypot(x2 - x1, y2 - y1)
	return total


def validate_solution(n, points, reported_length, order):
	normalized_order, error = normalize_order(n, order)
	if error:
		return False, error, None

	actual_length = tour_length(points, normalized_order)
	tolerance = 1e-6 * max(1.0, actual_length)
	if abs(reported_length - actual_length) > tolerance:
		return (
			False,
			f"Tour length mismatch: reported {reported_length}, actual {actual_length}",
			None,
		)

	return True, "Valid", actual_length


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
		parts.append(f"need <= {threshold_map[3]} for 3")
	if 5 in threshold_map and earned_points < 5:
		parts.append(f"need <= {threshold_map[5]} for 5")

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
	print("Running TSP checker...")
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
				n, points_data = parse_test(test_path)
			except Exception as error:
				message = f"ERROR: failed to parse test ({error})"
				points = 0
				reported_value = None
				print(message)
			else:
				reported_value, order, error, stderr_msg = run_solution(executable, test_data)
				if stderr_msg:
					print(f"\n[log] {test_name}: {stderr_msg}")

				if error:
					message = f"ERROR: {error}"
					points = 0
					reported_value = None
					print(message)
				else:
					valid, details, actual_length = validate_solution(
						n, points_data, reported_value, order
					)

					if not valid:
						message = f"ERROR: {details}"
						points = 0
						print(message)
					else:
						points = determine_points(actual_length, thresholds)
						total_points += points
						unmet_info = format_unmet_thresholds(points, threshold_map)
						message = (
							f"OK length={actual_length:.10f}, points={points}/{max_for_test}{unmet_info}"
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
		status = "PASS" if result["points"] > 0 else "FAIL"
		value_str = f"{result['value']:.10f}" if result["value"] is not None else "ERROR"
		unmet_info = format_unmet_thresholds(result["points"], result["threshold_map"])
		print(
			f"{status:4} {result['test']:20} | Length: {value_str:14} | "
			f"Points: {result['points']}/{result['max_points']}{unmet_info}"
		)

	print("=" * 72)
	print(f"TOTAL POINTS: {total_points}/{max_total_points}")
	print("=" * 72)

	return 0


if __name__ == "__main__":
	raise SystemExit(main())
