#include "tessera/textio/hex.hpp"

#include <array>
#include <cstdio>

namespace tessera::textio {

namespace {

constexpr char kHexDigits[] = "0123456789abcdef";

int hex_value(char character) {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f') {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'F') {
        return character - 'A' + 10;
    }
    return -1;
}

} // namespace

std::string to_hex(core::ByteSpan data) {
    std::string out;
    out.reserve(data.size * 2);
    for (std::size_t index = 0; index < data.size; ++index) {
        const std::uint8_t byte = data.data[index];
        out.push_back(kHexDigits[(byte >> 4U) & 0xFU]);
        out.push_back(kHexDigits[byte & 0xFU]);
    }
    return out;
}

core::Result<std::vector<std::uint8_t>> from_hex(std::string_view text) {
    if (text.size() % 2 != 0) {
        return core::Error{core::ErrorCode::InvalidLength, "hex string has odd length", text.size()};
    }
    std::vector<std::uint8_t> bytes;
    bytes.reserve(text.size() / 2);
    for (std::size_t index = 0; index < text.size(); index += 2) {
        const int high = hex_value(text[index]);
        const int low = hex_value(text[index + 1]);
        if (high < 0 || low < 0) {
            return core::Error{core::ErrorCode::InvalidEnum, "invalid hex digit", index};
        }
        bytes.push_back(static_cast<std::uint8_t>((high << 4) | low));
    }
    return bytes;
}

std::string hexdump(core::ByteSpan data) {
    std::string out;
    std::array<char, 16> line_ascii{};
    for (std::size_t offset = 0; offset < data.size; offset += 16) {
        std::array<char, 16> prefix{};
        std::snprintf(prefix.data(), prefix.size(), "%08zx  ", offset);
        out.append(prefix.data());

        std::size_t column = 0;
        for (; column < 16; ++column) {
            if (offset + column < data.size) {
                const std::uint8_t byte = data.data[offset + column];
                out.push_back(kHexDigits[(byte >> 4U) & 0xFU]);
                out.push_back(kHexDigits[byte & 0xFU]);
                out.push_back(' ');
                line_ascii[column] = (byte >= 0x20U && byte < 0x7FU) ? static_cast<char>(byte) : '.';
            } else {
                out += "   ";
                line_ascii[column] = ' ';
            }
        }
        out.push_back(' ');
        for (std::size_t index = 0; index < 16 && offset + index < data.size; ++index) {
            out.push_back(line_ascii[index]);
        }
        out.push_back('\n');
    }
    return out;
}

} // namespace tessera::textio
