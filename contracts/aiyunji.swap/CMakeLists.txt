add_contract(aiyunji.swap aiyunji.swap ${CMAKE_CURRENT_SOURCE_DIR}/src/swap.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/swap_token.cpp)

target_include_directories(aiyunji.swap
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(aiyunji.swap
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/aiyunji.swap.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/aiyunji.swap.contracts.md @ONLY )

target_compile_options( aiyunji.swap PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
