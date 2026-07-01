#pragma once

#include "tessera/tessera.hpp"

namespace tessera::test {

inline model::Container make_sample_container() {
    model::Container container{};
    container.header.magic = core::kContainerMagic;
    container.header.version = core::kContainerVersion;
    container.header.created_unix_ms = 1'700'000'000'000ULL;
    container.header.section_count = 1;

    model::SectionData section{};
    section.entry.id = 1;
    section.entry.name = "events";
    section.entry.flags = 0;

    section.schema.section_id = 1;
    section.schema.fields = {
        {"active", schema::FieldType::Bool, 0},
        {"code", schema::FieldType::U32, 0},
        {"label", schema::FieldType::Utf8, 64},
    };

    records::RecordRow row{};
    row.values = {true, std::uint32_t{42}, std::string{"boot"}};
    section.records.section_id = 1;
    section.records.row_count = 1;
    section.records.rows.push_back(std::move(row));

    container.sections.push_back(std::move(section));
    container.catalog.entries.push_back(container.sections.front().entry);
    return container;
}

int run_all_tests();

} // namespace tessera::test
