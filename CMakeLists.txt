cmake_minimum_required(VERSION 3.24)
project(CppConcurrency)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(ENABLE_TSAN "Build with ThreadSanitizer" OFF)

if(ENABLE_TSAN)
    add_compile_options(-fsanitize=thread -g)
    add_link_options(-fsanitize=thread)
endif()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)

# Suppress warnings for outdated cmake versions of libraries
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(test)

set(L1_CACHE_LINE_SIZE 64 CACHE STRING "L1 Data cache line size for target hardware")
