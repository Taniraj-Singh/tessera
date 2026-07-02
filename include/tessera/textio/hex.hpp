#pragma once

#include "tessera/core/types.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tessera::textio {

// Lowercase hex encoding of a byte range.
[[nodiscard]] std::string to_hex(core::ByteSpan data);

// Decodes an even-length hex string. Rejects odd length or non-hex digits.
[[nodiscard]] core::Result<std::vector<std::uint8_t>> from_hex(std::string_view text);

// Classic offset/hex/ascii hexdump, 16 bytes per line.
[[nodiscard]] std::string hexdump(core::ByteSpan data);

} // namespace tessera::textio
