// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include "hash_table.hpp"

struct TranslatedStringData
{
  void* unknown_0000_;
  char* uuid_;
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
