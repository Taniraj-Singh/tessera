# Tessera Binary Container SDK

Tessera is a production-oriented C++20 SDK for the **Tessera Binary Container (TBC)** format — a versioned, checksummed container for structured typed records.

**Author:** Taniraj Singh <tanirajsingh574@gmail.com>

## Features

- Modular parser, serializer, and multi-stage validator (structural, semantic, constraint)
- Compression codec layer (`store` / `rle` / `lz77`) with self-describing frames
- Multi-container archive format (TBA) with a checksummed frame directory
- Incremental push-model decoder/encoder and a composable processing pipeline
- Schema engine: type-system compatibility rules and a layout-resolving compiler
- Reflection, metadata registry, and hash/index structures for O(1) section lookup
- Transactional editing, container repair/recovery, and resource budgeting
- Object-graph traversal, visitor framework, statistics, and a profiler
- Diagnostics, deterministic tracing, leveled logging, and configuration
- Streaming reader, cross-platform memory-mapped file reader, plugin hooks
- Version negotiation for forward-compatible readers
- Conversion utilities (JSON summary, hex dump, canonical binary)
- CLI tools: `tbc_inspect`, `tbc_convert`
- Unit, integration, regression, and edge-case tests
- Seven libFuzzer targets with ClusterFuzzLite integration
- Offline build (no network fetches)

## Quick Start

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### Windows (Visual Studio)

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release
```

## CLI Usage

```bash
./build/tools/tbc_inspect/tbc_inspect sample.tbc --json
./build/tools/tbc_inspect/tbc_inspect sample.tbc --validate
./build/tools/tbc_convert/tbc_convert sample.tbc out.json --json
```

Generate a sample container:

```bash
./build/examples/create_sample/create_sample
```

## Sanitizers

```bash
cmake -S . -B build-asan -DTESSERA_ENABLE_SANITIZERS=ON
cmake --build build-asan
```

## Fuzzing (Clang + libFuzzer)

```bash
cmake -S . -B build-fuzz -DTESSERA_BUILD_FUZZ=ON -DCMAKE_CXX_COMPILER=clang++
cmake --build build-fuzz
./build-fuzz/fuzz/parser_fuzzer fuzz/corpus/
```

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Developer Guide](docs/DEVELOPER_GUIDE.md)
- [API Overview](docs/API.md)
- [Directory Layout](docs/DIRECTORY.md)

### Module guides

- Foundation: [Memory](docs/modules/MEMORY.md) · [Encoding](docs/modules/ENCODING.md)
- Data path: [Compression](docs/modules/COMPRESSION.md) · [Archive](docs/modules/ARCHIVE.md) · [Pipeline](docs/modules/PIPELINE.md)
- Analysis: [Index & Hash](docs/modules/INDEX.md) · [Reflection & Metadata](docs/modules/REFLECTION.md) · [Schema Engine](docs/modules/SCHEMA_ENGINE.md) · [Validation](docs/modules/VALIDATION.md)
- Support: [Observability](docs/modules/OBSERVABILITY.md) · [Lifecycle](docs/modules/LIFECYCLE.md) · [Introspection](docs/modules/INTROSPECTION.md)

## License

Proprietary — Copyright (c) 2026 Taniraj Singh. All rights reserved.
