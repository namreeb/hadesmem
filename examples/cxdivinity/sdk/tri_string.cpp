// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "tri_string.hpp"

#include <cstring>

#include <hadesmem/detail/trace.hpp>

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
