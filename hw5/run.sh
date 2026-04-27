#!/bin/bash

set -euo pipefail


if [[ $# -ne 1 ]]; then
	echo "Usage: $0 [solution|genetics]"
	exit 1
fi

if [[ $1 == "solution" ]]; then
	echo "Compiling solution.cpp..."
	g++ -O2 -std=c++17 solution.cpp -o a.out
elif [[ $1 == "genetics" ]]; then
	echo "Compiling genetics.cpp..."
	g++ -O2 -std=c++17 genetics.cpp -o a.out
elif [[ $1 == "aboba" ]]; then
	echo "Compiling aboba.cpp..."
	g++ -O2 -std=c++17 aboba.cpp -o a.out
else
	echo "Unknown argument: $1"
	echo "Usage: $0 [solution|genetics|aboba]"
	exit 1
fi
echo "Compilation successful"

echo
python3 checker.py
