#include "tessera/tessera.hpp"

#include "tessera/reflect/type_info.hpp"

#include <cstdint>
#include <iostream>
#include <string>

namespace {

tessera::model::Container build_sample() {
    tessera::model::Container container{};
    container.header.magic = tessera::core::kContainerMagic;
    container.header.version = tessera::core::kContainerVersion;
    container.header.section_count = 2;

    for (std::uint16_t id = 1; id <= 2; ++id) {
        tessera::model::SectionData section{};
        section.entry.id = id;
        section.entry.name = id == 1 ? "events" : "metrics";
        section.schema.section_id = id;
        section.schema.fields = {
            {"active", tessera::schema::FieldType::Bool, 0},
            {"code", tessera::schema::FieldType::U32, 0},
            {"label", tessera::schema::FieldType::Utf8, 32},
        };
        section.records.section_id = id;
        for (std::uint32_t row = 0; row < id * 3U; ++row) {
            tessera::records::RecordRow record{};
            record.values = {row % 2 == 0, row, std::string("v") + std::to_string(row)};
            section.records.rows.push_back(std::move(record));
        }
        section.records.row_count = static_cast<std::uint32_t>(section.records.rows.size());
        container.catalog.entries.push_back(section.entry);
        container.sections.push_back(std::move(section));
    }
    return container;
}

} // namespace

int main() {
    const tessera::model::Container container = build_sample();

    const tessera::stats::ContainerStats stats = tessera::stats::collect(container);
    std::cout << "Statistics:\n"
              << "  sections=" << stats.sections << '\n'
              << "  total_rows=" << stats.total_rows << '\n'
              << "  total_fields=" << stats.total_fields << '\n'
              << "  total_value_bytes=" << stats.total_value_bytes << '\n'
              << "  largest_section_rows=" << stats.largest_section_rows << '\n';

    const tessera::graph::Node graph = tessera::graph::build_graph(container, true);
    std::cout << "Object graph: nodes=" << tessera::graph::node_count(graph)
              << " depth=" << tessera::graph::max_depth(graph) << '\n';

    tessera::profile::Profiler profiler;
    for (const tessera::model::SectionData& section : container.sections) {
        profiler.count("sections");
        profiler.observe("rows_per_section", section.records.rows.size());
    }
    std::cout << profiler.report();
    return 0;
}
