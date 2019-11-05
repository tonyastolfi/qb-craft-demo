#pragma once

#include <string_view>
#include <unordered_map>

#include "qb_record.hpp"

/**
 * Represents a Record Collection.
 */
class QBRecordCollection {
private:
  // Internal representation of a QBRecord.  Differs from the external
  // representation in that it does not need to store the unique id column.
  //
  using QBRecordIntern = decltype(drop_first(std::declval<QBRecord>()));

  static_assert(QBRecordTraits::unique_id_column() == 0,
                "The first column must be the unique id."); // TODO - relax this
                                                            // requirement.

  // Lookup tables for fast matching against all columns in `QBRecord`.
  //
  using LookupTables = LookupsForRecord<QBRecord>;

public:
  using unique_id_type = QBRecordTraits::unique_id_type;

  // Inserts a new record into the collection.  If the record is already
  // present, return false and leave the collection unchanged.  Otherwise,
  // return true having successfully modified the collection.
  //
  bool insert(QBRecord &&record);

  /**
      Return records that contains a string in the StringValue field
      records - the initial set of records to filter
      matchString - the string to search for
  */
  std::vector<QBRecord>
  find_matching_records(std::string_view columnName,
                        std::string_view matchString) const;

private:
  // All the records in the collection, by primary key.
  //
  std::unordered_map<unique_id_type, const QBRecordIntern> by_unique_id_;

  // Indices of all other columns.
  //
  LookupTables lookups_;
};
