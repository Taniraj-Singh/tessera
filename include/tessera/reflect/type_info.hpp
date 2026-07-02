#pragma once

#include "tessera/schema/schema_block.hpp"

#include <cstddef>
#include <string_view>

namespace tessera::reflect {

// Runtime description of a schema field type. Lets generic code (validators,
// converters, statistics) reason about a field's wire footprint and category
// without a switch on FieldType at every call site.
struct TypeInfo {
    schema::FieldType type{schema::FieldType::U8};
    std::string_view name;
    bool fixed_size{true};    // false for length-prefixed Utf8/Bytes
    std::size_t wire_size{0}; // encoded byte size for fixed types; 0 if variable
    bool numeric{false};
    bool is_signed{false};
    bool aggregate{false};    // string or byte blob
};

[[nodiscard]] const TypeInfo& type_info(schema::FieldType type) noexcept;

// Encoded size of a fixed-width field, or 0 for variable-length fields. Handy
// for pre-computing a row's minimum footprint.
[[nodiscard]] std::size_t fixed_wire_size(schema::FieldType type) noexcept;

[[nodiscard]] std::string_view type_name(schema::FieldType type) noexcept;

} // namespace tessera::reflect
