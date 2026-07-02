add_executable(tbc_archive main.cpp)
target_link_libraries(tbc_archive PRIVATE Tessera::tessera)

install(TARGETS tbc_archive RUNTIME DESTINATION bin)
