# Directory Layout

```
tessera/
├── CMakeLists.txt              Root build definition
├── cmake/                      CMake modules (warnings, sanitizers, fuzz)
├── include/tessera/            Public headers
│   ├── core/                   Types, constants, Result, spans
│   ├── buffer/                 Bounded reader/writer
│   ├── checksum/               CRC32C
│   ├── header/                 Container header
│   ├── catalog/                Section catalog
│   ├── schema/                 Field type descriptors
│   ├── records/                Record batch encode/decode
│   ├── model/                  In-memory container model
│   ├── parse/                  Full container parser
│   ├── serialize/              Canonical serializer
│   ├── validate/               Multi-stage validator
│   ├── stream/                 Incremental stream reader
│   ├── mmap/                   Memory-mapped file I/O
│   ├── convert/                Format conversion utilities
│   ├── version/                Version negotiation
│   ├── plugins/                Plugin registry
│   └── tessera.hpp             Umbrella include
├── src/                        Implementation sources
├── tests/
│   ├── unit/                   Unit tests
│   ├── integration/            Round-trip and pipeline tests
│   ├── regression/             Fixed failure cases
│   ├── edge/                   Boundary conditions
│   └── support/                Test framework and helpers
├── fuzz/                       libFuzzer harnesses + corpus
├── tools/
│   ├── tbc_inspect/            Inspection CLI
│   └── tbc_convert/            Conversion CLI
├── examples/
│   ├── create_sample/          Sample container generator
│   └── validate_file/          Plugin demonstration
├── docs/                       Architecture and API docs
├── scripts/                    Build scripts
├── resources/                  Sample assets (generated at runtime)
└── .clusterfuzzlite/           ClusterFuzzLite configuration
```
