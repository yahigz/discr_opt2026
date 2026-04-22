#!/usr/bin/env python3

import json
import subprocess
import os
import sys
from pathlib import Path

def load_config():
    with open('config.json', 'r') as f:
        tests = json.load(f)
    return tests

def parse_input(test_path):
    """Parse the input test file to get sets and n."""
    try:
        with open(f'data/{test_path}', 'r') as f:
            lines = f.readlines()
        
        n, m = map(int, lines[0].split())
        sets = []
        costs = []
        
        for i in range(1, m + 1):
            parts = list(map(int, lines[i].split()))
            cost = parts[0]
            elements = parts[1:]
            costs.append(cost)
            sets.append(set(elements))
        
        return n, m, sets, costs
    except Exception as e:
        return None, None, None, None

def check_valid_setcover(set_indices, sets, n):
    """Check if selected sets cover all elements [0, n-1]."""
    coverage = set()
    for idx in set_indices:
        if idx < 1 or idx > len(sets):
            return False, f"Invalid set index: {idx}"
        coverage |= sets[idx - 1]  # set_indices are 1-indexed
    
    if coverage == set(range(0, n)):
        return True, "Valid cover"
    else:
        missing = set(range(0, n)) - coverage
        return False, f"Missing elements: {missing}"

def compute_solution_cost(set_indices, costs):
    total = 0
    for idx in set_indices:
        if idx < 1 or idx > len(costs):
            return None
        total += costs[idx - 1]
    return total

def run_solution(test_path):
    """Run solution with test input and return the output count and set indices."""
    try:
        with open(f'data/{test_path}', 'r') as f:
            test_input = f.read()
        
        result = subprocess.run(
            ['./a.out'],
            input=test_input,
            capture_output=True,
            text=True,
            timeout=360
        )
        
        stderr_msg = result.stderr.strip() if result.stderr else ""
        if result.returncode != 0:
            err = stderr_msg or "unknown error"
            return None, None, f"Runtime error: {err}", stderr_msg
        
        # Parse output: first line is count, second line is set indices
        lines = result.stdout.strip().split('\n')
        if len(lines) < 1:
            return None, None, "Empty output"
        
        try:
            count = int(lines[0])
        except ValueError:
            return None, None, f"Invalid output format for count: {lines[0]}"
        
        set_indices = []
        if len(lines) > 1 and lines[1].strip():
            try:
                set_indices = list(map(int, lines[1].split()))
            except ValueError:
                return None, None, f"Invalid output format for set indices: {lines[1]}"
        
        return count, set_indices, None, stderr_msg
    
    except subprocess.TimeoutExpired:
        return None, None, "Timeout", ""
    except Exception as e:
        return None, None, str(e), ""

def determine_points(count, target_5, target_7):
    """Determine points based on correctness."""
    if count <= target_7:
        return 5
    elif count <= target_5:
        return 3
    else:
        return 0

def main():
    tests = load_config()
    
    os.makedirs('output', exist_ok=True)
    
    total_points = 0
    results = []
    
    print("=" * 70)
    print("Running checker...")
    print("=" * 70)
    
    for i, test in enumerate(tests, 1):
        test_path = test['path']
        target_5 = test['5']
        target_7 = test['7']
        
        print(f"\n[{i}/{len(tests)}] Testing {test_path}...", end=" ", flush=True)
        
        # Parse input to get sets
        n, m, sets, costs = parse_input(test_path)
        if n is None:
            print(f"ERROR: Failed to parse input")
            points = 0
            result_str = "ERROR: Failed to parse input"
        else:
            # Run solution
            count, set_indices, error, stderr_msg = run_solution(test_path)
            
            if stderr_msg:
                print(f"\n[log] {test_path}: {stderr_msg}")

            if error:
                print(f"ERROR: {error}")
                points = 0
                result_str = f"ERROR: {error}"
            else:
                # Check if valid setcover
                valid, msg = check_valid_setcover(set_indices, sets, n)
                
                if not valid:
                    print(f"ERROR: {msg}")
                    points = 0
                    result_str = f"ERROR: Invalid setcover - {msg}"
                else:
                    real_cost = compute_solution_cost(set_indices, costs)
                    if real_cost is None:
                        print("ERROR: Invalid set index in cost check")
                        points = 0
                        result_str = "ERROR: Invalid set index in cost check"
                    elif real_cost != count:
                        print(f"ERROR: Reported cost {count} does not match real cost {real_cost}")
                        points = 0
                        result_str = f"ERROR: Cost mismatch (reported {count}, real {real_cost})"
                    else:
                        # Determine points based on count
                        points = determine_points(count, target_5, target_7)
                        result_str = str(count)
                    
                        if points == 5:
                            print(f"✓ Count: {count} (5 points)")
                        elif points == 3:
                            print(f"✓ Count: {count} (3 points)")
                        else:
                            print(f"✗ Count: {count} (0 points) [target 3pts: {target_5}, 5pts: {target_7}]")
        
        total_points += points
        
        log_file = f"output/{test_path}.log"
        with open(log_file, 'w') as f:
            f.write(result_str + '\n')
        
        results.append({
            'test': test_path,
            'count': count if error is None else None,
            'points': points,
            'target_5': target_5,
            'target_7': target_7
        })
    
    print("\n" + "=" * 70)
    print("SUMMARY")
    print("=" * 70)
    for result in results:
        status = "✓" if result['points'] > 0 else "✗"
        count_str = str(result['count']) if result['count'] is not None else "ERROR"
        print(f"{status} {result['test']:20} | Count: {count_str:8} | Points: {result['points']}/5")
    
    print("="* 70)
    print(f"TOTAL POINTS: {total_points}/{len(tests) * 5}")
    print("=" * 70)

if __name__ == '__main__':
    main()
