#include "support/test_framework.hpp"

#include "tessera/encoding/varint.hpp"
#include "tessera/encoding/zigzag.hpp"
#include "tessera/memory/byte_buffer.hpp"

#include <cstdint>

namespace tessera::test {

int run_encoding_tests() {
    int before = g_failures;

    {
        TESSERA_EXPECT_EQ(encoding::zigzag_encode32(0), std::uint32_t{0});
        TESSERA_EXPECT_EQ(encoding::zigzag_encode32(-1), std::uint32_t{1});
        TESSERA_EXPECT_EQ(encoding::zigzag_encode32(1), std::uint32_t{2});
        TESSERA_EXPECT_EQ(encoding::zigzag_decode32(4), 2);
        TESSERA_EXPECT_EQ(encoding::zigzag_decode64(encoding::zigzag_encode64(-123456789)), std::int64_t{-123456789});
    }

    {
        memory::ByteBuffer buffer;
        encoding::write_uvarint64(buffer, 0);
        encoding::write_uvarint64(buffer, 300);
        encoding::write_uvarint64(buffer, 0xFFFFFFFFFFFFFFFFULL);
        TESSERA_EXPECT_EQ(encoding::uvarint_size(300), std::size_t{2});

        buffer::Reader reader(buffer.span());
        auto a = encoding::read_uvarint64(reader);
        auto b = encoding::read_uvarint64(reader);
        auto c = encoding::read_uvarint64(reader);
        TESSERA_EXPECT_TRUE(a.ok() && b.ok() && c.ok());
        TESSERA_EXPECT_EQ(*a, std::uint64_t{0});
        TESSERA_EXPECT_EQ(*b, std::uint64_t{300});
        TESSERA_EXPECT_EQ(*c, std::uint64_t{0xFFFFFFFFFFFFFFFFULL});
    }

    {
        // A continuation byte that never terminates must be rejected, not looped.
        std::vector<std::uint8_t> unterminated(12, 0x80U);
        buffer::Reader reader(core::ByteSpan{unterminated.data(), unterminated.size()});
        auto value = encoding::read_uvarint64(reader);
        TESSERA_EXPECT_TRUE(!value.ok());
    }

    {
        // 32-bit reader rejects a value that fits in 64 bits but not 32.
        memory::ByteBuffer buffer;
        encoding::write_uvarint64(buffer, 0x1'0000'0000ULL);
        buffer::Reader reader(buffer.span());
        auto value = encoding::read_uvarint32(reader);
        TESSERA_EXPECT_TRUE(!value.ok());
    }

    {
        memory::ByteBuffer buffer;
        encoding::write_svarint64(buffer, -987654321);
        buffer::Reader reader(buffer.span());
        auto value = encoding::read_svarint64(reader);
        TESSERA_EXPECT_TRUE(value.ok());
        TESSERA_EXPECT_EQ(*value, std::int64_t{-987654321});
    }

    return g_failures - before;
}

} // namespace tessera::test
