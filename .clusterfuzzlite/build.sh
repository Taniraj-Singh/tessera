#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-fuzz"

cmake -S "$ROOT" -B "$BUILD" \
  -DCMAKE_BUILD_TYPE=Release \
  -DTESSERA_BUILD_TESTS=OFF \
  -DTESSERA_BUILD_EXAMPLES=OFF \
  -DTESSERA_BUILD_TOOLS=OFF \
  -DTESSERA_BUILD_FUZZ=ON \
  -DCMAKE_CXX_COMPILER=clang++

cmake --build "$BUILD" --parallel

cp "$BUILD/fuzz/parser_fuzzer" "$OUT/"
cp "$BUILD/fuzz/decoder_fuzzer" "$OUT/"
cp "$BUILD/fuzz/serializer_fuzzer" "$OUT/"
cp "$BUILD/fuzz/validator_fuzzer" "$OUT/"
