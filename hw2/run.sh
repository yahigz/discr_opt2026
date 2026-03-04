#!/bin/bash

set -euo pipefail

mode="${1:-cpp}"

if [ "$mode" = "cpp" ]; then
	echo "Compiling solution.cpp..."
	g++ -O2 -std=c++17 solution.cpp -o a.out
	echo "Compilation successful"
else
	echo "Usage: $0 [cpp]"
	exit 1
fi

echo
python3 checker.py
