#pragma once

#include "tessera/core/types.hpp"
#include "tessera/model/container.hpp"

namespace tessera::convert {

enum class OutputFormat {
    JsonSummary,
    HexDump,
    CanonicalBinary,
};

struct ConvertOptions {
    OutputFormat format{OutputFormat::JsonSummary};
};

[[nodiscard]] core::Result<std::string> convert_container(const model::Container& container, const ConvertOptions& options);
[[nodiscard]] core::Result<std::vector<std::uint8_t>> convert_bytes(core::ByteSpan input, const ConvertOptions& options);

} // namespace tessera::convert
