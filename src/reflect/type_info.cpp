#include "tessera/reflect/type_info.hpp"

#include <array>

namespace tessera::reflect {

namespace {

// Indexed by FieldType value (1..10). Slot 0 is an "unknown" sentinel so an
// out-of-range enum degrades to a safe, variable-size descriptor rather than
// reading past the table.
constexpr std::array<TypeInfo, 11> kTypeTable = {{
    {schema::FieldType::U8, "unknown", false, 0, false, false, false},
    {schema::FieldType::Bool, "bool", true, 1, false, false, false},
    {schema::FieldType::U8, "u8", true, 1, true, false, false},
    {schema::FieldType::I32, "i32", true, 4, true, true, false},
    {schema::FieldType::U32, "u32", true, 4, true, false, false},
    {schema::FieldType::I64, "i64", true, 4, true, true, false},
    {schema::FieldType::U64, "u64", true, 8, true, false, false},
    {schema::FieldType::F64, "f64", true, 8, true, true, false},
    {schema::FieldType::Utf8, "utf8", false, 0, false, false, true},
    {schema::FieldType::Bytes, "bytes", false, 0, false, false, true},
    {schema::FieldType::TimestampMs, "timestamp_ms", true, 8, true, false, false},
}};

std::size_t table_slot(schema::FieldType type) noexcept {
    const auto raw = static_cast<std::size_t>(type);
    return raw < kTypeTable.size() ? raw : 0;
}

} // namespace

const TypeInfo& type_info(schema::FieldType type) noexcept {
    return kTypeTable[table_slot(type)];
}

std::size_t fixed_wire_size(schema::FieldType type) noexcept {
    const TypeInfo& info = type_info(type);
    return info.fixed_size ? info.wire_size : 0;
}

std::string_view type_name(schema::FieldType type) noexcept {
    return type_info(type).name;
}

} // namespace tessera::reflect
