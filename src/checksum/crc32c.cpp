#include "tessera/checksum/crc32c.hpp"

#include <array>

namespace tessera::checksum {

namespace {

constexpr std::uint32_t kPolynomial = 0x82F63B78U;

constexpr std::uint32_t reflect(std::uint32_t value, int bits) {
    std::uint32_t result = 0;
    for (int index = 0; index < bits; ++index) {
        if ((value >> index) & 1U) {
            result |= 1U << (bits - 1 - index);
        }
    }
    return result;
}

constexpr std::array<std::uint32_t, 256> make_table() {
    std::array<std::uint32_t, 256> table{};
    for (std::uint32_t byte = 0; byte < 256; ++byte) {
        std::uint32_t value = reflect(byte, 8) << 24U;
        for (int bit = 0; bit < 8; ++bit) {
            if (value & 0x80000000U) {
                value = (value << 1U) ^ kPolynomial;
            } else {
                value <<= 1U;
            }
        }
        table[byte] = reflect(value, 32);
    }
    return table;
}

constexpr auto kTable = make_table();

} // namespace

std::uint32_t crc32c(core::ByteSpan data) noexcept {
    std::uint32_t crc = 0xFFFFFFFFU;
    for (const std::uint8_t* cursor = data.begin(); cursor != data.end(); ++cursor) {
        crc = kTable[(crc ^ *cursor) & 0xFFU] ^ (crc >> 8U);
    }
    return crc ^ 0xFFFFFFFFU;
}

core::Result<void> verify(core::ByteSpan data, std::uint32_t expected) noexcept {
    const std::uint32_t actual = crc32c(data);
    if (actual != expected) {
        return core::Error{core::ErrorCode::ChecksumMismatch, "CRC32C mismatch", 0};
    }
    return core::Ok();
}

} // namespace tessera::checksum
