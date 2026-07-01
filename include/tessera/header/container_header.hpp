#pragma once

#include "tessera/core/constants.hpp"
#include "tessera/core/types.hpp"

namespace tessera::header {

struct ContainerHeader {
    std::uint32_t magic{core::kContainerMagic};
    std::uint16_t version{core::kContainerVersion};
    std::uint16_t flags{0};
    std::uint64_t created_unix_ms{0};
    std::uint32_t section_count{0};
    std::uint32_t header_crc32c{0};
};

[[nodiscard]] core::Result<ContainerHeader> parse(core::ByteSpan input);
[[nodiscard]] core::Result<std::vector<std::uint8_t>> serialize(const ContainerHeader& header);

} // namespace tessera::header
