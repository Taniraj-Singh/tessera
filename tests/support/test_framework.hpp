#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

namespace tessera::test {

inline int g_failures = 0;

inline void check(bool condition, const char* expression, const char* file, int line) {
    if (!condition) {
        ++g_failures;
        std::cerr << file << ':' << line << ": assertion failed: " << expression << '\n';
    }
}

} // namespace tessera::test

#define TESSERA_EXPECT_TRUE(expr) ::tessera::test::check(static_cast<bool>(expr), #expr, __FILE__, __LINE__)
#define TESSERA_EXPECT_EQ(lhs, rhs) ::tessera::test::check((lhs) == (rhs), #lhs " == " #rhs, __FILE__, __LINE__)

#define TESSERA_TEST(name) \
    void tessera_test_##name(); \
    struct TesseraTestRegistrar_##name { \
        TesseraTestRegistrar_##name() { \
            std::cout << "Running " #name "... "; \
            tessera_test_##name(); \
            if (::tessera::test::g_failures == 0) { \
                std::cout << "ok\n"; \
            } else { \
                std::cout << "failed\n"; \
            } \
        } \
    } tessera_test_registrar_##name; \
    void tessera_test_##name()
