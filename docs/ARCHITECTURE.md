# Tessera Architecture

## Overview

Tessera implements the Tessera Binary Container (TBC) format as a layered SDK. Each layer validates bounded input and returns structured errors — the parse path never trusts attacker-controlled lengths.

## Layer Diagram

```
┌──────────── CLI / Examples / Fuzz Harnesses ────────────┐
│  tbc_inspect   tbc_convert   create_sample   fuzz/*     │
└───────────────────────────┬─────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────┐
│  Orchestration: parse · serialize · validate · convert  │
│  stream · mmap · version · plugins                      │
└───────────────────────────┬─────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────┐
│  Artifact modules: header · catalog · schema · records  │
└───────────────────────────┬─────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────┐
│  Foundation: core · buffer · checksum                   │
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

## Design Principles

| Principle | Application |
|-----------|-------------|
| Fail-closed | All failures return `Result<T, Error>` with byte offset |
| Bounded I/O | `buffer::Reader` rejects overruns before allocation |
| No global parse state | Container model passed explicitly; plugin registry is the only singleton |
| Deterministic output | Serializer computes stable offsets for round-trip tests |
| Fuzz-friendly surface | Independent fuzz targets per artifact module |

## Plugin Architecture

Plugins implement `tessera::plugins::Plugin` and register with `Registry::instance()`. Hooks are synchronous and may reject containers with structured errors — useful for policy enforcement (e.g., required timestamps).

## Version Negotiation

Readers declare a supported `[min, max]` version range. Containers outside the range are rejected before catalog parsing, preventing partial reads of incompatible layouts.

## Memory-Mapped I/O

`tessera::mmap::MappedFile` provides zero-copy access on POSIX (`mmap`) and Windows (`MapViewOfFile`). The inspection CLI prefers mmap and falls back to buffered I/O.

## Streaming API

`tessera::stream::StreamReader` yields incremental events (`Header`, `Catalog`, `SectionBegin`, `Complete`) for large files without building the full in-memory model.
