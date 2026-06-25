cmake_minimum_required(VERSION 3.24)
project(ArmPrep)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(ArmPrep main.cpp)

set_property(TARGET ArmPrep PROPERTY COMPILE_WARNING_AS_ERROR ON)
target_compile_options(ArmPrep PRIVATE -Wall -Wextra -Wpedantic -Werror -Wdeprecated-copy-dtor)