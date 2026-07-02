#include "tessera/archive/archive_reader.hpp"

#include "tessera/buffer/reader_writer.hpp"
#include "tessera/checksum/crc32c.hpp"
#include "tessera/compress/frame.hpp"
#include "tessera/parse/parser.hpp"

namespace tessera::archive {

namespace {

core::Result<ArchiveHeader> parse_header(core::ByteSpan archive) {
    if (archive.size < kArchiveHeaderSize) {
        return core::Error{core::ErrorCode::TruncatedInput, "archive header truncated", archive.size};
    }

    buffer::Reader reader(archive);
    ArchiveHeader header{};
    if (auto magic = reader.read_u32(); magic) {
        header.magic = *magic;
    } else {
        return magic.error();
    }
    if (header.magic != kArchiveMagic) {
        return core::Error{core::ErrorCode::InvalidMagic, "invalid archive magic", 0};
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
    if (auto entry_count = reader.read_u32(); entry_count) {
        header.entry_count = *entry_count;
    } else {
        return entry_count.error();
    }
    if (auto directory_length = reader.read_u32(); directory_length) {
        header.directory_length = *directory_length;
    } else {
        return directory_length.error();
    }
    if (auto crc = reader.read_u32(); crc) {
        header.directory_crc32c = *crc;
    } else {
        return crc.error();
    }
    if (auto created = reader.read_u64(); created) {
        header.created_unix_ms = *created;
    } else {
        return created.error();
    }

    if (header.entry_count > kMaxArchiveEntries) {
        return core::Error{core::ErrorCode::InvalidLength, "archive entry count exceeds limit", 8};
    }
    return header;
}

} // namespace

core::Result<ArchiveReader> ArchiveReader::open(core::ByteSpan archive) {
    ArchiveReader reader{};
    reader.archive_ = archive;

    auto header = parse_header(archive);
    if (!header) {
        return header.error();
    }
    reader.header_ = *header;

    const std::size_t directory_end = kArchiveHeaderSize + reader.header_.directory_length;
    if (directory_end < kArchiveHeaderSize || directory_end > archive.size) {
        return core::Error{core::ErrorCode::SectionOutOfBounds, "archive directory out of bounds", kArchiveHeaderSize};
    }

    const core::ByteSpan directory{archive.data + kArchiveHeaderSize, reader.header_.directory_length};
    if (auto verified = checksum::verify(directory, reader.header_.directory_crc32c); !verified) {
        return verified.error();
    }

    buffer::Reader directory_reader(directory);
    reader.entries_.reserve(reader.header_.entry_count);
    for (std::uint32_t index = 0; index < reader.header_.entry_count; ++index) {
        ArchiveEntry entry{};
        if (auto id = directory_reader.read_u16(); id) {
            entry.id = *id;
        } else {
            return id.error();
        }
        if (auto name_length = directory_reader.read_u8(); name_length) {
            if (*name_length == 0 || *name_length > kMaxArchiveNameLength) {
                return core::Error{core::ErrorCode::InvalidLength, "invalid archive entry name length", directory_reader.position()};
            }
            if (auto name = directory_reader.read_string(*name_length); name) {
                entry.name = std::move(*name);
            } else {
                return name.error();
            }
        } else {
            return name_length.error();
        }
        if (auto offset = directory_reader.read_u64(); offset) {
            entry.frame_offset = *offset;
        } else {
            return offset.error();
        }
        if (auto length = directory_reader.read_u64(); length) {
            entry.frame_length = *length;
        } else {
            return length.error();
        }
        if (auto crc = directory_reader.read_u32(); crc) {
            entry.content_crc32c = *crc;
        } else {
            return crc.error();
        }

        // Frames live after the directory and must sit entirely within the
        // archive with no address wraparound.
        const std::uint64_t frame_end = entry.frame_offset + entry.frame_length;
        if (frame_end < entry.frame_offset) {
            return core::Error{core::ErrorCode::SectionOutOfBounds, "archive frame length overflow", 0};
        }
        if (entry.frame_offset < directory_end || frame_end > archive.size) {
            return core::Error{core::ErrorCode::SectionOutOfBounds, "archive frame outside container", 0};
        }
        reader.entries_.push_back(std::move(entry));
    }

    return reader;
}

core::Result<std::vector<std::uint8_t>> ArchiveReader::read_entry(std::size_t index) const {
    if (index >= entries_.size()) {
        return core::Error{core::ErrorCode::SectionOutOfBounds, "archive entry index out of range", index};
    }
    const ArchiveEntry& entry = entries_[index];
    const core::ByteSpan frame{
        archive_.data + entry.frame_offset,
        static_cast<std::size_t>(entry.frame_length),
    };

    auto container_bytes = compress::decompress_frame(frame);
    if (!container_bytes) {
        return container_bytes.error();
    }
    if (auto verified = checksum::verify(
            core::ByteSpan{container_bytes->data(), container_bytes->size()}, entry.content_crc32c);
        !verified) {
        return verified.error();
    }
    return container_bytes;
}

core::Result<model::Container> ArchiveReader::parse_entry(std::size_t index) const {
    auto bytes = read_entry(index);
    if (!bytes) {
        return bytes.error();
    }
    return parse::parse_container(core::ByteSpan{bytes->data(), bytes->size()});
}

const ArchiveEntry* ArchiveReader::find(std::string_view name) const {
    for (const ArchiveEntry& entry : entries_) {
        if (entry.name == name) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace tessera::archive
