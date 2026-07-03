# Tessera Architecture

## Overview

Tessera implements the Tessera Binary Container (TBC) format as a layered SDK. Each layer parses its input and returns structured errors with byte offsets.

## Layer Diagram

```
┌──────────── CLI / Examples / Fuzz Harnesses ────────────┐
│  tbc_inspect   tbc_convert   create_sample   fuzz/*     │
└───────────────────────────┬─────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────┐
│  Orchestration: parse · serialize · validate · convert  │
│  stream · pipeline · mmap · version · plugins           │
└───────────────────────────┬─────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────┐
│  Archive: archive reader/writer over framed containers  │
└───────────────────────────┬─────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────┐
│  Artifact modules: header · catalog · schema · records  │
│  compress (codec + frame) · index · reflect · metadata  │
└───────────────────────────┬─────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────┐
│  Foundation: core · util · buffer · memory · encoding   │
│              checksum                                    │
└─────────────────────────────────────────────────────────┘
```

## Container Layout

1. **Fixed 64-byte header** — magic, version, flags, timestamp, section count, header CRC32C
2. **Catalog block** — section table with offsets and lengths
3. **Section payloads** — each section contains a schema block followed by a record batch

## Validation Pipeline

Parsing proceeds in strict order:

1. Header parse + header checksum
2. Version negotiation
3. Optional plugin hook (`AfterHeader`)
4. Catalog parse + entry bounds checks
5. Optional plugin hook (`AfterCatalog`)
6. Per-section schema parse
7. Optional plugin hook (`AfterSchema`)
8. Per-section record decode
9. Optional plugin hook (`AfterRecords`)

Serialization runs the reverse path and invokes `BeforeSerialize` plugins.

## Conventions

| Convention | Application |
|-----------|-------------|
| Structured errors | Failures return `Result<T, Error>` with a byte offset |
| Explicit state | The container model is passed explicitly; the plugin registry is the only singleton |
| Deterministic output | The serializer computes stable offsets for round-trip tests |
| Per-module targets | Independent unit-test and fuzz targets exercise the artifact modules |

## Plugin Architecture

Plugins implement `tessera::plugins::Plugin` and register with `Registry::instance()`. Hooks are synchronous and may reject containers with structured errors — useful for policy enforcement (e.g., required timestamps).

## Version Negotiation

Readers declare a supported `[min, max]` version range. Containers outside the range are rejected before catalog parsing, preventing partial reads of incompatible layouts.

## Memory-Mapped I/O

`tessera::mmap::MappedFile` provides zero-copy access on POSIX (`mmap`) and Windows (`MapViewOfFile`). The inspection CLI prefers mmap and falls back to buffered I/O.

## Streaming API

`tessera::stream::StreamReader` yields incremental events (`Header`, `Catalog`, `SectionBegin`, `Complete`) for large files without building the full in-memory model.
