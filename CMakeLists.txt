project(QuickBaseCraftDemo)
cmake_minimum_required(VERSION 2.8.12)

add_definitions("-std=c++17 -DBOOST_ERROR_CODE_HEADER_ONLY")
include_directories(. ./src)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_library(QBCraftDemo
            src/qb_column_lookup.cpp
            src/qb_record_collection.cpp)

add_executable(StringTrieTest src/string_trie_test.cpp)
target_link_libraries(StringTrieTest ${CONAN_LIBS_GTEST})

add_executable(QBRecordCollectionTest src/qb_record_collection_test.cpp)
target_link_libraries(QBRecordCollectionTest QBCraftDemo ${CONAN_LIBS_GTEST})

enable_testing()

add_test(NAME StringTrie
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
         COMMAND StringTrieTest)

add_test(NAME QBRecordCollection
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
         COMMAND QBRecordCollectionTest)
