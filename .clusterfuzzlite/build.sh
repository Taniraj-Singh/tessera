#!/bin/sh
set -eu

# Resolve the repository root robustly. ClusterFuzzLite runs this script from the
# repository root (as <root>/build.sh), whereas a local checkout keeps it under
# .clusterfuzzlite/. Pick whichever directory actually contains CMakeLists.txt.
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
if [ -f "$SCRIPT_DIR/CMakeLists.txt" ]; then
  ROOT="$SCRIPT_DIR"
elif [ -f "$SCRIPT_DIR/../CMakeLists.txt" ]; then
  ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
elif [ -n "${SRC:-}" ] && [ -f "${SRC}/CMakeLists.txt" ]; then
  ROOT="$SRC"
else
  ROOT="$(pwd)"
fi

BUILD="$ROOT/build-fuzz"

cmake -S "$ROOT" -B "$BUILD" \
  -DCMAKE_BUILD_TYPE=Release \
  -DTESSERA_BUILD_TESTS=OFF \
  -DTESSERA_BUILD_EXAMPLES=OFF \
  -DTESSERA_BUILD_TOOLS=OFF \
  -DTESSERA_BUILD_FUZZ=ON \
  -DTESSERA_WARNINGS_AS_ERRORS=OFF \
  -DCMAKE_CXX_COMPILER="${CXX:-clang++}"

cmake --build "$BUILD" --parallel

for target in parser_fuzzer decoder_fuzzer serializer_fuzzer validator_fuzzer compress_fuzzer archive_fuzzer pipeline_fuzzer; do
  cp "$BUILD/fuzz/$target" "$OUT/"
done
