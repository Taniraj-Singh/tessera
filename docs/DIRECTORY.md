# Directory Layout

```
tessera/
├── CMakeLists.txt              Root build definition
├── cmake/                      CMake modules (warnings, sanitizers, fuzz)
├── include/tessera/            Public headers
│   ├── core/                   Types, constants, Result, spans
│   ├── util/                   Header-only bit/alignment helpers
│   ├── buffer/                 Bounded reader/writer
│   ├── memory/                 Growable buffer, arena, object pool
│   ├── encoding/               LEB128 varint and zigzag codecs
│   ├── checksum/               CRC32C
│   ├── hash/                   FNV-1a and MurmurHash3
│   ├── compress/               Codec layer (store/rle/lz77) + frames
│   ├── index/                  Hash / string / section indexes
│   ├── reflect/                Field type info and typed accessors
│   ├── metadata/               Section descriptor registry
│   ├── archive/                Multi-container archive reader/writer
│   ├── pipeline/               Incremental decode/encode + orchestration
│   ├── diag/                   Diagnostic sink and tracer
│   ├── log/                    Leveled logger and sinks
│   ├── config/                 Key-value configuration store
│   ├── txn/                    Transactional container editing
│   ├── recovery/               Container repair utilities
│   ├── resource/               Resource budget manager
│   ├── graph/                  Object graph materialization
│   ├── visit/                  Container visitor framework
│   ├── stats/                  Container statistics
│   ├── profile/                Counter / distribution profiler
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
│   ├── tbc_inspect/            Inspection CLI (--json/--validate/--stream/--stats)
│   ├── tbc_convert/            Conversion CLI
│   ├── tbc_archive/            Archive pack/list/extract CLI
│   └── corpus_gen/             Fuzz seed corpus generator
├── examples/
│   ├── create_sample/          Sample container generator
│   ├── validate_file/          Plugin demonstration
│   ├── archive_bundle/         Build/read an archive + run the pipeline
│   ├── compress_demo/          Codec ratios and frame round-trip
│   └── inspect_stats/          Statistics, object graph, profiler
├── docs/                       Architecture and API docs
├── scripts/                    Build scripts
├── resources/                  Sample assets (generated at runtime)
└── .clusterfuzzlite/           ClusterFuzzLite configuration
```
