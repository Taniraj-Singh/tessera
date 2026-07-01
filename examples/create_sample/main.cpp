#include "tessera/tessera.hpp"

#include <fstream>
#include <iostream>

int main() {
    const tessera::model::Container container = [] {
        tessera::model::Container value{};
        value.header.magic = tessera::core::kContainerMagic;
        value.header.version = tessera::core::kContainerVersion;
        value.header.created_unix_ms = 1'700'000'000'000ULL;
        value.header.section_count = 1;

        tessera::model::SectionData section{};
        section.entry.id = 1;
        section.entry.name = "metrics";
        section.schema.section_id = 1;
        section.schema.fields = {{"temperature", tessera::schema::FieldType::F64, 0}};

        tessera::records::RecordRow row{};
        row.values = {42.5};
        section.records.section_id = 1;
        section.records.row_count = 1;
        section.records.rows.push_back(std::move(row));

        value.sections.push_back(std::move(section));
        value.catalog.entries.push_back(value.sections.front().entry);
        return value;
    }();

    if (auto bytes = tessera::serialize::serialize_container(container, {}); bytes) {
        std::ofstream output("sample.tbc", std::ios::binary);
        output.write(reinterpret_cast<const char*>(bytes->data()), static_cast<std::streamsize>(bytes->size()));
        std::cout << "Wrote sample.tbc (" << bytes->size() << " bytes)\n";
        return 0;
    }

    std::cerr << "serialization failed\n";
    return 1;
}
