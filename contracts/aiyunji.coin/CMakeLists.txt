add_contract(aiyunji.coin aiyunji.coin ${CMAKE_CURRENT_SOURCE_DIR}/src/aiyunji.coin.cpp)

target_include_directories(aiyunji.coin
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(aiyunji.coin
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/aiyunji.coin.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/aiyunji.coin.contracts.md @ONLY )

target_compile_options( aiyunji.coin PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
