add_contract(aiyunji.mall aiyunji.mall ${CMAKE_CURRENT_SOURCE_DIR}/src/aiyunji.mall.cpp)

target_include_directories(aiyunji.mall
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(aiyunji.mall
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/aiyunji.mall.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/aiyunji.mall.contracts.md @ONLY )

target_compile_options( aiyunji.mall PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
