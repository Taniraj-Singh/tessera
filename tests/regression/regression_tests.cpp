#include "support/test_framework.hpp"

#include "tessera/tessera.hpp"

namespace tessera::test {

int run_regression_tests() {
    int before = g_failures;

    std::vector<std::uint8_t> truncated = {0x54, 0x42, 0x43, 0x53};
    if (auto parsed = parse::parse_container(core::ByteSpan{truncated.data(), truncated.size()}, {}); parsed) {
        TESSERA_EXPECT_TRUE(false);
    } else {
        TESSERA_EXPECT_TRUE(parsed.error().code == core::ErrorCode::TruncatedInput);
    }

    return g_failures - before;
}

} // namespace tessera::test
