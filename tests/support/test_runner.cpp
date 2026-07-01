#include "support/test_framework.hpp"
#include "support/test_helpers.hpp"

namespace tessera::test {

int run_core_tests();
int run_integration_tests();
int run_regression_tests();
int run_edge_tests();

int run_all_tests() {
    int failures = 0;
    failures += run_core_tests();
    failures += run_integration_tests();
    failures += run_regression_tests();
    failures += run_edge_tests();
    return failures;
}

} // namespace tessera::test
