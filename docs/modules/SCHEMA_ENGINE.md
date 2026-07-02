# Schema Engine

Beyond the wire-level `schema::parse`/`serialize`, the schema engine adds a type
system for evolution and a compiler that resolves per-field layout.

## Type system (`schema/type_system.hpp`)

`is_widening(from, to)` encodes the loss-free conversions:

- unsigned widening `U8 → U32 → U64`;
- signed widening `I32 → I64`;
- unsigned-into-wider-signed `U8 → I32`, `U32 → I64`;
- integers ≤32 bits into `F64`;
- `U64 ↔ TimestampMs` (shared storage).

`compare_field` combines this with the length cap: lifting or widening a cap is
`Compatible`, shrinking it is `Incompatible`. `compare_schema` walks the shared
prefix, treats appended trailing fields as `Compatible` and removed fields as
`Incompatible`, and reports the worst per-field outcome.

## Schema compiler (`schema/schema_compiler.hpp`)

`compile(schema)` produces a `CompiledSchema`:

- one `CompiledField` per field with `fixed`/`wire_size` resolved from
  `reflect::TypeInfo`;
- `min_row_size()` — the smallest encoded row (fixed widths plus each variable
  field's length prefix). A `row_count` whose product with this exceeds the
  section payload is provably impossible;
- a `StringIndex` for `find_field(name)` in O(1);
- duplicate field names are rejected up front.

The compiler is pure metadata: it never reads record bytes.
