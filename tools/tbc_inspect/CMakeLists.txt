add_executable(tbc_inspect main.cpp)
target_link_libraries(tbc_inspect PRIVATE Tessera::tessera)

install(TARGETS tbc_inspect RUNTIME DESTINATION bin)
