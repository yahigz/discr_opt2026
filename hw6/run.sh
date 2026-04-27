#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
SOURCE_FILE="${1:-$ROOT_DIR/main.cpp}"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/.build}"
SOLVER_BIN="${SOLVER_BIN:-$BUILD_DIR/solver}"
TIME_LIMIT="${TIME_LIMIT:-800}"
CXX_BIN="${CXX_BIN:-g++}"
CXXFLAGS_VALUE="${CXXFLAGS_VALUE:--std=c++20 -O2 -Wall -Wextra -pedantic}"

mkdir -p "$BUILD_DIR"

echo "Compiling $SOURCE_FILE"
"$CXX_BIN" $CXXFLAGS_VALUE "$SOURCE_FILE" -o "$SOLVER_BIN"

echo "Running scored tests"
python3 "$ROOT_DIR/checker.py" \
  --config "$ROOT_DIR/config.json" \
  --data-dir "$ROOT_DIR/data" \
  --timeout "$TIME_LIMIT" \
  --solver "$SOLVER_BIN"
