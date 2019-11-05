#include "trie.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <functional>

#include "timer.hpp"
#include "words.hpp"

namespace {

using ::testing::Return;

class MockIndexFn {
public:
  MOCK_CONST_METHOD1(invoke, void(int));

  void operator()(int n) const { invoke(n); }
};

TEST(BranchSetTest, Smoke) {
  BranchSet b;
  {
    MockIndexFn fn;
    b.for_each(std::cref(fn));
  }
  for (char ch : std::string("hello, world")) {
    b.set((int)ch, true);
  }
  {
    MockIndexFn fn;

    EXPECT_CALL(fn, invoke(' ')).WillOnce(Return());
    EXPECT_CALL(fn, invoke(',')).WillOnce(Return());
    EXPECT_CALL(fn, invoke('d')).WillOnce(Return());
    EXPECT_CALL(fn, invoke('e')).WillOnce(Return());
    EXPECT_CALL(fn, invoke('h')).WillOnce(Return());
    EXPECT_CALL(fn, invoke('l')).WillOnce(Return());
    EXPECT_CALL(fn, invoke('o')).WillOnce(Return());
    EXPECT_CALL(fn, invoke('r')).WillOnce(Return());
    EXPECT_CALL(fn, invoke('w')).WillOnce(Return());

    b.for_each(std::cref(fn));
  }
}

TEST(TrieTest, SubstringSearch) {
  using std::chrono::steady_clock;

  // Load our corpus.
  //
  const std::vector<std::string> words = load_words();

  // Build the search index.
  //
  auto start0 = steady_clock::now();
  std::cerr << " building index..." << std::flush;
  auto index_ptr = std::make_unique<StringTrie<int>>();
  StringTrie<int> &index = *index_ptr;
  for (int i = 0; i < words.size(); ++i) {
    index.insert_suffixes(words[i], i);
  }
  std::cerr << " done. (" << elapsed_seconds(start0) << "s)" << std::endl;

  // Search various patterns.
  //
  double total_slow = 0, total_fast = 0;

  for (const char *pattern : {"x", "ill", "zing", "uniquely", "notawordXYZ",
                              "bob", "aa", "niqu", "ly", "are", "ss", "ZZG",
                              "raft", "th", "lo", "term", "expect", "lease"}) {
    auto start1 = steady_clock::now();
    std::set<int> expected;
    for (int i = 0; i < words.size(); ++i) {
      if (words[i].find(pattern) != std::string::npos) {
        expected.insert(i);
      }
    }
    total_slow += elapsed_seconds(start1);
    if (!std::strcmp(pattern, "uniquely")) {
      EXPECT_EQ(expected.size(), 1u);
    }
    if (!std::strcmp(pattern, "notawordXYZ")) {
      EXPECT_EQ(expected.size(), 0u);
    }

    auto start2 = steady_clock::now();
    std::set<int> actual;
    index.for_each_prefix_match(pattern, [&](int i) { actual.insert(i); });
    total_fast += elapsed_seconds(start2);

    EXPECT_THAT(actual, ::testing::ContainerEq(expected));
  }

  std::cerr << "no index:   " << total_slow << "s\n"
            << "with index: " << total_fast << "s" << std::endl;

  auto start3 = steady_clock::now();
  std::cerr << " deleting index..." << std::flush;
  index_ptr.reset();
  std::cerr << " done. (" << elapsed_seconds(start3) << "s)" << std::endl;
}

} // namespace
