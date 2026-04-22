#!/bin/bash

set -euo pipefail

mode="${1:-cpp}"
lambda_val="${2:-}"

if [ -n "$lambda_val" ]; then
    export LAMBDA="$lambda_val"
fi

if [ "$mode" = "cpp" ]; then
    echo "Compiling solution_greedy.cpp..."
    g++ -O2 -std=c++17 solution_greedy.cpp -o a.out
    echo "Compilation successful!"
elif [ "$mode" = "beam" ]; then
    echo "Compiling solution_beam_search.cpp..."
    g++ -O2 -std=c++17 solution_beam_search.cpp -o a.out
    echo "Compilation successful!"
elif [ "$mode" = "solution" ]; then
    echo "Compiling solution.cpp..."
    g++ -O2 -std=c++17 solution.cpp -o a.out
    echo "Compilation successful!"
else
    echo "Usage: $0 [cpp|beam|solution] [LAMBDA]"
    exit 1
fi

echo ""
python3 checker.py