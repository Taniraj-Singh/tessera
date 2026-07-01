#include "support/test_framework.hpp"
#include "support/test_helpers.hpp"

#include "tessera/tessera.hpp"

namespace tessera::test {

int run_edge_tests() {
    int before = g_failures;

    model::Container empty = make_sample_container();
    empty.sections.clear();
    empty.catalog.entries.clear();
    empty.header.section_count = 0;

    if (auto bytes = serialize::serialize_container(empty, {}); bytes) {
        if (auto parsed = parse::parse_container(core::ByteSpan{bytes->data(), bytes->size()}, {}); parsed) {
            TESSERA_EXPECT_EQ(parsed->sections.size(), std::size_t{0});
        } else {
            TESSERA_EXPECT_TRUE(false);
        }
    } else {
        TESSERA_EXPECT_TRUE(false);
    }

    {
        version::VersionRange range{0x0200, 0x0200};
        auto negotiated = version::negotiate(core::kContainerVersion, range);
        TESSERA_EXPECT_TRUE(!negotiated.ok());
    }

    return g_failures - before;
}

} // namespace tessera::test
