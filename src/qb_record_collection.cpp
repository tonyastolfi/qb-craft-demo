#include "qb_record_collection.hpp"

#include <boost/lexical_cast.hpp>

// Local helper functions.
//
namespace {

decltype(auto) get_unique_id(const QBRecordCollection::record_type &record) {
  return std::get<QBRecordCollection::traits_type::unique_id_column()>(record);
}

} // namespace

bool QBRecordCollection::insert(record_type &&record) {
  const auto id = get_unique_id(record);
  if (by_unique_id_.count(id)) {
    return false;
  }

  auto[iter, inserted] =
      by_unique_id_.emplace(id, drop_first(std::move(record)));

  assert(inserted);

  const auto &stored = iter->second;

  for_each_upto<num_columns() - 1>([&](auto i) {
    constexpr int I = decltype(i)::value;
    std::get<I>(lookups_).insert(id, std::get<I>(stored));
  });

  return true;
}

auto QBRecordCollection::find_matching_records(
    std::string_view columnName, std::string_view matchString) const
    -> std::vector<record_type> {

  auto maybe_column_num = parse_column_name<QBRecordTraits>(columnName);
  if (!maybe_column_num) {
    // TODO - maybe report this error in a more dramatic way?
    return {};
  }
  const int column_num = *maybe_column_num;

  // The set of matching records to return.
  //
  std::vector<record_type> results;

  // -- Local Helper Functions ---------------------------------------------
  // Given a unique id (column0), looks up and adds the corresponding record
  // to the result set, returning true if the record was found, false
  // otherwise.
  //
  const auto addByUniqueId = [&](unique_id_type key) {
    auto record_iter = by_unique_id_.find(key);
    if (record_iter != by_unique_id_.end()) {
      results.emplace_back(
          std::tuple_cat(std::make_tuple(key), record_iter->second));
      return true;
    }
    return false;
  };
  // -----------------------------------------------------------------------

  // Find and add all matching records.
  //
  if (column_num == QBRecordTraits::unique_id_column()) {
    // Add the record with the specified id directly.
    //
    addByUniqueId(boost::lexical_cast<unique_id_type>(matchString));
  } else {
    // Use the appropriate index for the search column.
    //
    visit_tuple_element(
        column_num - 1, lookups_, [&](const auto &column_lookup) {
          column_lookup.for_each_match(matchString, addByUniqueId);
        });
  }

  return results;
}
