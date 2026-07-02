#include "tessera/tessera.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Generates valid seed inputs for the fuzz harnesses. Fuzzing mutates outward
// from these, so shipping structurally valid seeds is the single best way to
// reach the deep, format-gated logic quickly. Run: corpus_gen <output_dir>
// (the directory must already exist).

namespace {

tessera::model::Container build_container() {
    tessera::model::Container container{};
    container.header.magic = tessera::core::kContainerMagic;
    container.header.version = tessera::core::kContainerVersion;
    container.header.created_unix_ms = 1'700'000'000'000ULL;
    container.header.section_count = 1;

    tessera::model::SectionData section{};
    section.entry.id = 1;
    section.entry.name = "events";
    section.schema.section_id = 1;
    section.schema.fields = {
        {"active", tessera::schema::FieldType::Bool, 0},
        {"code", tessera::schema::FieldType::U32, 0},
        {"label", tessera::schema::FieldType::Utf8, 32},
    };
    section.records.section_id = 1;
    for (std::uint32_t index = 0; index < 4; ++index) {
        tessera::records::RecordRow row{};
        row.values = {index % 2 == 0, index, std::string("row") + std::to_string(index)};
        section.records.rows.push_back(std::move(row));
    }
    section.records.row_count = static_cast<std::uint32_t>(section.records.rows.size());

    container.sections.push_back(std::move(section));
    container.catalog.entries.push_back(container.sections.front().entry);
    return container;
}

bool write_file(const std::string& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        std::cerr << "cannot open " << path << '\n';
        return false;
    }
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(output);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: corpus_gen <output_dir>\n";
        return 1;
    }
    const std::string dir = argv[1];
    const tessera::model::Container container = build_container();

    auto container_bytes = tessera::serialize::serialize_container(container, {});
    if (!container_bytes) {
        std::cerr << "serialize failed\n";
        return 2;
    }
    if (!write_file(dir + "/container.tbc", *container_bytes)) {
        return 3;
    }

    // A standalone schema block seeds the decoder harness's schema parse path.
    if (auto schema_bytes = tessera::schema::serialize(container.sections.front().schema); schema_bytes) {
        (void)write_file(dir + "/schema.bin", *schema_bytes);
    }

    // A compression frame wrapping the container seeds the compress harness.
    if (auto frame = tessera::compress::compress_frame(
            tessera::core::ByteSpan{container_bytes->data(), container_bytes->size()},
            tessera::compress::CodecId::Lz77);
        frame) {
        (void)write_file(dir + "/frame.tfz", *frame);
    }

    // A two-entry archive seeds the archive and pipeline harnesses.
    tessera::archive::ArchiveWriter writer;
    (void)writer.add_container("events", 1, container, tessera::compress::CodecId::Lz77);
    (void)writer.add_container("mirror", 2, container, tessera::compress::CodecId::Store);
    if (auto archive = writer.build(); archive) {
        (void)write_file(dir + "/archive.tba", *archive);
    }

    std::cout << "seed corpus written to " << dir << '\n';
    return 0;
}
