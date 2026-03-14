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
		raise ValueError("invalid header: expected 'n m'")

	n = int(first[0])
	m = int(first[1])

	if len(lines) < m + 1:
		raise ValueError(f"invalid number of lines: expected at least {m + 1}")

	edges = []
	for i in range(1, m + 1):
		parts = lines[i].split()
		if len(parts) != 2:
			raise ValueError(f"invalid edge format at line {i + 1}")
		u = int(parts[0])
		v = int(parts[1])
		edges.append((u, v))

	return n, m, edges


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
		reported_colors = int(tokens[0])
	except ValueError:
		return None, None, f"Invalid objective value: {tokens[0]}", stderr_msg

	assignment = []
	for token in tokens[1:]:
		try:
			assignment.append(int(token))
		except ValueError:
			return None, None, f"Invalid color token: {token}", stderr_msg

	return reported_colors, assignment, None, stderr_msg


def normalize_edges(n, edges):
	if all(0 <= u < n and 0 <= v < n for u, v in edges):
		return edges, None

	if all(1 <= u <= n and 1 <= v <= n for u, v in edges):
		return [(u - 1, v - 1) for u, v in edges], None

	return None, "Test contains edge endpoint out of range"


def validate_solution(n, edges, reported_colors, assignment):
	if len(assignment) != n:
		return False, f"Expected {n} vertex colors, got {len(assignment)}", None

	if reported_colors < 0:
		return False, "Reported color count must be non-negative", None

	if any(color < 0 for color in assignment):
		return False, "Colors must be non-negative integers", None

	normalized_edges, edges_error = normalize_edges(n, edges)
	if edges_error:
		return False, edges_error, None

	for u, v in normalized_edges:
		if assignment[u] == assignment[v]:
			return False, f"Invalid coloring: edge ({u}, {v}) has same color", None

	used_colors = len(set(assignment))
	if used_colors != reported_colors:
		return (
			False,
			f"Color count mismatch: reported {reported_colors}, used {used_colors}",
			None,
		)

	return True, "Valid", used_colors


def get_thresholds(test_entry):
	thresholds = []
	for key, value in test_entry.items():
		if key == "path":
			continue
		if key.lstrip("-").isdigit():
			thresholds.append((int(key), int(value)))

	thresholds.sort(key=lambda x: x[0])
	return thresholds


def determine_points(reported_colors, thresholds):
	earned = 0
	for points, boundary in thresholds:
		if reported_colors <= boundary:
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
	print("Running graph coloring checker...")
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
			reported_colors = None
			print(message)
		else:
			try:
				test_data = test_path.read_text(encoding="utf-8")
				n, _, edges = parse_test(test_path)
			except Exception as error:
				message = f"ERROR: failed to parse test ({error})"
				points = 0
				reported_colors = None
				print(message)
			else:
				reported_colors, assignment, error, stderr_msg = run_solution(
					executable, test_data
				)
				if stderr_msg:
					print(f"\n[log] {test_name}: {stderr_msg}")

				if error:
					message = f"ERROR: {error}"
					points = 0
					reported_colors = None
					print(message)
				else:
					valid, details, used_colors = validate_solution(
						n, edges, reported_colors, assignment
					)

					if not valid:
						message = f"ERROR: {details}"
						points = 0
						print(message)
					else:
						points = determine_points(reported_colors, thresholds)
						total_points += points
						unmet_info = format_unmet_thresholds(points, threshold_map)
						message = (
							f"OK colors={used_colors}, points={points}/{max_for_test}{unmet_info}"
						)
						print(message)

		log_path = output_dir / f"{test_name}.log"
		log_path.write_text(message + "\n", encoding="utf-8")

		results.append(
			{
				"test": test_name,
				"colors": reported_colors,
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
		colors_str = str(result["colors"]) if result["colors"] is not None else "ERROR"
		unmet_info = format_unmet_thresholds(result["points"], result["threshold_map"])
		print(
			f"{status:4} {result['test']:20} | Colors: {colors_str:10} | "
			f"Points: {result['points']}/{result['max_points']}{unmet_info}"
		)

	print("=" * 72)
	print(f"TOTAL POINTS: {total_points}/{max_total_points}")
	print("=" * 72)

	return 0


if __name__ == "__main__":
	raise SystemExit(main())
