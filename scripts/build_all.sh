#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${1:-$ROOT/build}"

cmake -S "$ROOT" -B "$BUILD" -DTESSERA_BUILD_FUZZ=OFF
cmake --build "$BUILD" --parallel
ctest --test-dir "$BUILD" --output-on-failure
