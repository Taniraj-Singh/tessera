# Observability Subsystems

Cross-cutting support for diagnostics, tracing, logging, and configuration.

## Diagnostics (`tessera::diag`)

`DiagnosticSink` collects severity-tagged, categorized `Record`s. Query by
severity (`count`, `has_errors`), render with `format()`, or fold a
`validate::ValidationReport` in via `ingest`. It is the common destination for
validation, recovery, and pipeline messages.

## Tracing (`tessera::diag::Tracer`)

A deterministic tracer: time is a **logical tick** incremented per event, so
traces reproduce exactly and need no clock. Events (`Enter`/`Exit`/`Mark`) land
in a fixed-capacity ring buffer; `ScopedSpan` brackets a scope with a
depth-balanced enter/exit pair. `events()` returns the retained window in
chronological order.

## Logging (`tessera::log`)

`Logger` fans leveled messages out to borrowed `Sink`s, dropping anything below
the active `Level` before formatting. `BufferSink` captures lines in memory for
tests and diagnostics.

## Configuration (`tessera::config`)

`Config::parse` reads `key = value` lines (with `#` comments), reporting a
malformed line by byte offset. Typed getters (`get_int`, `get_bool`,
`get_double`, `get_string`) coerce the stored string and fall back to a default
when the key is absent or unparseable, so lookups never need guarding. Lookups
use a transparent comparator for `string_view` keys without allocating.
