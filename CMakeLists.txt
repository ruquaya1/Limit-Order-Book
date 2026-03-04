cmake_minimum_required(VERSION 3.20)
project(hplob LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(HPLob_BUILD_TESTS "Build tests" ON)
option(HPLob_BUILD_EXAMPLES "Build examples" ON)

add_library(hplob
  src/order_book.cpp
)

target_include_directories(hplob PUBLIC include)
target_compile_features(hplob PUBLIC cxx_std_20)

if(HPLob_BUILD_EXAMPLES)
  add_executable(basic_sim examples/basic_sim.cpp)
  target_link_libraries(basic_sim PRIVATE hplob)
endif()

if(HPLob_BUILD_TESTS)
  add_executable(hplob_tests tests/test_main.cpp)
  target_link_libraries(hplob_tests PRIVATE hplob)
endif()
