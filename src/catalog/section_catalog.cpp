#include "tessera/catalog/section_catalog.hpp"

#include "tessera/buffer/reader_writer.hpp"
#include "tessera/checksum/crc32c.hpp"
#include "tessera/core/constants.hpp"

namespace tessera::catalog {

namespace {

core::Result<void> validate_entries(const Catalog& catalog, std::uint64_t container_size) {
    std::vector<std::uint16_t> seen_ids;
    std::vector<std::string> seen_names;

    for (const SectionEntry& entry : catalog.entries) {
        if (entry.name.empty() || entry.name.size() > core::kMaxSectionNameLength) {
            return core::Error{core::ErrorCode::InvalidLength, "invalid section name length", 0};
        }
        if (entry.offset + entry.length < entry.offset) {
            return core::Error{core::ErrorCode::SectionOutOfBounds, "section offset overflow", 0};
        }
        if (entry.offset + entry.length > container_size) {
            return core::Error{core::ErrorCode::SectionOutOfBounds, "section extends past container", 0};
        }
        for (std::uint16_t seen : seen_ids) {
            if (seen == entry.id) {
                return core::Error{core::ErrorCode::DuplicateSectionId, "duplicate section id", 0};
            }
        }
        seen_ids.push_back(entry.id);
        for (const std::string& seen : seen_names) {
            if (seen == entry.name) {
                return core::Error{core::ErrorCode::DuplicateSectionName, "duplicate section name", 0};
            }
        }
        seen_names.push_back(entry.name);
    }
    return core::Ok();
}

} // namespace

core::Result<Catalog> parse(core::ByteSpan block, std::uint64_t container_size) {
    if (block.size < 8) {
        return core::Error{core::ErrorCode::TruncatedInput, "catalog block truncated", 0};
    }

    const std::uint32_t block_length = core::read_le_u32(block.data);
    if (block_length > block.size || block_length < 8) {
        return core::Error{core::ErrorCode::InvalidLength, "catalog block length invalid", 0};
    }

    const core::ByteSpan bounded{block.data, block_length};
    buffer::Reader reader(bounded);
    Catalog catalog{};

    if (auto length_field = reader.read_u32(); length_field) {
        if (*length_field != block_length) {
            return core::Error{core::ErrorCode::InvalidLength, "catalog length field mismatch", 0};
        }
    } else {
        return length_field.error();
    }

    if (auto crc = reader.read_u32(); crc) {
        catalog.block_crc32c = *crc;
    } else {
        return crc.error();
    }

    const core::ByteSpan payload{bounded.data + 8, bounded.size - 8};
    if (auto verified = checksum::verify(payload, catalog.block_crc32c); !verified) {
        return verified.error();
    }

    if (auto entry_count = reader.read_u32(); entry_count) {
        if (*entry_count > core::kMaxSectionCount) {
            return core::Error{core::ErrorCode::InvalidLength, "catalog entry count exceeds limit", 0};
        }
        catalog.entries.reserve(*entry_count);
        for (std::uint32_t index = 0; index < *entry_count; ++index) {
            SectionEntry entry{};
            if (auto id = reader.read_u16(); id) {
                entry.id = *id;
            } else {
                return id.error();
            }
            if (auto name_length = reader.read_u8(); name_length) {
                if (*name_length > core::kMaxSectionNameLength) {
                    return core::Error{core::ErrorCode::InvalidLength, "section name too long", reader.position()};
                }
                if (auto name = reader.read_string(*name_length); name) {
                    entry.name = std::move(*name);
                } else {
                    return name.error();
                }
            } else {
                return name_length.error();
            }
            if (auto offset = reader.read_u64(); offset) {
                entry.offset = *offset;
            } else {
                return offset.error();
            }
            if (auto length = reader.read_u64(); length) {
                entry.length = *length;
            } else {
                return length.error();
            }
            if (auto flags = reader.read_u16(); flags) {
                entry.flags = *flags;
            } else {
                return flags.error();
            }
            catalog.entries.push_back(std::move(entry));
        }
    } else {
        return entry_count.error();
    }

    if (auto valid = validate_entries(catalog, container_size); !valid) {
        return valid.error();
    }
    return catalog;
}

core::Result<std::vector<std::uint8_t>> serialize(const Catalog& catalog) {
    std::vector<std::uint8_t> body(4);
    buffer::Writer body_writer(core::MutableByteSpan{body.data(), body.size()});
    if (auto status = body_writer.write_u32(static_cast<std::uint32_t>(catalog.entries.size())); !status) {
        return status.error();
    }

    for (const SectionEntry& entry : catalog.entries) {
        std::vector<std::uint8_t> scratch(2 + 1 + entry.name.size() + 8 + 8 + 2);
        buffer::Writer entry_writer(core::MutableByteSpan{scratch.data(), scratch.size()});
        if (auto status = entry_writer.write_u16(entry.id); !status) {
            return status.error();
        }
        if (entry.name.size() > core::kMaxSectionNameLength) {
            return core::Error{core::ErrorCode::InvalidLength, "section name too long", 0};
        }
        if (auto status = entry_writer.write_u8(static_cast<std::uint8_t>(entry.name.size())); !status) {
            return status.error();
        }
        if (auto status = entry_writer.write_string(entry.name); !status) {
            return status.error();
        }
        if (auto status = entry_writer.write_u64(entry.offset); !status) {
            return status.error();
        }
        if (auto status = entry_writer.write_u64(entry.length); !status) {
            return status.error();
        }
        if (auto status = entry_writer.write_u16(entry.flags); !status) {
            return status.error();
        }
        body.insert(body.end(), scratch.begin(), scratch.begin() + static_cast<std::ptrdiff_t>(entry_writer.written()));
    }

    const std::uint32_t crc = checksum::crc32c(core::ByteSpan{body.data(), body.size()});
    std::vector<std::uint8_t> output(8 + body.size());
    buffer::Writer writer(core::MutableByteSpan{output.data(), output.size()});
    const std::uint32_t block_length = static_cast<std::uint32_t>(8 + body.size());
    if (auto status = writer.write_u32(block_length); !status) {
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

} // namespace tessera::catalog
