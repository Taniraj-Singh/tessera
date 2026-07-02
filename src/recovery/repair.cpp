#include "tessera/recovery/repair.hpp"

#include "tessera/core/constants.hpp"

namespace tessera::recovery {

void rebuild_catalog(model::Container& container) {
    container.catalog.entries.clear();
    container.catalog.entries.reserve(container.sections.size());
    for (const model::SectionData& section : container.sections) {
        container.catalog.entries.push_back(section.entry);
    }
    container.header.section_count = static_cast<std::uint32_t>(container.sections.size());
}

std::size_t realign_section_ids(model::Container& container) {
    std::size_t realigned = 0;
    for (model::SectionData& section : container.sections) {
        if (section.schema.section_id != section.entry.id) {
            section.schema.section_id = section.entry.id;
            ++realigned;
        }
        if (section.records.section_id != section.entry.id) {
            section.records.section_id = section.entry.id;
            ++realigned;
        }
    }
    return realigned;
}

std::size_t trim_row_counts(model::Container& container) {
    std::size_t trimmed = 0;
    for (model::SectionData& section : container.sections) {
        const auto actual = static_cast<std::uint32_t>(section.records.rows.size());
        if (section.records.row_count != actual) {
            section.records.row_count = actual;
            ++trimmed;
        }
    }
    return trimmed;
}

std::size_t drop_unsalvageable(model::Container& container) {
    const std::size_t before = container.sections.size();
    std::vector<model::SectionData> kept;
    kept.reserve(container.sections.size());

    for (model::SectionData& section : container.sections) {
        if (section.entry.name.empty() || section.entry.name.size() > core::kMaxSectionNameLength) {
            continue;
        }
        bool rows_consistent = true;
        for (const records::RecordRow& row : section.records.rows) {
            if (row.values.size() != section.schema.fields.size()) {
                rows_consistent = false;
                break;
            }
        }
        if (!rows_consistent) {
            continue;
        }
        kept.push_back(std::move(section));
    }

    container.sections = std::move(kept);
    return before - container.sections.size();
}

RepairReport repair(model::Container& container) {
    RepairReport report{};
    report.sections_dropped = drop_unsalvageable(container);
    report.ids_realigned = realign_section_ids(container);
    report.rows_trimmed = trim_row_counts(container);
    rebuild_catalog(container);
    report.catalog_rebuilt = true;
    return report;
}

} // namespace tessera::recovery
