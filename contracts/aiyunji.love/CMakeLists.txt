add_contract(aiyunji.love aiyunji.love ${CMAKE_CURRENT_SOURCE_DIR}/src/love.cpp)

target_include_directories(aiyunji.love
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(aiyunji.love
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/aiyunji.love.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/aiyunji.love.contracts.md @ONLY )

target_compile_options( aiyunji.love PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
