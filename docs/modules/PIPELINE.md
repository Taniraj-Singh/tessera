# Pipeline Subsystem

`tessera::pipeline` adds the SDK's stateful streaming surfaces and the top-level
orchestration that binds archive, compression, parse, and validation together.

## IncrementalDecoder

A push-model container decoder. Bytes arrive in arbitrary chunks via `feed`; the
decoder advances a state machine as each structural boundary becomes available:

```
NeedHeader → NeedCatalogLength → NeedCatalog → NeedSections → Complete
```

Cross-chunk state it carries: the parsed header, the catalog block length, and
the total `expected_size` derived from the catalog's furthest section end.
`NeedSections` completes only when every declared byte is buffered, at which
point the authoritative full parse runs. A catalog implying more than
`kMaxIncrementalContainerSize` is rejected so the decoder never waits on bytes
that cannot arrive. Any structural error transitions to a sticky `Failed`.

## IncrementalEncoder

The mirror image: `begin` → `add_section`* → `finish`. Sections stream in, the
header's section count stays in sync, and `finish` emits the canonical byte
layout via the serializer. Empty section names are rejected.

## Stages (`pipeline/stage.hpp`)

`ByteTransform` is a `ByteSpan → Result<vector<uint8_t>>` step. `compose` chains
two; `run_chain` runs a vector of them, short-circuiting on the first failure.
Decompression, canonicalization, and conversion all fit this shape.

## Archive pipeline

`run_archive_pipeline(bytes)` opens an archive and drives every entry through
decode + structural + semantic validation, folding per-entry `EntryOutcome`s into
a `PipelineReport`. It is the end-to-end path the `pipeline_fuzzer` drives, and
the deepest reachable orchestration in the SDK.
