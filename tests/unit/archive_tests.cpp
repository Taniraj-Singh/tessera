#include "support/test_framework.hpp"
#include "support/test_helpers.hpp"

#include "tessera/archive/archive_reader.hpp"
#include "tessera/archive/archive_writer.hpp"

#include <cstdint>
#include <vector>

namespace tessera::test {

int run_archive_tests() {
    int before = g_failures;

    model::Container first = make_sample_container();
    model::Container second = make_sample_container();
    second.sections.front().entry.id = 2;
    second.sections.front().entry.name = "metrics";
    second.sections.front().schema.section_id = 2;
    second.sections.front().records.section_id = 2;
    second.catalog.entries.front() = second.sections.front().entry;

    {
        archive::ArchiveWriter writer;
        writer.set_created(1'700'000'000'000ULL);
        TESSERA_EXPECT_TRUE(writer.add_container("events", 1, first, compress::CodecId::Lz77).ok());
        TESSERA_EXPECT_TRUE(writer.add_container("metrics", 2, second, compress::CodecId::Rle).ok());

        auto built = writer.build();
        TESSERA_EXPECT_TRUE(built.ok());

        auto reader = archive::ArchiveReader::open(core::ByteSpan{built->data(), built->size()});
        TESSERA_EXPECT_TRUE(reader.ok());
        TESSERA_EXPECT_EQ(reader->size(), std::size_t{2});

        const archive::ArchiveEntry* metrics = reader->find("metrics");
        TESSERA_EXPECT_TRUE(metrics != nullptr && metrics->id == 2);
        TESSERA_EXPECT_TRUE(reader->find("absent") == nullptr);

        for (std::size_t index = 0; index < reader->size(); ++index) {
            auto parsed = reader->parse_entry(index);
            TESSERA_EXPECT_TRUE(parsed.ok());
            TESSERA_EXPECT_EQ(parsed->sections.size(), std::size_t{1});
        }
    }

    {
        // A single-container archive should round-trip through the store codec.
        archive::ArchiveWriter writer;
        TESSERA_EXPECT_TRUE(writer.add_container("only", 1, first, compress::CodecId::Store).ok());
        auto built = writer.build();
        TESSERA_EXPECT_TRUE(built.ok());
        auto reader = archive::ArchiveReader::open(core::ByteSpan{built->data(), built->size()});
        TESSERA_EXPECT_TRUE(reader.ok());
        auto bytes = reader->read_entry(0);
        TESSERA_EXPECT_TRUE(bytes.ok());
    }

    {
        // Corrupting the directory CRC must be rejected at open().
        archive::ArchiveWriter writer;
        TESSERA_EXPECT_TRUE(writer.add_container("only", 1, first, compress::CodecId::Store).ok());
        auto built = writer.build();
        TESSERA_EXPECT_TRUE(built.ok());
        // The directory begins right after the 28-byte header.
        (*built)[archive::kArchiveHeaderSize] ^= 0xFFU;
        auto reader = archive::ArchiveReader::open(core::ByteSpan{built->data(), built->size()});
        TESSERA_EXPECT_TRUE(!reader.ok());
    }

    {
        // Truncating an archive below its header must fail cleanly.
        std::vector<std::uint8_t> tiny(4, 0);
        auto reader = archive::ArchiveReader::open(core::ByteSpan{tiny.data(), tiny.size()});
        TESSERA_EXPECT_TRUE(!reader.ok());
    }

    return g_failures - before;
}

} // namespace tessera::test
