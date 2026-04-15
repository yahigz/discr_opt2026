#!/bin/bash

set -euo pipefail

echo "Compiling solution.cpp..."
g++ -O2 -std=c++17 solution.cpp -o a.out
echo "Compilation successful"

echo
python3 checker.py
