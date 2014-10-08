// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>

namespace divinity
{

template <typename Key, typename Value> struct HashTable
{
  struct Entry
  {
    Entry* next_;
    Key key_;
    Value value_;
  };

  std::size_t num_buckets_;
  Entry** table_;
  std::size_t num_entries_;
};

}
