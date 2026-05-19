cmake_minimum_required(VERSION 3.20)
project(CompressionTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /Ox /Ot /Oi)
else()
    add_compile_options(-Wall -Wextra -O3 -march=native)
endif()

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_library(CompressionTool STATIC
    src/BitReader.cpp
    src/BitWriter.cpp
)
target_include_directories(CompressionTool PUBLIC include)

add_executable(tests tests/BitReaderWriter_test.cpp)
target_link_libraries(tests PRIVATE CompressionTool GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(tests)