#include "tessera/version/negotiation.hpp"

namespace tessera::version {

core::Result<void> negotiate(std::uint16_t container_version, const VersionRange& range) noexcept {
    const std::uint16_t major = static_cast<std::uint16_t>((container_version >> 8U) & 0xFFU);
    const std::uint16_t supported_major = static_cast<std::uint16_t>((range.max_supported >> 8U) & 0xFFU);
    const std::uint16_t min_major = static_cast<std::uint16_t>((range.min_supported >> 8U) & 0xFFU);

    if (major < min_major || major > supported_major) {
        return core::Error{core::ErrorCode::UnsupportedVersion, "container major version unsupported", 4};
    }
    if (container_version < range.min_supported || container_version > range.max_supported) {
        return core::Error{core::ErrorCode::VersionMismatch, "container version outside negotiated range", 4};
    }
    return core::Ok();
}

} // namespace tessera::version
