#include "tessera/schema/schema_block.hpp"

#include "tessera/buffer/reader_writer.hpp"
#include "tessera/checksum/crc32c.hpp"
#include "tessera/core/constants.hpp"

namespace tessera::schema {

namespace {

core::Result<FieldType> parse_field_type(std::uint8_t raw) {
    switch (raw) {
    case static_cast<std::uint8_t>(FieldType::Bool):
    case static_cast<std::uint8_t>(FieldType::U8):
    case static_cast<std::uint8_t>(FieldType::I32):
    case static_cast<std::uint8_t>(FieldType::U32):
    case static_cast<std::uint8_t>(FieldType::I64):
    case static_cast<std::uint8_t>(FieldType::U64):
    case static_cast<std::uint8_t>(FieldType::F64):
    case static_cast<std::uint8_t>(FieldType::Utf8):
    case static_cast<std::uint8_t>(FieldType::Bytes):
    case static_cast<std::uint8_t>(FieldType::TimestampMs):
        return static_cast<FieldType>(raw);
    default:
        return core::Error{core::ErrorCode::InvalidEnum, "unknown field type", 0};
    }
}

} // namespace

core::Result<SchemaBlock> parse(core::ByteSpan block) {
    if (block.size < 8) {
        return core::Error{core::ErrorCode::TruncatedInput, "schema block truncated", 0};
    }

    const std::uint32_t block_length = core::read_le_u32(block.data);
    if (block_length > block.size || block_length < 8) {
        return core::Error{core::ErrorCode::InvalidLength, "schema block length invalid", 0};
    }

    const core::ByteSpan bounded{block.data, block_length};
    buffer::Reader reader(bounded);
    SchemaBlock schema{};

    if (auto length_field = reader.read_u32(); length_field) {
        if (*length_field != block_length) {
            return core::Error{core::ErrorCode::InvalidLength, "schema length field mismatch", 0};
        }
    } else {
        return length_field.error();
    }

    if (auto crc = reader.read_u32(); crc) {
        schema.block_crc32c = *crc;
    } else {
        return crc.error();
    }

    const core::ByteSpan payload{bounded.data + 8, bounded.size - 8};
    if (auto verified = checksum::verify(payload, schema.block_crc32c); !verified) {
        return verified.error();
    }

    if (auto section_id = reader.read_u16(); section_id) {
        schema.section_id = *section_id;
    } else {
        return section_id.error();
    }

    if (auto field_count = reader.read_u16(); field_count) {
        schema.fields.reserve(*field_count);
        for (std::uint16_t index = 0; index < *field_count; ++index) {
            FieldDescriptor field{};
            if (auto name_length = reader.read_u8(); name_length) {
                if (*name_length == 0 || *name_length > core::kMaxSectionNameLength) {
                    return core::Error{core::ErrorCode::InvalidLength, "invalid field name length", reader.position()};
                }
                if (auto name = reader.read_string(*name_length); name) {
                    field.name = std::move(*name);
                } else {
                    return name.error();
                }
            } else {
                return name_length.error();
            }
            if (auto type = reader.read_u8(); type) {
                if (auto parsed = parse_field_type(*type); parsed) {
                    field.type = *parsed;
                } else {
                    return parsed.error();
                }
            } else {
                return type.error();
            }
            if (auto max_length = reader.read_u16(); max_length) {
                field.max_length = *max_length;
            } else {
                return max_length.error();
            }
            schema.fields.push_back(std::move(field));
        }
    } else {
        return field_count.error();
    }

    return schema;
}

core::Result<std::vector<std::uint8_t>> serialize(const SchemaBlock& schema) {
    std::vector<std::uint8_t> body(4);
    buffer::Writer body_writer(core::MutableByteSpan{body.data(), body.size()});
    if (auto status = body_writer.write_u16(schema.section_id); !status) {
        return status.error();
    }
    if (auto status = body_writer.write_u16(static_cast<std::uint16_t>(schema.fields.size())); !status) {
        return status.error();
    }

    for (const FieldDescriptor& field : schema.fields) {
        std::vector<std::uint8_t> scratch(1 + field.name.size() + 1 + 2);
        buffer::Writer field_writer(core::MutableByteSpan{scratch.data(), scratch.size()});
        if (field.name.empty() || field.name.size() > core::kMaxSectionNameLength) {
            return core::Error{core::ErrorCode::InvalidLength, "invalid field name length", 0};
        }
        if (auto status = field_writer.write_u8(static_cast<std::uint8_t>(field.name.size())); !status) {
            return status.error();
        }
        if (auto status = field_writer.write_string(field.name); !status) {
            return status.error();
        }
        if (auto status = field_writer.write_u8(static_cast<std::uint8_t>(field.type)); !status) {
            return status.error();
        }
        if (auto status = field_writer.write_u16(field.max_length); !status) {
            return status.error();
        }
        body.insert(body.end(), scratch.begin(), scratch.begin() + static_cast<std::ptrdiff_t>(field_writer.written()));
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

} // namespace tessera::schema
