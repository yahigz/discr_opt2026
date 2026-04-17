#!/bin/bash

set -euo pipefail

solver="${1:-solution.cpp}"

case "$solver" in
	solution|solution.cpp)
		solver_file="solution.cpp"
		;;
	genetics|genetics.cpp)
		solver_file="genetics.cpp"
		;;
	*.cpp)
		solver_file="$solver"
		;;
	*)
		echo "Usage: $0 [solution|genetics|<solver.cpp>]"
		exit 1
		;;
esac

if [[ ! -f "$solver_file" ]]; then
	echo "Solver file not found: $solver_file"
	exit 1
fi

echo "Compiling $solver_file..."
g++ -O2 -std=c++17 "$solver_file" -o a.out
echo "Compilation successful"

echo
python3 checker.py
