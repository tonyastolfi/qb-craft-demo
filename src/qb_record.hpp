#pragma once

#include <algorithm>
#include <array>
#include <iterator>
#include <string>
#include <tuple>

// TODO - for some reason std::option can't be found, even though I'm specifying
// c++17? Is this a limitation of my Clang version?
//
#include <boost/optional/optional.hpp>

// Traits class containing the metadata for a record.
//
struct QBRecordTraits {
  static const auto &column_names() {
    static const std::array<std::string, 4> columnNames = {
        {"column0", "column1", "column2", "column3"}};

    return columnNames;
  }

  static constexpr int unique_id_column() { return 0; }

  // unique id column type.
  //
  using unique_id_type = unsigned int;

  using columns_type =
      std::tuple<unique_id_type, std::string, long, std::string>;
};

/**
 * Represents a Record Object.
 */
using QBRecord = QBRecordTraits::columns_type;

// Attempt to parse `s` as a column name, given the passed record traits
// `Traits`.  `Traits` must expose a static method `column_names()` returning a
// sorted collection of strings which will be binary-searched to find `s`.
//
// Returns the index in `Traits::column_names()` of the matched name if found;
// boost::none otherwise.
//
template <typename Traits>
boost::optional<int> parse_column_name(std::string_view s) {
  // Binary search the column names to find the one we're interested in.
  //
  const auto name_range = std::equal_range(Traits::column_names().cbegin(),
                                           Traits::column_names().cend(), s);

  if (name_range.first == name_range.second) {
    return boost::none;
  }
  assert(std::distance(name_range.first, name_range.second) == 1);
  return std::distance(Traits::column_names().cbegin(), name_range.first);
}
