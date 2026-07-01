#pragma once

#include "tessera/core/types.hpp"

#include <string>
#include <vector>

namespace tessera::catalog {

struct SectionEntry {
    std::uint16_t id{0};
    std::string name;
    std::uint64_t offset{0};
    std::uint64_t length{0};
    std::uint16_t flags{0};
};

struct Catalog {
    std::uint32_t block_crc32c{0};
    std::vector<SectionEntry> entries;
};

[[nodiscard]] core::Result<Catalog> parse(core::ByteSpan block, std::uint64_t container_size);
[[nodiscard]] core::Result<std::vector<std::uint8_t>> serialize(const Catalog& catalog);

} // namespace tessera::catalog
