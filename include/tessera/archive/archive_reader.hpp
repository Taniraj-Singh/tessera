#pragma once

#include "tessera/archive/archive_format.hpp"
#include "tessera/core/types.hpp"
#include "tessera/model/container.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace tessera::archive {

// Read-only view over a TBA byte range. open() validates the header and the
// directory (magic, entry-count bound, directory CRC, and per-entry frame
// bounds) up front; the frame bytes themselves are decompressed lazily by
// read_entry / parse_entry so a caller can pull a single container without
// touching the others.
class ArchiveReader {
public:
    [[nodiscard]] static core::Result<ArchiveReader> open(core::ByteSpan archive);

    [[nodiscard]] const ArchiveHeader& header() const noexcept { return header_; }
    [[nodiscard]] const std::vector<ArchiveEntry>& entries() const noexcept { return entries_; }
    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }

    // Decompresses entry `index` and returns the raw container bytes, verifying
    // the content CRC.
    [[nodiscard]] core::Result<std::vector<std::uint8_t>> read_entry(std::size_t index) const;

    // Decompresses and fully parses entry `index` into a container model.
    [[nodiscard]] core::Result<model::Container> parse_entry(std::size_t index) const;

    [[nodiscard]] const ArchiveEntry* find(std::string_view name) const;

private:
    ArchiveReader() = default;

    core::ByteSpan archive_{};
    ArchiveHeader header_{};
    std::vector<ArchiveEntry> entries_;
};

} // namespace tessera::archive
