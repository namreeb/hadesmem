// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>

#include <hadesmem/process.hpp>

#include "std_string.hpp"

struct TriString
{
  virtual ~TriString()
  {
  }

  char* fixed_narrow_;
  StdStringA std_narrow_;
  StdStringW std_wide_;
};

struct TriStringPairPoly
{
  virtual ~TriStringPairPoly()
  {
  }

  TriString name_1_;
  TriString name_2_;
};

void DumpTriString(hadesmem::Process const& process,
                   TriString const& tri_string,
                   std::string const& type_name,
                   std::string const& sub_name);

void DumpTriStringPairPoly(hadesmem::Process const& process,
                           TriStringPairPoly const& name_data,
                           std::string const& type_name);
