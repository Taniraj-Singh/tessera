#pragma once

#include "tessera/model/container.hpp"

#include <cstddef>
#include <vector>

namespace tessera::diff {

// Row-level comparison of two versions of a section. Rows are compared
// positionally: matching indices whose decoded values differ are "modified",
// trailing rows present in only one side are "added"/"removed".
struct SectionDiff {
    std::size_t unchanged{0};
    std::size_t modified{0};
    std::size_t added{0};
    std::size_t removed{0};
    std::vector<std::size_t> modified_rows; // indices (in the shared prefix) that differ
    bool schema_changed{false};

    [[nodiscard]] bool identical() const noexcept {
        return modified == 0 && added == 0 && removed == 0 && !schema_changed;
    }
};

[[nodiscard]] SectionDiff diff_sections(const model::SectionData& base, const model::SectionData& updated);

// Whole-container diff keyed by section id: a per-section SectionDiff plus a
// count of sections that appear on only one side.
struct ContainerDiff {
    std::size_t sections_added{0};
    std::size_t sections_removed{0};
    std::size_t sections_changed{0};
    std::size_t total_rows_modified{0};
};

[[nodiscard]] ContainerDiff diff_containers(const model::Container& base, const model::Container& updated);

} // namespace tessera::diff
