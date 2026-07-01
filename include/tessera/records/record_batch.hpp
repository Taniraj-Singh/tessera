#pragma once

#include "tessera/core/types.hpp"
#include "tessera/schema/schema_block.hpp"

#include <string>
#include <variant>
#include <vector>

namespace tessera::records {

using FieldValue = std::variant<
    bool,
    std::uint8_t,
    std::int32_t,
    std::uint32_t,
    std::int64_t,
    std::uint64_t,
    double,
    std::string,
    std::vector<std::uint8_t>>;

struct RecordRow {
    std::vector<FieldValue> values;
};

struct RecordBatch {
    std::uint32_t block_crc32c{0};
    std::uint16_t section_id{0};
    std::uint32_t row_count{0};
    std::vector<RecordRow> rows;
};

[[nodiscard]] core::Result<RecordBatch> decode(core::ByteSpan block, const schema::SchemaBlock& schema);
[[nodiscard]] core::Result<std::vector<std::uint8_t>> encode(const RecordBatch& batch, const schema::SchemaBlock& schema);

} // namespace tessera::records
