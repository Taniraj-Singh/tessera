#include "support/test_helpers.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    const int failures = tessera::test::run_all_tests();
    if (failures != 0) {
        std::cerr << failures << " test group failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "All Tessera tests passed\n";
    return EXIT_SUCCESS;
}
