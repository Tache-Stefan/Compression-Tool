cmake_minimum_required(VERSION 3.20)
project(CompressionTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /Ox /Ot /Oi)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -march=native)
endif()

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)
FetchContent_MakeAvailable(benchmark)

enable_testing()

add_library(CompressionTool STATIC
    src/BitReader.cpp
    src/BitWriter.cpp
    src/LZ77.cpp
    src/Engine.cpp
)
target_include_directories(CompressionTool PUBLIC include)

add_executable(archiver src/main.cpp)
target_link_libraries(archiver PRIVATE CompressionTool)

add_executable(tests 
    tests/BitReaderWriter_test.cpp
    tests/LZ77_test.cpp)
target_link_libraries(tests PRIVATE CompressionTool GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(tests)

add_executable(compression_benchmark benchmarks/compression_benchmark.cpp)
target_link_libraries(compression_benchmark PRIVATE CompressionTool benchmark::benchmark)