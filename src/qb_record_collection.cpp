#include "qb_record_collection.hpp"

bool QBRecordCollection::insert(QBRecord &&record) {
  if (by_unique_id_.count(record.column0)) {
    return false;
  }

  by_unique_id_.emplace(record.column0,
                        QBRecordIntern{
                            /*column1=*/std::move(record.column1),
                            /*column2=*/record.column2,
                            /*column3=*/std::move(record.column3),
                        });

  by_column1_.insert_suffixes(record.column1, record.column0);
  by_column2_.emplace(record.column2, record.column0);
  by_column3_.insert_suffixes(record.column3, record.column0);

  return true;
}

std::vector<QBRecord> QBRecordCollection::find_matching_records(
    const std::string &columnName, const std::string &matchString) const {

  auto maybe_column_num = parse_column_name<QBRecordTraits>(columnName);
  if (!maybe_column_num.has_value()) {
    // TODO - maybe report this error in a more dramatic way?
    return {};
  }
  const int column_num = *maybe_column_num;

  // The set of matching records to return.
  //
  std::vector<QBRecord> results;

  // -- Local Helper Functions ---------------------------------------------

  // Given a unique id (column0), looks up and adds the corresponding record
  // to the result set, returning true if the record was found, false
  // otherwise.
  //
  const auto addByUniqueId = [&](unique_id_type key) {
    auto record_iter = by_unique_id_.find(key);
    if (record_iter != by_unique_id_.end()) {
      results.emplace_back(QBRecord{
          key, *record_iter->second.column1, record_iter->second.column2,
          *record_iter->second.column3,
      });
      return true;
    }
    return false;
  };

  // Given `match_range`, a pair of ForwardIterator values defining a range
  // of secondary index items, look up the corresponding record by its
  // unique id (`second` of each item in the range) and add it to the
  // result set.
  //
  const auto addMatchingRange = [&](const auto &match_range) {
    std::for_each(match_range.first, match_range.second,
                  [&](const auto &item) { addByColumn0(item.second); });
  };

  // -----------------------------------------------------------------------

  if (column_num == QBRecordTraits::unique_id_column()) {
    addByUniqueId(std::stoul(matchString));
  } else {
  }

  // If the range is non-empty, then find matching records using the
  // appropriate per-column index.
  //

  // Use the appropriate index for the search column.
  //
  switch () {
  case 0:
    addByColumn0(std::stoul(matchString));
    break;
  case 1:
    by_column1_.for_each_prefix_match(matchString, addByColumn0);
    break;
  case 2:
    addMatchingRange(by_column2_.equal_range(std::stol(matchString)));
    break;
  case 3:
    by_column3_.for_each_prefix_match(matchString, addByColumn0);
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

const std::string *InternString(std::string &&s) {
  return &*strings_.emplace(std::move(s)).first;
}
