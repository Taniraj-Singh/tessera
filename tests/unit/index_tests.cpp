#include "support/test_framework.hpp"
#include "support/test_helpers.hpp"

#include "tessera/hash/fnv.hpp"
#include "tessera/hash/murmur.hpp"
#include "tessera/index/hash_index.hpp"
#include "tessera/index/section_index.hpp"
#include "tessera/index/string_index.hpp"

#include <cstdint>
#include <string>

namespace tessera::test {

int run_index_tests() {
    int before = g_failures;

    {
        const std::string text = "tessera";
        const core::ByteSpan span{reinterpret_cast<const std::uint8_t*>(text.data()), text.size()};
        TESSERA_EXPECT_TRUE(hash::fnv1a64(text) != 0);
        TESSERA_EXPECT_TRUE(hash::murmur3_x86_32(span, 0) != 0);
        TESSERA_EXPECT_TRUE(hash::fmix64(1) != hash::fmix64(2));
    }

    {
        index::HashIndex table(4);
        for (std::uint32_t value = 0; value < 200; ++value) {
            TESSERA_EXPECT_TRUE(table.insert(value * 7U + 1U, value));
        }
        TESSERA_EXPECT_EQ(table.size(), std::size_t{200});
        for (std::uint32_t value = 0; value < 200; ++value) {
            const std::uint32_t* found = table.find(value * 7U + 1U);
            TESSERA_EXPECT_TRUE(found != nullptr && *found == value);
        }
        TESSERA_EXPECT_TRUE(table.find(3U) == nullptr);
        TESSERA_EXPECT_TRUE(table.erase(1U));
        TESSERA_EXPECT_TRUE(table.find(1U) == nullptr);
        TESSERA_EXPECT_EQ(table.size(), std::size_t{199});
        // Overwrite semantics.
        table.insert(8U, 42U);
        table.insert(8U, 99U);
        const std::uint32_t* overwritten = table.find(8U);
        TESSERA_EXPECT_TRUE(overwritten != nullptr && *overwritten == 99U);
    }

    {
        index::StringIndex names(2);
        names.insert("alpha", 1);
        names.insert("beta", 2);
        names.insert("gamma", 3);
        names.insert("beta", 20); // overwrite
        const std::uint32_t* beta = names.find("beta");
        TESSERA_EXPECT_TRUE(beta != nullptr && *beta == 20U);
        TESSERA_EXPECT_TRUE(names.find("delta") == nullptr);
        TESSERA_EXPECT_EQ(names.size(), std::size_t{3});
    }

    {
        model::Container container = make_sample_container();
        index::SectionIndex index = index::SectionIndex::build(container);
        const std::uint32_t* by_id = index.find_by_id(1);
        const std::uint32_t* by_name = index.find_by_name("events");
        TESSERA_EXPECT_TRUE(by_id != nullptr && *by_id == 0U);
        TESSERA_EXPECT_TRUE(by_name != nullptr && *by_name == 0U);
        TESSERA_EXPECT_TRUE(index.find_by_id(999) == nullptr);
        TESSERA_EXPECT_TRUE(index.find_by_name("missing") == nullptr);
    }

    return g_failures - before;
}

} // namespace tessera::test
