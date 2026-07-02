#pragma once

#include "tessera/core/types.hpp"
#include "tessera/index/string_index.hpp"
#include "tessera/schema/schema_block.hpp"

#include <cstddef>
#include <string_view>
#include <vector>

namespace tessera::schema {

// A field with its layout precomputed so record validation and (later) fast
// decoding avoid re-deriving type properties per row.
struct CompiledField {
    FieldDescriptor descriptor;
    bool fixed{true};
    std::size_t wire_size{0}; // fixed field byte size; length-prefix size for variable
};

// A schema with per-field layout resolved and a name index attached. Produced
// once per section by compile(); consumed by the semantic and constraint
// validators and by the statistics pass.
class CompiledSchema {
public:
    [[nodiscard]] std::uint16_t section_id() const noexcept { return section_id_; }
    [[nodiscard]] const std::vector<CompiledField>& fields() const noexcept { return fields_; }
    [[nodiscard]] std::size_t field_count() const noexcept { return fields_.size(); }

    // Smallest possible encoded size of a single row: fixed fields contribute
    // their width, variable fields contribute only their length prefix. A row
    // count whose product with this exceeds the section payload is impossible.
    [[nodiscard]] std::size_t min_row_size() const noexcept { return min_row_size_; }
    [[nodiscard]] bool all_fixed() const noexcept { return all_fixed_; }

    [[nodiscard]] const CompiledField* find_field(std::string_view name) const;

    friend core::Result<CompiledSchema> compile(const SchemaBlock& schema);

private:
    std::uint16_t section_id_{0};
    std::vector<CompiledField> fields_;
    index::StringIndex field_by_name_;
    std::size_t min_row_size_{0};
    bool all_fixed_{true};
};

// Compiles a schema, rejecting duplicate field names. The resulting layout is
// pure metadata — it never touches record bytes.
[[nodiscard]] core::Result<CompiledSchema> compile(const SchemaBlock& schema);

} // namespace tessera::schema
