// QBColumnLookup & helper utilities - a generalized per-column indexing
// mechanism.
//
#pragma once

#include <algorithm>
#include <string_view>
#include <unordered_map>

#include <boost/lexical_cast.hpp>

#include "string_trie.hpp"
#include "tuples.hpp"

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

  void for_each_match(std::string_view matchString,
                      std::function<void(UniqueId)> emitRecord) const;

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

  void for_each_match(std::string_view matchString,
                      std::function<void(UniqueId)> emitRecord) const;

private:
  // TODO - fix this; this is needed because the way we are generically
  // transforming a record tuple into a tuple of QBColumnLookup objects requires
  // copy/move construction (which is currently not implemented in StringTrie).
  //
  std::unique_ptr<StringTrie<UniqueId>> impl_ =
      std::make_unique<StringTrie<UniqueId>>();
};

//
// end - QBColumnLookup types.

// Returns a tuple of lookup tables for the columns of a given record type.
//
template <typename Tuple> auto make_lookups_for_columns(Tuple &&record) {
  using FirstColumn = std::tuple_element_t<0, std::decay_t<Tuple>>;
  using OtherColumns = decltype(drop_first(std::forward<Tuple>(record)));

  using UniqueId = FirstColumn;

  return tuple_transform(
      drop_first(
          std::forward<Tuple>(record)), // Don't include the Unique Id column
      [](auto &&column) {
        return QBColumnLookup<UniqueId, std::decay_t<decltype(column)>>{};
      });
}

template <typename RecordTuple>
using LookupsForRecord =
    decltype(make_lookups_for_columns(std::declval<RecordTuple>()));

// =============================================================================
// Partial Template Instantiation Impls
// =============================================================================

// -- Integer (long) lookup ----------------------------------------------------
//
template <typename UniqueId>
void QBColumnLookup<UniqueId, long>::insert(UniqueId rowId, long columnValue) {
  impl_.emplace(columnValue, rowId);
}

template <typename UniqueId>
void QBColumnLookup<UniqueId, long>::for_each_match(
    std::string_view matchString,
    std::function<void(UniqueId)> emitRecord) const {
  // Parse the matchString.
  //
  const long matchNumber = boost::lexical_cast<long>(matchString);

  // Find it in the hash table.
  //
  const auto found = impl_.equal_range(matchNumber);

  // Emit all matches.
  //
  std::for_each(found.first, found.second,
                [&](const auto &item) { emitRecord(item.second); });
}

// -- String lookup ------------------------------------------------------------
//
template <typename UniqueId>
void QBColumnLookup<UniqueId, std::string>::insert(UniqueId rowId,
                                                   std::string_view value) {
  impl_->insert_suffixes(value, rowId);
}

template <typename UniqueId>
void QBColumnLookup<UniqueId, std::string>::for_each_match(
    std::string_view matchString,
    std::function<void(UniqueId)> emitRecord) const {
  impl_->for_each_prefix_match(matchString, emitRecord);
}
