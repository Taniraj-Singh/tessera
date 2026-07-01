#include "tessera/core/types.hpp"

namespace tessera::core {

std::uint16_t read_le_u16(const std::uint8_t* bytes) noexcept {
    return static_cast<std::uint16_t>(bytes[0]) | (static_cast<std::uint16_t>(bytes[1]) << 8U);
}

std::uint32_t read_le_u32(const std::uint8_t* bytes) noexcept {
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[3]) << 24U);
}

std::uint64_t read_le_u64(const std::uint8_t* bytes) noexcept {
    std::uint64_t value = 0;
    for (int index = 0; index < 8; ++index) {
        value |= static_cast<std::uint64_t>(bytes[index]) << (8U * static_cast<unsigned>(index));
    }
    return value;
}

void write_le_u16(std::uint8_t* bytes, std::uint16_t value) noexcept {
    bytes[0] = static_cast<std::uint8_t>(value & 0xFFU);
    bytes[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
}

void write_le_u32(std::uint8_t* bytes, std::uint32_t value) noexcept {
    for (int index = 0; index < 4; ++index) {
        bytes[index] = static_cast<std::uint8_t>((value >> (8U * static_cast<unsigned>(index))) & 0xFFU);
    }
}

void write_le_u64(std::uint8_t* bytes, std::uint64_t value) noexcept {
    for (int index = 0; index < 8; ++index) {
        bytes[index] = static_cast<std::uint8_t>((value >> (8U * static_cast<unsigned>(index))) & 0xFFU);
    }
}

bool is_valid_utf8(std::string_view text) noexcept {
    std::size_t index = 0;
    while (index < text.size()) {
        const unsigned char byte = static_cast<unsigned char>(text[index]);
        if (byte <= 0x7FU) {
            ++index;
            continue;
        }
        std::size_t continuation = 0;
        if ((byte & 0xE0U) == 0xC0U) {
            continuation = 1;
        } else if ((byte & 0xF0U) == 0xE0U) {
            continuation = 2;
        } else if ((byte & 0xF8U) == 0xF0U) {
            continuation = 3;
        } else {
            return false;
        }
        if (index + continuation >= text.size()) {
            return false;
        }
        for (std::size_t offset = 1; offset <= continuation; ++offset) {
            const unsigned char next = static_cast<unsigned char>(text[index + offset]);
            if ((next & 0xC0U) != 0x80U) {
                return false;
            }
        }
        index += continuation + 1;
    }
    return true;
}

} // namespace tessera::core
