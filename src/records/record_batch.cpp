#include "tessera/records/record_batch.hpp"

#include "tessera/buffer/reader_writer.hpp"
#include "tessera/checksum/crc32c.hpp"
#include "tessera/core/constants.hpp"
#include "tessera/schema/schema_compiler.hpp"

#include <cstring>

namespace tessera::records {

namespace {

// Decodes one fixed-width field with a direct little-endian read, advancing the
// cursor by the field's on-wire width. Used only by the fixed-layout fast path,
// which has already validated that the row region holds every row; the variable
// types never reach here.
FieldValue read_fixed_field(schema::FieldType type, const std::uint8_t*& cursor) {
    switch (type) {
    case schema::FieldType::Bool: {
        const bool value = *cursor != 0;
        cursor += 1;
        return FieldValue{value};
    }
    case schema::FieldType::U8: {
        const std::uint8_t value = *cursor;
        cursor += 1;
        return FieldValue{value};
    }
    case schema::FieldType::I32: {
        const std::uint32_t raw = core::read_le_u32(cursor);
        cursor += 4;
        return FieldValue{static_cast<std::int32_t>(raw)};
    }
    case schema::FieldType::U32: {
        const std::uint32_t raw = core::read_le_u32(cursor);
        cursor += 4;
        return FieldValue{raw};
    }
    case schema::FieldType::I64: {
        const std::uint64_t raw = core::read_le_u64(cursor);
        cursor += 8;
        return FieldValue{static_cast<std::int64_t>(raw)};
    }
    case schema::FieldType::U64: {
        const std::uint64_t raw = core::read_le_u64(cursor);
        cursor += 8;
        return FieldValue{raw};
    }
    case schema::FieldType::F64: {
        const std::uint64_t bits = core::read_le_u64(cursor);
        cursor += 8;
        double value = 0.0;
        std::memcpy(&value, &bits, sizeof(value));
        return FieldValue{value};
    }
    case schema::FieldType::TimestampMs: {
        const std::uint64_t raw = core::read_le_u64(cursor);
        cursor += 8;
        return FieldValue{raw};
    }
    case schema::FieldType::Utf8:
    case schema::FieldType::Bytes:
        break;
    }
    return FieldValue{std::uint8_t{0}};
}

core::Result<FieldValue> read_field(buffer::Reader& reader, const schema::FieldDescriptor& field) {
    switch (field.type) {
    case schema::FieldType::Bool: {
        auto value = reader.read_u8();
        if (!value) {
            return value.error();
        }
        return FieldValue{static_cast<bool>(*value != 0)};
    }
    case schema::FieldType::U8: {
        auto value = reader.read_u8();
        if (!value) {
            return value.error();
        }
        return FieldValue{*value};
    }
    case schema::FieldType::I32: {
        auto value = reader.read_u32();
        if (!value) {
            return value.error();
        }
        return FieldValue{static_cast<std::int32_t>(*value)};
    }
    case schema::FieldType::U32: {
        auto value = reader.read_u32();
        if (!value) {
            return value.error();
        }
        return FieldValue{*value};
    }
    case schema::FieldType::I64: {
        auto value = reader.read_u64();
        if (!value) {
            return value.error();
        }
        return FieldValue{static_cast<std::int64_t>(*value)};
    }
    case schema::FieldType::U64: {
        auto value = reader.read_u64();
        if (!value) {
            return value.error();
        }
        return FieldValue{*value};
    }
    case schema::FieldType::F64: {
        auto bits = reader.read_u64();
        if (!bits) {
            return bits.error();
        }
        double value = 0.0;
        static_assert(sizeof(value) == sizeof(*bits));
        std::memcpy(&value, &(*bits), sizeof(value));
        return FieldValue{value};
    }
    case schema::FieldType::Utf8: {
        auto length = reader.read_u16();
        if (!length) {
            return length.error();
        }
        if (field.max_length != 0 && *length > field.max_length) {
            return core::Error{core::ErrorCode::InvalidLength, "utf8 field exceeds max length", reader.position()};
        }
        auto text = reader.read_string(*length);
        if (!text) {
            return text.error();
        }
        return FieldValue{std::move(*text)};
    }
    case schema::FieldType::Bytes: {
        auto length = reader.read_u32();
        if (!length) {
            return length.error();
        }
        if (field.max_length != 0 && *length > field.max_length) {
            return core::Error{core::ErrorCode::InvalidLength, "bytes field exceeds max length", reader.position()};
        }
        auto data = reader.read_bytes(*length);
        if (!data) {
            return data.error();
        }
        return FieldValue{std::move(*data)};
    }
    case schema::FieldType::TimestampMs: {
        auto value = reader.read_u64();
        if (!value) {
            return value.error();
        }
        return FieldValue{*value};
    }
    }
    return core::Error{core::ErrorCode::InvalidEnum, "unsupported field type", reader.position()};
}

core::Result<void> write_field(buffer::Writer& writer, const schema::FieldDescriptor& field, const FieldValue& value) {
    switch (field.type) {
    case schema::FieldType::Bool: {
        const bool raw = std::get<bool>(value);
        return writer.write_u8(raw ? 1U : 0U);
    }
    case schema::FieldType::U8:
        return writer.write_u8(std::get<std::uint8_t>(value));
    case schema::FieldType::I32:
        return writer.write_u32(static_cast<std::uint32_t>(std::get<std::int32_t>(value)));
    case schema::FieldType::U32:
        return writer.write_u32(std::get<std::uint32_t>(value));
    case schema::FieldType::I64:
        return writer.write_u64(static_cast<std::uint64_t>(std::get<std::int64_t>(value)));
    case schema::FieldType::U64:
        return writer.write_u64(std::get<std::uint64_t>(value));
    case schema::FieldType::F64: {
        const double raw = std::get<double>(value);
        std::uint64_t bits = 0;
        static_assert(sizeof(bits) == sizeof(raw));
        std::memcpy(&bits, &raw, sizeof(raw));
        return writer.write_u64(bits);
    }
    case schema::FieldType::Utf8: {
        const std::string& text = std::get<std::string>(value);
        if (field.max_length != 0 && text.size() > field.max_length) {
            return core::Error{core::ErrorCode::InvalidLength, "utf8 field exceeds max length", writer.written()};
        }
        if (auto status = writer.write_u16(static_cast<std::uint16_t>(text.size())); !status) {
            return status.error();
        }
        return writer.write_string(text);
    }
    case schema::FieldType::Bytes: {
        const std::vector<std::uint8_t>& bytes = std::get<std::vector<std::uint8_t>>(value);
        if (field.max_length != 0 && bytes.size() > field.max_length) {
            return core::Error{core::ErrorCode::InvalidLength, "bytes field exceeds max length", writer.written()};
        }
        if (auto status = writer.write_u32(static_cast<std::uint32_t>(bytes.size())); !status) {
            return status.error();
        }
        return writer.write_bytes(core::ByteSpan{bytes.data(), bytes.size()});
    }
    case schema::FieldType::TimestampMs:
        return writer.write_u64(std::get<std::uint64_t>(value));
    }
    return core::Error{core::ErrorCode::InvalidEnum, "unsupported field type", writer.written()};
}

} // namespace

core::Result<RecordBatch> decode(core::ByteSpan block, const schema::SchemaBlock& schema) {
    if (block.size < 8) {
        return core::Error{core::ErrorCode::TruncatedInput, "record batch truncated", 0};
    }

    const std::uint32_t block_length = core::read_le_u32(block.data);
    if (block_length > block.size || block_length < 8) {
        return core::Error{core::ErrorCode::InvalidLength, "record batch length invalid", 0};
    }

    const core::ByteSpan bounded{block.data, block_length};
    buffer::Reader reader(bounded);
    RecordBatch batch{};

    if (auto length_field = reader.read_u32(); length_field) {
        if (*length_field != block_length) {
            return core::Error{core::ErrorCode::InvalidLength, "record batch length field mismatch", 0};
        }
    } else {
        return length_field.error();
    }

    if (auto crc = reader.read_u32(); crc) {
        batch.block_crc32c = *crc;
    } else {
        return crc.error();
    }

    const core::ByteSpan payload{bounded.data + 8, bounded.size - 8};
    if (auto verified = checksum::verify(payload, batch.block_crc32c); !verified) {
        return verified.error();
    }

    if (auto section_id = reader.read_u16(); section_id) {
        batch.section_id = *section_id;
    } else {
        return section_id.error();
    }
    if (batch.section_id != schema.section_id) {
        return core::Error{core::ErrorCode::SchemaMismatch, "record batch section id mismatch", reader.position()};
    }

    if (auto row_count = reader.read_u32(); row_count) {
        batch.row_count = *row_count;
    } else {
        return row_count.error();
    }

    // Fixed-layout fast path. When every field is fixed-width the row stride is
    // a constant, so the whole row region can be bounds-checked in a single
    // comparison and each row decoded with direct little-endian reads instead of
    // the per-field bounded Reader. The compiled schema supplies the stride;
    // variable-width schemas fall through to the general path below.
    if (auto compiled = schema::compile(schema); compiled && compiled->all_fixed()) {
        const std::size_t stride = compiled->min_row_size();
        const std::size_t available = bounded.size - reader.position();
        if (stride != 0 && batch.row_count <= available / stride) {
            const std::uint8_t* cursor = bounded.data + reader.position();
            batch.rows.reserve(batch.row_count);
            for (std::uint32_t row_index = 0; row_index < batch.row_count; ++row_index) {
                RecordRow row{};
                row.values.reserve(schema.fields.size());
                for (const schema::FieldDescriptor& field : schema.fields) {
                    row.values.push_back(read_fixed_field(field.type, cursor));
                }
                batch.rows.push_back(std::move(row));
            }
            return batch;
        }
    }

    batch.rows.reserve(batch.row_count);
    for (std::uint32_t row_index = 0; row_index < batch.row_count; ++row_index) {
        RecordRow row{};
        row.values.reserve(schema.fields.size());
        for (const schema::FieldDescriptor& field : schema.fields) {
            if (auto value = read_field(reader, field); value) {
                row.values.push_back(std::move(*value));
            } else {
                return value.error();
            }
        }
        batch.rows.push_back(std::move(row));
    }

    return batch;
}

namespace {

std::size_t field_scratch_capacity(const schema::FieldDescriptor& field) {
    switch (field.type) {
    case schema::FieldType::Utf8:
        return 2U + (field.max_length != 0 ? field.max_length : core::kMaxSectionNameLength);
    case schema::FieldType::Bytes:
        return 4U + (field.max_length != 0 ? field.max_length : core::kMaxBlockSize);
    default:
        return 16U;
    }
}

} // namespace

core::Result<std::vector<std::uint8_t>> encode(const RecordBatch& batch, const schema::SchemaBlock& schema) {
    if (batch.section_id != schema.section_id) {
        return core::Error{core::ErrorCode::SchemaMismatch, "record batch section id mismatch", 0};
    }
    if (batch.row_count != batch.rows.size()) {
        return core::Error{core::ErrorCode::RecordCountMismatch, "row count does not match rows vector", 0};
    }

    std::vector<std::uint8_t> body;
    {
        std::vector<std::uint8_t> scratch(8);
        buffer::Writer header_writer(core::MutableByteSpan{scratch.data(), scratch.size()});
        if (auto status = header_writer.write_u16(batch.section_id); !status) {
            return status.error();
        }
        if (auto status = header_writer.write_u32(batch.row_count); !status) {
            return status.error();
        }
        body.insert(body.end(), scratch.begin(), scratch.begin() + static_cast<std::ptrdiff_t>(header_writer.written()));
    }

    for (const RecordRow& row : batch.rows) {
        if (row.values.size() != schema.fields.size()) {
            return core::Error{core::ErrorCode::SchemaMismatch, "row field count mismatch", body.size()};
        }
        for (std::size_t index = 0; index < schema.fields.size(); ++index) {
            const schema::FieldDescriptor& field = schema.fields[index];
            std::vector<std::uint8_t> scratch(field_scratch_capacity(field));
            buffer::Writer field_writer(core::MutableByteSpan{scratch.data(), scratch.size()});
            if (auto status = write_field(field_writer, field, row.values[index]); !status) {
                return status.error();
            }
            body.insert(body.end(), scratch.begin(), scratch.begin() + static_cast<std::ptrdiff_t>(field_writer.written()));
        }
    }

    const std::uint32_t crc = checksum::crc32c(core::ByteSpan{body.data(), body.size()});
    std::vector<std::uint8_t> output(8 + body.size());
    buffer::Writer writer(core::MutableByteSpan{output.data(), output.size()});
    if (auto status = writer.write_u32(static_cast<std::uint32_t>(output.size())); !status) {
        return status.error();
    }
    if (auto status = writer.write_u32(crc); !status) {
        return status.error();
    }
    if (auto status = writer.write_bytes(core::ByteSpan{body.data(), body.size()}); !status) {
        return status.error();
    }
    return output;
}

} // namespace tessera::records
