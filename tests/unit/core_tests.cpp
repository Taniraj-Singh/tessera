#include "support/test_framework.hpp"
#include "support/test_helpers.hpp"

#include "tessera/buffer/reader_writer.hpp"
#include "tessera/checksum/crc32c.hpp"
#include "tessera/version/negotiation.hpp"

namespace tessera::test {

int run_core_tests() {
    int before = g_failures;

    {
        std::vector<std::uint8_t> bytes{0x01, 0x02, 0x03};
        const std::uint32_t crc = tessera::checksum::crc32c(core::ByteSpan{bytes.data(), bytes.size()});
        TESSERA_EXPECT_TRUE(crc != 0);
    }

    {
        std::vector<std::uint8_t> storage(8, 0);
        tessera::buffer::Writer writer(core::MutableByteSpan{storage.data(), storage.size()});
        TESSERA_EXPECT_TRUE(writer.write_u32(0xAABBCCDDU));
        TESSERA_EXPECT_EQ(core::read_le_u32(storage.data()), 0xAABBCCDDU);
    }

    {
        TESSERA_EXPECT_TRUE(core::is_valid_utf8("tessera"));
        TESSERA_EXPECT_TRUE(!core::is_valid_utf8("\xFF"));
    }

    {
        auto negotiated = tessera::version::negotiate(core::kContainerVersion, tessera::version::VersionRange{core::kContainerVersion, core::kContainerVersion});
        TESSERA_EXPECT_TRUE(negotiated.ok());
    }

    return g_failures - before;
}

} // namespace tessera::test
