cmake_minimum_required(VERSION 3.29)
project(btree)

set(CMAKE_CXX_STANDARD 23)

add_library(btree INTERFACE
        include/btree.h)
target_include_directories(btree INTERFACE ${CMAKE_SOURCE_DIR}/include)

include(CTest)
enable_testing()
add_subdirectory(test)
