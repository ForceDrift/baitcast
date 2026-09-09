cmake_minimum_required(VERSION 3.14)
project(baitcast LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(baitcast INTERFACE)
target_include_directories(baitcast INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

include(FetchContent)
set(FETCHCONTENT_QUIET ON)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(test_align test/test_align.cpp)
target_link_libraries(test_align PRIVATE baitcast GTest::gtest_main)
add_test(NAME test_align COMMAND test_align)

add_executable(test_coroutine test/test_coroutine.cpp)
target_link_libraries(test_coroutine PRIVATE baitcast GTest::gtest_main)
add_test(NAME test_coroutine COMMAND test_coroutine)