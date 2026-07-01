#pragma once

#include "tessera/core/types.hpp"

#include <string>
#include <vector>

namespace tessera::schema {

enum class FieldType : std::uint8_t {
    Bool = 1,
    U8 = 2,
    I32 = 3,
    U32 = 4,
    I64 = 5,
    U64 = 6,
    F64 = 7,
    Utf8 = 8,
    Bytes = 9,
    TimestampMs = 10,
};

struct FieldDescriptor {
    std::string name;
    FieldType type{FieldType::U8};
    std::uint16_t max_length{0};
};

struct SchemaBlock {
    std::uint32_t block_crc32c{0};
    std::uint16_t section_id{0};
    std::vector<FieldDescriptor> fields;
};

[[nodiscard]] core::Result<SchemaBlock> parse(core::ByteSpan block);
[[nodiscard]] core::Result<std::vector<std::uint8_t>> serialize(const SchemaBlock& schema);

} // namespace tessera::schema
