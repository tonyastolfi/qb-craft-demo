using _TCHAR = char;

#include <algorithm>
#include <array>
#include <assert.h>
#include <chrono>
#include <iostream>
#include <ratio>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
