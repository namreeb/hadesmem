// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <cstdio>

#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

#include "hash_table.hpp"

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

struct TranslatedStringRepository
{
  void* vtable_;
  int field_0[5];
  HashTable<char*, TranslatedNumberData> numbers_;
  int field_20;
  HashTable<char*, TranslatedStringData> strings_;
  int field_2C[50];
};

void DumpHashTableEntry(hadesmem::Process const& process,
                        TranslatedStringData const& data);

void DumpHashTableEntry(hadesmem::Process const& process,
                        TranslatedNumberData const& data);

void DumpStringRepository(hadesmem::Process const& process, std::uint8_t* base);

template <typename HashTableT>
void DumpStringRespositoryHashTable(hadesmem::Process const& process,
                                    HashTableT const& hash_table)
{
  for (std::uint32_t i = 0; i < hash_table.size_; ++i)
  {
    auto const entry_ptr = hadesmem::Read<typename HashTableT::Entry*>(
      process, hash_table.table_ + i);
    printf("\nHead: %p\n", static_cast<void*>(entry_ptr));

    for (auto entry =
           hadesmem::Read<typename HashTableT::Entry>(process, entry_ptr);
         ;
         entry =
           hadesmem::Read<typename HashTableT::Entry>(process, entry.next_))
    {
      printf("\n");

      printf("Next: %p\n", static_cast<void*>(entry.next_));
      printf("String (Address): %p\n", entry.key_);
      printf("String: %s\n",
             hadesmem::ReadString<char>(process, entry.key_).c_str());
      DumpHashTableEntry(process, entry.value_);

      if (entry.next_ == nullptr)
      {
        break;
      }
    }

    printf("\n");
  }
}
