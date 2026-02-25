#!/bin/bash

set -euo pipefail

mode="${1:-cpp}"
lambda_val="${2:-}"

if [ -n "$lambda_val" ]; then
    export LAMBDA="$lambda_val"
fi

if [ "$mode" = "cpp" ]; then
    echo "Compiling solution.cpp..."
    g++ -O2 -std=c++17 solution_greedy.cpp -o a.out
    echo "Compilation successful!"
elif [ "$mode" = "beam" ]; then
    echo "Compiling Beam Search with g++..."
    g++ -O2 -std=c++17 solution_beam_search.cpp -o a.out
    echo "Compilation successful!"
else
    echo "Usage: $0 [cpp|beam] [LAMBDA]"
    exit 1
fi

echo ""
python3 checker.py
