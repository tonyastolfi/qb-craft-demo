using _TCHAR = char;

#include <algorithm>
#include <array>
#include <assert.h>
#include <chrono>
#include <iostream>
#include <ratio>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/**
 * Represents a Record Object.
 */
struct QBRecord {
  uint column0; // unique id column
  std::string column1;
  long column2;
  std::string column3;
};

/**
 * Represents a Record Collection.
 */
class QBRecordCollection {
private:
  // Internal representation of a QBRecord.  Differs from the external
  // representation in that string-typed columns are pointers to their interned
  // (canonical) objects; also it does not need to store the primary key column.
  //
  struct QBRecordIntern {
    const std::string *column1;
    long column2;
    const std::string *column3;
  };

public:
  // unique id column type.
  //
  using primary_key_type = uint;

  // Inserts a new record into the collection.  If the record is already
  // present, return false and leave the collection unchanged.  Otherwise,
  // return true having successfully modified the collection.
  //
  bool Insert(QBRecord &&record) {
    if (by_column0_.count(record.column0)) {
      return false;
    }

    const std::string *const column1_intern =
        InternString(std::move(record.column1));

    const std::string *const column3_intern =
        InternString(std::move(record.column3));

    by_column0_.emplace(record.column0, QBRecordIntern{
                                            /*column1=*/column1_intern,
                                            /*column2=*/record.column2,
                                            /*column3=*/column3_intern,
                                        });

    by_column1_.emplace(column1_intern, record.column0);
    by_column2_.emplace(record.column2, record.column0);
    by_column3_.emplace(column3_intern, record.column0);

    return true;
  }

  /**
      Return records that contains a string in the StringValue field
      records - the initial set of records to filter
      matchString - the string to search for
  */
  std::vector<QBRecord>
  FindMatchingRecords(const std::string &columnName,
                      const std::string &matchString) const {
    static const std::array<std::string, 4> columnNames = {
        {"column0", "column1", "column2", "column3"}};

    // The set of matching records to return.
    //
    std::vector<QBRecord> results;

    // Binary search the column names to find the one we're interested in.
    //
    auto name_range =
        std::equal_range(columnNames.cbegin(), columnNames.cend(), columnName);

    // If the range is non-empty, then find matching records using the
    // appropriate per-column index.
    //
    if (name_range.first != name_range.second) {

      // -- Local Helper Functions ---------------------------------------------

      // Given a primary key value (column0), looks up and adds the
      // corresponding record to the result set, returning true if the record
      // was found, false otherwise.
      //
      const auto AddByColumn0 = [&](primary_key_type key) {
        auto record_iter = by_column0_.find(key);
        if (record_iter != by_column0_.end()) {
          results.emplace_back(QBRecord{
              key, *record_iter->second.column1, record_iter->second.column2,
              *record_iter->second.column3,
          });
          return true;
        }
        return false;
      };

      // Search for `matchString` in the string table; if found return the
      // interned string pointer, else return nullptr.
      //
      const auto InternMatchStr = [this,
                                   &matchString]() -> const std::string * {
        auto match_iter = strings_.find(matchString);
        if (match_iter == strings_.end()) {
          return nullptr;
        }
        return &*match_iter;
      };

      // Given `match_range`, a pair of ForwardIterator values defining a range
      // of secondary index items, look up the corresponding record by its
      // primary key (`second` of each item in the range) and add it to the
      // result set.
      //
      const auto AddMatchingRange = [&](const auto &match_range) {
        std::for_each(match_range.first, match_range.second,
                      [&](const auto &item) { AddByColumn0(item.second); });
      };

      // -----------------------------------------------------------------------

      // Use the appropriate index for the search column.
      //
      switch (std::distance(columnNames.cbegin(), name_range.first)) {
      case 0:
        AddByColumn0(std::stoul(matchString));
        break;
      case 1:
        AddMatchingRange(by_column1_.equal_range(InternMatchStr()));
        break;
      case 2:
        AddMatchingRange(by_column2_.equal_range(std::stol(matchString)));
        break;
      case 3:
        AddMatchingRange(by_column3_.equal_range(InternMatchStr()));
        break;
      default:
        std::cerr << "Invalid column name: " << columnName << std::endl;
        std::terminate();
      }
    }

    // Done!
    //
    return results;
  }

private:
  const std::string *InternString(std::string &&s) {
    return &*strings_.emplace(std::move(s)).first;
  }

  // All the strings referenced by the records of this collection. Rehashing
  // this container on insert may invalidate iterators (buckets) but it is
  // guaranteed *not* to invalidate the referenced objects, so this is safe.
  //
  std::unordered_set<std::string> strings_;

  // All the records in the collection, by primary key.
  //
  std::unordered_map<primary_key_type, QBRecordIntern> by_column0_;

  // Indices of all other columns.
  //
  std::unordered_multimap<const std::string *, primary_key_type> by_column1_;
  std::unordered_multimap<long, primary_key_type> by_column2_;
  std::unordered_multimap<const std::string *, primary_key_type> by_column3_;
};

/**
    Utility to populate a record collection
    prefix - prefix for the string value for every record
    numRecords - number of records to populate in the collection
*/
QBRecordCollection populateDummyData(const std::string &prefix,
                                     int numRecords) {
  QBRecordCollection data;
  for (uint i = 0; i < numRecords; i++) {
    QBRecord rec = {i, prefix + std::to_string(i), i % 100,
                    std::to_string(i) + prefix};
    data.Insert(std::move(rec));
  }
  return data;
}

int main(int argc, _TCHAR *argv[]) {
  using namespace std::chrono;

  // populate a bunch of data
  //
  auto data = populateDummyData("testdata", 100000);

  // Find a record that contains and measure the perf
  //
  auto startTimer = steady_clock::now();
  auto filteredSet = data.FindMatchingRecords("column1", "testdata500");
  auto filteredSet2 = data.FindMatchingRecords("column2", "24");

  std::cout << "search time: "
            << double((steady_clock::now() - startTimer).count()) * 1000.0 *
                   steady_clock::period::num / steady_clock::period::den
            << "ms" << std::endl;

  // make sure that the function is correct
  assert(filteredSet.size() == 1);
  return 0;
}
