#pragma once

#include "tessera/records/record_batch.hpp"
#include "tessera/reflect/type_info.hpp"
#include "tessera/schema/schema_block.hpp"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace tessera::reflect {

// Non-owning view pairing a schema field descriptor with one decoded value.
// Provides typed accessors over the records::FieldValue variant and a
// consistency check between the declared type and the stored alternative, which
// the semantic validator relies on.
class FieldRef {
public:
    FieldRef(const schema::FieldDescriptor& descriptor, const records::FieldValue& value) noexcept
        : descriptor_(&descriptor), value_(&value) {}

    [[nodiscard]] schema::FieldType type() const noexcept { return descriptor_->type; }
    [[nodiscard]] const std::string& name() const noexcept { return descriptor_->name; }
    [[nodiscard]] const TypeInfo& info() const noexcept { return type_info(descriptor_->type); }

    [[nodiscard]] const bool* as_bool() const noexcept { return std::get_if<bool>(value_); }
    [[nodiscard]] const std::uint8_t* as_u8() const noexcept { return std::get_if<std::uint8_t>(value_); }
    [[nodiscard]] const std::int32_t* as_i32() const noexcept { return std::get_if<std::int32_t>(value_); }
    [[nodiscard]] const std::uint32_t* as_u32() const noexcept { return std::get_if<std::uint32_t>(value_); }
    [[nodiscard]] const std::int64_t* as_i64() const noexcept { return std::get_if<std::int64_t>(value_); }
    [[nodiscard]] const std::uint64_t* as_u64() const noexcept { return std::get_if<std::uint64_t>(value_); }
    [[nodiscard]] const double* as_f64() const noexcept { return std::get_if<double>(value_); }
    [[nodiscard]] const std::string* as_string() const noexcept { return std::get_if<std::string>(value_); }
    [[nodiscard]] const std::vector<std::uint8_t>* as_bytes() const noexcept {
        return std::get_if<std::vector<std::uint8_t>>(value_);
    }

    // True when the stored variant alternative matches the declared field type.
    // Both I64/U64 and the TimestampMs alias resolve to their integer storage.
    [[nodiscard]] bool matches_type() const noexcept {
        switch (descriptor_->type) {
        case schema::FieldType::Bool:
            return as_bool() != nullptr;
        case schema::FieldType::U8:
            return as_u8() != nullptr;
        case schema::FieldType::I32:
            return as_i32() != nullptr;
        case schema::FieldType::U32:
            return as_u32() != nullptr;
        case schema::FieldType::I64:
            return as_i64() != nullptr;
        case schema::FieldType::U64:
        case schema::FieldType::TimestampMs:
            return as_u64() != nullptr;
        case schema::FieldType::F64:
            return as_f64() != nullptr;
        case schema::FieldType::Utf8:
            return as_string() != nullptr;
        case schema::FieldType::Bytes:
            return as_bytes() != nullptr;
        }
        return false;
    }

    // Byte footprint of the value as it would be encoded, including any length
    // prefix. Used by the statistics subsystem to estimate section sizes.
    [[nodiscard]] std::size_t encoded_size() const noexcept {
        switch (descriptor_->type) {
        case schema::FieldType::Utf8: {
            const std::string* text = as_string();
            return 2U + (text != nullptr ? text->size() : 0U);
        }
        case schema::FieldType::Bytes: {
            const std::vector<std::uint8_t>* blob = as_bytes();
            return 4U + (blob != nullptr ? blob->size() : 0U);
        }
        default:
            return fixed_wire_size(descriptor_->type);
        }
    }

private:
    const schema::FieldDescriptor* descriptor_;
    const records::FieldValue* value_;
};

} // namespace tessera::reflect
