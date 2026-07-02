#include "support/test_framework.hpp"
#include "support/test_helpers.hpp"

#include "tessera/compress/delta_codec.hpp"
#include "tessera/compress/frame.hpp"
#include "tessera/textio/container_json.hpp"
#include "tessera/textio/csv.hpp"
#include "tessera/textio/hex.hpp"
#include "tessera/textio/json.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tessera::test {

int run_textio_tests() {
    int before = g_failures;

    {
        auto value = textio::parse_json(R"({"a":1,"b":[true,null,"x"],"c":3.5})");
        TESSERA_EXPECT_TRUE(value.ok());
        TESSERA_EXPECT_TRUE(value->is_object());
        const textio::JsonValue* a = value->find("a");
        TESSERA_EXPECT_TRUE(a != nullptr && a->is_number() && a->as_number() == 1.0);
        const textio::JsonValue* b = value->find("b");
        TESSERA_EXPECT_TRUE(b != nullptr && b->is_array() && b->items().size() == 3);
        TESSERA_EXPECT_TRUE(b->items()[0].is_bool() && b->items()[0].as_bool());
        TESSERA_EXPECT_TRUE(b->items()[1].is_null());
        TESSERA_EXPECT_TRUE(b->items()[2].is_string() && b->items()[2].as_string() == "x");

        // Serialize and re-parse: structure must survive the round trip.
        const std::string dumped = value->dump(false);
        auto reparsed = textio::parse_json(dumped);
        TESSERA_EXPECT_TRUE(reparsed.ok() && reparsed->size() == value->size());

        TESSERA_EXPECT_TRUE(!textio::parse_json("{bad").ok());
        TESSERA_EXPECT_TRUE(!textio::parse_json("[1,2,]extra").ok());
    }

    {
        // Escapes and unicode.
        auto value = textio::parse_json(R"("line\nbreak A")");
        TESSERA_EXPECT_TRUE(value.ok() && value->is_string());
        TESSERA_EXPECT_TRUE(value->as_string() == std::string("line\nbreak A"));
    }

    {
        model::Container sample = make_sample_container();
        const std::string json = textio::container_to_json_string(sample, false);
        auto restored = textio::container_from_json_string(json);
        TESSERA_EXPECT_TRUE(restored.ok());
        TESSERA_EXPECT_EQ(restored->sections.size(), std::size_t{1});
        TESSERA_EXPECT_EQ(restored->sections.front().schema.fields.size(), std::size_t{3});
        TESSERA_EXPECT_EQ(restored->sections.front().records.rows.size(), std::size_t{1});
        TESSERA_EXPECT_TRUE(restored->sections.front().records.rows.front().values ==
                            sample.sections.front().records.rows.front().values);
    }

    {
        const model::SectionData& section = make_sample_container().sections.front();
        const std::string csv = textio::section_to_csv(section);
        TESSERA_EXPECT_TRUE(csv.find("active") != std::string::npos);
        auto rows = textio::rows_from_csv(csv, section.schema);
        TESSERA_EXPECT_TRUE(rows.ok());
        TESSERA_EXPECT_EQ(rows->size(), std::size_t{1});
        TESSERA_EXPECT_TRUE(rows->front().values == section.records.rows.front().values);
    }

    {
        std::vector<std::uint8_t> bytes = {0xDE, 0xAD, 0xBE, 0xEF};
        const std::string hex = textio::to_hex(core::ByteSpan{bytes.data(), bytes.size()});
        TESSERA_EXPECT_EQ(hex, std::string{"deadbeef"});
        auto decoded = textio::from_hex(hex);
        TESSERA_EXPECT_TRUE(decoded.ok() && *decoded == bytes);
        TESSERA_EXPECT_TRUE(!textio::from_hex("abc").ok());
        TESSERA_EXPECT_TRUE(!textio::from_hex("zz").ok());
        TESSERA_EXPECT_TRUE(!textio::hexdump(core::ByteSpan{bytes.data(), bytes.size()}).empty());
    }

    {
        std::vector<std::uint8_t> data;
        for (int index = 0; index < 200; ++index) {
            data.push_back(static_cast<std::uint8_t>(index));
        }
        const compress::DeltaCodec delta;
        auto compressed = delta.compress(core::ByteSpan{data.data(), data.size()});
        TESSERA_EXPECT_TRUE(compressed.ok());
        auto restored = delta.decompress(core::ByteSpan{compressed->data(), compressed->size()}, data.size());
        TESSERA_EXPECT_TRUE(restored.ok() && *restored == data);

        auto frame = compress::compress_frame(core::ByteSpan{data.data(), data.size()}, compress::CodecId::Delta);
        TESSERA_EXPECT_TRUE(frame.ok());
        auto unframed = compress::decompress_frame(core::ByteSpan{frame->data(), frame->size()});
        TESSERA_EXPECT_TRUE(unframed.ok() && *unframed == data);
    }

    return g_failures - before;
}

} // namespace tessera::test
