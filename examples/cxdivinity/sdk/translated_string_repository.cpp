// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "translated_string_repository.hpp"

#include <hadesmem/detail/trace.hpp>

#include "offset.hpp"

divinity::TranslatedStringRepository* GetTranslatedStringRepository()
{
  auto const base =
    reinterpret_cast<std::uint8_t*>(::GetModuleHandleW(nullptr));
  auto const translated_string_repository =
    *reinterpret_cast<divinity::TranslatedStringRepository**>(
      base + divinity::DataOffsets::g_translated_string_repository);
  return translated_string_repository;
}

void DumpTranslatedStringRepositoryHashTableEntry(
  divinity::TranslatedUnknownData* data)
{
  (void)data;
  HADESMEM_DETAIL_TRACE_FORMAT_A("Value::Unknown0000: %p", data->unknown_0000_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Value::Unknown0004: %p", data->unknown_0004_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Value::Unknown0008: %p", data->unknown_0008_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Value::Unknown000C: %p", data->unknown_000C_);
}

void DumpTranslatedStringRepositoryHashTableEntry(
  divinity::TranslatedNumberData* data)
{
  (void)data;
  HADESMEM_DETAIL_TRACE_FORMAT_A("Value::Unknown0000: %p", data->unknown_0000_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Value::Unknown0004: %p", data->unknown_0004_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Value::Unknown0008: %p", data->unknown_0008_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Value::Unknown000C: %p", data->unknown_000C_);
}

void DumpTranslatedStringRepositoryHashTableEntry(
  divinity::TranslatedStringData* data)
{
  (void)data;
  HADESMEM_DETAIL_TRACE_FORMAT_A("Value::Unknown0000: %p", data->unknown_0000_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Value::UUID (Address): %p", data->uuid_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Value::UUID: %s", data->uuid_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Value::Unknown0008: %p", data->unknown_0008_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Value::Unknown000C: %p", data->unknown_000C_);
}

void DumpTranslatedStringRepositoryHashTableEntry(divinity::TriString* data)
{
  DumpTriString("Value", "", data);
}

void DumpTranslatedStringRepositoryHashTableKey(char* key)
{
  (void)key;
  HADESMEM_DETAIL_TRACE_FORMAT_A("Key: %s", key);
}

void DumpTranslatedStringRepositoryHashTableKey(std::uint32_t key)
{
  (void)key;
  HADESMEM_DETAIL_TRACE_FORMAT_A("Key: %08X", key);
}

void DumpTranslatedStringRepository()
{
  auto const translated_string_repository = GetTranslatedStringRepository();

  HADESMEM_DETAIL_TRACE_FORMAT_A("TranslatedStringRepository: %p",
                                 translated_string_repository);
  HADESMEM_DETAIL_TRACE_FORMAT_A("TranslatedStringRepository::VTable: %p",
                                 translated_string_repository->vtable_);

  HADESMEM_DETAIL_TRACE_A("Dumping translated unknown.");
  DumpTranslatedStringRepositoryHashTable(
    &translated_string_repository->unknown_);

  HADESMEM_DETAIL_TRACE_A("Dumping translated numbers.");
  DumpTranslatedStringRepositoryHashTable(
    &translated_string_repository->numbers_);

  HADESMEM_DETAIL_TRACE_A("Dumping translated strings.");
  DumpTranslatedStringRepositoryHashTable(
    &translated_string_repository->strings_);
}
