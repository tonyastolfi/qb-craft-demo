// QBColumnLookup & helper utilities - a generalized per-column indexing
// mechanism.
//
#pragma once

#include <string_view>
#include <unordered_multimap>

#include "StringTrie.hpp"

/* Lookup table type for a given column type (Value) and unique id type
 * (UniqueId).
 *
 * Instantiations of this template must meet the following type requirements:
 *
 * ```
 * QBColumnLookup<Id, T> col;
 *
 * // Add a row with `id` in this column with value `value` to the lookup table.
 * //
 * Id id;
 * T value;
 * col.insert(id, value);
 *
 * // Invoke `emitRecord` for all row ids in the lookup table whose value
 * // matches `matchString`.
 * std::string_view matchString;
 * std::function<void(UniqueId)> emitRecord;
 * col.find_matching_records(matchString, emitRecord);
 * ```
 */
template <typename UniqueId, typename Value> class QBColumnLookup;

// Column lookup table for `long` types; requires exact match of values.  Uses
// hash table to do lookup.
//
template <typename UniqueId> class QBColumnLookup<UniqueId, long> {
public:
  void insert(UniqueId rowId, long columnValue);

  void find_matching_records(std::string_view matchString,
                             std::function<void(UniqueId)> emitRecord);

private:
  std::unordered_multimap<long, UniqueId> impl_;
};

// Column lookup table for `std::string` types; supports substring matching.
// Uses StringTrie to do efficient lookups at the cost of additional memory and
// insertion time.
//
template <typename UniqueId> class QBColumnLookup<UniqueId, std::string> {
public:
  void insert(UniqueId rowId, std::string_view value);

  void find_matching_records(std::string_view matchString,
                             std::function<void(UniqueId)> emitRecord);

private:
  StringTrie<UniqueId> impl_;
};

//
// end - QBColumnLookup types.

// Returns a tuple of lookup tables for the columns of a given record type.
//
template <typename Tuple> auto make_lookups_for_columns(Tuple &&record) {
  using FirstColumn = std::tuple_element_t<std::decay_t<Tuple>, 0>;
  using OtherColumns = decltype(drop_first(std::forward<Tuple>(record)));

  using UniqueId = FirstColumn;

  return tuple_map(
      drop_first(
          std::forward<Tuple>(record)), // Don't include the Unique Id column
      [](auto &&column) {
        return QBColumnLookup<UniqueId, std::decay_t<decltype(column)>>{};
      });
}

template <typename RecordTuple>
using LookupsForRecord =
    decltype(make_lookups_for_columns(std::declval<RecordTuple>()));
