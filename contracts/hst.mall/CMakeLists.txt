add_contract(hst.mall hst.mall ${CMAKE_CURRENT_SOURCE_DIR}/src/mall.cpp)

target_include_directories(hst.mall
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(hst.mall
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/hst.mall.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/hst.mall.contracts.md @ONLY )

target_compile_options( hst.mall PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
