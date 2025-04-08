cmake_minimum_required(VERSION 3.10)
project(SAT_DPLL)

set(CMAKE_CXX_STANDARD 11)

include_directories(allocator)
include_directories(src)

file(GLOB SOURCES "src/*.cpp" "allocator/*.cpp")

add_executable(SAT_DPLL ${SOURCES})