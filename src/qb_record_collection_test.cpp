#include "qb_record_collection.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <limits>
#include <random>

#include "baseline.hpp"
#include "timer.hpp"
#include "words.hpp"

namespace {

template <typename T> T make_copy(const T &val) { return val; }

// Test Plan:
//  1. Query empty database: no results
//  2. Fail to find match on:
//     a. unique_id
//     b. string field
//     c. long field
//  3. Find exactly one match for all present unique_ids
//  4. Single match for:
//     a. string field
//     b. long field
//  5. Multiple matches for:
//     a. string field
//     b. long field
//  6. Stress test (large database, >10 million entries)
//     Repeatedly query for:
//     a. no matches
//     b. one match
//     c. few matches
//     d. many matches
//     e. all of the above
//
class QBRecordCollectionTest : public ::testing::Test {
protected:
  void populateRecords(int count) {
    std::uniform_int_distribution<int> pick_word_index(0, words_.size() - 1);
    std::uniform_int_distribution<int> pick_word_count(1, 3);
    std::uniform_int_distribution<long> pick_long(-count / 4, count / 4);

    const auto random_string = [&] {
      std::ostringstream oss;
      const int wc = pick_word_count(rng_);
      for (int n = 0; n < wc; ++n) {
        const int i = pick_word_index(rng_);
        oss << words_[i];
      }
      return std::move(oss).str();
    };

    // Add id = {0, 1, ..., count - 1} in some pseudo-random order.
    //
    std::vector<int> ids(count);
    std::iota(ids.begin(), ids.end(), 0);
    std::shuffle(ids.begin(), ids.end(), rng_);

    base_.resize(count);
    for (const int id : ids) {
      QBRecord record{id, random_string(), pick_long(rng_), random_string()};
      base_[id] = baseline::QBRecord{
          std::get<0>(record), std::get<1>(record), std::get<2>(record),
          std::get<3>(record),
      };
      db_.insert(std::move(record));
    }
  }

  std::default_random_engine rng_{/*seed=*/1};

  // All three-letter dictionary words.
  //
  std::vector<std::string> words_ =
      load_words([](std::string_view word) { return word.length() == 3; });

  // The collection under test.
  //
  QBRecordCollection db_;

  // The baseline implementation.
  //
  baseline::QBRecordCollection base_;
};

//  1. Query empty database: no results
//
TEST_F(QBRecordCollectionTest, QueryEmpty) {
  auto results = db_.find_matching_records("column1", "5");
  EXPECT_THAT(results, ::testing::IsEmpty());
}

//  2. Fail to find match on:
//     a. unique_id
//
TEST_F(QBRecordCollectionTest, UniqueIdNotFound) {
  populateRecords(100);
  {
    auto results = db_.find_matching_records("column0", "100");
    EXPECT_THAT(results, ::testing::IsEmpty());
  }
  {
    auto results = db_.find_matching_records("column0", "101");
    EXPECT_THAT(results, ::testing::IsEmpty());
  }
  {
    auto results = db_.find_matching_records("column0", "-1");
    EXPECT_THAT(results, ::testing::IsEmpty());
  }
}

//  2. Fail to find match on:
//     b. string field
//
TEST_F(QBRecordCollectionTest, StringMatchNotFound) {
  populateRecords(100);

  auto results = db_.find_matching_records("column1", "crazyStringADSOIJAS");

  EXPECT_THAT(results, ::testing::IsEmpty());
}

//  2. Fail to find match on:
//     c. long field
//
TEST_F(QBRecordCollectionTest, IntegerNotFound) {
  populateRecords(100);

  auto results = db_.find_matching_records("column2", "200");

  EXPECT_THAT(results, ::testing::IsEmpty());
}

//  3. Find exactly one match for all present unique_ids
//
TEST_F(QBRecordCollectionTest, AllUniqueIdsFound) {
  populateRecords(100);

  for (unsigned id = 0; id < 100; ++id) {
    auto results = db_.find_matching_records(
        "column0", boost::lexical_cast<std::string>(id));

    ASSERT_THAT(results, ::testing::SizeIs(1));
    EXPECT_EQ(std::get<0>(results[0]), id);

    // Compare to baseline record.
    //
    EXPECT_EQ(std::get<1>(results[0]), base_[id].column1);
    EXPECT_EQ(std::get<2>(results[0]), base_[id].column2);
    EXPECT_EQ(std::get<3>(results[0]), base_[id].column3);
  }
}

//  4. Single match for:
//     a. string field
//
TEST_F(QBRecordCollectionTest, StringSingleMatch) {
  populateRecords(100);

  std::string pattern;
  for (const auto &r : base_) {
    if (r.column1.length() == 9) {
      pattern = r.column1.substr(2, 6);
      if (baseline::QBFindMatchingRecords(base_, "column1", pattern).size() ==
          1) {
        break;
      }
    }
  }

  auto results = db_.find_matching_records("column1", pattern);
  ASSERT_THAT(results, ::testing::SizeIs(1)) << "pattern=" << pattern;
  EXPECT_THAT(std::get<1>(results[0]), ::testing::HasSubstr(pattern));
}

//  4. Single match for:
//     b. long field
//
TEST_F(QBRecordCollectionTest, IntegerSingleMatch) {
  populateRecords(100);

  const auto custom_record = QBRecord{100, "hello", 999, "world"};

  db_.insert(make_copy(custom_record));
  auto results = db_.find_matching_records("column2", "999");

  ASSERT_THAT(results, ::testing::SizeIs(1));
  EXPECT_EQ(results[0], custom_record);
}

//  5. Multiple matches for:
//     a. string field
//
TEST_F(QBRecordCollectionTest, StringManyMatch) {
  populateRecords(100);

  auto results = db_.find_matching_records("column3", "e");

  EXPECT_GT(results.size(), 2u);
}

//  5. Multiple matches for:
//     b. long field
//
TEST_F(QBRecordCollectionTest, IntegerManyMatch) {}

TEST_F(QBRecordCollectionTest, Perf) {
  using std::chrono::steady_clock;

  std::cerr << "RECORDS LOOP NEW_ID(q/s) BASE_ID(q/s) NEW_NUM(q/s) "
               "BASE_NUM(q/s) NEW_STR(q/s) BASE_STR(q/s)"
            << std::endl;

  for (int count : {10, 100, 1000, 10 * 1000}) {
    populateRecords(count);
    for (int loop = 0; loop < 10; ++loop) {
      std::cerr << count << " " << loop;

      // Search for all ids.
      //
      {
        auto start = steady_clock::now();
        for (QBRecordCollection::unique_id_type id = 0; id < count; ++id) {
          db_.find_matching_records("column0", std::to_string(id));
        }
        std::cerr << " " << count / elapsed_seconds(start);
      }
      {
        auto start = steady_clock::now();
        for (QBRecordCollection::unique_id_type id = 0; id < count; ++id) {
          QBFindMatchingRecords(base_, "column0", std::to_string(id));
        }
        std::cerr << " " << count / elapsed_seconds(start);
      }

      // Search for all nums.
      //
      {
        auto start = steady_clock::now();
        for (long num = -count / 4; num <= count / 4; ++num) {
          db_.find_matching_records("column2", std::to_string(num));
        }
        std::cerr << " " << count / 2 / elapsed_seconds(start);
      }
      {
        auto start = steady_clock::now();
        for (long num = -count / 4; num <= count / 4; ++num) {
          QBFindMatchingRecords(base_, "column2", std::to_string(num));
        }
        std::cerr << " " << count / 2 / elapsed_seconds(start);
      }

      // Search for all words.
      //
      {
        auto start = steady_clock::now();
        for (const auto &w : words_) {
          db_.find_matching_records("column1", w);
        }
        std::cerr << " " << words_.size() / elapsed_seconds(start);
      }
      {
        auto start = steady_clock::now();
        for (const auto &w : words_) {
          QBFindMatchingRecords(base_, "column1", w);
        }
        std::cerr << " " << words_.size() / elapsed_seconds(start);
      }

      std::cerr << std::endl;
    }
  }
}

} // namespace
