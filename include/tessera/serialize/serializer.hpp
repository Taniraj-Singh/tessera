#pragma once

#include "tessera/core/types.hpp"
#include "tessera/model/container.hpp"
#include "tessera/plugins/registry.hpp"

namespace tessera::serialize {

struct SerializeOptions {
    bool run_plugins{true};
};

[[nodiscard]] core::Result<std::vector<std::uint8_t>> serialize_container(
    const model::Container& container,
    const SerializeOptions& options = {});

} // namespace tessera::serialize
