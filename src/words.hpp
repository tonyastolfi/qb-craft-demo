#pragma once

#include <assert.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Loads and returns the contents of the MacOSX system dictionary.  Each word is
// on its own
// line; the dictionary is in alphabetical order.
//
inline std::vector<std::string> load_words() {
  std::vector<std::string> words;
  {
    std::ifstream ifs("/usr/share/dict/words");
    assert(ifs.good());

    while (ifs.good()) {
      std::string line;
      std::getline(ifs, line);
      words.emplace_back(std::move(line));
    }
  } // end of scope closes the file.

  return words;
}
