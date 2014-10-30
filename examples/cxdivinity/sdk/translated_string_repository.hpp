// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <cstdio>

#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/static_assert_x86.hpp>

#include "hash_table.hpp"
#include "tri_string.hpp"

namespace divinity
{
struct TranslatedStringData
{
  void* unknown_0000_;
  char* uuid_;
  void* unknown_0008_;
  void* unknown_000C_;
  // More data? Size is a complete guess.
};

struct TranslatedNumberData
{
  void* unknown_0000_;
  void* unknown_0004_;
  void* unknown_0008_;
  void* unknown_000C_;
  // More data? Size is a complete guess.
};

struct TranslatedUnknownData
{
  void* unknown_0000_;
  void* unknown_0004_;
  void* unknown_0008_;
  void* unknown_000C_;
  // More data? Size is a complete guess.
};

struct TranslatedStringRepository
{
  void* vtable_;
  int field_4;
  int field_8;
  HashTable<std::uint32_t, TranslatedUnknownData> unknown_;
  HashTable<char*, TranslatedNumberData> numbers_;
  HashTable<char*, TriString> strings_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(TranslatedStringRepository) == 0x30);
}

void DumpTranslatedStringRepositoryHashTableEntry(
  divinity::TranslatedUnknownData* data);

void DumpTranslatedStringRepositoryHashTableEntry(
  divinity::TranslatedNumberData* data);

void DumpTranslatedStringRepositoryHashTableEntry(
  divinity::TranslatedStringData* data);

void DumpTranslatedStringRepositoryHashTableEntry(divinity::TriString* data);

void DumpTranslatedStringRepositoryHashTableKey(char* key);

void DumpTranslatedStringRepositoryHashTableKey(std::uint32_t key);

void DumpTranslatedStringRepository();

template <typename HashTableT>
void DumpTranslatedStringRepositoryHashTable(HashTableT* hash_table)
{
  for (std::uint32_t i = 0; i < hash_table->num_buckets_; ++i)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A("Bucket: %u", i);

    auto entry = hash_table->table_[i];
    while (entry)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A("Entry: %p", entry);
      HADESMEM_DETAIL_TRACE_FORMAT_A("Next: %p", entry->next_);
      DumpTranslatedStringRepositoryHashTableKey(entry->key_);
      DumpTranslatedStringRepositoryHashTableEntry(&entry->value_);

      entry = entry->next_;
    }
  }
}
