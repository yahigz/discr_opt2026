#!/usr/bin/env python3

import argparse
import json
import math
import shlex
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


ABS_TOLERANCE = 1e-6
REL_TOLERANCE = 1e-6


class CheckerError(Exception):
    pass


@dataclass
class Instance:
    path: Path
    node_count: int
    vehicle_count: int
    capacity: int
    demands: list[int]
    coords: list[tuple[float, float]]


@dataclass
class Threshold:
    score: int
    limit: float


@dataclass
class CheckResult:
    ok: bool
    claimed: float | None
    actual: float | None
    delta: float | None
    errors: list[str]
    route_count: int


def load_instance(path: Path) -> Instance:
    try:
        raw_lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise CheckerError(f"cannot read instance {path}: {exc}") from exc

    lines = [line.strip() for line in raw_lines if line.strip()]
    if not lines:
        raise CheckerError(f"instance {path} is empty")

    header = lines[0].split()
    if len(header) != 3:
        raise CheckerError(
            f"instance {path}: first line must contain exactly 3 integers: n vehicles capacity"
        )

    try:
        node_count, vehicle_count, capacity = (int(token) for token in header)
    except ValueError as exc:
        raise CheckerError(f"instance {path}: header must contain integers") from exc

    if node_count <= 0:
        raise CheckerError(f"instance {path}: node count must be positive")
    if vehicle_count <= 0:
        raise CheckerError(f"instance {path}: vehicle count must be positive")
    if capacity < 0:
        raise CheckerError(f"instance {path}: capacity must be non-negative")

    if len(lines) != node_count + 1:
        raise CheckerError(
            f"instance {path}: expected {node_count} customer/depot lines, found {len(lines) - 1}"
        )

    demands: list[int] = []
    coords: list[tuple[float, float]] = []
    for line_index, line in enumerate(lines[1:], start=2):
        parts = line.split()
        if len(parts) != 3:
            raise CheckerError(
                f"instance {path}:{line_index}: each node line must contain demand x y"
            )
        try:
            demand = int(parts[0])
            x = float(parts[1])
            y = float(parts[2])
        except ValueError as exc:
            raise CheckerError(
                f"instance {path}:{line_index}: failed to parse demand/x/y"
            ) from exc
        if demand < 0:
            raise CheckerError(f"instance {path}:{line_index}: demand must be non-negative")
        demands.append(demand)
        coords.append((x, y))

    return Instance(path, node_count, vehicle_count, capacity, demands, coords)


def parse_solution_lines(raw_lines: list[str]) -> tuple[float, list[list[int]]]:
    lines = [line.strip() for line in raw_lines if line.strip()]
    if not lines:
        raise CheckerError("solver output is empty")

    first_line = lines[0].split()
    if len(first_line) != 1:
        raise CheckerError("the first line of solver output must contain exactly one number")

    try:
        claimed = float(first_line[0])
    except ValueError as exc:
        raise CheckerError("failed to parse the claimed total cost on the first line") from exc

    if not math.isfinite(claimed):
        raise CheckerError("claimed total cost must be finite")

    routes: list[list[int]] = []
    for line_index, line in enumerate(lines[1:], start=2):
        tokens = line.split()
        try:
            route_size = int(tokens[0])
        except ValueError as exc:
            raise CheckerError(
                f"solution line {line_index}: the first token must be the route length"
            ) from exc

        if route_size < 0:
            raise CheckerError(f"solution line {line_index}: route length cannot be negative")

        try:
            route = [int(token) for token in tokens[1:]]
        except ValueError as exc:
            raise CheckerError(
                f"solution line {line_index}: route vertices must be integers"
            ) from exc

        if len(route) != route_size:
            raise CheckerError(
                f"solution line {line_index}: declared route length is {route_size}, "
                f"but {len(route)} vertices were provided"
            )

        routes.append(route)

    return claimed, routes


def parse_solution(path: Path) -> tuple[float, list[list[int]]]:
    try:
        raw_lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise CheckerError(f"cannot read solution {path}: {exc}") from exc
    return parse_solution_lines(raw_lines)


def dist(a: tuple[float, float], b: tuple[float, float]) -> float:
    return math.hypot(a[0] - b[0], a[1] - b[1])


def is_close(a: float, b: float) -> bool:
    return math.isclose(a, b, rel_tol=REL_TOLERANCE, abs_tol=ABS_TOLERANCE)


def normalize_route(route: list[int], depot: int, route_index: int) -> tuple[list[int], list[str]]:
    depot_positions = [idx for idx, vertex in enumerate(route) if vertex == depot]
    if not depot_positions:
        return route, []

    if len(depot_positions) > 1:
        return route, [f"route {route_index}: depot {depot} appears more than once"]

    depot_pos = depot_positions[0]
    if len(route) == 1:
        return [], []

    normalized = route[depot_pos + 1 :] + route[:depot_pos]
    return normalized, []


def check_solution(instance: Instance, claimed: float, routes: list[list[int]]) -> CheckResult:
    errors: list[str] = []
    route_count = len(routes)

    if route_count > instance.vehicle_count:
        errors.append(
            f"used {route_count} routes, but the instance allows at most {instance.vehicle_count}"
        )

    seen = [0] * (instance.node_count + 1)
    depot = 1
    actual = 0.0

    for route_index, raw_route in enumerate(routes, start=1):
        route, route_errors = normalize_route(raw_route, depot, route_index)
        errors.extend(route_errors)
        route_load = 0
        prev = depot
        for pos, vertex in enumerate(route, start=1):
            if vertex < 1 or vertex > instance.node_count:
                errors.append(
                    f"route {route_index}, position {pos}: vertex {vertex} is outside [1, {instance.node_count}]"
                )
                prev = None
                continue

            if seen[vertex]:
                errors.append(
                    f"route {route_index}, position {pos}: vertex {vertex} is visited more than once"
                )
            seen[vertex] += 1

            if vertex != depot:
                route_load += instance.demands[vertex - 1]

            if prev is not None and vertex != depot:
                actual += dist(instance.coords[prev - 1], instance.coords[vertex - 1])
            prev = vertex

        if route:
            last_vertex = route[-1]
            if 1 <= last_vertex <= instance.node_count and last_vertex != depot:
                actual += dist(instance.coords[last_vertex - 1], instance.coords[depot - 1])

        if route_load > instance.capacity:
            errors.append(
                f"route {route_index}: load {route_load} exceeds capacity {instance.capacity}"
            )

    missing = [vertex for vertex in range(2, instance.node_count + 1) if seen[vertex] == 0]
    if missing:
        preview = " ".join(str(vertex) for vertex in missing[:10])
        suffix = "" if len(missing) <= 10 else " ..."
        errors.append(f"missing customers: {preview}{suffix}")

    if seen[depot] > 0:
        errors.append("depot 1 must not be counted as a served customer")

    delta = abs(claimed - actual)
    if not errors and not is_close(claimed, actual):
        errors.append(
            "claimed total cost does not match the recomputed cost: "
            f"claimed={claimed:.10f}, actual={actual:.10f}, delta={delta:.10f}"
        )

    return CheckResult(
        ok=not errors,
        claimed=claimed,
        actual=actual,
        delta=delta,
        errors=errors,
        route_count=route_count,
    )


def load_thresholds(config_path: Path) -> dict[str, list[Threshold]]:
    try:
        raw = json.loads(config_path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise CheckerError(f"cannot read config {config_path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise CheckerError(f"failed to parse config {config_path}: {exc}") from exc

    if not isinstance(raw, list):
        raise CheckerError("config must be a JSON array")

    result: dict[str, list[Threshold]] = {}
    for entry_index, entry in enumerate(raw, start=1):
        if not isinstance(entry, dict):
            raise CheckerError(f"config entry {entry_index} must be an object")
        if "path" not in entry:
            raise CheckerError(f"config entry {entry_index} is missing key 'path'")
        path = str(entry["path"])
        thresholds: list[Threshold] = []
        for key, value in entry.items():
            if key == "path":
                continue
            try:
                score = int(key)
                limit = float(value)
            except ValueError as exc:
                raise CheckerError(
                    f"config entry {entry_index}: threshold '{key}: {value}' is invalid"
                ) from exc
            thresholds.append(Threshold(score=score, limit=limit))
        thresholds.sort(key=lambda item: (item.limit, item.score), reverse=True)
        result[path] = thresholds

    return result


def best_score(actual: float, thresholds: Iterable[Threshold]) -> int:
    score = 0
    for threshold in thresholds:
        if actual <= threshold.limit:
            score = max(score, threshold.score)
    return score


def next_unpassed(actual: float, thresholds: Iterable[Threshold]) -> Threshold | None:
    remaining = [threshold for threshold in thresholds if actual > threshold.limit]
    if not remaining:
        return None
    return max(remaining, key=lambda item: (item.limit, item.score))


def format_float(value: float | None) -> str:
    if value is None:
        return "-"
    if not math.isfinite(value):
        return str(value)
    return f"{value:.6f}"


def summarize_note(errors: list[str], stderr_text: str | None = None) -> str:
    note = errors[0] if errors else "ok"
    if stderr_text:
        cleaned = " ".join(stderr_text.strip().split())
        if cleaned:
            note = f"{note}; stderr: {cleaned[:120]}"
    return note


def print_table(rows: list[dict[str, str]]) -> None:
    headers = ["Test", "Status", "Claimed", "Actual", "Score", "Next", "Note"]
    widths = {header: len(header) for header in headers}
    for row in rows:
        for header in headers:
            widths[header] = max(widths[header], len(row[header]))

    def line(sep: str, fill: str) -> str:
        parts = [fill * (widths[header] + 2) for header in headers]
        return sep + sep.join(parts) + sep

    def render_row(row: dict[str, str]) -> str:
        parts = [f" {row[header]:<{widths[header]}} " for header in headers]
        return "|" + "|".join(parts) + "|"

    print(line("+", "-"))
    print(render_row({header: header for header in headers}))
    print(line("+", "="))
    for row in rows:
        print(render_row(row))
        print(line("+", "-"))


def print_progress(index: int, total: int, row: dict[str, str]) -> None:
    print(
        f"[{index}/{total}] {row['Test']}: {row['Status']}, "
        f"score={row['Score']}, claimed={row['Claimed']}, actual={row['Actual']}, "
        f"next={row['Next']}, note={row['Note']}",
        flush=True,
    )


def run_solver(instance_path: Path, solver_cmd: list[str], timeout_seconds: float) -> subprocess.CompletedProcess[bytes]:
    try:
        instance_data = instance_path.read_bytes()
    except OSError as exc:
        raise CheckerError(f"cannot read instance {instance_path}: {exc}") from exc

    try:
        return subprocess.run(
            solver_cmd,
            input=instance_data,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout_seconds,
            check=False,
        )
    except subprocess.TimeoutExpired as exc:
        raise CheckerError(
            f"solver timed out after {timeout_seconds:g}s"
        ) from exc
    except OSError as exc:
        joined = " ".join(shlex.quote(part) for part in solver_cmd)
        raise CheckerError(f"failed to run solver '{joined}': {exc}") from exc


def batch_mode(config_path: Path, data_dir: Path, solver_cmd: list[str], timeout_seconds: float) -> int:
    thresholds_by_path = load_thresholds(config_path)
    test_items = list(thresholds_by_path.items())
    total_tests = len(test_items)
    rows: list[dict[str, str]] = []
    total_score = 0
    max_score = 0
    had_failures = False

    for test_index, (test_name, thresholds) in enumerate(test_items, start=1):
        max_score += max((threshold.score for threshold in thresholds), default=0)
        instance_path = data_dir / test_name
        print(f"[{test_index}/{total_tests}] running {test_name}...", flush=True)

        try:
            instance = load_instance(instance_path)
            proc = run_solver(instance_path, solver_cmd, timeout_seconds)
            stderr_text = proc.stderr.decode("utf-8", errors="replace").strip()

            if proc.returncode != 0:
                had_failures = True
                easiest = max(thresholds, key=lambda item: (item.limit, item.score), default=None)
                next_text = (
                    "-"
                    if easiest is None
                    else f"{easiest.score} pts @ <= {format_float(easiest.limit)}"
                )
                rows.append(
                    {
                        "Test": test_name,
                        "Status": f"RE{proc.returncode}",
                        "Claimed": "-",
                        "Actual": "-",
                        "Score": "0",
                        "Next": next_text,
                        "Note": summarize_note(
                            [f"solver exited with code {proc.returncode}"], stderr_text
                        ),
                    }
                )
                print_progress(test_index, total_tests, rows[-1])
                continue

            claimed, routes = parse_solution_lines(
                proc.stdout.decode("utf-8", errors="replace").splitlines()
            )

            result = check_solution(instance, claimed, routes)
            if result.ok and result.actual is not None:
                score = best_score(result.actual, thresholds)
                total_score += score
                next_target = next_unpassed(result.actual, thresholds)
                next_text = (
                    "-"
                    if next_target is None
                    else f"{next_target.score} pts @ <= {format_float(next_target.limit)}"
                )
                status = "OK"
            else:
                had_failures = True
                score = 0
                easiest = max(thresholds, key=lambda item: (item.limit, item.score), default=None)
                next_text = (
                    "-"
                    if easiest is None
                    else f"{easiest.score} pts @ <= {format_float(easiest.limit)}"
                )
                status = "WA"

            rows.append(
                {
                    "Test": test_name,
                    "Status": status,
                    "Claimed": format_float(result.claimed),
                    "Actual": format_float(result.actual),
                    "Score": str(score),
                    "Next": next_text,
                    "Note": summarize_note(result.errors, stderr_text),
                }
            )
            print_progress(test_index, total_tests, rows[-1])
        except CheckerError as exc:
            had_failures = True
            easiest = max(thresholds, key=lambda item: (item.limit, item.score), default=None)
            next_text = (
                "-"
                if easiest is None
                else f"{easiest.score} pts @ <= {format_float(easiest.limit)}"
            )
            rows.append(
                {
                    "Test": test_name,
                    "Status": "ERR",
                    "Claimed": "-",
                    "Actual": "-",
                    "Score": "0",
                    "Next": next_text,
                    "Note": str(exc),
                }
            )
            print_progress(test_index, total_tests, rows[-1])

    print_table(rows)
    print(f"Total score: {total_score} / {max_score}")
    return 1 if had_failures else 0


def single_mode(instance_path: Path, solution_path: Path) -> int:
    instance = load_instance(instance_path)
    claimed, routes = parse_solution(solution_path)
    result = check_solution(instance, claimed, routes)

    if result.ok:
        print("OK")
        print(f"Claimed: {format_float(result.claimed)}")
        print(f"Actual:  {format_float(result.actual)}")
        print(f"Routes:  {result.route_count}")
        return 0

    print("WA")
    print(f"Claimed: {format_float(result.claimed)}")
    print(f"Actual:  {format_float(result.actual)}")
    for error in result.errors:
        print(f"- {error}")
    return 1


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate VRP solver output and run scored tests from config.json."
    )
    parser.add_argument("--instance", type=Path, help="path to one VRP instance")
    parser.add_argument("--output", type=Path, help="path to one solver output file")
    parser.add_argument("--config", type=Path, default=Path("config.json"), help="path to config.json")
    parser.add_argument("--data-dir", type=Path, default=Path("data"), help="directory with VRP instances")
    parser.add_argument("--solver", nargs="+", help="solver command for batch mode")
    parser.add_argument("--timeout", type=float, default=120.0, help="per-test timeout in seconds")
    args = parser.parse_args()

    try:
        if args.solver:
            return batch_mode(args.config, args.data_dir, args.solver, args.timeout)

        if args.instance and args.output:
            return single_mode(args.instance, args.output)

        parser.error("use either --solver ... for batch mode or --instance ... --output ... for single mode")
    except CheckerError as exc:
        print(f"checker error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
