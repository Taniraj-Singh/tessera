#pragma once

#include "tessera/core/types.hpp"
#include "tessera/model/container.hpp"

#include <cstddef>

namespace tessera::recovery {

// Summary of the changes a repair pass applied.
struct RepairReport {
    bool catalog_rebuilt{false};
    std::size_t ids_realigned{0};   // schema/record ids forced to match the entry id
    std::size_t sections_dropped{0}; // structurally hopeless sections removed
    std::size_t rows_trimmed{0};    // declared row counts corrected to actual
};

// Recomputes the catalog from the section list and syncs the header count.
void rebuild_catalog(model::Container& container);

// Forces each section's schema/record section ids to match its catalog id and
// corrects declared row counts to the actual row vector length.
[[nodiscard]] std::size_t realign_section_ids(model::Container& container);
[[nodiscard]] std::size_t trim_row_counts(model::Container& container);

// Drops sections that cannot be salvaged (empty/oversized name, or rows whose
// field count disagrees with the schema). Returns the number removed.
[[nodiscard]] std::size_t drop_unsalvageable(model::Container& container);

// Runs the full repair sequence and reports what changed. The result is a
// container that should pass structural validation.
[[nodiscard]] RepairReport repair(model::Container& container);

} // namespace tessera::recovery
