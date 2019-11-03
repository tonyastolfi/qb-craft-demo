#pragma once

#include <assert.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

inline std::vector<std::string> load_words() {
  std::vector<std::string> words;
  {
    std::ifstream ifs("/usr/share/dict/words");
    assert(ifs.good());

    std::string line;
    while (ifs.good()) {
      std::getline(ifs, line);
      words.emplace_back(line);
    }
  }
  return words;
}
