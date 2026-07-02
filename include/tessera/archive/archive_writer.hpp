#pragma once

#include "tessera/archive/archive_format.hpp"
#include "tessera/compress/codec.hpp"
#include "tessera/core/types.hpp"
#include "tessera/model/container.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tessera::archive {

// Builds a TBA archive from a set of containers. Each container is serialized to
// canonical TBC bytes, wrapped in a compression frame, and recorded in the
// directory with a checksum of its uncompressed form. build() lays out the
// header, directory, and frame region and computes all offsets.
class ArchiveWriter {
public:
    ArchiveWriter() = default;

    void set_created(std::uint64_t created_unix_ms) noexcept { created_ = created_unix_ms; }

    // Serializes and frames a container under the given name/id. Returns an
    // error if the name is invalid or the container cannot be serialized.
    [[nodiscard]] core::Result<void> add_container(
        std::string name,
        std::uint16_t id,
        const model::Container& container,
        compress::CodecId codec = compress::CodecId::Lz77);

    [[nodiscard]] core::Result<std::vector<std::uint8_t>> build() const;

    [[nodiscard]] std::size_t size() const noexcept { return pending_.size(); }

private:
    struct Pending {
        std::uint16_t id{0};
        std::string name;
        std::vector<std::uint8_t> frame;
        std::uint32_t content_crc32c{0};
    };

    std::vector<Pending> pending_;
    std::uint64_t created_{0};
};

} // namespace tessera::archive
