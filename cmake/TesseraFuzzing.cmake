# Fuzz harness integration.
#
# Under ClusterFuzzLite / OSS-Fuzz the environment provides sanitizer
# instrumentation via CFLAGS/CXXFLAGS and a fuzzing engine via
# $LIB_FUZZING_ENGINE; harnesses simply link that engine. For a local Clang
# toolchain (no $LIB_FUZZING_ENGINE) we fall back to -fsanitize=fuzzer directly.

function(tessera_add_fuzz_target target_name)
    add_executable(${target_name} ${ARGN})
    target_link_libraries(${target_name} PRIVATE Tessera::tessera)

    if(DEFINED ENV{LIB_FUZZING_ENGINE})
        target_link_libraries(${target_name} PRIVATE $ENV{LIB_FUZZING_ENGINE})
    else()
        target_compile_options(${target_name} PRIVATE -fsanitize=fuzzer,address,undefined)
        target_link_options(${target_name} PRIVATE -fsanitize=fuzzer,address,undefined)
    endif()
endfunction()
