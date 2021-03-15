add_contract(aiyunji.mall.lite aiyunji.mall.lite ${CMAKE_CURRENT_SOURCE_DIR}/src/mall.cpp)

target_include_directories(aiyunji.mall.lite
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(aiyunji.mall.lite
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/aiyunji.mall.lite.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/aiyunji.mall.lite.contracts.md @ONLY )

target_compile_options( aiyunji.mall.lite PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
