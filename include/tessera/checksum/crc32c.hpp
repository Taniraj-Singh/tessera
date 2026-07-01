#pragma once

#include "tessera/core/types.hpp"

namespace tessera::checksum {

[[nodiscard]] std::uint32_t crc32c(core::ByteSpan data) noexcept;
[[nodiscard]] core::Result<void> verify(core::ByteSpan data, std::uint32_t expected) noexcept;

} // namespace tessera::checksum
