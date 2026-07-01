cmake_minimum_required(VERSION 3.20)

project(
    Tessera
    VERSION 1.0.0
    DESCRIPTION "Tessera Binary Container SDK"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(TesseraOptions)
include(TesseraCompilerWarnings)
include(TesseraSanitizers)

add_library(tessera)
add_library(Tessera::tessera ALIAS tessera)

target_include_directories(
    tessera
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(tessera PUBLIC cxx_std_20)
tessera_apply_warnings(tessera)

add_subdirectory(src)

if(TESSERA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(TESSERA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(TESSERA_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(TESSERA_BUILD_FUZZ)
    include(TesseraFuzzing)
    add_subdirectory(fuzz)
endif()

install(
    TARGETS tessera
    EXPORT TesseraTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/tessera DESTINATION include)

install(
    EXPORT TesseraTargets
    FILE TesseraTargets.cmake
    NAMESPACE Tessera::
    DESTINATION lib/cmake/Tessera
)
