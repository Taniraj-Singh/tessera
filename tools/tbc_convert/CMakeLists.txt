add_executable(tbc_convert main.cpp)
target_link_libraries(tbc_convert PRIVATE Tessera::tessera)

install(TARGETS tbc_convert RUNTIME DESTINATION bin)
