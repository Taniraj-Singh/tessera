#pragma once

#include "tessera/schema/schema_block.hpp"

#include <string>
#include <vector>

namespace tessera::schema {

// Type compatibility rules for schema evolution. When a reader built for one
// schema encounters a container written against a newer schema, these rules
// decide whether the newer layout can still be read (widening) or is a breaking
// change.
enum class Compatibility {
    Identical,    // byte-for-byte the same field
    Compatible,   // differs but a reader of the old schema can still consume it
    Incompatible, // breaking change
};

// True when a value of type `from` can be represented without loss in `to`
// (e.g. U8 -> U32, I32 -> I64, U32 -> F64). Reflexive.
[[nodiscard]] bool is_widening(FieldType from, FieldType to) noexcept;

// Compares two descriptors occupying the same position in old/new schemas.
[[nodiscard]] Compatibility compare_field(const FieldDescriptor& old_field, const FieldDescriptor& new_field) noexcept;

struct SchemaDiff {
    Compatibility overall{Compatibility::Identical};
    std::vector<std::string> notes;
};

// Diffs two schemas for the same section. Appended trailing fields are
// Compatible; removed or narrowed fields are Incompatible. The overall result
// is the worst per-field outcome.
[[nodiscard]] SchemaDiff compare_schema(const SchemaBlock& old_schema, const SchemaBlock& new_schema);

[[nodiscard]] Compatibility worse_of(Compatibility lhs, Compatibility rhs) noexcept;

} // namespace tessera::schema
