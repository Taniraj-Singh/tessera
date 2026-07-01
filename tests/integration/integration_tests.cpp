#include "support/test_framework.hpp"
#include "support/test_helpers.hpp"

namespace tessera::test {

int run_integration_tests() {
    int before = g_failures;

    model::Container sample = make_sample_container();
    if (auto bytes = serialize::serialize_container(sample, {}); bytes) {
        if (auto parsed = parse::parse_container(core::ByteSpan{bytes->data(), bytes->size()}, {}); parsed) {
            TESSERA_EXPECT_EQ(parsed->sections.size(), std::size_t{1});
            TESSERA_EXPECT_EQ(parsed->sections.front().records.rows.size(), std::size_t{1});
        } else {
            TESSERA_EXPECT_TRUE(false);
        }
    } else {
        TESSERA_EXPECT_TRUE(false);
    }

    if (auto bytes = serialize::serialize_container(sample, {}); bytes) {
        auto report = validate::validate_bytes(core::ByteSpan{bytes->data(), bytes->size()}, {});
        TESSERA_EXPECT_TRUE(report.valid);
    }

    if (auto bytes = serialize::serialize_container(sample, {}); bytes) {
        stream::StreamReader reader(core::ByteSpan{bytes->data(), bytes->size()});
        if (auto event = reader.next(); event) {
            TESSERA_EXPECT_TRUE(event->kind == stream::StreamEventKind::Header);
        } else {
            TESSERA_EXPECT_TRUE(false);
        }
    }

    if (auto json = convert::convert_container(sample, {convert::OutputFormat::JsonSummary}); json) {
        TESSERA_EXPECT_TRUE(json->find("events") != std::string::npos);
    } else {
        TESSERA_EXPECT_TRUE(false);
    }

    return g_failures - before;
}

} // namespace tessera::test
