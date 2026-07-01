#pragma once

#include "tessera/core/types.hpp"
#include "tessera/model/container.hpp"
#include "tessera/plugins/registry.hpp"
#include "tessera/version/negotiation.hpp"

namespace tessera::parse {

struct ParseOptions {
    version::VersionRange version_range{};
    bool run_plugins{true};
};

[[nodiscard]] core::Result<model::Container> parse_container(core::ByteSpan input, const ParseOptions& options = {});

} // namespace tessera::parse
