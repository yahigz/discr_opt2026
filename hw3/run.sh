#!/bin/bash

set -euo pipefail

mode="${1:-cpp}"
solver_args=()

if [ "$mode" = "cpp" ]; then
	echo "Compiling solution.cpp..."
	g++ -O2 -std=c++17 solution.cpp -o a.out
	echo "Compilation successful"
elif [ "$mode" = "rlf" ]; then
	echo "Compiling solution_rlf.cpp..."
	g++ -O2 -std=c++17 solution_rlf.cpp -o a.out
	echo "Compilation successful"
elif [ "$mode" = "fcns" ]; then
	echo "Compiling fcns.cpp..."
	g++ -O2 -std=c++17 fcns.cpp -o a.out
	echo "Compilation successful"
	if [ "$#" -ge 2 ]; then
		solver_args=("${@:2}")
	fi
else
	echo "Usage: $0 [cpp|rlf|fcns [brelaz|nonsingleton]]"
	exit 1
fi

echo
python3 checker.py "${solver_args[@]}"
