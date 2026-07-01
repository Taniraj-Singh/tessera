#pragma once

#include "tessera/core/constants.hpp"
#include "tessera/core/types.hpp"

namespace tessera::version {

struct VersionRange {
    std::uint16_t min_supported{core::kContainerVersion};
    std::uint16_t max_supported{core::kContainerVersion};
};

[[nodiscard]] core::Result<void> negotiate(std::uint16_t container_version, const VersionRange& range) noexcept;

} // namespace tessera::version
