#pragma once

#include "tessera/core/types.hpp"

namespace tessera::core {

inline constexpr std::uint32_t kContainerMagic = 0x53434254U; // "TBCS" little-endian
inline constexpr std::uint16_t kContainerVersionMajor = 1;
inline constexpr std::uint16_t kContainerVersionMinor = 0;
inline constexpr std::uint16_t kContainerVersion = 0x0100U;
inline constexpr std::size_t kHeaderSize = 64;
inline constexpr std::size_t kMaxSectionNameLength = 255;
inline constexpr std::size_t kMaxSectionCount = 4096;
inline constexpr std::size_t kMaxBlockSize = 64U * 1024U * 1024U;

} // namespace tessera::core
