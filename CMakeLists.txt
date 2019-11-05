project(QuickBaseCraftDemo)
cmake_minimum_required(VERSION 2.8.12)

add_definitions("-std=c++17 -DBOOST_ERROR_CODE_HEADER_ONLY")
include_directories(. ./src)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_library(QBCraftDemo
            src/qb_record_collection.cpp)

add_executable(TrieTest src/trie_test.cpp)
target_link_libraries(TrieTest ${CONAN_LIBS_GTEST})

enable_testing()
add_test(NAME Trie
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
         COMMAND TrieTest)
