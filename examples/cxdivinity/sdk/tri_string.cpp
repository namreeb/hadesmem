// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "tri_string.hpp"

#include <cstring>

#include <hadesmem/detail/trace.hpp>

#include "offset.hpp"

namespace divinity
{

TriStringPairPoly* ConstructTriStringPairPoly()
{
  typedef TriStringPairPoly*(__fastcall * tTriStringPairPoly__Constructor)(
    TriStringPairPoly * p, int dummy_edx);
  auto const base =
    reinterpret_cast<std::uint8_t*>(::GetModuleHandleW(nullptr));
  auto const tri_string_pair_poly_constructor =
    reinterpret_cast<tTriStringPairPoly__Constructor>(
      base + FunctionOffsets::g_tri_string_pair_poly_constructor);
  TriStringPairPoly* p =
    reinterpret_cast<TriStringPairPoly*>(new char[sizeof(TriStringPairPoly)]);
  tri_string_pair_poly_constructor(p, 0xDEADBEEF);
  return p;
}

void DestructTriStringPairPoly(TriStringPairPoly* p)
{
  typedef int(__fastcall * tTriStringPairPoly__Destructor)(
    TriStringPairPoly * p, int dummy_edx, char a2);
  auto const base =
    reinterpret_cast<std::uint8_t*>(::GetModuleHandleW(nullptr));
  auto const tri_string_pair_poly_destructor =
    reinterpret_cast<tTriStringPairPoly__Destructor>(
      base + FunctionOffsets::g_tri_string_pair_poly_destructor);
  tri_string_pair_poly_destructor(p, 0xDEADBEEF, 0);
  delete[] reinterpret_cast<char*>(p);
}

std::shared_ptr<TriStringPairPoly> MakeTriStringPairPoly()
{
  auto const p = ConstructTriStringPairPoly();
  return {p, &DestructTriStringPairPoly};
}
}

void DumpTriString(std::string const& type_name,
                   std::string const& sub_name,
                   divinity::TriString* tri_string)
{
  (void)type_name;
  (void)tri_string;

  if (sub_name.empty())
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "%s::FixedString: %s.", type_name.c_str(), tri_string->fixed_narrow_);
    HADESMEM_DETAIL_TRACE_FORMAT_A("%s::StdNarrow: %s.",
                                   type_name.c_str(),
                                   GetStdStringRaw(&tri_string->std_narrow_));
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"%hs::StdWide: %s.",
                                   type_name.c_str(),
                                   GetStdStringRaw(&tri_string->std_wide_));
  }
  else
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A("%s::%s::FixedString: %s.",
                                   type_name.c_str(),
                                   sub_name.c_str(),
                                   tri_string->fixed_narrow_);
    HADESMEM_DETAIL_TRACE_FORMAT_A("%s::%s::StdNarrow: %s.",
                                   type_name.c_str(),
                                   sub_name.c_str(),
                                   GetStdStringRaw(&tri_string->std_narrow_));
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"%hs::%hs::StdWide: %s.",
                                   type_name.c_str(),
                                   sub_name.c_str(),
                                   GetStdStringRaw(&tri_string->std_wide_));
  }
}

void DumpTriStringPairPoly(std::string const& type_name,
                           divinity::TriStringPairPoly* tri_string_pair)
{
  DumpTriString(type_name, "Name1", &tri_string_pair->name_1_);
  DumpTriString(type_name, "Name2", &tri_string_pair->name_2_);

  std::string const name2_fixed_narrow = tri_string_pair->name_2_.fixed_narrow_;
  std::string const name2_std_narrow =
    ConvertStdString(&tri_string_pair->name_2_.std_narrow_);
  std::wstring const name2_std_wide =
    ConvertStdString(&tri_string_pair->name_2_.std_wide_);
  if ((!name2_fixed_narrow.empty() &&
       name2_fixed_narrow !=
         "ls::TranslatedStringRepository::s_HandleUnknown") ||
      (!name2_std_narrow.empty() &&
       name2_std_narrow != "ls::TranslatedStringRepository::s_HandleUnknown") ||
      (!name2_std_wide.empty() &&
       name2_std_wide != L"ls::TranslatedStringRepository::s_HandleUnknown"))
  {
    HADESMEM_DETAIL_TRACE_A("Attention! Found interesting string.");
  }
}
