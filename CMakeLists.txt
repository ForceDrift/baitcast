cmake_minimum_required(VERSION 3.14)
project(baitcast LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(baitcast INTERFACE)
target_include_directories(baitcast INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_align test/test_align.cpp)
target_link_libraries(test_align PRIVATE baitcast)
