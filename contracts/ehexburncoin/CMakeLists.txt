add_contract(ehexburncoin ehexburncoin
   ${CMAKE_CURRENT_SOURCE_DIR}/src/burncoin.cpp
)

target_include_directories(ehexburncoin
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(ehexburncoin
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/ehexburncoin.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/ehexburncoin.contracts.md @ONLY )

target_compile_options( ehexburncoin PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
