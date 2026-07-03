# Reflection & Metadata Subsystems

`tessera::reflect` and `tessera::metadata` give generic code a way to reason
about fields and sections without switching on `schema::FieldType` at every call
site.

## Reflection (`tessera::reflect`)

- **`TypeInfo`** (`reflect/type_info.hpp`) — a runtime record per field type:
  display name, whether it is fixed- or variable-width, its fixed wire size,
  and category flags (`numeric`, `is_signed`, `aggregate`). Backed by a static
  table indexed by the enum value, with slot 0 an "unknown" fallback for an
  out-of-range enum.
- **`FieldRef`** (`reflect/field_ref.hpp`) — a non-owning pairing of a
  `FieldDescriptor` with a decoded `records::FieldValue`. Provides typed
  accessors (`as_u32`, `as_string`, …) via `std::get_if`, a `matches_type()`
  consistency check between the declared type and the stored variant
  alternative, and `encoded_size()` including any length prefix.

`matches_type()` treats `TimestampMs` as its `uint64_t` storage, matching how
`records::decode` stores timestamps.

## Metadata (`tessera::metadata`)

`MetadataRegistry::build(container)` produces one `SectionDescriptor` per section
(id, name, flags, row count, field list) and a `SectionIndex` over them. `by_id`
and `by_name` return the descriptor in O(1). Because descriptors and the index
are built in container order, an index slot is also the descriptor slot. This is
the shared introspection snapshot the semantic/constraint validators, converter,
and statistics passes consume.
