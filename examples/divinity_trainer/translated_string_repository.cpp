// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "translated_string_repository.hpp"

#include "offsets.hpp"

void DumpHashTableEntry(hadesmem::Process const& process,
  TranslatedStringData const& data)
{
  printf("Value::Unknown0000: %p\n", data.unknown_0000_);
  printf("Value::UUID (Address): %p\n", data.uuid_);
  printf("Value::UUID: %s\n",
    hadesmem::ReadString<char>(process, data.uuid_).c_str());
  printf("Value::Unknown0008: %p\n", data.unknown_0008_);
  printf("Value::Unknown000C: %p\n", data.unknown_000C_);
}

void DumpHashTableEntry(hadesmem::Process const& /*process*/,
  TranslatedNumberData const& data)
{
  printf("Value::Unknown0000: %p\n", data.unknown_0000_);
  printf("Value::Unknown0004: %p\n", data.unknown_0004_);
  printf("Value::Unknown0008: %p\n", data.unknown_0008_);
  printf("Value::Unknown000C: %p\n", data.unknown_000C_);
}

void DumpStringRepository(hadesmem::Process const& process, std::uint8_t* base)
{
  auto const translated_string_repository_ptr = hadesmem::Read<void*>(
    process, base + Offsets::g_translated_string_repository);
  auto const translated_string_repository =
    hadesmem::Read<TranslatedStringRepository>(
    process, translated_string_repository_ptr);

  printf("TranslatedStringRepository: %p\n", translated_string_repository_ptr);
  printf("TranslatedStringRepository::VTable: %p\n",
    translated_string_repository.vtable_);
  printf("\n");

  printf("Dumping translated strings.\n\n");
  DumpStringRespositoryHashTable(process,
    translated_string_repository.strings_);

  printf("Dumping translated numbers.\n\n");
  DumpStringRespositoryHashTable(process,
    translated_string_repository.numbers_);
}
