#include "tessera/tessera.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

namespace {

tessera::model::Container make_container(std::uint16_t id, const std::string& name, std::uint32_t rows) {
    tessera::model::Container container{};
    container.header.magic = tessera::core::kContainerMagic;
    container.header.version = tessera::core::kContainerVersion;
    container.header.created_unix_ms = 1'700'000'000'000ULL;
    container.header.section_count = 1;

    tessera::model::SectionData section{};
    section.entry.id = id;
    section.entry.name = name;
    section.schema.section_id = id;
    section.schema.fields = {
        {"seq", tessera::schema::FieldType::U32, 0},
        {"label", tessera::schema::FieldType::Utf8, 32},
    };
    section.records.section_id = id;
    for (std::uint32_t index = 0; index < rows; ++index) {
        tessera::records::RecordRow row{};
        row.values = {index, std::string("row-") + std::to_string(index)};
        section.records.rows.push_back(std::move(row));
    }
    section.records.row_count = rows;

    container.sections.push_back(std::move(section));
    container.catalog.entries.push_back(container.sections.front().entry);
    return container;
}

} // namespace

int main() {
    tessera::archive::ArchiveWriter writer;
    writer.set_created(1'700'000'000'000ULL);
    (void)writer.add_container("events", 1, make_container(1, "events", 8), tessera::compress::CodecId::Lz77);
    (void)writer.add_container("metrics", 2, make_container(2, "metrics", 16), tessera::compress::CodecId::Rle);

    auto bytes = writer.build();
    if (!bytes) {
        std::cerr << "archive build failed: " << bytes.error().message << '\n';
        return 1;
    }

    std::ofstream output("bundle.tba", std::ios::binary);
    output.write(reinterpret_cast<const char*>(bytes->data()), static_cast<std::streamsize>(bytes->size()));
    output.close();
    std::cout << "Wrote bundle.tba (" << bytes->size() << " bytes)\n";

    const tessera::core::ByteSpan span{bytes->data(), bytes->size()};
    if (auto reader = tessera::archive::ArchiveReader::open(span); reader) {
        std::cout << "Archive entries:\n";
        for (const tessera::archive::ArchiveEntry& entry : reader->entries()) {
            std::cout << "  id=" << entry.id << " name=" << entry.name
                      << " frame=" << entry.frame_length << "B\n";
        }
    }

    const tessera::pipeline::PipelineReport report = tessera::pipeline::run_archive_pipeline(span, {});
    std::cout << "Pipeline: opened=" << (report.opened ? "yes" : "no")
              << " entries=" << report.entries
              << " decoded=" << report.decoded
              << " valid=" << report.valid << '\n';
    for (const tessera::pipeline::EntryOutcome& outcome : report.outcomes) {
        std::cout << "  " << outcome.name << ": sections=" << outcome.section_count
                  << " rows=" << outcome.row_total
                  << " valid=" << (outcome.valid ? "yes" : "no") << '\n';
    }
    return 0;
}
