// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <string>

#include <hadesmem/detail/static_assert_x86.hpp>

#include "std_string.hpp"

namespace divinity
{
struct TriString
{
  void* vtable_;
  char* fixed_narrow_;
  StdStringA std_narrow_;
  StdStringW std_wide_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(TriString) == 0x40);

struct TriStringPairPoly
{
  void* vtable_;
  TriString name_1_;
  TriString name_2_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(TriStringPairPoly) == 0x84);

TriStringPairPoly* ConstructTriStringPairPoly();

void DestructTriStringPairPoly(TriStringPairPoly* p);

std::shared_ptr<TriStringPairPoly> MakeTriStringPairPoly();
}

void DumpTriString(std::string const& type_name,
                   std::string const& sub_name,
                   divinity::TriString* tri_string);

void DumpTriStringPairPoly(std::string const& type_name,
                           divinity::TriStringPairPoly* tri_string_pair);
