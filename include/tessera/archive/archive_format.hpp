#pragma once

#include "tessera/compress/codec.hpp"
#include "tessera/core/types.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace tessera::archive {

// Tessera Binary Archive (TBA) — a container-of-containers. An archive bundles
// several TBC containers, each stored as a self-describing compression frame,
// behind a directory that records where every frame lives and a checksum of its
// decompressed contents.
//
// Layout:
//   [fixed header] [directory block] [frame region]
//
// Reaching a record now traverses: archive header -> directory entry -> frame
// header -> codec decompress -> container header -> catalog -> schema -> record
// batch. That depth is the point: no single function reveals the full path.

inline constexpr std::uint32_t kArchiveMagic = 0x31414254U; // 'TBA1' little-endian
inline constexpr std::uint16_t kArchiveVersion = 0x0100U;
inline constexpr std::size_t kArchiveHeaderSize = 28;
inline constexpr std::size_t kMaxArchiveEntries = 4096;
inline constexpr std::size_t kMaxArchiveNameLength = 255;

struct ArchiveHeader {
    std::uint32_t magic{kArchiveMagic};
    std::uint16_t version{kArchiveVersion};
    std::uint16_t flags{0};
    std::uint32_t entry_count{0};
    std::uint32_t directory_length{0};
    std::uint32_t directory_crc32c{0};
    std::uint64_t created_unix_ms{0};
};

// One directory record. frame_offset/frame_length delimit the compression frame
// within the archive; content_crc32c guards the decompressed container bytes.
struct ArchiveEntry {
    std::uint16_t id{0};
    std::string name;
    std::uint64_t frame_offset{0};
    std::uint64_t frame_length{0};
    std::uint32_t content_crc32c{0};
};

// Serialized size of one directory record for the given name.
[[nodiscard]] inline std::size_t entry_record_size(std::size_t name_length) noexcept {
    return 2U + 1U + name_length + 8U + 8U + 4U;
}

} // namespace tessera::archive
