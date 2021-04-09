add_contract(hst.token hst.token ${CMAKE_CURRENT_SOURCE_DIR}/src/hst.token.cpp)

target_include_directories(hst.token
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(hst.token
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/hst.token.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/hst.token.contracts.md @ONLY )

target_compile_options( hst.token PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
