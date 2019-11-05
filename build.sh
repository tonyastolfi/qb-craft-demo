#!/bin/bash
set -e
cd ~/projects/quickbase_interview/tastolfi_craft_demo/build/
conan install ..
cmake -DCMAKE_BUILD_TYPE=Release ..
make VERBOSE=1
bin/StringTrieTest
bin/QBRecordCollectionTest
