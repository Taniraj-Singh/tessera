#include "support/test_framework.hpp"
#include "support/test_helpers.hpp"

#include "tessera/archive/archive_writer.hpp"
#include "tessera/pipeline/incremental_decoder.hpp"
#include "tessera/pipeline/incremental_encoder.hpp"
#include "tessera/pipeline/pipeline.hpp"
#include "tessera/pipeline/stage.hpp"
#include "tessera/serialize/serializer.hpp"

#include <cstdint>
#include <vector>

namespace tessera::test {

int run_pipeline_tests() {
    int before = g_failures;

    model::Container sample = make_sample_container();
    auto serialized = serialize::serialize_container(sample, {});
    TESSERA_EXPECT_TRUE(serialized.ok());

    {
        // Feed the serialized container one byte at a time; the decoder must
        // stay incomplete until the final byte and then parse successfully.
        pipeline::IncrementalDecoder decoder;
        for (std::size_t index = 0; index + 1 < serialized->size(); ++index) {
            TESSERA_EXPECT_TRUE(decoder.feed(core::ByteSpan{serialized->data() + index, 1}).ok());
            TESSERA_EXPECT_TRUE(!decoder.complete());
        }
        TESSERA_EXPECT_TRUE(decoder.feed(core::ByteSpan{serialized->data() + serialized->size() - 1, 1}).ok());
        TESSERA_EXPECT_TRUE(decoder.complete());
        auto container = decoder.take();
        TESSERA_EXPECT_TRUE(container.ok());
        TESSERA_EXPECT_EQ(container->sections.size(), std::size_t{1});
    }

    {
        // A whole-buffer feed should also complete.
        pipeline::IncrementalDecoder decoder;
        TESSERA_EXPECT_TRUE(decoder.feed(core::ByteSpan{serialized->data(), serialized->size()}).ok());
        TESSERA_EXPECT_TRUE(decoder.complete());
    }

    {
        // Garbage that fails the header must land in the Failed state.
        std::vector<std::uint8_t> junk(80, 0x00);
        pipeline::IncrementalDecoder decoder;
        (void)decoder.feed(core::ByteSpan{junk.data(), junk.size()});
        TESSERA_EXPECT_TRUE(decoder.failed());
        TESSERA_EXPECT_TRUE(!decoder.take().ok());
    }

    {
        // Incremental encoder rebuilds an equivalent container.
        pipeline::IncrementalEncoder encoder;
        encoder.begin(core::kContainerVersion, 1'700'000'000'000ULL, 0);
        TESSERA_EXPECT_TRUE(encoder.add_section(sample.sections.front()).ok());
        TESSERA_EXPECT_EQ(encoder.section_count(), std::size_t{1});
        auto bytes = encoder.finish();
        TESSERA_EXPECT_TRUE(bytes.ok());
        auto parsed = parse::parse_container(core::ByteSpan{bytes->data(), bytes->size()}, {});
        TESSERA_EXPECT_TRUE(parsed.ok());
        TESSERA_EXPECT_EQ(parsed->sections.size(), std::size_t{1});

        pipeline::IncrementalEncoder empty_encoder;
        TESSERA_EXPECT_TRUE(!empty_encoder.add_section(model::SectionData{}).ok()); // empty name rejected
    }

    {
        // Stage composition: identity then a prefix-dropping transform.
        pipeline::ByteTransform identity = [](core::ByteSpan input) -> core::Result<std::vector<std::uint8_t>> {
            return std::vector<std::uint8_t>(input.begin(), input.end());
        };
        pipeline::ByteTransform drop_first = [](core::ByteSpan input) -> core::Result<std::vector<std::uint8_t>> {
            if (input.size == 0) {
                return core::Error{core::ErrorCode::TruncatedInput, "empty stage input", 0};
            }
            return std::vector<std::uint8_t>(input.begin() + 1, input.end());
        };
        auto chained = pipeline::run_chain({identity, drop_first}, core::ByteSpan{serialized->data(), serialized->size()});
        TESSERA_EXPECT_TRUE(chained.ok());
        TESSERA_EXPECT_EQ(chained->size(), serialized->size() - 1);

        auto composed = pipeline::compose(identity, drop_first)(core::ByteSpan{serialized->data(), serialized->size()});
        TESSERA_EXPECT_TRUE(composed.ok());
        TESSERA_EXPECT_EQ(composed->size(), serialized->size() - 1);
    }

    {
        // Full archive pipeline over a two-entry archive.
        archive::ArchiveWriter writer;
        TESSERA_EXPECT_TRUE(writer.add_container("events", 1, sample, compress::CodecId::Lz77).ok());
        model::Container second = make_sample_container();
        second.sections.front().entry.id = 2;
        second.sections.front().entry.name = "metrics";
        second.sections.front().schema.section_id = 2;
        second.sections.front().records.section_id = 2;
        TESSERA_EXPECT_TRUE(writer.add_container("metrics", 2, second, compress::CodecId::Store).ok());
        auto built = writer.build();
        TESSERA_EXPECT_TRUE(built.ok());

        pipeline::PipelineReport report = pipeline::run_archive_pipeline(core::ByteSpan{built->data(), built->size()}, {});
        TESSERA_EXPECT_TRUE(report.opened);
        TESSERA_EXPECT_EQ(report.entries, std::size_t{2});
        TESSERA_EXPECT_EQ(report.decoded, std::size_t{2});
        TESSERA_EXPECT_EQ(report.valid, std::size_t{2});
    }

    return g_failures - before;
}

} // namespace tessera::test
