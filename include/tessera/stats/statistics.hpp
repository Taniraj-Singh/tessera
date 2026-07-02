#pragma once

#include "tessera/model/container.hpp"

#include <array>
#include <cstddef>

namespace tessera::stats {

// Aggregate shape/size metrics for a container. type_counts is indexed by the
// raw FieldType value (matching reflect's table), so callers can read a
// per-type field histogram directly.
struct ContainerStats {
    std::size_t sections{0};
    std::size_t total_rows{0};
    std::size_t total_fields{0};
    std::size_t total_value_bytes{0};
    std::size_t largest_section_rows{0};
    std::array<std::size_t, 11> type_counts{};
};

// Collects statistics by traversing the container once with the visitor
// framework. Pure read-only aggregation.
[[nodiscard]] ContainerStats collect(const model::Container& container);

} // namespace tessera::stats
