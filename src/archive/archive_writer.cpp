#include "tessera/archive/archive_writer.hpp"

#include "tessera/checksum/crc32c.hpp"
#include "tessera/compress/frame.hpp"
#include "tessera/memory/byte_buffer.hpp"
#include "tessera/serialize/serializer.hpp"

namespace tessera::archive {

core::Result<void> ArchiveWriter::add_container(
    std::string name,
    std::uint16_t id,
    const model::Container& container,
    compress::CodecId codec) {
    if (name.empty() || name.size() > kMaxArchiveNameLength) {
        return core::Error{core::ErrorCode::InvalidLength, "invalid archive entry name", 0};
    }
    if (pending_.size() >= kMaxArchiveEntries) {
        return core::Error{core::ErrorCode::InvalidLength, "archive entry count exceeds limit", 0};
    }

    auto container_bytes = serialize::serialize_container(container, {});
    if (!container_bytes) {
        return container_bytes.error();
    }
    const core::ByteSpan content{container_bytes->data(), container_bytes->size()};

    auto frame = compress::compress_frame(content, codec);
    if (!frame) {
        return frame.error();
    }

    Pending pending{};
    pending.id = id;
    pending.name = std::move(name);
    pending.content_crc32c = checksum::crc32c(content);
    pending.frame = std::move(*frame);
    pending_.push_back(std::move(pending));
    return core::Ok();
}

core::Result<std::vector<std::uint8_t>> ArchiveWriter::build() const {
    std::size_t directory_length = 0;
    for (const Pending& pending : pending_) {
        directory_length += entry_record_size(pending.name.size());
    }

    const std::uint64_t frames_base = static_cast<std::uint64_t>(kArchiveHeaderSize) + directory_length;

    memory::ByteBuffer directory(directory_length);
    std::uint64_t offset = frames_base;
    for (const Pending& pending : pending_) {
        directory.append_u16(pending.id);
        directory.append_u8(static_cast<std::uint8_t>(pending.name.size()));
        directory.append_string(pending.name);
        directory.append_u64(offset);
        directory.append_u64(pending.frame.size());
        directory.append_u32(pending.content_crc32c);
        offset += pending.frame.size();
    }

    const std::uint32_t directory_crc = checksum::crc32c(directory.span());

    memory::ByteBuffer out(static_cast<std::size_t>(frames_base));
    out.append_u32(kArchiveMagic);
    out.append_u16(kArchiveVersion);
    out.append_u16(0);
    out.append_u32(static_cast<std::uint32_t>(pending_.size()));
    out.append_u32(static_cast<std::uint32_t>(directory_length));
    out.append_u32(directory_crc);
    out.append_u64(created_);
    out.append_bytes(directory.span());
    for (const Pending& pending : pending_) {
        out.append_bytes(core::ByteSpan{pending.frame.data(), pending.frame.size()});
    }

    return out.take();
}

} // namespace tessera::archive
