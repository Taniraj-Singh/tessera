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
cp "$BUILD/fuzz/compress_fuzzer" "$OUT/"
cp "$BUILD/fuzz/archive_fuzzer" "$OUT/"
cp "$BUILD/fuzz/pipeline_fuzzer" "$OUT/"

# Best-effort seed corpus generation. The fuzzers are already staged in $OUT, so
# anything below is a bonus: it is fully guarded and can never fail the build.
generate_seeds() {
  CORPUS_BUILD="$ROOT/build-corpus"
  cmake -S "$ROOT" -B "$CORPUS_BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DTESSERA_BUILD_TESTS=OFF \
    -DTESSERA_BUILD_EXAMPLES=OFF \
    -DTESSERA_BUILD_TOOLS=ON \
    -DTESSERA_BUILD_FUZZ=OFF
  cmake --build "$CORPUS_BUILD" --target corpus_gen --parallel

  SEED_SRC="$ROOT/seed-src"
  rm -rf "$SEED_SRC"
  mkdir -p "$SEED_SRC"
  "$CORPUS_BUILD/tools/corpus_gen/corpus_gen" "$SEED_SRC"

  pack_seed() {
    harness="$1"
    shift
    stage="$ROOT/seed-$harness"
    rm -rf "$stage"
    mkdir -p "$stage"
    for seed in "$@"; do
      if [ -f "$SEED_SRC/$seed" ]; then
        cp "$SEED_SRC/$seed" "$stage/"
      fi
    done
    (cd "$stage" && zip -qr "$OUT/${harness}_seed_corpus.zip" .)
  }

  pack_seed parser_fuzzer container.tbc
  pack_seed decoder_fuzzer schema.bin container.tbc
  pack_seed serializer_fuzzer container.tbc
  pack_seed validator_fuzzer container.tbc
  pack_seed compress_fuzzer frame.tfz
  pack_seed archive_fuzzer archive.tba
  pack_seed pipeline_fuzzer container.tbc archive.tba
}

generate_seeds || echo "seed corpus generation skipped"
