#include "support/test_framework.hpp"

#include "tessera/memory/arena.hpp"
#include "tessera/memory/byte_buffer.hpp"
#include "tessera/memory/object_pool.hpp"
#include "tessera/util/bit_ops.hpp"

#include <cstdint>

namespace tessera::test {

namespace {

struct Node {
    int value{0};
    Node* next{nullptr};
};

} // namespace

int run_memory_tests() {
    int before = g_failures;

    {
        memory::ByteBuffer buffer;
        buffer.append_u32(0xAABBCCDDU);
        buffer.append_u16(0x1122U);
        buffer.append_u8(0x7FU);
        TESSERA_EXPECT_EQ(buffer.size(), std::size_t{7});
        TESSERA_EXPECT_EQ(core::read_le_u32(buffer.data()), 0xAABBCCDDU);
        TESSERA_EXPECT_EQ(core::read_le_u16(buffer.data() + 4), std::uint16_t{0x1122U});

        TESSERA_EXPECT_TRUE(buffer.patch_u32(0, 0x01020304U).ok());
        TESSERA_EXPECT_EQ(core::read_le_u32(buffer.data()), 0x01020304U);
        TESSERA_EXPECT_TRUE(!buffer.patch_u32(4, 0).ok());
    }

    {
        memory::Arena arena(128);
        auto first = arena.allocate(16, 8);
        auto second = arena.allocate(32, 16);
        TESSERA_EXPECT_TRUE(first.ok());
        TESSERA_EXPECT_TRUE(second.ok());
        const auto second_address = reinterpret_cast<std::uintptr_t>(second.value());
        TESSERA_EXPECT_EQ(second_address % 16U, std::uintptr_t{0});
        TESSERA_EXPECT_TRUE(!arena.allocate(8, 3).ok()); // non power-of-two alignment
        TESSERA_EXPECT_TRUE(arena.bytes_allocated() >= 48U);
        arena.reset();
        TESSERA_EXPECT_EQ(arena.bytes_allocated(), std::size_t{0});
    }

    {
        memory::ObjectPool<Node> pool(4);
        Node* a = pool.acquire();
        a->value = 5;
        Node* b = pool.acquire();
        b->value = 9;
        TESSERA_EXPECT_EQ(pool.live_count(), std::size_t{2});
        pool.release(a);
        TESSERA_EXPECT_EQ(pool.live_count(), std::size_t{1});
        Node* c = pool.acquire(); // should recycle a's slot
        TESSERA_EXPECT_TRUE(c == a);
        pool.release(b);
        pool.release(c);
        TESSERA_EXPECT_EQ(pool.live_count(), std::size_t{0});
    }

    {
        TESSERA_EXPECT_EQ(util::next_power_of_two(0), std::uint64_t{1});
        TESSERA_EXPECT_EQ(util::next_power_of_two(17), std::uint64_t{32});
        TESSERA_EXPECT_TRUE(util::is_power_of_two(64));
        TESSERA_EXPECT_TRUE(!util::is_power_of_two(48));
        TESSERA_EXPECT_EQ(util::align_up(13, 8), std::size_t{16});
        TESSERA_EXPECT_EQ(util::popcount(0xF0FU), 8);
    }

    return g_failures - before;
}

} // namespace tessera::test
