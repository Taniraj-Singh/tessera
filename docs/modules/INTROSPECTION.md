# Introspection Subsystems

Read-only traversal and measurement over the container model.

## Object graph (`tessera::graph`)

`build_graph(container)` materializes a uniform tree mirroring the model's
structure: `container → section → {schema → field, records → row → value}`. Value
nodes are labeled with their reflected type name. Passing `include_rows = false`
yields a lightweight structural skeleton. `node_count` and `max_depth` summarize
the tree. Having one homogeneous `Node` type lets traversal, diffing, and
rendering code avoid switching over the heterogeneous model types.

## Visitor (`tessera::visit`)

`Visitor` exposes `enter`/`leave` hooks bracketing each section plus per-field and
per-row callbacks, all defaulting to no-ops. `traverse(container, visitor)` walks
the model in structural order. Stateful visitors scope per-section context in the
enter/leave pair — the statistics collector is one such visitor.

## Statistics (`tessera::stats`)

`collect(container)` runs a `StatsVisitor` over `traverse` to produce
`ContainerStats`: section/row/field totals, total encoded value bytes (via
`reflect::FieldRef::encoded_size`), the largest section by rows, and a per-type
field histogram indexed by `FieldType`.

## Profiler (`tessera::profile`)

`Profiler` holds named counters and named `Stat` distributions (count/sum/min/max
+ mean). It samples whatever measure the caller feeds it — bytes, rows, logical
ticks — so it stays deterministic and clock-free. Used to instrument the pipeline
and recovery paths.
