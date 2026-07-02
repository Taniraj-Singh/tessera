#include "support/test_framework.hpp"
#include "support/test_helpers.hpp"

namespace tessera::test {

int run_core_tests();
int run_memory_tests();
int run_encoding_tests();
int run_compress_tests();
int run_index_tests();
int run_reflect_tests();
int run_schema_tests();
int run_archive_tests();
int run_pipeline_tests();
int run_observability_tests();
int run_lifecycle_tests();
int run_introspection_tests();
int run_textio_tests();
int run_query_tests();
int run_integration_tests();
int run_regression_tests();
int run_edge_tests();

int run_all_tests() {
    int failures = 0;
    failures += run_core_tests();
    failures += run_memory_tests();
    failures += run_encoding_tests();
    failures += run_compress_tests();
    failures += run_index_tests();
    failures += run_reflect_tests();
    failures += run_schema_tests();
    failures += run_archive_tests();
    failures += run_pipeline_tests();
    failures += run_observability_tests();
    failures += run_lifecycle_tests();
    failures += run_introspection_tests();
    failures += run_textio_tests();
    failures += run_query_tests();
    failures += run_integration_tests();
    failures += run_regression_tests();
    failures += run_edge_tests();
    return failures;
}

} // namespace tessera::test
