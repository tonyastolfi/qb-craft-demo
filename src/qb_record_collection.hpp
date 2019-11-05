#pragma once

#include <string_view>
#include <type_traits>
#include <unordered_map>

#include "qb_column_lookup.hpp"
#include "qb_record.hpp"
#include "tuples.hpp"

/**
 * Represents a Record Collection.
 */
class QBRecordCollection {
public:
  // This defines the schema for the database.  Change this type alias (or make
  // it a template parameter) to support arbitrary column sets.
  //
  using traits_type = QBRecordTraits;

  // The record type stored by this collection.
  //
  using record_type = traits_type::columns_type;

  // The number of columns in `record_type`.
  //
  static constexpr int num_columns() {
    return std::tuple_size<record_type>::value;
  }

private:
  // Internal representation of a QBRecord.  Differs from the external
  // representation in that it does not need to store the unique id column.
  //
  using QBRecordIntern = decltype(drop_first(std::declval<record_type>()));

  static_assert(QBRecordTraits::unique_id_column() == 0,
                "The first column must be the unique id."); // TODO - relax this
                                                            // requirement.

  // Lookup tables for fast matching against all columns in `record_type`.
  //
  using LookupTables = LookupsForRecord<record_type>;

public:
  using unique_id_type = traits_type::unique_id_type;

  // Inserts a new record into the collection.  If the record is already
  // present, return false and leave the collection unchanged.  Otherwise,
  // return true having successfully modified the collection.
  //
  bool insert(record_type &&record);

  /**
      Return records that contains a string in the StringValue field
      records - the initial set of records to filter
      matchString - the string to search for
  */
  std::vector<record_type>
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
