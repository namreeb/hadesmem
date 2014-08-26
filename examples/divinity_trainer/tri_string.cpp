// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "tri_string.hpp"

#include <cstdio>

void DumpTriString(hadesmem::Process const& process,
                   TriString const& tri_string,
                   std::string const& type_name,
                   std::string const& sub_name)
{
  if (sub_name.empty())
  {
    printf(
      "%s::FixedString: %s.\n",
      type_name.c_str(),
      hadesmem::ReadString<char>(process, tri_string.fixed_narrow_).c_str());
    printf("%s::StdNarrow: %s.\n",
           type_name.c_str(),
           GetString(process, tri_string.std_narrow_).c_str());
    printf("%s::StdWide: %ls.\n",
           type_name.c_str(),
           GetString(process, tri_string.std_wide_).c_str());
  }
  else
  {
    printf(
      "%s::%s::FixedString: %s.\n",
      type_name.c_str(),
      sub_name.c_str(),
      hadesmem::ReadString<char>(process, tri_string.fixed_narrow_).c_str());
    printf("%s::%s::StdNarrow: %s.\n",
           type_name.c_str(),
           sub_name.c_str(),
           GetString(process, tri_string.std_narrow_).c_str());
    printf("%s::%s::StdWide: %ls.\n",
           type_name.c_str(),
           sub_name.c_str(),
           GetString(process, tri_string.std_wide_).c_str());
  }
}

void DumpTriStringPairPoly(hadesmem::Process const& process,
                           TriStringPairPoly const& name_data,
                           std::string const& type_name)
{
  DumpTriString(process, name_data.name_1_, type_name, "Name1");
  DumpTriString(process, name_data.name_1_, type_name, "Name2");

  auto const name2_fixed_narrow =
    hadesmem::ReadString<char>(process, name_data.name_2_.fixed_narrow_);
  auto const name2_std_narrow =
    GetString(process, name_data.name_2_.std_narrow_);
  auto const name2_std_wide = GetString(process, name_data.name_2_.std_wide_);
  if ((!name2_fixed_narrow.empty() &&
       name2_fixed_narrow !=
         "ls::TranslatedStringRepository::s_HandleUnknown") ||
      (!name2_std_narrow.empty() &&
       name2_std_narrow != "ls::TranslatedStringRepository::s_HandleUnknown") ||
      (!name2_std_wide.empty() &&
       name2_std_wide != L"ls::TranslatedStringRepository::s_HandleUnknown"))
  {
    printf("FOUND INTERESTING STRING\n");
  }
}
