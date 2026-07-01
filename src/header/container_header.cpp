#include "tessera/header/container_header.hpp"

#include "tessera/buffer/reader_writer.hpp"
#include "tessera/checksum/crc32c.hpp"

namespace tessera::header {

namespace {

core::Result<void> validate_fields(const ContainerHeader& header) {
    if (header.magic != core::kContainerMagic) {
        return core::Error{core::ErrorCode::InvalidMagic, "invalid container magic", 0};
    }
    if (header.section_count > core::kMaxSectionCount) {
        return core::Error{core::ErrorCode::InvalidLength, "section count exceeds limit", 20};
    }
    return core::Ok();
}

} // namespace

core::Result<ContainerHeader> parse(core::ByteSpan input) {
    if (input.size < core::kHeaderSize) {
        return core::Error{core::ErrorCode::TruncatedInput, "header truncated", input.size};
    }

    buffer::Reader reader(input);
    ContainerHeader header{};
    if (auto magic = reader.read_u32(); magic) {
        header.magic = *magic;
    } else {
        return magic.error();
    }
    if (auto version = reader.read_u16(); version) {
        header.version = *version;
    } else {
        return version.error();
    }
    if (auto flags = reader.read_u16(); flags) {
        header.flags = *flags;
    } else {
        return flags.error();
    }
    if (auto created = reader.read_u64(); created) {
        header.created_unix_ms = *created;
    } else {
        return created.error();
    }
    if (auto count = reader.read_u32(); count) {
        header.section_count = *count;
    } else {
        return count.error();
    }
    if (auto crc = reader.read_u32(); crc) {
        header.header_crc32c = *crc;
    } else {
        return crc.error();
    }

    const core::ByteSpan payload{input.data, 20};
    if (auto verified = checksum::verify(payload, header.header_crc32c); !verified) {
        return verified.error();
    }

    if (auto valid = validate_fields(header); !valid) {
        return valid.error();
    }
    return header;
}

core::Result<std::vector<std::uint8_t>> serialize(const ContainerHeader& header) {
    std::vector<std::uint8_t> output(core::kHeaderSize, 0);
    buffer::Writer writer(core::MutableByteSpan{output.data(), output.size()});

    if (auto status = writer.write_u32(header.magic); !status) {
        return status.error();
    }
    if (auto status = writer.write_u16(header.version); !status) {
        return status.error();
    }
    if (auto status = writer.write_u16(header.flags); !status) {
        return status.error();
    }
    if (auto status = writer.write_u64(header.created_unix_ms); !status) {
        return status.error();
    }
    if (auto status = writer.write_u32(header.section_count); !status) {
        return status.error();
    }

    const core::ByteSpan payload{output.data(), 20};
    const std::uint32_t crc = checksum::crc32c(payload);
    if (auto status = writer.write_u32(crc); !status) {
        return status.error();
    }
    return output;
}

} // namespace tessera::header
