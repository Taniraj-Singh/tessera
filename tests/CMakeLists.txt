add_executable(tessera_tests
    test_main.cpp
    support/test_runner.cpp
    unit/core_tests.cpp
    unit/memory_tests.cpp
    unit/encoding_tests.cpp
    unit/compress_tests.cpp
    unit/index_tests.cpp
    unit/reflect_tests.cpp
    unit/schema_tests.cpp
    unit/archive_tests.cpp
    unit/pipeline_tests.cpp
    unit/observability_tests.cpp
    unit/lifecycle_tests.cpp
    unit/introspection_tests.cpp
    integration/integration_tests.cpp
    regression/regression_tests.cpp
    edge/edge_tests.cpp
)

target_include_directories(tessera_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tessera_tests PRIVATE Tessera::tessera)

add_test(NAME tessera_tests COMMAND tessera_tests)
