# Tessera Binary Container SDK

Tessera is a production-oriented C++20 SDK for the **Tessera Binary Container (TBC)** format — a versioned, checksummed container for structured typed records.

**Author:** Taniraj Singh <tanirajsingh574@gmail.com>

## Features

- Modular parser, serializer, and multi-stage validator
- Streaming reader for incremental inspection
- Cross-platform memory-mapped file reader
- Plugin hooks at major pipeline stages
- Version negotiation for forward-compatible readers
- Conversion utilities (JSON summary, hex dump, canonical binary)
- CLI tools: `tbc_inspect`, `tbc_convert`
- Unit, integration, regression, and edge-case tests
- libFuzzer targets with ClusterFuzzLite integration
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

## License

Proprietary — Copyright (c) 2026 Taniraj Singh. All rights reserved.
